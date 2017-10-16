#include <Arduino.h>
#include <EEPROM.h>
#include <Wire.h>
#include "LiquidCrystal_I2C.h"
#include "SimpleWebRadio.h"
#include "SimpleControl.h"
#include "SimpleWebServer.h"
#include "SimpleUtils.h"
#include "SimplePrint.h"

#define DEBUG_MODE

#define RADIO_STOP 0
#define RADIO_PLAY 1

byte macaddr[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };    // mac address
byte iplocal[] = { 192, 168,   1,  62 };                    // lan ip (e.g. "192.168.1.178")
byte gateway[] = { 192, 168,   1,   1 };                    // router gateway
byte subnet[]  = { 255, 255, 255,   0 };                    // subnet mask
byte DNS[]     = {   8,   8,   8,   8 };                    // DNS server (Google)

SimpleRadio       radio;                                    // radio     object  (to play ICYcast streams)
SimpleWebServer   myServer( 80);                            // server    object (to respond on API calls)
SimpleScheduler   scheduler( 1000);                         // scheduler object (to process rotary + button handling)
SimpleButton      buttonR( A0, false);                       // button    object (to switch between preset + volume setting)
SimpleRotary      rotaryR( A1, A2);                          // rotary    object (to change preset + volume)
// SimpleButton      buttonL( A8, false);                       // button    object (to switch between preset + volume setting)
// SimpleRotary      rotaryL( A9, A10);                          // rotary    object (to change preset + volume)
Stopwatch         stopwatch( 750);                         // stopwatch object (set for 1 sec)
LiquidCrystal_I2C lcd( 0x3F, 20, 4);

presetInfo presetData;                                      // preset    object (to hold station url or IP data)
byte       preset =  0;                                      // current preset playing
byte       volume = 70;                                      // current volume playing
byte       state  = RADIO_STOP;

void loadSettings();                                        // load preset + volume from EEPROM
void saveSettings();                                        // save preset + volume to   EEPROM
void hndlPlayer();
void hndlDevice();
void hndlServer();
bool copyPreset( char*);                                    // copy url to presetData (but not to EEPROM)
bool loadPreset( int i);                                    // load presetData from EEPROM slot i
bool savePreset( int i, char* = NULL);                      // save presetData to   EERPOM slot i
void initStatus();                                          //
void showStatus();                                          // show preset + volume (on lcd)

void setup()
{
	Serial.begin( 115200);

  #ifdef DEBUG_MODE
  LINE( Serial, F( "-------------------------"));           // show header
	LINE( Serial, F( "- Arduino WebRadio V0.6 -"));
  LINE( Serial, F( "-------------------------"));
  LINE( Serial, F("#"));
  #endif

  Ethernet.begin( macaddr, iplocal, DNS, gateway, subnet);

  #ifdef DEBUG_MODE
  ATTR_( Serial, F( "# server listening at "), Ethernet.localIP());
  ATTR ( Serial, F( ":")                     , myServer.getPort());
  #endif

  myServer.begin();                                         // starting webserver

  //loadSettings();                                           // load preset + volume from EEPROM
  loadPreset( preset);                                      // load presetData from EEPROM slot indicated by preset

  radio.setPlayer( 2, 6, 7, 8);                             // initialize MP3 player
  radio.setVolume( volume);                                 // set volume of player

  // rotaryL.setMinMax( 0, PRESET_MAX - 1, true);               // set rotary boundaries
  // rotaryL.setPosition( preset);                              // set rotary to preset
  rotaryR.setMinMax( 100, 0, 10);                      // set proper rotary boundaries
  rotaryR.setPosition( volume);                        // set proper rotary position (= last volume)

  // scheduler.attachHandler( rotaryL.handle);                  // include rotary in scheduler
  // scheduler.attachHandler( buttonL.handle);                  // include button in scheduler
  scheduler.attachHandler( rotaryR.handle);                  // include rotary in scheduler
  scheduler.attachHandler( buttonR.handle);                  // include button in scheduler
  scheduler.start();                                        // start scheduler

  lcd.begin();

  initStatus();
  showStatus();                                             // show preset + volume

  #ifdef DEBUG_MODE
  LINE( Serial, F( "#"));
  LINE( Serial, F( "# hold button to switch on"));
  #endif
}

void loop()
{
  switch ( state) {
    case RADIO_PLAY:
      hndlPlayer();
      break;
    case RADIO_STOP:
      break;
  }

  hndlDevice();
  hndlServer();

  stopwatch.check( showStatus);                           // save Settings every 10 sec
}

void hndlPlayer()
{
  if ( radio.connected()) {                                 // if iCYcast stream active
    radio.readICYcastStream();                              // receive next stream data
    radio.hndlICYcastStream();                              // process next stream data
  } else {
    radio.stopICYcastStream();                              // open new ICYcast stream
    radio.openICYcastStream( &presetData);                  // open new ICYcast stream
    radio.readICYcastStream();                              // receive next stream data
    radio.hndlICYcastHeader();                              // process next stream data
  }
}

void hndlDevice()
{
  // if ( buttonL.available()) {                                // if button processed
  //   switch ( buttonL.read()) {
  //     case BUTTON_NORMAL :                                              // allow preset selection
  //     break;
  //     case BUTTON_HOLD :                                              // allow volume selection
  //     break;
  //   }
  // }
  //
  // if ( rotaryL.changed()) {                                  // if rotary turned
  //   initStatus();
  //   radio.stopICYcastStream();                            // stop radio playing
  //   preset = rotaryL.position();                           // read rotaty position
  //   loadPreset( preset);                                  // read preset from EEPROM
  // }

  if ( buttonR.available()) {                                // if button processed
    switch ( buttonR.read()) {
      case BUTTON_NORMAL :                                              // allow preset selection
      lcd.backlight();
      state = RADIO_PLAY;
      break;
      case BUTTON_HOLD :
      lcd.noBacklight();
      state = RADIO_STOP;
      break;
    }
  }

  if ( rotaryR.changed()) {                                  // if rotary turned
    volume = rotaryR.position();                           // read rotary position
    radio.setVolume( volume);                             // read rotary position
  }

  // buttonL.handle();
  // rotaryL.handle();
  // buttonR.handle();
  // rotaryR.handle();
}

