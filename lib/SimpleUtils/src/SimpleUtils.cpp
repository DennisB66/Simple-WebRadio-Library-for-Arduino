// Copyright  : Dennis Buis (2017)
// License    : MIT
// Platform   : Arduino
// Library    : Simple Utils
// File       : SimpleUtils.h
// Purpose    : provide simple macros to replace more complex expressions
// Repository : https://github.com/DennisB66/

#include "SimpleUtils.h"

char* shiftL( char* s, char a, char b)
{
  char* ptr_s = strchr ( s    , a);
  char* ptr_e = strrchr( ptr_s, b ? b : a);

  if ( ptr_s == NULL) return s;
  if ( ptr_e == NULL) return s;

  *ptr_e = 0;

  for ( int i = 0; i < ptr_e - ptr_s; i++) {
    s[i] = ptr_s[i + 1];
  }

  return s;
}

Stopwatch::Stopwatch( unsigned long l)
{
  lapse( l);
}

void Stopwatch::lapse( unsigned long l)
{
  _lapse = l; reset();
}

void Stopwatch::reset()
{
  _ticks = millis() + _lapse;
}

bool Stopwatch::check( StopwatchFunc f)
{
  if ( millis() > _ticks) {
    if ( f) ( *f)();

    reset();

    return true;
  }

  return false;
}

// padding strings
const char* fill( const char* s, int w, bool c)           // width = available chars on display
{
  static char* line = NULL;                                 // name pointer
  int          size = min((int) strlen( s), w);
  int          padd = 0;

  if ( c) {
    padd = min(( w - size) / 2, w);                          //
  }

  if ( line) {                                               // if name is alread initialized
    line = (char*) realloc( line, ( w + 1) * sizeof( char)); // reallocate if needed
  } else {
    line = (char*)  malloc(       ( w + 1) * sizeof( char)); // allocate widt + 1 chars
  }

  if ( w) {                                                 // if width is provided
    memset ( line       , 32, w);                           // clear name with spaces
    memset ( line + w   ,  0, 1);                           // terminate name
    strncpy( line + padd,  s, size);                        // copy _name to name (centered)
    return   line;                                          // return centered name
  } else {
    return   s;                                             // return standard name
  }
}
