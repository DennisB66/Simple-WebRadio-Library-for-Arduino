// Copyright  : Dennis Buis (2017)
// License    : MIT
// Platform   : Arduino
// Library    : Simple WebRadio Library for Arduino
// File       : SimpleRadio.cpp
// Purpose    : create a simple webradio playing ICYcast streams
// Repository : https://github.com/DennisB66/Simple-WebRadio-Library-for-Arduino

#include "SimpleWebRadio.h"
#include "SimpleUtils.h"

#define NO_SIMPLE_WEBRADIO_DEBUG_L0
#define NO_SIMPLE_WEBRADIO_DEBUG_L1
#define NO_SIMPLE_WEBRADIO_DEBUG_L2
#define NO_SIMPLE_WEBRADIO_DEBUG_L3

VS1053*        player = NULL;                               // radio player object
EthernetClient client;                                      // HTTP  client object

byte playBuffer[ ICY_BUFF_SIZE];                            // ICYcast stream buffer

// initialize player
void SimpleRadio::setPlayer( byte _dreq_pin, byte _cs_pin, byte _dcs_pin, byte _reset_pin)
{
  SPI.begin();                                              // Start Serial Peripheral Interface (SPI)

  player = new VS1053( _cs_pin, _dcs_pin, _dreq_pin, _reset_pin);

  if ( player) {                                            // if player object exists
    player->begin();                                        // start player
    player->setVolume( _volume = 50);                       // Set the volume (default = 50)
  }
}

// return station name
char* SimpleRadio::getName( int width)                      // width = available chars on display
{
  static char* name = NULL;                                 // name pointer
  int          leng = max(( width - strlen( _name)) / 2, 0);
                                                            // leng = # free chars on both sides of _name
  if ( name) {                                              // if name is alread initialized
    name = (char*) realloc( name, ( width + 1) * sizeof( char));
  } else {                                                  // reallocate if needed
    name = (char*) malloc(        ( width + 1) * sizeof( char));
  }                                                         // allocate widt + 1 chars

  if ( width) {                                             // if width is provided
    memset ( name    , 32, width);                          // clear name with spaces
    memset ( name + width,  0, 1);                          // terminate name
    strncpy( name + leng, _name, min( (int) strlen( _name), width));
                                                            // copy _name to name (centered)
    return  name;                                           // return centered name
  } else {
    return _name;                                           // return standard name
  }
}

// return station (bit) rate
char* SimpleRadio::getRate()
{
  return _rate;
}

