// Copyright  : Dennis Buis (2017)
// License    : MIT
// Platform   : Arduino
// Library    : htTP utility library
// File       : htTPutils.h
// Purpose    : ...
// Repository : ...

#include "SimpleHTTP.h"

// get HTTP method from HTTP method string
HTTPMethod HTTP_Method( const char* method)
{
  if ( strcmp_P( method, PSTR( "GET"    )) == 0) return HTTP_GET;
  if ( strcmp_P( method, PSTR( "HEAD"   )) == 0) return HTTP_HEAD;
  if ( strcmp_P( method, PSTR( "POST"   )) == 0) return HTTP_POST;
  if ( strcmp_P( method, PSTR( "PUT"    )) == 0) return HTTP_PUT;
  if ( strcmp_P( method, PSTR( "DELETE" )) == 0) return HTTP_DELETE;
  if ( strcmp_P( method, PSTR( "CONNECT")) == 0) return HTTP_CONNECT;
  if ( strcmp_P( method, PSTR( "OPTIONS")) == 0) return HTTP_OPTIONS;
  if ( strcmp_P( method, PSTR( "PATCH"  )) == 0) return HTTP_PATCH;
  return HTTP_ANY;
}

// get HTTP method string from HTTP method
const char* HTTP_Method( HTTPMethod method)
{
  switch ( method) {
    case HTTP_GET     : return "GET";
    case HTTP_HEAD    : return "HEAD";
    case HTTP_POST    : return "POST";
    case HTTP_PUT     : return "PUT";
    case HTTP_DELETE  : return "DELETE";
    case HTTP_CONNECT : return "CONNECT";
    case HTTP_PATCH   : return "PATCH";
    case HTTP_OPTIONS : return "OPTIONS";
    default           : return "GET";
  }
}

// get HTTP code response message
const char* HTTP_CodeMessage( int code) {
  switch ( code) {
    case 100: return PSTR( "Continue");
    case 101: return PSTR( "Switching Protocols");
    case 200: return PSTR( "OK");
    case 201: return PSTR( "Created");
    case 202: return PSTR( "Accepted");
    case 203: return PSTR( "Non-Authoritative Information");
    case 204: return PSTR( "No Content");
    case 205: return PSTR( "Reset Content");
    case 206: return PSTR( "Partial Content");
    case 300: return PSTR( "Multiple Choices");
    case 301: return PSTR( "Moved Permanently");
    case 302: return PSTR( "Found");
    case 303: return PSTR( "See Other");
    case 304: return PSTR( "Not Modified");
    case 305: return PSTR( "Use Proxy");
    case 307: return PSTR( "Temporary Redirect");
    case 400: return PSTR( "Bad Request");
    case 401: return PSTR( "Unauthorized");
    case 402: return PSTR( "Payment Required");
    case 403: return PSTR( "Forbidden");
    case 404: return PSTR( "Not Found");
    case 405: return PSTR( "Method Not Allowed");
    case 406: return PSTR( "Not Acceptable");
    case 407: return PSTR( "Proxy Authentication Required");
    case 408: return PSTR( "Request Time-out");
    case 409: return PSTR( "Conflict");
    case 410: return PSTR( "Gone");
    case 411: return PSTR( "Length Required");
    case 412: return PSTR( "Precondition Failed");
    case 413: return PSTR( "Request Entity Too Large");
    case 414: return PSTR( "Request-URI Too Large");
    case 415: return PSTR( "Unsupported Media Type");
    case 416: return PSTR( "Requested range not satisfiable");
    case 417: return PSTR( "Expectation Failed");
    case 500: return PSTR( "Internal Server Error");
    case 501: return PSTR( "Not Implemented");
    case 502: return PSTR( "Bad Gateway");
    case 503: return PSTR( "Service Unavailable");
    case 504: return PSTR( "Gateway Time-out");
    case 505: return PSTR( "HTTP Version not supported");
    default:  return PSTR( "");
  }
}

// url character conversion
char* HTTP_urlDecode( char* line)
{
  char hex[] = "0x00";                            // decode template
  int  l     = strlen( line);                     // total chars to be decoded
  int  i     = 0;                                 // encode char index
  int  d     = 0;                                 // decode char index

  while ( i <= l) {                               // include '/0' terminator
    char enc = line[i++];                         // encoded char
    char dec;                                     // decoded char

    if (( enc == '%') && ( i + 1 < l)) {          // found encoded char
      hex[2] = line[i++];                         // read 1st hex digit
      hex[3] = line[i++];                         // read 2nd hex digit

      dec = (char) strtol( hex, NULL, 16);        // convert hex to char
    } else {
      dec = ( enc == '+') ? ' ' : enc;            // convert '+' to space
    }

    line[d++] = dec;                              // save decoded char
  }

	return line;
}
