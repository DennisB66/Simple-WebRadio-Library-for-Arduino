#include <Arduino.h>
#include <EEPROM.h>
#include <Wire.h>
#include "LiquidCrystal_I2C.h"
#include "SimpleWebRadio.h"
#include "SimpleControl.h"
#include "SimpleWebServer.h"
#include "SimpleUtils.h"

#define NO_DEBUG_MODE

byte macaddr[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };    // mac address
byte iplocal[] = { 192, 168,   1,  62 };                    // lan ip (e.g. "192.168.1.178")
byte gateway[] = { 192, 168,   1,   1 };                    // router gateway
byte subnet[]  = { 255, 255, 255,   0 };                    // subnet mask
byte DNS[]     = {   8,   8,   8,   8 };                    // DNS server (Google)

SimpleRadio       radio;                                    // radio     object  (to play ICYcast streams)
SimpleWebServer   myServer( 80);                            // server    object (to respond on API calls)
SimpleScheduler   scheduler( 1000);                         // scheduler object (to process rotary + button handling)
SimpleRotary      rotary( A1, A2);                          // rotary    object (to change preset + volume)
SimpleButton      button( A0, false);                       // button    object (to switch between preset + volume setting)
Stopwatch         stopwatch( 750);                         // stopwatch object (set for 1 sec)
LiquidCrystal_I2C lcd( 0x3F, 20, 4);

presetInfo presetData;                                      // preset    object (to hold station url or IP data)
byte       preset = 0;                                      // current preset playing
byte       volume = 0;                                      // current volume playing

void loadSettings();                                        // load preset + volume from EEPROM
void saveSettings();                                        // save preset + volume to   EEPROM
bool copyPreset( char*);                                    // copy url to presetData (but not to EEPROM)
bool loadPreset( int i);                                    // load presetData from EEPROM slot i
bool savePreset( int i, char* = NULL);                      // save presetData to   EERPOM slot i
void showStatus();                                          // show preset + volume (on lcd)

void setup()
{
	Serial.begin( 9600);

  #ifdef DEBUG_MODE
  LINE( Serial, F( "-------------------------"));           // show header
	LINE( Serial, F( "- Arduino WebRadio V0.6 -"));
  LINE( Serial, F( "-------------------------"));
  LINE( Serial, F("#"));
  #endif

  Ethernet.begin( macaddr, iplocal, DNS, gateway, subnet);
  myServer.begin();                                         // starting webserver

  #ifdef DEBUG_MODE
  ATTR_( Serial, F( "# server listening at "), Ethernet.localIP());
  ATTR ( Serial, F( ":")                     , myServer.getPort());
  #endif

  loadSettings();                                           // load preset + volume from EEPROM
  loadPreset( preset);                                      // load presetData from EEPROM slot indicated by preset

  radio.setPlayer( 2, 6, 7, 8);                             // initialize MP3 player
  radio.setVolume( volume);                                 // set volume of player

  rotary.setMinMax( 0, PRESET_MAX - 1, true);               // set rotary boundaries
  rotary.setPosition( preset);                              // set rotary to preset

  scheduler.attachHandler( rotary.handle);                  // include rotary in scheduler
  scheduler.attachHandler( button.handle);                  // include button in scheduler
  scheduler.start();                                        // start scheduler

  lcd.begin();                                              // LCD Initialization

  showStatus();                                             // show preset + volume

  #ifdef DEBUG_MODE
  LINE( Serial, F( "# ready"));
  #endif
}

void loop()
{
  static byte mode = 0;                                     // 0 = change preset / 1 = change volume

  if ( radio.connected()) {                                 // if iCYcast stream active
    radio.readICYcastStream();                              // receive next stream data
    radio.hndlICYcastStream();                              // process next stream data
  } else {
    radio.stopICYcastStream();                              // stop radio
    radio.openICYcastStream( &presetData);                  // open new ICYcast stream

    delay(2000);                                            // wait a short while after connecting
  }

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

  if ( radio.available()) {                                 // if playing
    LCD1( lcd,  0, 0, space( radio.getName(), 20, true));   // show station name
    LCD1( lcd,  0, 1, space( radio.getType(), 20, true));   // show station genre
    LCD2( lcd,  6, 2, F( "Rate "), radio.getRate());        // show station bit rate

    #ifdef DEBUG_MODE
    ATTR_( Serial, F( "# Station "), radio.getName());
    ATTR_( Serial, F( " / type = "), radio.getType());
    ATTR ( Serial, F( " / rate = "), radio.getRate());
    #endif

    //showStatus( mode);                                      // show preset + volume
  }

  if ( button.available()) {                                // if button processed
    while ( button.read());                                 // read button value
    mode = !mode;                                           // switch selection mode
    switch ( mode) {
      case 0 :                                              // allow preset selection
        rotary.setMinMax( 0, PRESET_MAX - 1, true);         // set proper rotary boundaries
        rotary.setPosition( preset);                        // set proper rotary position (= last preset)
        break;
      case 1 :                                              // allow volume selection
        rotary.setMinMax( 100, 0, 10);                      // set proper rotary boundaries
        rotary.setPosition( volume);                        // set proper rotary position (= last volume)
        break;
    }
  }

  if ( rotary.changed()) {                                  // if rotary turned
    switch ( mode) {
    case 0 :                                                // allow preset selection
      radio.stopICYcastStream();                            // stop radio playing
      preset = rotary.position();                           // read rotaty position
      loadPreset( preset);                                  // read preset from EEPROM
      break;
    case 1 :                                                // allow volume selection
      volume = rotary.position();                           // read rotary position
      radio.setVolume( volume);                             // read rotary position
      break;
    }
  }

  stopwatch.check( showStatus);                           // save Settings every 10 sec
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

  #ifdef DEBUG_MODE
  LINE( Serial, F("# settings saved"));
  #endif
}

// copy station url to presetData (but not to EEPROM)
bool copyPreset( char* url)
{
  if ( url) {                                               // if valid url
    strCpy( presetData.url, url, PRESET_PATH_LENGTH - 1);   // copy url to presetData

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

// show preset + volume
void showStatus()
{
  static int count = 0;

  if ( count) {
    saveSettings();
  }

  LCD1( lcd,  3, 2, count % 2 ? F( "-") : F( " "));
  LCD1( lcd, 16, 2, count % 2 ? F( "-") : F( " "));
  LCD2( lcd,  0, 3, F( "CHANNEL = "), preset + 1  );          // show preset on LCD
  LCD2( lcd, 12, 3, F( "VOL = " ), 100 - volume);          // show volume on LCD

  #ifdef DEBUG_MODE
  ATTR_( Serial, F(  "> preset] = "),       preset + 1);
  ATTR ( Serial, F( " / volume] = "), 100 - volume    );
  #endif

  count +=  1;
  count %= 10;
}