void hndlServer()
{
  if ( myServer.available()) {                              // check incoming HTTP request
    int  code = 400;                                        // default return code = error

    char* chn = myServer.path( 1);                          // get preset id     (if present)
    char* url = myServer.arg( "url");                       // get preset url    (if present)
    char* vol = myServer.arg( "volume");                    // get player volume (if present)

    if (  myServer.path( 0, "presets")) {                   // if presets addressed
      if (( myServer.getMethod() == HTTP_PUT) && chn && url) {
        savePreset( atoi( chn) - 1, url);                   // save url in EEPROM
        code = 200;
      }
    }

    if (  myServer.path( 0, "webradio")) {                  // if player addressed
      if (( myServer.getMethod() == HTTP_GET) && !chn && url) {
        copyPreset( url);                                   // load url (but don't store in EEPROM)
        code = 200;
      }

      if (( myServer.getMethod() == HTTP_GET) && chn && !url) {
        loadPreset( preset = atoi( chn) - 1);               // load preset from EEPROM
        code = 200;
      }

      if (( myServer.getMethod() == HTTP_GET) && vol) {
        radio.setVolume( volume = 100 - atoi( vol));         // set player volume
        code = 200;
      }

      if (( myServer.getMethod() == HTTP_GET) && myServer.arg( "play")) {
        initStatus();
        radio.stopICYcastStream();
        radio.openICYcastStream( &presetData);             // play url in memory
        code = 200;
      }

      if (( myServer.getMethod() == HTTP_GET) && myServer.arg( "stop")) {
        radio.stopICYcastStream();                         // stop player
        code = 200;
      }
    }

    myServer.response( code);                               // return response (headers + code)
  }
}

// load preset from EEPROM
void loadSettings()
{
  EEPROM.get( 0, preset);                                   // load preset from EEPROM
  EEPROM.get( 1, volume);                                   // load volume from EEPROM
}

// save preset to EEPROM
void saveSettings()
{
  EEPROM.put( 0, preset);                                   // save preset to EEPROM
  EEPROM.put( 1, volume);                                   // save preset to EEPROM

  // #ifdef DEBUG_MODE
  // LINE( Serial, F("# settings saved"));
  // #endif
}

// copy station url to presetData (but not to EEPROM)
bool copyPreset( char* url)
{
  if ( url) {                                               // if valid url
    strCpy( presetData.url, url, PRESET_PATH_LENGTH);       // copy url to presetData

    return true;                                            // return success
  } else {
    return false;                                           // return failure
  }
}

// load presetData from EEPROM
bool loadPreset( int preset)
{
  if (( preset >= 0) && ( preset < PRESET_MAX)) {           // if valid preset
    EEPROM.get( 2 + preset * sizeof( presetInfo), presetData);
                                                            // load presetData from EEPROM
    return true;                                            // return success
  } else {
    return false;                                           // return failure
  }
}

// save presetData to EEPROM
bool savePreset( int p, char* url)
{
  copyPreset( url);                                         // copy url (if provided)

  if (( p >= 0) && ( p < PRESET_MAX)) {                     // if valid preset
    EEPROM.put( 2 + p * sizeof( presetInfo), presetData);   // save presetData from EEPROM

    return true;                                            // return success
  } else {
    return false;                                           // return failure
  }
}

void initStatus()
{
  LCD1( lcd,  0,  0, fill( "< ---------- >", 20, true));
  LCD1( lcd,  0,  1, fill( ""              , 20, true));
  LCD1( lcd,  4,  2, F( "Bit Rate" ));
  LCD1( lcd,  0,  3, F( "CHANNEL ="));          // show preset on LCD
  LCD1( lcd, 12,  3, F( "VOL ="    ));          // show volume on LCD
}

// show preset + volume
void showStatus()
{
  static char label[2] = { ' ', '-'};
  static int  cnt = 0;
  static int  len = 0;

  if ( radio.available()) {                                 // if playing
    LCD1( lcd,  0, 0, fill( radio.getName(), 20, true));    // show station name
    LCD1( lcd,  0, 1, fill( radio.getType(), 20, true));    // show station genre
    LCD1( lcd, 13, 2, fill( radio.getRate(),  3));          // show station bit rate

    #ifdef DEBUG_MODE
    ATTR_( Serial, F(  "# name = "), radio.getName());
    ATTR_( Serial, F( " / type = "), radio.getType());
    ATTR ( Serial, F( " / rate = "), radio.getRate());
    #endif

    len = max( 0, strlen( radio.getType()) - 20);
  }

  LCD1( lcd,  2, 2, radio.receiving() ? label[cnt % 2] : label[0]);
  LCD1( lcd, 17, 2, radio.receiving() ? label[cnt % 2] : label[0]);
  LCD1( lcd, 10, 3, preset + 1  );                          // show preset on LCD
  LCD1( lcd, 18, 3, 100 - volume);                          // show volume on LCD

  if ( len > 0) {
    LCD1( lcd,  0, 1, fill( radio.getType() + minMax( cnt - 2, 0, len), 20));
  }

  if ( cnt % 10 == 0) {
    saveSettings();
  }

  cnt %= ( len + 4); cnt++;
  //cnt += 1;
}
