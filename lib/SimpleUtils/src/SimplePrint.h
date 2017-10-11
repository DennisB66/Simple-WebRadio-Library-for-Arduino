// Copyright  : Dennis Buis (2017)
// License    : MIT
// Platform   : Arduino
// Library    : Simple Utils
// File       : SimplePrint.h
// Purpose    : provide simple macros to replace regular print statements
// Repository : https://github.com/DennisB66/

#ifndef _SIMPLE_PRINT_H
#define _SIMPLE_PRINT_H

#include <Arduino.h>

#define LINE_(S,L)              S.print(L)
#define LINE(S,L)               S.println(L)

#define ATTR_(S,L,V)            S.print(L);S.print(V)
#define ATTR(S,L,V)             S.print(L);S.println(V)

#define ATTQ_(S,L,V)            S.print(L);S.print("'");S.print(V);S.print("'")
#define ATTQ(S,L,V)             S.print(L);S.print("'");S.print(V);S.println("'")

#define ADDR_(S,L,IP)           S.print(L);\
                                S.print(IP[0]);S.print(F("."));\
                                S.print(IP[1]);S.print(F("."));\
                                S.print(IP[2]);S.print(F("."));\
                                S.print(IP[3])
#define ADDR(S,L,IP)            S.print(L);\
                                S.print(IP[0]);S.print(F("."));\
                                S.print(IP[1]);S.print(F("."));\
                                S.print(IP[2]);S.print(F("."));\
                                S.print(IP[3]);S.println()

#define LCD1( L, X, Y, A)       L.setCursor(X,Y);L.print(A)
#define LCD2( L, X, Y, A, B)    L.setCursor(X,Y);L.print(A);L.print(B)
#define LCD3( L, X, Y, A, B, C) L.setCursor(X,Y);L.print(A);L.print(B);L.print(C)

#endif
