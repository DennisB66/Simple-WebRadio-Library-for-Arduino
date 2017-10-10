// Copyright  : Dennis Buis (2017)
// License    : MIT
// Platform   : Arduino
// Library    : Simple WebRadio Library for Arduino
// File       : SimpleWebRadio.h
// Purpose    : create a simple webradio playing ICYcast streams
// Repository : https://github.com/DennisB66/Simple-WebRadio-Library-for-Arduino

#ifndef _SIMPLE_RADIO_H
#define _SIMPLE_RADIO_H

#include <Arduino.h>

#include "Ethernet.h"
#include <SPI.h>
#include "Dns.h"
#include <VS1053.h>
#include <TimerOne.h>

#define PRESET_MAX 8                                        // max presets

#define PRESET_PATH_LENGTH 64                               // max path length
#define PRESET_NAME_LENGTH 20                               // max name length
#define PRESET_RATE_LENGTH  4                               // max rate length

#define ICY_BUFF_SIZE     400                               // play buffer length.

struct presetInfo {
  char url[PRESET_PATH_LENGTH];                             // preset HTTP url
  byte ip4[4];                                              // preset HTTP ip address
  word port;                                                // preset HTTP port
};

class SimpleRadio {                                         // SimpleRadio object
public:
  void setPlayer( uint8_t, uint8_t, uint8_t, uint8_t);      // initialize player

  char* getName();                                          // return station name
  char* getType();                                          // return station genre
  char* getRate();                                          // return station bit rate

  void         setVolume( int);                             // set player volume
  unsigned int getVolume();                                 // get player volume

  bool connected();                                         // true = station connected
  bool available();                                         // true = station data available

  bool openICYcastStream( presetInfo* preset);              // open ICYcast stream
  void stopICYcastStream();                                 // stop ICYcast stream
  void readICYcastStream();                                 // recieve ICYcast stream data
  void hndlICYcastStream();                                 // process ICYcast stream data

private:
  int           _volume;                                    // player volume

  char          _name[ PRESET_NAME_LENGTH];                 // station name
  char          _type[ PRESET_NAME_LENGTH];                 // station genre
  char          _rate[ PRESET_RATE_LENGTH];                 // station bit rate

  bool          _receivedHead;                              // true = station header processed
  bool          _receivedDisp;                              // true = station meta data available
  bool          _receivedData;                              // total size of received ICYcast stream data
  unsigned long _receivedSize;                              // last  size of received ICYcast stream data

  bool _findICYcastStream( const char*);                    // find label in ICYcast stream data
  bool _findICYcastStream( const char*, char*, int);        // find value in ICYcast stream data
};

#endif
