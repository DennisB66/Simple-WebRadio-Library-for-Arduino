#include <Arduino.h>
#include <EEPROM.h>
#include <Wire.h>
#include "LiquidCrystal_I2C.h"
#include "SimpleWebRadio.h"
#include "SimpleControl.h"
//#include "SimpleWebServer.h"
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
//SimpleWebServer   server( 80);                              // server    object (to respond on API calls)
SimpleScheduler   scheduler( 1000);                         // scheduler object (to process rotary + button handling)
SimpleButton      button( A0, false);                       // button    object (to switch between preset + volume setting)
SimpleRotary      rotary( A1, A2);                          // rotary    object (to change preset + volume)
LiquidCrystal_I2C lcd( 0x3F, 20, 4);

PresetInfo presetData;                                      // preset    object (to hold station url or IP data)
byte       preset =  0;                                     // current preset playing
byte       volume = 70;                                     // current volume playing
byte       state  = RADIO_STOP;

void loadSettings();                                        // load preset + volume from EEPROM
void saveSettings();                                        // save preset + volume to   EEPROM
void hndlPlayer();
void hndlDevice();
//void hndlServer();
bool copyPreset( char*);                                    // copy url to presetData (but not to EEPROM)
bool loadPreset( int i);                                    // load presetData from EEPROM slot i
bool savePreset( int i, char* = NULL);                      // save presetData to   EERPOM slot i
void initStatus();                                          //
void showStatus();                                          // show preset + volume (on lcd)

void setup()
{
	BEGIN( 9600);

  #ifdef DEBUG_MODE
  PRINT( F( "# ---------------------")) LF;                   // show header
	PRINT( F( "# - Arduino WebRadio  -")) LF;
  PRINT( F( "# ---------------------")) LF;
  PRINT( F( "#")) LF;
  #endif

  Ethernet.begin( macaddr, iplocal, DNS, gateway, subnet);

  //server.begin();                                         // starting webserver

  loadSettings();                                           // load preset + volume from EEPROM
  loadPreset( preset);                                      // load presetData from EEPROM slot indicated by preset

  #ifdef DEBUG_MODE
  PRINT( F( "# client hosted at ")); PRINT( Ethernet.localIP()) LF;
  VALUE( F( "# preset"), preset); VALUE( F( "/ volume"), volume);
  PRINT( F( "/ preset url = ")); QUOTE( presetData.url); PRINT( F( " at "));
  PRINT( presetData.ip4); PRINT( ':'); PRINT( presetData.port) LF;
  PRINT( F( "#")) LF;
  #endif

  radio.setPlayer( 2, 6, 7, 8);                             // initialize MP3 player
  radio.setVolume( volume);                                 // set volume of player

  rotary.setMinMax( 0, RADIO_PRESET_MAX - 1, true);               // set rotary boundaries
  rotary.setPosition( preset);                              // set rotary to preset

  scheduler.begin();                                        // start checking ratary & button action

  lcd.begin();

  initStatus();
  showStatus();                                             // show preset + volume

  #ifdef DEBUG_MODE
  PRINT( F( "# hold button to switch on")) LF;
  #endif
}

void loop()
{
  static Stopwatch save( 9000, saveSettings);
  static Stopwatch disp(  750, showStatus);

  switch ( state) {
    case RADIO_PLAY:
      hndlPlayer();
      break;
    case RADIO_STOP:
      break;
  }

  hndlDevice();
  //hndlServer();

  save.check();
  disp.check();
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
  static bool mode = 0;

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

  if ( button.available()) {                                // if button processed
    switch ( button.read()) {
    case BUTTON_NORMAL :
      lcd.backlight();
      state = RADIO_PLAY;
      break;
    case BUTTON_HOLD :
      lcd.noBacklight();
      state = RADIO_STOP;
      break;
    case BUTTON_DOUBLE :
      mode = !mode;
      switch (mode) {
      case 0 :                                              // allow preset selection
        rotary.setMinMax( 0, RADIO_PRESET_MAX - 1, true);         // set proper rotary boundaries
        rotary.setPosition( preset);                        // set proper rotary position (= last preset)
        break;
      case 1 :                                              // allow volume selection
        rotary.setMinMax( 100, 0, 10);                      // set proper rotary boundaries
        rotary.setPosition( volume);                        // set proper rotary position (= last volume)
        break;
      }
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
}

// void hndlServer()
// {
//   if ( server.available()) {                                // check incoming HTTP request
//     int  code = 400;                                        // default return code = error
//
//     char* chn = server.path( 1);                            // get preset id     (if present)
//     char* url = server.arg( "url");                         // get preset url    (if present)
//     char* vol = server.arg( "volume");                      // get player volume (if present)
//
//     if (  server.path( 0, "presets")) {                     // if presets addressed
//       if (( server.getMethod() == HTTP_PUT) && chn && url) {
//         savePreset( atoi( chn) - 1, url);                   // save url in EEPROM
//         code = 200;
//       }
//     }
//
//     if (  server.path( 0, "webradio")) {                    // if player addressed
//       if (( server.getMethod() == HTTP_GET) && !chn && url) {
//         copyPreset( url);                                   // load url (but don't store in EEPROM)
//         code = 200;
//       }
//
//       if (( server.getMethod() == HTTP_GET) && chn && !url) {
//         loadPreset( preset = atoi( chn) - 1);               // load preset from EEPROM
//         code = 200;
//       }
//
//       if (( server.getMethod() == HTTP_GET) && vol) {
//         radio.setVolume( volume = 100 - atoi( vol));        // set player volume
//         code = 200;
//       }
//
//       if (( server.getMethod() == HTTP_GET) && server.arg( "play")) {
//         initStatus();
//         radio.stopICYcastStream();
//         radio.openICYcastStream( &presetData);              // play url in memory
//         code = 200;
//       }
//
//       if (( server.getMethod() == HTTP_GET) && server.arg( "stop")) {
//         radio.stopICYcastStream();                          // stop player
//         code = 200;
//       }
//     }
//
//     server.response( code);                                 // return response (headers + code)
//   }
// }

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
  PresetInfo data;

  if (( preset >= 0) && ( preset < RADIO_PRESET_MAX)) {     // if valid preset
    EEPROM.get( 2 + preset * sizeof( PresetInfo), data);    // load presetData from EEPROM

    strcpy( presetData.url, data.url);
    presetData.ip4  = data.ip4 ;
    presetData.port = data.port;

    return true;
  } else {
    return false;                                           // return failure
  }


  //IPAddress ip = presetData.ip4; memcpy( presetData.ip4, ip, sizeof( ip));
}

// save presetData to EEPROM
bool savePreset( int preset, char* url)
{
  copyPreset( url);                                         // copy url (if provided)

  if (( preset >= 0) && ( preset < RADIO_PRESET_MAX)) {                     // if valid preset
    EEPROM.put( 2 + preset * sizeof( PresetInfo), presetData);   // save presetData from EEPROM

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
    LCD1( lcd,  0, 1, fill( radio.getInfo(), 20, true));    // show station genre
    LCD1( lcd, 13, 2, fill( radio.getRate(),  3));          // show station bit rate

    #ifdef DEBUG_MODE
    VALUE( F( "# name"), radio.getName());
    VALUE( F(   "info"), radio.getInfo());
    VALUE( F(   "rate"), radio.getRate()); LF;
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


  cnt %= ( len + 4); cnt++;
  //cnt += 1;
}