// set player volume
void SimpleRadio::setVolume( int v)
{
  if ( player) {                                            // if player object exists
    player->setVolume( _volume = minMax( v, 0, 255));       // set player volume
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
  return client.connected();                                // true = connected to ICYcast server
}

// true = station data available
bool SimpleRadio::available()
{
  bool   disp =  _receivedDisp; _receivedDisp = false;      // return true only once (before opening a new stream)
  return disp && _receivedHead;                             // true = station meta data available
}

// open ICYcast stream
bool SimpleRadio::openICYcastStream( presetInfo* preset)
{
  strcpy_P( _name, PSTR( "< ------ >"));                    // initialize station name
  strcpy_P( _rate, PSTR( "###"));                           // initialize station bit rate

  _receivedHead = false;                                    // false = ICYcast header not processed
  _receivedData = 0;                                        // no iCYcast stream data received so far

  if ( strlen( preset->url) > 0) {                          // url = host/path
    char* host = preset->url;                               //       ^ host
    char* path = strchr( preset->url, '/');                 //           ^ path

    if ( path) {                                            // if path found
      host[ path - preset->url] = 0;                        // terminate host string ('/' > '\0')
    } else {
      return false;                                         // no valid url found
    }

    #ifdef SIMPLE_WEBRADIO_DEBUG_L0                         // print debug info
    PRINT_ATTR( F( "host = "), host);
    PRINT_ATTR( F( "path = "), path + 1);
    #endif

    IPAddress hostIP( preset->ip4);                         // extract host IP from presetData

    if ( strlen( host) > 0) {                               // if host Found
      DNSClient dns;                                        // initialize DNS object
      dns.begin( Ethernet.dnsServerIP());                   // attach default DNS server
      dns.getHostByName( host, hostIP);                     // search for host IP
    }

    ATTR_( Serial, F( "> Radio station searching: "), preset->url);
    client.connect( hostIP, preset->port);                  // connect to ICYcast server

    if ( client.connected()) {                              // if connection is successful
      LINE_( Serial, F( " > "));

      client.print  ( F( "GET /" )); client.print  ( path + 1); client.println( F( " HTTP/1.0"));
      client.print  ( F( "Host: ")); client.println( host    );
      client.println( F( "Accept: */*"));
      client.println();                                     // send ICYcast streaming request

      LINE( Serial, F( "success"));

      #ifdef SIMPLE_WEBRADIO_DEBUG_L1                       // print debug info
      ADDR_( Serial, F( "server = "), hostIP);
      ATTR ( Serial, F( ":")        , preset->port);

      Serial.print  ( F("GET /"));  Serial.print  ( path + 1); Serial.println( F(" HTTP/1.0"));
      Serial.print  ( F("Host: ")); Serial.println( host    );
      Serial.println( F("Accept: */*"));
      Serial.println();

      LINE( Serial, F( "Radio station connection succes"));
      LINE( Serial, F(""));
      #endif
    } else {
      LINE( Serial, F(  " > failure"));

      #ifdef SIMPLE_WEBRADIO_DEBUG_L1
      LINE( Serial, F( "Radio station connection failed"));
      LINE( Serial, F(""));
      #endif
    }

    if ( path) {                                            // if url has been changed (by replacing '/' by '\0')
      host[ path - preset->url] = '/';                      // repair url (openICYcastStream has to be idempotent)
    }
  }

  return client.connected();                                // return true if connected to ICYcast server
}

// stop ICYcast stream
void SimpleRadio::stopICYcastStream()
{
  if ( player) player->stopSong();                          // stop radio player
  if ( client.connected()) client.stop();                   // stop HTTP  client
}

// recieve ICYcast stream data
void SimpleRadio::readICYcastStream()
{
  if ( client.available()) {                                // if ICYcast stream data available
    _receivedSize = client.read( playBuffer, ICY_BUFF_SIZE);
                                                            // read ICYcast stream data from server
    #ifdef SIMPLE_WEBRADIO_DEBUG_L3
    ATTR( Serial, F( "read buffer = "), _receivedSize);
    #endif
  }
}

// process ICYcast stream data
void SimpleRadio::hndlICYcastStream()
{
  if ( _receivedHead == false) {                            // if ICYcast header not processed
    _findICYcastStream( PSTR( "icy-name:"), _name, PRESET_NAME_LENGTH);
    _findICYcastStream( PSTR( "icy-br:"  ), _rate, PRESET_RATE_LENGTH);
                                                            // find station name + bit rate
    if ( _findICYcastStream( PSTR( "\r\n\r\n" ))) {         // find end of ICY cast header
      _receivedHead = true;                                 // true = header received
      _receivedDisp = true;                                 // true = station data available
    }
  } else {
    if ( player) {                                          // if player object created
      player->playChunk( playBuffer, _receivedSize);        // play ICYcast stream
    }
  }

  _receivedData += _receivedSize;                           // keep total bytes received
}

// find label in ICYcast stream data
bool SimpleRadio::_findICYcastStream( const char* label)
{
 return ( strstr_P(( const char *) playBuffer, label) > 0); // true = label found
}

// find value in ICYcast stream data
bool SimpleRadio::_findICYcastStream( const char* label, char* value, int size)
{
  char* s = strstr_P(( const char *) playBuffer, label);    // s = start of label
  char* e = strstr_P(( const char *) s,  PSTR( "\r\n"));    // e = start of eol (\r\n)

  if ( s && e) {                                            // if label + eol is found
    s += strlen_P( label);                                  // s points at first char after label
    while ( *s == ' ') s++;                                 // remove trailing spaces
    if ( s < e) {                                           // if eol is found for this label
      strCpy( value, s, min( e - s, size - 1));             // copy value

      return true;                                          // return success
    }
  }

  return false;                                             // return failure
}
