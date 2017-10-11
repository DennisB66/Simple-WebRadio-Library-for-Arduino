// Copyright  : Dennis Buis (2017)
// License    : MIT
// Platform   : Arduino
// Library    : Simple Utils
// File       : SimpleUtils.h
// Purpose    : provide simple macros to replace more complex expressions
// Repository : https://github.com/DennisB66/

#ifndef _SIMPLE_UTILS_H
#define _SIMPLE_UTILS_H

#include <Arduino.h>

#define minMax(A,B,C)     min(max(A,B),C)
#define strLen(A)         (strlen(A)  !=0)
#define strCmp(A,B)       (strcmp(A,B)==0)
#define strCpy(D,S,L)     strncpy(D,S,L-1);D[L-1]=0
#define strClr(D)         D[0]=0

char* shiftL( char*, char, char = 0);

typedef void ( *StopwatchFunc)();

class Stopwatch {
public:
  Stopwatch ( unsigned long);
  void lapse( unsigned long);
  void reset();
  bool check( StopwatchFunc f = nullptr);
private:
  unsigned long _lapse;
  unsigned long _ticks;
};

const char* fill( const char*, int, bool = false);

#endif
