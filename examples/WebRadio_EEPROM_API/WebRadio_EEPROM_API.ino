#include <Arduino.h>
#include <EEPROM.h>
#include "SimpleWebRadio.h"
#include "SimpleControl.h"
#include "SimpleWebServer.h"
#include "SimpleUtils.h"
#include <PCD8544.h>

#define DEBUG_MODE

#define SERVER_NAME "WebRadio01"
#define SERVER_PORT 80

byte macaddr[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };    // mac address
byte iplocal[] = { 192, 168,   1,  62 };                    // lan ip (e.g. "192.168.1.178")
byte gateway[] = { 192, 168,   1,   1 };                    // router gateway
byte subnet[]  = { 255, 255, 255,   0 };                    // subnet mask
byte DNS[]     = {   8,   8,   8,   8 };                    // DNS server (Google)

SimpleRadio     radio;                                      // radio     object  (to play ICYcast streams)
SimpleWebServer myServer( SERVER_PORT);                     // server    object (to respond on API calls)
SimpleScheduler scheduler( 1000);                           // scheduler object (to process rotary + button handling)
SimpleRotary    rotary( A4, A5);                            // rotary    object (to change preset + volume)
SimpleButton    button( A3, false);                         // button    object (to switch between preset + volume setting)
Stopwatch       stopwatch( 10000);                          // stopwatch object (set for 10 sec)
PCD8544         lcd( 3, 4, 5, 9, A0);                       // lcd       object (to display station info)
// CLK, DI, DS, RESET, ENABLE
presetInfo presetData;                                      // preset    object (to hold station url or IP data)
byte       preset = 0;                                      // current preset playing
byte       volume = 0;                                      // current volume playing

void loadSettings();                                        // load preset + volume from EEPROM
void saveSettings();                                        // save preset + volume to   EEPROM
bool copyPreset( char*);                                    // copy url to presetData (but not to EEPROM)
bool loadPreset( int i);                                    // load presetData from EEPROM slot i
bool savePreset( int i, char* = NULL);                      // save presetData to   EERPOM slot i
void showStatus( int = 0);                                  // show preset + volume (on lcd)

void setup()
{
	Serial.begin( 9600);

  LINE( Serial, F( "--------------------"));                // show header
	LINE( Serial, F( "- Arduino WebRadio -"));
  LINE( Serial, F( "--------------------"));
  LINE( Serial, F("#"));

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
  lcd.begin(  84, 48);                                      // initialize lcd screen

  showStatus();                                             // show preset + volume

  LINE( Serial, F("# ready"));                              // show ready
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
        code = savePreset( atoi( chn) - 1, url) ? 200 : 400;
      }                                                     // save url in EEPROM
    }

    if (  myServer.path( 0, "webradio")) {                  // if player addressed
      if (( myServer.getMethod() == HTTP_GET) && !chn && url) {
        code = copyPreset( url) ? 200 : 400;                // load url (but don't store in EEPROM)
      }

      if (( myServer.getMethod() == HTTP_GET) && chn && !url) {
        code = loadPreset( preset = atoi( chn) - 1) ? 200 : 400;
      }                                                     // load preset from EEPROM

      if (( myServer.getMethod() == HTTP_GET) && vol) {
        radio.setVolume( 100 - ( volume = atoi( vol))); code = 200;
      }                                                     // set player volume

      if (( myServer.getMethod() == HTTP_GET) && myServer.arg( "play")) {
        radio.stopICYcastStream();
        code = radio.openICYcastStream( &presetData) ? 200 : 400;
      }                                                     // play url in memory

      if (( myServer.getMethod() == HTTP_GET) && myServer.arg( "stop")) {
        radio.stopICYcastStream(); code = 200;              // stop player
      }
    }

    myServer.response( code);                               // return response (headers + code)
  }

  if ( radio.available()) {                                 // if playing
    LCD1( lcd,  0, 0, radio.getName( 15));                  // show station name
    LCD3( lcd, 27, 1, F( "("), radio.getRate(), F( ")"));   // show station bit rate

    ATTR_( Serial, F( "# Station "), radio.getName());
    ATTR ( Serial, F( " / rate = "), radio.getRate());

    showStatus( mode);                                      // show preset + volume
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
    showStatus( mode);                                      // show preset + volume
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
    showStatus( mode);                                      // show preset + volume
  }

  stopwatch.check( saveSettings);                           // save Settings every 10 sec
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
bool savePreset( int preset, char* url)
{
  copyPreset( url);                                         // copy url (if provided)

  if (( preset >= 0) && ( preset < PRESET_MAX)) {           // if valid preset
    EEPROM.put( 2 + preset * sizeof( presetInfo), presetData);
                                                            // save presetData from EEPROM
    return true;                                            // return success
  } else {
    return false;                                           // return failure
  }
}

// show preset + volume
void showStatus( int mode)
{
  LCD2( lcd, 11, 3, F( "preset =  "), preset + 1  );        // show preset on LCD
  LCD2( lcd, 11, 4, F( "volume = " ), 100 - volume);        // show volume on LCD

  ATTR_( Serial, (mode == 0) ? F(  "> [preset] = ") : F(  ">  preset  = "),       preset + 1);
  ATTR ( Serial, (mode == 1) ? F( " / [volume] = ") : F( " /  volume  = "), 100 - volume    );
}
