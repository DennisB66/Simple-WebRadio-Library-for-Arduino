
#include <Arduino.h>
#include <EEPROM.h>
#include "SimpleWebRadio.h"
#include "SimpleUtils.h"
#include "SimplePrint.h"

const PresetInfo presetList[RADIO_PRESET_MAX] = {
  { "icecast.omroep.nl/radio1-bb-mp3",            {   0,   0,   0,   0}, 80 },
  { "icecast.omroep.nl/radio2-bb-mp3",            {   0,   0,   0,   0}, 80 },
  { "icecast.omroep.nl/3fm-bb-mp3",               {   0,   0,   0,   0}, 80 },
  { "icecast.omroep.nl/radio4-bb-mp3",            {   0,   0,   0,   0}, 80 },
  { "18973.live.streamtheworld.com/RADIO538.mp3", {   0,   0,   0,   0}, 80 },
  { "19993.live.streamtheworld.com/SKYRADIO_SC",  {   0,   0,   0,   0}, 80 },
  { "/",                                          {  91, 221, 151, 155}, 80 },
  { "icecast-bnr-cdp.triple-it.nl/bnr_mp3_96_04", {   0,   0,   0,   0}, 80 },
};

byte preset =  0;
byte volume = 50;

void preset2EEPROM()
{
  EEPROM.put( 0, preset);
  EEPROM.put( 1, volume);

  for ( int i = 0; i < RADIO_PRESET_MAX; i++) {
    EEPROM.put( 2 + i * sizeof( PresetInfo), presetList[ i]);
  }
}

void EEPROM2Preset()
{
  EEPROM.get( 0, preset);
  EEPROM.get( 0, volume);

  for ( int i = 0; i < RADIO_PRESET_MAX; i++) {
    EEPROM.get( 2 + i * sizeof( PresetInfo), presetList[ i]);
  }
}

void setup()
{
  Serial.begin( 9600);    				// Start serial port with 57600 bits per seconds
  PRINT( F( "--------------------------------------")) LF;
  PRINT( F( "-  Arduino WebRadio Preset to EEPROM -")) LF;
  PRINT( F( "-  V0.6                  (DennisB66) -")) LF;
  PRINT( F( "--------------------------------------")) LF;

  LABEL( F( "> Preset size ="), sizeof( presetList[ 0])) LF;

  preset2EEPROM();

  memset( (void*) presetList, 0, RADIO_PRESET_MAX * sizeof( PresetInfo));

  EEPROM2Preset();

  LABEL( F( "> Preset"), preset) LF;
  LABEL( F( "> Volume"), volume) LF;

  for ( int i = 0; i < RADIO_PRESET_MAX; i++) {
    LABEL( F( "> Preset url"), presetList[ i].url);
    LABEL( F( " at") , presetList[ i].ip4); LF;
  }
}

void loop()
{
}
