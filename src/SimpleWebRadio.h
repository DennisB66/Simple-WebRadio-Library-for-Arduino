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
#include <VS1053.h>
#include <TimerOne.h>

#define PRESET_MAX 8                                        // max presets

#define PRESET_PATH_LENGTH  64                              // max path length
#define PRESET_NAME_LENGTH  20                              // max name length
#define PRESET_RATE_LENGTH   4                              // max rate length
#define PRESET_SIZE_LENGTH   8                              // max size length
#define PRESET_META_LENGTH 128                              // max name length

#define ICY_BUFF_SIZE      400                              // play buffer length.

struct presetInfo {
  char url[PRESET_PATH_LENGTH];                             // preset HTTP url
  byte ip4[4];                                              // preset HTTP ip address
  word port;                                                // preset HTTP port
};

class SimpleRadio {                                         // SimpleRadio object
public:
  void  setPlayer( uint8_t, uint8_t, uint8_t, uint8_t);     // initialize player

  char* getName();                                          // return station name
  char* getType();                                          // return station genre
  char* getRate();                                          // return station bit rate

  void          setVolume( int);                            // set player volume
  unsigned int  getVolume();                                // get player volume

  bool connected();                                         // true = stream connected
  bool available();                                         // true = stream data available
  bool receiving();                                         // true = stream keeps active

  bool openICYcastStream( presetInfo* preset);              // open ICYcast stream
  void stopICYcastStream();                                 // stop ICYcast stream
  void readICYcastStream();                                 // recieve ICYcast stream data
  void hndlICYcastHeader();                                 // process ICYcast stream data
  void hndlICYcastStream();                                 // process ICYcast stream data

private:
  int           _volume;                                    // player volume

  char          _name[ PRESET_NAME_LENGTH];                 // stream name
  char          _type[ PRESET_NAME_LENGTH];                 // stream genre
  char          _rate[ PRESET_RATE_LENGTH];                 // stream bit rate
  char          _meta[ PRESET_META_LENGTH];                 // stream metadata

  unsigned int  _interval;                                  // total bytes to next metadata
  unsigned int  _dataLeft;                                  // left  bytes to next metadata
  unsigned int  _dataNext;                                  // next (requested) chunk size
  unsigned int  _dataLast;                                  // last (received)  chunk size

  bool          _dataHead;                                  // true = stream header processed
  bool          _dataDisp;                                  // true = new meta data available
  bool          _dataStop;                                  // true = stream time-out occured

  int   _findICYcastHeader( const char*);                   // find label in ICYcast stream data
  int   _findICYcastHeader( const char*, char*, int);       // find value in ICYcast stream data
  void  _playICYcastStream( unsigned int = 0, bool = false);
};                                                          // play stream data

#endif
