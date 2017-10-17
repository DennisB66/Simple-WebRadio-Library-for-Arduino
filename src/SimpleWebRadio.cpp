// Copyright  : Dennis Buis (2017)
// License    : MIT
// Platform   : Arduino
// Library    : Simple WebRadio Library for Arduino
// File       : SimpleRadio.cpp
// Purpose    : create a simple webradio playing ICYcast streams
// Repository : https://github.com/DennisB66/Simple-WebRadio-Library-for-Arduino

#include <Arduino.h>
#include "SimpleWebRadio.h"
#include "SimpleUtils.h"
#include "SimplePrint.h"

#define NO_SIMPLE_WEBRADIO_DEBUG_L0
#define NO_SIMPLE_WEBRADIO_DEBUG_L1
#define NO_SIMPLE_WEBRADIO_DEBUG_L2
#define NO_SIMPLE_WEBRADIO_DEBUG_L3

VS1053*        player = NULL;                               // radio player object
EthernetClient client;                                      // HTTP  client object

char playBuffer[ ICY_BUFF_SIZE];                            // ICYcast stream buffer

// initialize player
void SimpleRadio::setPlayer( byte _dreq_pin, byte _cs_pin, byte _dcs_pin, byte _reset_pin)
{
  SPI.begin();                                              // Start Serial Peripheral Interface (SPI)

  player = new VS1053( _cs_pin, _dcs_pin, _dreq_pin, _reset_pin);

  if ( player) {                                            // if player object created
    player->begin();                                        // start player
    player->setVolume( _volume = 50);                       // Set the volume (default = 50)
  }
}

// return station name
char* SimpleRadio::getName()
{
  return _name;                                             // return station name
}

// return station genre
char* SimpleRadio::getType()
{
  return _type;                                             // return station genre
}

// return station (bit) rate
char* SimpleRadio::getRate()
{
  return _rate;                                             // return station (bit) rate
}

// return station info
char* SimpleRadio::getInfo()
{
  return _info;                                             // return station info
}

// set volume
void SimpleRadio::setVolume( int v)
{
  if ( player) {                                            // if player object created
    player->setVolume( _volume = minMax( v, 0, 255));       // set volume (0 = loud, 255 = silent)
  }
}

// get player volume
unsigned int SimpleRadio::getVolume()
{
  return _volume;                                           // return player volume
}

// true = station connected
bool SimpleRadio::connected()
{
  return client.connected() & _dataHead;                    // true = connected & ICYcast header received
}

// true = station connected
bool SimpleRadio::receiving()
{
  return _dataStop == false;                                // true = ICYcast data stream stopped
}

// true = station header or (new) info data available
bool SimpleRadio::available()
{
  bool   disp =  _dataDisp; _dataDisp = false;              // reset after checking (= disply once)
  return disp && _dataHead;                                 // true = station meta data available
}

// open ICYcast stream
bool SimpleRadio::openICYcastStream( PresetInfo* preset)
{
  //PRINT( F( "> openICYcastStream")) LF;

  strcpy_P( _name, PSTR( "< ---------- >"));                // initialize station name
  strcpy_P( _info, PSTR( "< ---------- >"));                // initialize station info
  strcpy_P( _rate, PSTR( "###"));                           // initialize station bit rate


  if ( strlen( preset->url) > 0) {                          // url = host/path
    char* host = preset->url;                               //       ^ host
    char* path = strchr( preset->url, '/');                 //           ^ path

    if ( path) {                                            // if path found
      host[ path - preset->url] = 0;                        // terminate host string ('/' = '\0')
    } else {
      return false;                                         // no valid url found
    }

    IPAddress hostIP( preset->ip4);                         // extract host IP from presetData

    PRINT( F( "# searching "));
    if ( strlen( host) > 0) {
      //#ifdef SIMPLE_WEBRADIO_DEBUG_L0                       // print debug info
      LABEL( F( "host "), host);
      LABEL( F( "/")    , path + 1);
      //#endif

      client.connect( host, preset->port);                // connect to ICYcast server
    } else {
      //#ifdef SIMPLE_WEBRADIO_DEBUG_L0                       // print debug info
      VALUE( F( "host"), hostIP);
      VALUE( F( ":")   , preset->port);
      //#endif

      client.connect( hostIP, preset->port);                // connect to ICYcast server
    }

    if ( client.connected()) {                              // if connection is successful
      client.print  ( F( "GET /" )); client.print  ( path + 1); client.println( F( " HTTP/1.0"));
      client.print  ( F( "Host: ")); client.println( host    );
      client.println( F( "Icy-MetaData: 1"));
      client.println( F( "Accept: */*"));
    //client.println( F( "Connection: close"));
      client.println();                                     // send ICYcast streaming request

      PRINT( F( "> success!")) LF;
    } else {
      PRINT( F( "> failure!")) LF;
    }

    if ( path) {                                            // if url has been changed (by replacing '/' by '\0')
      host[ path - preset->url] = '/';                      // repair url (openICYcastStream has to be idempotent)
    }
  }

  _interval =  ICY_BUFF_SIZE;
  _dataNext = _interval;
  _dataLeft = _interval;
  _dataHead = false;                                    // false = ICYcast header not processed

  delay( 500);

  return client.connected();                                // return true if connected to ICYcast server
}

