
#include <Arduino.h>
#include <EEPROM.h>
#include "SimpleWebRadio.h"
#include "SimpleUtils.h"

#include <Ethernet.h>
#include <VS1053.h>
#include <SPI.h>
#include <TimerOne.h>

const presetInfo presetList[PRESET_MAX] = {
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

  for ( int i = 0; i < PRESET_MAX; i++) {
    EEPROM.put( 2 + i * sizeof( presetInfo), presetList[ i]);
  }
}

void EEPROM2Preset()
{
  EEPROM.get( 0, preset);
  EEPROM.get( 0, volume);

  for ( int i = 0; i < PRESET_MAX; i++) {
    EEPROM.get( 2 + i * sizeof( presetInfo), presetList[ i]);
  }
}

void setup()
{
  Serial.begin( 9600);    				// Start serial port with 57600 bits per seconds
  PRINT_LINE();
  PRINT_LINE( F( "Arduino WebRadio Preset to EEPROM V0.2"));
  PRINT_LINE();

  preset2EEPROM();

  memset( (void*) presetList, 0, PRESET_MAX * sizeof( presetInfo));

  EEPROM2Preset();

  PRINT_ATTR( F( "Preset = "), preset);
  PRINT_ATTR( F( "Volume = "), volume);
  PRINT_LINE();

  for ( int i = 0; i < PRESET_MAX; i++) {
    PRINT_ATTR( F( "Preset url = "), presetList[ i].url);
  }

  PRINT_LINE();
}

void loop()
{
}
