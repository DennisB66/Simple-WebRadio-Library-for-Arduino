#include <Arduino.h>
#include <EEPROM.h>
#include <Wire.h>
#include "LiquidCrystal_I2C.h"
#include "SimpleWebRadio.h"
#include "SimpleControl.h"

#include "SimpleUtils.h"
#include "SimplePrint.h"

#define VERBOSE_MODE
#define DEBUG_MODE

#define RADIO_STOP 0
#define RADIO_PLAY 1

byte macaddr[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };    // mac address
byte iplocal[] = { 192, 168,   1,  62 };                    // lan ip (e.g. "192.168.1.178")
byte gateway[] = { 192, 168,   1,   1 };                    // router gateway
byte subnet[]  = { 255, 255, 255,   0 };                    // subnet mask
byte DNS[]     = {   8,   8,   8,   8 };                    // DNS server (Google)

SimpleRadio       radio;                                    // radio     object  (to play ICYcast streams)
SimpleScheduler   scheduler( 1000);                         // scheduler object (to process rotary + button handling)
SimpleButton      button( A0, false);                       // button    object (to switch between preset + volume setting)
SimpleRotary      rotary( A1, A2);                          // rotary    object (to change preset + volume)
LiquidCrystal_I2C lcd( 0x3F, 20, 4);
PresetInfo presetData;                                      // preset    object (to hold station url or IP data)
byte       preset =  6;                                     // current preset playing
byte       volume = 70;                                     // current volume playing
byte       state  = RADIO_STOP;                             // start in silent mode

void loadSettings();                                        // load preset + volume from EEPROM
void saveSettings();                                        // save preset + volume to   EEPROM
void hndlPlayer();                                          // feed music player (vs1053b)
void hndlDevice();                                          // read input device (rotary + button)

bool copyPreset( char*);                                    // copy url to presetData (but not to EEPROM)
bool loadPreset( int i);                                    // load presetData from EEPROM slot i
bool savePreset( int i, char* = NULL);                      // save presetData to   EERPOM slot i
void initStatus();                                          // static info on lcd screen
void showStatus();                                          // update info on lcd screen

void setup()
{
	BEGIN( 9600);                                             // Serial at 9600

  #ifdef VERBOSE_MODE
  PRINT( F( "# ----------------------")) LF;                // show header
	PRINT( F( "# -  Arduino WebRadio  -")) LF;
  PRINT( F( "# -  V0.7 (DennisB66)  -")) LF;
  PRINT( F( "# ----------------------")) LF;
  PRINT( F( "#")) LF;
  #endif

  Ethernet.begin( macaddr, iplocal, DNS, gateway, subnet);  // start ethernet

  loadSettings();                                           // load preset + volume from EEPROM
  loadPreset( preset);                                      // load presetData from EEPROM slot preset

  #ifdef VERBOSE_MODE
  PRINT( F( "# client hosted at ")); PRINT( Ethernet.localIP()) LF;
  LABEL( F( "# preset"), preset);
  LABEL( F( " / volume"), volume);
  PRINT( F( " / preset url =")); QUOTE( presetData.url); PRINT( F( " at "));
  PRINT( presetData.ip4); PRINT( ':'); PRINT( presetData.port) LF;
  PRINT( F( "#")) LF;
  #endif

  radio.setPlayer( 2, 6, 7, 8);                             // initialize MP3 player
  radio.setVolume( volume);                                 // set volume of player

  rotary.setMinMax( 0, RADIO_PRESET_MAX - 1, true);         // set rotary boundaries
  rotary.setPosition( preset);                              // set rotary to preset

  scheduler.start();                                        // start checking ratary & button action

  initScreen();                                             // static info on lcd screen
  nextScreen();                                             // update info on lcd screen

  #ifdef VERBOSE_MODE
  PRINT( F( "# click button to switch on")) LF;
  #endif
}