// stop ICYcast stream
void SimpleRadio::stopICYcastStream()
{
  // PRINT( F( "> stopICYcastStream")) lF;

  if ( player) player->stopSong();                          // stop radio player
  client.stop();                                            // disconnect from ICYcast server
}

// recieve ICYcast stream data
void SimpleRadio::readICYcastStream()
{
  static Stopwatch sw( 5000); if (sw.check()) _dataStop = true;

  if ( client.connected() && client.available()) {                                // if ICYcast stream data available
    _dataLast = client.read((uint8_t*) playBuffer, _dataNext);
                                                            // read ICYcast stream data from server
    if ( _dataLast > 0) {
      _dataStop = false;
      sw.reset();
    }

    // VALUE( F( "> readICYcastStream > total = "), _dataLeft);
    // VALUE( F( " / count = "), _dataNext)   ;
    // VALUE( F( " / "        ), _dataLast) LF;
  } else {
    _dataLast = 0;
  }

}

// process ICYcast stream data
void SimpleRadio::hndlICYcastHeader()
{
  // VALUE( F( "> hndlICYcastHeader > tot = "), _dataLeft);
  // VALUE( F( " / req = "), _dataNext)   ;
  // VALUE( F( " / rec = "), _dataLast) LF;

  char iVal[PRESET_SIZE_LENGTH]; strcpy( iVal, "0");

  bool found = false;
  found |= _findICYcastHeader( PSTR( "icy-name:"   ), _name, PRESET_NAME_LENGTH);
  found |= _findICYcastHeader( PSTR( "icy-genre:"  ), _type, PRESET_NAME_LENGTH);
  found |= _findICYcastHeader( PSTR( "icy-br:"     ), _rate, PRESET_RATE_LENGTH);
  found |= _findICYcastHeader( PSTR( "icy-metaint:"),  iVal, PRESET_SIZE_LENGTH);                                                            // find station name + bit rate

  _interval = atoi( iVal);

  _dataHead = found;                               // true = header received
  _dataDisp = found;

  #ifdef SIMPLE_WEBRADIO_DEBUG_L1
  VALUE( F( "name = "), _name);
  VALUE( F( "type = "), _type);
  VALUE( F( "rate = "), _rate);
  VALUE( F( "interval = "), iVal) LF;
  #endif

  int skip = _findICYcastHeader( PSTR( "\r\n\r\n"));

  // VALUE( F( "> hndlICYcastHeader > header length = "), skip + 4) LF;

  if ( skip) {
    _playICYcastStream( skip + 4, true);
  }
}

void SimpleRadio::hndlICYcastStream()
{
  // VALUE( F( "> hndlICYcastStream > tot = "), _dataLeft);
  // VALUE( F( " / req = "), _dataNext)   ;
  // VALUE( F( " / rec = "), _dataLast) LF;

  if (( _dataLast > 0) && ( _dataLeft != 0)) {
    _playICYcastStream();
  } else
  if (( _dataLast > 0) && ( _dataLeft == 0)) {
    int skip = (int) ( playBuffer[0] * 16);

    if ( skip > 0) {
      strCpy( _info, playBuffer + 1, min( skip, PRESET_META_LENGTH));
      shiftL( _info, 39);

      _dataDisp = true;

      #ifdef SIMPLE_WEBRADIO_DEBUG_L1
      VALUE( "> metadata = ", skip);
      VALUE( " / ", _info);
      #endif
    } else {

    }

    _playICYcastStream( skip + 1, true);
  }

}

void SimpleRadio::_playICYcastStream( unsigned int skip, bool reset)
{
  // VALUE( F( "> playICYcastStream > tot = "), _dataLeft);
  // VALUE( F( " / req = "), _dataNext)   ;
  // VALUE( F( " / rec = "), _dataLast) LF;

  if ( player) {
    player->playChunk((uint8_t*) playBuffer + skip, _dataLast - skip);   // play ICYcast stream
  }

  if ( skip > 0) {
    _dataLeft = (  _interval > 0) ? _interval : ICY_BUFF_SIZE;
  }                                                         // reset dataLeft to next interval

  _dataLeft -= ( _dataLast - skip);                         // decrease dataLeft with bytes played
  _dataNext = (( _dataLeft > 0) && ( _dataLeft < ICY_BUFF_SIZE)) ? _dataLeft : ICY_BUFF_SIZE;
}                                                           // set next chunk size

// find label in ICYcast stream data
int SimpleRadio::_findICYcastHeader( const char* label)
{
  char*  p = strstr_P((const char *) playBuffer, label);
                                                            // ptr to label
  return p ? p - playBuffer : 0;                            // return position of label
}

int SimpleRadio::_findICYcastHeader( const char* label, char* value, int size)
{
  char* s = strstr_P((const char *) playBuffer, label);    // s = start of label
  char* e = strstr_P((const char *) s,  PSTR( "\r\n"));    // e = start of eol (\r\n)

  if ( s && e) {                                            // if label + eol is found
    s += strlen_P( label);                                  // s points at first char after label
    while ( *s == ' ') s++;                                 // remove trailing spaces
    if ( s < e) {                                           // if eol is found for this label
      strCpy( value, s, min( e + 1 - s, size));             // copy value

      return s - playBuffer;                                // return position of label
    }
  }

  return 0;                                                 // return failure
}