void loop()
{
  static Stopwatch save( 10000, saveSettings);               // update EEPROM every  10 sec
  static Stopwatch disp(   500, showStatus);                 // update screen every 500 msec

  switch ( state) {
    case RADIO_PLAY:                                         // play station
      hndlPlayer();
      break;
    case RADIO_STOP:                                         // stop playing
      break;
  }

  hndlDevice();                                              // read input device (rotary + button)

  save.check();                                              // check if EEPROM needs update
  disp.check();                                              // check if screen needs update
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
  static bool mode = 0;                                     // 0 = change station / 1 = change volume

  if ( button.available()) {                                // if button processed
    switch ( button.read()) {                               // read button value
    case BUTTON_NORMAL :                                    // if normal press
      lcd.backlight();                                      // switch on lcd backlight
      state = RADIO_PLAY;                                   // play radio
      break;
    case BUTTON_HOLD :                                      // if press & hold
      lcd.noBacklight();                                    // switch off lcd backlight
      state = RADIO_STOP;                                   // stop radio
      break;
    case BUTTON_DOUBLE :                                    // if double press
      mode = !mode;                                         // toggle mode (0 = preset / 1 = volume)
      switch (mode) {
      case 0 :                                              // allow preset selection
        rotary.setMinMax( 0, RADIO_PRESET_MAX - 1, true);   // set proper rotary boundaries
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
    strCpy( presetData.url, url, PRESET_PATH_LENGTH);       // copy url to presetData

    return true;                                            // return success
  } else {
    return false;                                           // return failure
  }
}

// load presetData from EEPROM
bool loadPreset( int preset)
{
  PresetInfo data;                                          // preset buffer (note: includes 2 extra IOstream bytes)

  if (( preset >= 0) && ( preset < RADIO_PRESET_MAX)) {     // if valid preset
    EEPROM.get( 2 + preset * sizeof( PresetInfo), data);    // load preset data from EEPROM

    strcpy( presetData.url, data.url);                      // copy preset url
    presetData.ip4  = data.ip4 ;                            // copy preset ip4
    presetData.port = data.port;                            // copy preset port

    return true;                                            // return success
  } else {
    return false;                                           // return failure
  }
}

// save presetData to EEPROM
bool savePreset( int preset, char* url)
{
  copyPreset( url);                                         // copy url (if provided)

  if (( preset >= 0) && ( preset < RADIO_PRESET_MAX)) {     // if valid preset
    EEPROM.put( 2 + preset * sizeof( PresetInfo), presetData);
                                                            // save presetData from EEPROM
    return true;                                            // return success
  } else {
    return false;                                           // return failure
  }
}

// initialize lcd
void initStatus()
{
  lcd.begin();                                              // initialize lcd

  LCD1( lcd,  0,  0, fill( "< ---------- >", 20, true));    // show empty name
  LCD1( lcd,  0,  1, fill( ""              , 20, true));    // show empty info
  LCD1( lcd,  4,  2, F( "Bit Rate" ));                      // show bit rate
  LCD1( lcd,  0,  3, F( "CHANNEL ="));                      // show preset
  LCD1( lcd, 12,  3, F( "VOL ="    ));                      // show volume
}

// update lcd
void showStatus()
{
  static char label[2] = { ' ', '-'};                       // heart beat symbols
  static int  cnt = 0;                                      // info field scroll position
  static int  len = 0;                                      // info field scroll size

  if ( radio.available()) {                                 // if playing
    LCD1( lcd,  0, 0, fill( radio.getName(), 20, true));    // show station name
    LCD1( lcd,  0, 1, fill( radio.getInfo(), 20, true));    // show station genre
    LCD1( lcd, 13, 2, fill( radio.getRate(),  3));          // show station bit rate

    #ifdef VERBOSE_MODE
    LABEL( F( "# name"), radio.getName());
    LABEL( F(  " info"), radio.getInfo());
    LABEL( F(  " rate"), radio.getRate()); LF;
    #endif

    len = max( 0, strlen( radio.getInfo()) - 20);           // info field display scroll size
  }

  LCD1( lcd,  2, 2, radio.receiving() ? label[cnt % 2] : label[0]);
  LCD1( lcd, 17, 2, radio.receiving() ? label[cnt % 2] : label[0]);
                                                            // show heart beat
  LCD1( lcd, 10, 3, preset + 1  );                          // show preset on LCD
  LCD1( lcd, 18, 3, 100 - volume);                          // show volume on LCD

  if ( len > 0) {                                           // if scrolling needed
    LCD1( lcd,  0, 1, fill( radio.getInfo() + minMax( cnt - 2, 0, len), 20));
  }                                                         // scroll station info

  cnt %= ( len + 4); cnt++;                                 // update scroll postion
}
