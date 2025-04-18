#ifndef EXTENSIONFUNCTION_H_
#define EXTENSIONFUNCTION_H_
#pragma once

/*
This library will provide common mathematical and string functions in
SQL queries using the operating system libraries or provided
definitions.  It includes the following functions:

Math: acos, asin, atan, atn2, atan2, acosh, asinh, atanh, difference,
degrees, radians, cos, sin, tan, cot, cosh, sinh, tanh, coth, exp,
log, log10, power, sign, sqrt, square, ceil, floor, pi.

String: replicate, charindex, leftstr, rightstr, ltrim, rtrim, trim,
replace, reverse, proper, padl, padr, padc, strfilter.

Aggregate: stdev, variance, mode, median, lower_quartile,
upper_quartile.

The string functions ltrim, rtrim, trim, replace are included in
recent versions of SQLite and so by default do not build.

Compilation instructions:
 Compile this C source file into a dynamic library as follows:
 * Linux:
   gcc -fPIC -lm -shared extension-functions.c -o libsqlitefunctions.so
 * Mac OS X:
   gcc -fno-common -dynamiclib extension-functions.c -o libsqlitefunctions.dylib
 (You may need to add flags
  -I /opt/local/include/ -L/opt/local/lib -lsqlite3
  if your sqlite3 is installed from Mac ports, or
  -I /sw/include/ -L/sw/lib -lsqlite3
  if installed with Fink.)
 * Windows:
  1. Install MinGW (http://www.mingw.org/) and you will get the gcc
  (gnu compiler collection)
  2. add the path to your path variable (isn't done during the
   installation!)
  3. compile:
   gcc -shared -I "path" -o libsqlitefunctions.so extension-functions.c
   (path = path of sqlite3ext.h; i.e. C:\programs\sqlite)

Usage instructions for applications calling the sqlite3 API functions:
  In your application, call sqlite3_enable_load_extension(db,1) to
  allow loading external libraries.  Then load the library libsqlitefunctions
  using sqlite3_load_extension; the third argument should be 0.
  See http://www.sqlite.org/cvstrac/wiki?p=LoadableExtensions.
  Select statements may now use these functions, as in
  SELECT cos(radians(inclination)) FROM satsum WHERE satnum = 25544;

Usage instructions for the sqlite3 program:
  If the program is built so that loading extensions is permitted,
  the following will work:
   sqlite> SELECT load_extension('./libsqlitefunctions.so');
   sqlite> select cos(radians(45));
   0.707106781186548
  Note: Loading extensions is by default prohibited as a
  security measure; see "Security Considerations" in
  http://www.sqlite.org/cvstrac/wiki?p=LoadableExtensions.
  If the sqlite3 program and library are built this
  way, you cannot use these functions from the program, you
  must write your own program using the sqlite3 API, and call
  sqlite3_enable_load_extension as described above, or else
  rebuilt the sqlite3 program to allow loadable extensions.

Alterations:
The instructions are for Linux, Mac OS X, and Windows; users of other
OSes may need to modify this procedure.  In particular, if your math
library lacks one or more of the needed trig or log functions, comment
out the appropriate HAVE_ #define at the top of file.  If you do not
wish to make a loadable module, comment out the define for
COMPILE_SQLITE_EXTENSIONS_AS_LOADABLE_MODULE.  If you are using a
version of SQLite without the trim functions and replace, comment out
the HAVE_TRIM #define.

Liam Healy

History:
2010-01-06 Correct check for argc in squareFunc, and add Windows
compilation instructions.
2009-06-24 Correct check for argc in properFunc.
2008-09-14 Add check that memory was actually allocated after
sqlite3_malloc or sqlite3StrDup, call sqlite3_result_error_nomem if
not.  Thanks to Robert Simpson.
2008-06-13 Change to instructions to indicate use of the math library
and that program might work.
2007-10-01 Minor clarification to instructions.
2007-09-29 Compilation as loadable module is optional with
COMPILE_SQLITE_EXTENSIONS_AS_LOADABLE_MODULE.
2007-09-28 Use sqlite3_extension_init and macros
SQLITE_EXTENSION_INIT1, SQLITE_EXTENSION_INIT2, so that it works with
sqlite3_load_extension.  Thanks to Eric Higashino and Joe Wilson.
New instructions for Mac compilation.
2007-09-17 With help from Joe Wilson and Nuno Luca, made use of
external interfaces so that compilation is no longer dependent on
SQLite source code.  Merged source, header, and README into a single
file.  Added casts so that Mac will compile without warnings (unsigned
and signed char).
2007-09-05 Included some definitions from sqlite 3.3.13 so that this
will continue to work in newer versions of sqlite.  Completed
description of functions available.
2007-03-27 Revised description.
2007-03-23 Small cleanup and a bug fix on the code.  This was mainly
letting errno flag errors encountered in the math library and checking
the result, rather than pre-checking.  This fixes a bug in power that
would cause an error if any non-positive number was raised to any
power.
2007-02-07 posted by Mikey C to sqlite mailing list.
Original code 2006 June 05 by relicoder.

*/

namespace missionx
{
  #include <inttypes.h>
  #include "../../../src/core/MxUtils.h" // missionx - saar


//#include "config.h"

//#define COMPILE_SQLITE_EXTENSIONS_AS_LOADABLE_MODULE 1
#define HAVE_ACOSH 1
#define HAVE_ASINH 1
#define HAVE_ATANH 1
#define HAVE_SINH 1
#define HAVE_COSH 1
#define HAVE_TANH 1
#define HAVE_LOG10 1
#define HAVE_ISBLANK 1
#define SQLITE_SOUNDEX 1
#define HAVE_TRIM 1		/* LMH 2007-03-25 if sqlite has trim functions */

#ifdef COMPILE_SQLITE_EXTENSIONS_AS_LOADABLE_MODULE
#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1
#else
#include "sqlite3.h"
#endif

#include <ctype.h>
/* relicoder */
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>		/* LMH 2007-03-25 */

#include <stdlib.h>
#include <assert.h>

#ifndef _MAP_H_
#define _MAP_H_

#include <stdint.h>

/*
** Simple binary tree implementation to use in median, mode and quartile calculations
** Tree is not necessarily balanced. That would require something like red&black trees of AVL
*/

typedef int(*cmp_func)(const void*, const void*);
typedef void(*map_iterator)(void*, int64_t, void*);

typedef struct _xx_node {
  struct _xx_node* l;
  struct _xx_node* r;
  void* data;
  int64_t count;
} xx_node;

typedef struct _xx_map {
  xx_node* base;
  cmp_func cmp;
  short free;
} xx_map;

/*
** creates a xx_map given a comparison function
*/
xx_map map_make(cmp_func cmp);

/*
** inserts the element e into xx_map m
*/
void map_insert(xx_map* m, void* e);

/*
** executes function iter over all elements in the xx_map, in key increasing order
*/
void map_iterate(xx_map* m, map_iterator iter, void* p);

/*
** frees all memory used by a xx_map
*/
void map_destroy(xx_map* m);

/*
** compares 2 integers
** to use with map_make
*/
int int_cmp(const void* a, const void* b);

/*
** compares 2 doubles
** to use with map_make
*/
int double_cmp(const void* a, const void* b);

#endif /* _MAP_H_ */

typedef uint8_t         u8;
typedef uint16_t        u16;
typedef int64_t         i64;

static char* sqlite3StrDup(const char* z) {
  //char* res = sqlite3_malloc(strlen(z) + 1);
  char* res = (char*)sqlite3_malloc( (int)(strlen(z) + 1) );
  return strcpy(res, z);
}

/*
** These are copied verbatim from fun.c so as to not have the names exported
*/

/* LMH from sqlite3 3.3.13 */
/*
** This table maps from the first byte of a UTF-8 character to the number
** of trailing bytes expected. A value '4' indicates that the table key
** is not a legal first byte for a UTF-8 character.
*/
static const u8 xtra_utf8_bytes[256] = {
  /* 0xxxxxxx */
  0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,

  /* 10wwwwww */
  4, 4, 4, 4, 4, 4, 4, 4,     4, 4, 4, 4, 4, 4, 4, 4,
  4, 4, 4, 4, 4, 4, 4, 4,     4, 4, 4, 4, 4, 4, 4, 4,
  4, 4, 4, 4, 4, 4, 4, 4,     4, 4, 4, 4, 4, 4, 4, 4,
  4, 4, 4, 4, 4, 4, 4, 4,     4, 4, 4, 4, 4, 4, 4, 4,

  /* 110yyyyy */
  1, 1, 1, 1, 1, 1, 1, 1,     1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,     1, 1, 1, 1, 1, 1, 1, 1,

  /* 1110zzzz */
  2, 2, 2, 2, 2, 2, 2, 2,     2, 2, 2, 2, 2, 2, 2, 2,

  /* 11110yyy */
  3, 3, 3, 3, 3, 3, 3, 3,     4, 4, 4, 4, 4, 4, 4, 4,
};


/*
** This table maps from the number of trailing bytes in a UTF-8 character
** to an integer constant that is effectively calculated for each character
** read by a naive implementation of a UTF-8 character reader. The code
** in the READ_UTF8 macro explains things best.
*/
static const int xtra_utf8_bits[] = {
  0,
  12416,          /* (0xC0 << 6) + (0x80) */
  925824,         /* (0xE0 << 12) + (0x80 << 6) + (0x80) */
  63447168        /* (0xF0 << 18) + (0x80 << 12) + (0x80 << 6) + 0x80 */
};

/*
** If a UTF-8 character contains N bytes extra bytes (N bytes follow
** the initial byte so that the total character length is N+1) then
** masking the character with utf8_mask[N] must produce a non-zero
** result.  Otherwise, we have an (illegal) overlong encoding.
*/
static const int utf_mask[] = {
  0x00000000,
  (int)0xffffff80,
  (int)0xfffff800,
  (int)0xffff0000,
};

/* LMH salvaged from sqlite3 3.3.13 source code src/utf.c */
#define READ_UTF8(zIn, c) { \
  int xtra;                                            \
  c = *(zIn)++;                                        \
  xtra = xtra_utf8_bytes[c];                           \
  switch( xtra ){                                      \
    case 4: c = (int)0xFFFD; break;                    \
case 3: c = (c<<6) + *(zIn)++; [[fallthrough]];        \
case 2: c = (c<<6) + *(zIn)++; [[fallthrough]];        \
case 1: c = (c<<6) + *(zIn)++;                         \
    c -= xtra_utf8_bits[xtra];                         \
    if( (utf_mask[xtra]&c)==0                          \
        || (c&0xFFFFF800)==0xD800                      \
        || (c&0xFFFFFFFE)==0xFFFE ){  c = 0xFFFD; }    \
  }                                                    \
}

static int sqlite3ReadUtf8(const unsigned char* z) {
  int c;
  READ_UTF8(z, c);
  return c;
}

#define SKIP_UTF8(zIn) {                               \
  zIn += (xtra_utf8_bytes[*(u8 *)zIn] + 1);            \
}

/*
** pZ is a UTF-8 encoded unicode string. If nByte is less than zero,
** return the number of unicode characters in pZ up to (but not including)
** the first 0x00 byte. If nByte is not less than zero, return the
** number of unicode characters in the first nByte of pZ (or up to
** the first 0x00, whichever comes first).
*/
static int sqlite3Utf8CharLen(const char* z, int nByte) {
  int r = 0;
  const char* zTerm;
  if (nByte >= 0) {
    zTerm = &z[nByte];
  }
  else {
    zTerm = (const char*)(-1);
  }
  assert(z <= zTerm);
  while (*z != 0 && z < zTerm) {
    SKIP_UTF8(z);
    r++;
  }
  return r;
}

/*
** X is a pointer to the first byte of a UTF-8 character.  Increment
** X so that it points to the next character.  This only works right
** if X points to a well-formed UTF-8 string.
*/
#define sqliteNextChar(X)  while( (0xc0&*++(X))==0x80 ){}
#define sqliteCharVal(X)   sqlite3ReadUtf8(X)

/*
** This is a macro that facilitates writting wrappers for math.h functions
** it creates code for a function to use in SQlite that gets one numeric input
** and returns a floating point value.
**
** Could have been implemented using pointers to functions but this way it's inline
** and thus more efficient. Lower * ranking though...
**
** Parameters:
** name:      function name to de defined (eg: sinFunc)
** function:  function defined in math.h to wrap (eg: sin)
** domain:    boolean condition that CAN'T happen in terms of the input parameter rVal
**            (eg: rval<0 for sqrt)
*/
/* LMH 2007-03-25 Changed to use errno and remove domain; no pre-checking for errors. */
#define GEN_MATH_WRAP_DOUBLE_1(name, function) \
static void name(sqlite3_context *context, int argc, sqlite3_value **argv){\
  double rVal = 0.0, val;\
  assert( argc==1 );\
  switch( sqlite3_value_type(argv[0]) ){\
    case SQLITE_NULL: {\
      sqlite3_result_null(context);\
      break;\
    }\
    default: {\
      rVal = sqlite3_value_double(argv[0]);\
      errno = 0;\
      val = function(rVal);\
      if (errno == 0) {\
        sqlite3_result_double(context, val);\
      } else {\
        sqlite3_result_error(context, strerror(errno), errno);\
      }\
      break;\
    }\
  }\
}\


/*
** Example of GEN_MATH_WRAP_DOUBLE_1 usage
** this creates function sqrtFunc to wrap the math.h standard function sqrt(x)=x^0.5
*/
GEN_MATH_WRAP_DOUBLE_1(sqrtFunc, sqrt)

/* trignometric functions */
GEN_MATH_WRAP_DOUBLE_1(acosFunc, acos)
GEN_MATH_WRAP_DOUBLE_1(asinFunc, asin)
GEN_MATH_WRAP_DOUBLE_1(atanFunc, atan)

/*
** Many of systems don't have inverse hyperbolic trig functions so this will emulate
** them on those systems in terms of log and sqrt (formulas are too trivial to demand
** written proof here)
*/

#ifndef HAVE_ACOSH
static double acosh(double x) {
  return log(x + sqrt(x * x - 1.0));
}
#endif

GEN_MATH_WRAP_DOUBLE_1(acoshFunc, acosh)

#ifndef HAVE_ASINH
static double asinh(double x) {
  return log(x + sqrt(x * x + 1.0));
}
#endif

GEN_MATH_WRAP_DOUBLE_1(asinhFunc, asinh)

#ifndef HAVE_ATANH
static double atanh(double x) {
  return (1.0 / 2.0) * log((1 + x) / (1 - x));
}
#endif

GEN_MATH_WRAP_DOUBLE_1(atanhFunc, atanh)

/*
** math.h doesn't require cot (cotangent) so it's defined here
*/
static double cot(double x) {
  return 1.0 / tan(x);
}

GEN_MATH_WRAP_DOUBLE_1(sinFunc, sin)
GEN_MATH_WRAP_DOUBLE_1(cosFunc, cos)
GEN_MATH_WRAP_DOUBLE_1(tanFunc, tan)
GEN_MATH_WRAP_DOUBLE_1(cotFunc, cot)

static double coth(double x) {
  return 1.0 / tanh(x);
}

/*
** Many systems don't have hyperbolic trigonometric functions so this will emulate
** them on those systems directly from the definition in terms of exp
*/
#ifndef HAVE_SINH
static double sinh(double x) {
  return (exp(x) - exp(-x)) / 2.0;
}
#endif

GEN_MATH_WRAP_DOUBLE_1(sinhFunc, sinh)

#ifndef HAVE_COSH
static double cosh(double x) {
  return (exp(x) + exp(-x)) / 2.0;
}
#endif

GEN_MATH_WRAP_DOUBLE_1(coshFunc, cosh)

#ifndef HAVE_TANH
static double tanh(double x) {
  return sinh(x) / cosh(x);
}
#endif

GEN_MATH_WRAP_DOUBLE_1(tanhFunc, tanh)

GEN_MATH_WRAP_DOUBLE_1(cothFunc, coth)

/*
** Some systems lack log in base 10. This will emulate it
*/

#ifndef HAVE_LOG10
static double log10(double x) {
  static double l10 = -1.0;
  if (l10 < 0.0) {
    l10 = log(10.0);
  }
  return log(x) / l10;
}
#endif

GEN_MATH_WRAP_DOUBLE_1(logFunc, log)
GEN_MATH_WRAP_DOUBLE_1(log10Func, log10)
GEN_MATH_WRAP_DOUBLE_1(expFunc, exp)

/*
** Fallback for systems where math.h doesn't define M_PI
*/
#undef M_PI
#ifndef M_PI
/*
** static double PI = acos(-1.0);
** #define M_PI (PI)
*/
#define M_PI 3.14159265358979323846
#endif

/* Convert Degrees into Radians */
static double deg2rad(double x) {
  return x * M_PI / 180.0;
}

/* Convert Radians into Degrees */
static double rad2deg(double x) {
  return 180.0 * x / M_PI;
}

GEN_MATH_WRAP_DOUBLE_1(rad2degFunc, rad2deg)
GEN_MATH_WRAP_DOUBLE_1(deg2radFunc, deg2rad)

/* constant function that returns the value of PI=3.1415... */
static void piFunc(sqlite3_context* context, int argc, sqlite3_value** argv) {
  sqlite3_result_double(context, M_PI);
}

/*
** Implements the sqrt function, it has the peculiarity of returning an integer when the
** the argument is an integer.
** Since SQLite isn't strongly typed (almost untyped actually) this is a bit pedantic
*/
static void squareFunc(sqlite3_context* context, int argc, sqlite3_value** argv) {
  i64 iVal = 0;
  double rVal = 0.0;
  assert(argc == 1);
  switch (sqlite3_value_type(argv[0])) {
  case SQLITE_INTEGER: {
    iVal = sqlite3_value_int64(argv[0]);
    sqlite3_result_int64(context, iVal * iVal);
    break;
  }
  case SQLITE_NULL: {
    sqlite3_result_null(context);
    break;
  }
  default: {
    rVal = sqlite3_value_double(argv[0]);
    sqlite3_result_double(context, rVal * rVal);
    break;
  }
  }
}

/*
** Wraps the pow math.h function
** When both the base and the exponent are integers the result should be integer
** (see sqrt just before this). Here the result is always double
*/
/* LMH 2007-03-25 Changed to use errno; no pre-checking for errors.  Also removes
  but that was present in the pre-checking that called sqlite3_result_error on
  a non-positive first argument, which is not always an error. */
static void powerFunc(sqlite3_context* context, int argc, sqlite3_value** argv) {
  double r1 = 0.0;
  double r2 = 0.0;
  double val;

  assert(argc == 2);

  if (sqlite3_value_type(argv[0]) == SQLITE_NULL || sqlite3_value_type(argv[1]) == SQLITE_NULL) {
    sqlite3_result_null(context);
  }
  else {
    r1 = sqlite3_value_double(argv[0]);
    r2 = sqlite3_value_double(argv[1]);
    errno = 0;
    val = pow(r1, r2);
    if (errno == 0) {
      sqlite3_result_double(context, val);
    }
    else {
      sqlite3_result_error(context, strerror(errno), errno);
    }
  }
}

/*
** atan2 wrapper
*/
static void atn2Func(sqlite3_context* context, int argc, sqlite3_value** argv) {
  double r1 = 0.0;
  double r2 = 0.0;

  assert(argc == 2);

  if (sqlite3_value_type(argv[0]) == SQLITE_NULL || sqlite3_value_type(argv[1]) == SQLITE_NULL) {
    sqlite3_result_null(context);
  }
  else {
    r1 = sqlite3_value_double(argv[0]);
    r2 = sqlite3_value_double(argv[1]);
    sqlite3_result_double(context, atan2(r1, r2));
  }
}

/*
** Implementation of the sign() function
** return one of 3 possibilities +1,0 or -1 when the argument is respectively
** positive, 0 or negative.
** When the argument is NULL the result is also NULL (completly conventional)
*/
static void signFunc(sqlite3_context* context, int argc, sqlite3_value** argv) {
  double rVal = 0.0;
  i64 iVal = 0;
  assert(argc == 1);
  switch (sqlite3_value_type(argv[0])) {
  case SQLITE_INTEGER: {
    iVal = sqlite3_value_int64(argv[0]);
    iVal = (iVal > 0) ? 1 : (iVal < 0) ? -1 : 0;
    sqlite3_result_int64(context, iVal);
    break;
  }
  case SQLITE_NULL: {
    sqlite3_result_null(context);
    break;
  }
  default: {
    /* 2nd change below. Line for abs was: if( rVal<0 ) rVal = rVal * -1.0;  */

    rVal = sqlite3_value_double(argv[0]);
    rVal = (rVal > 0) ? 1 : (rVal < 0) ? -1 : 0;
    sqlite3_result_double(context, rVal);
    break;
  }
  }
}


/*
** smallest integer value not less than argument
*/
static void ceilFunc(sqlite3_context* context, int argc, sqlite3_value** argv) {
  double rVal = 0.0;
  //i64 iVal = 0;
  assert(argc == 1);
  switch (sqlite3_value_type(argv[0])) {
  case SQLITE_INTEGER: {
    i64 iVal = sqlite3_value_int64(argv[0]);
    sqlite3_result_int64(context, iVal);
    break;
  }
  case SQLITE_NULL: {
    sqlite3_result_null(context);
    break;
  }
  default: {
    rVal = sqlite3_value_double(argv[0]);
    sqlite3_result_int64(context, (i64)ceil(rVal));
    break;
  }
  }
}

/*
** largest integer value not greater than argument
*/
static void floorFunc(sqlite3_context* context, int argc, sqlite3_value** argv) {
  double rVal = 0.0;
  //i64 iVal = 0;
  assert(argc == 1);
  switch (sqlite3_value_type(argv[0])) {
  case SQLITE_INTEGER: {
    i64 iVal = sqlite3_value_int64(argv[0]);
    sqlite3_result_int64(context, iVal);
    break;
  }
  case SQLITE_NULL: {
    sqlite3_result_null(context);
    break;
  }
  default: {
    rVal = sqlite3_value_double(argv[0]);
    sqlite3_result_int64(context, (i64)floor(rVal));
    break;
  }
  }
}


/*
** missionx bearing
*/


double
calc_bearing(const double lat1, const double lon1, const double lat2, const double lon2)
{
  const double teta1  = deg2rad(lat1);
  const double teta2  = deg2rad(lat2);
  const double delta2 = deg2rad(lon2 - lon1);

  const double y = sin(delta2) * cos(teta2);
  const double x = cos(teta1) * sin(teta2) - sin(teta1) * cos(teta2) * cos(delta2);

  double brng = atan2(y, x);
  brng        = rad2deg(brng); // radians to degrees
  brng        = (((int)brng + 360) % 360);

  return brng;
}


double
calc_distance_between_2_points_nm(const double lat1, const double lon1, const double lat2, const double lon2)
{
  return mxUtils::mxCalcDistanceBetween2Points(lat1, lon1, lat2, lon2);
}

// https://gis.stackexchange.com/questions/252672/calculate-bearing-between-two-decimal-gps-coordinates-arduino-c?noredirect=1&lq=1
static void mx_bearing (sqlite3_context* context, int argc, sqlite3_value** argv) {

  //i64 iVal = 0;
  assert(argc == 4);
  switch (sqlite3_value_type(argv[0]))
  {
    case SQLITE_NULL:
    {
      sqlite3_result_null(context);
      break;
    }
    default:
    {

      double rValat1 = sqlite3_value_double(argv[0]); // lat1
      double rValon1 = sqlite3_value_double(argv[1]); // lon1
      double rValat2 = sqlite3_value_double(argv[2]); // lat2
      double rValon2 = sqlite3_value_double(argv[3]); // lon2

      const double brng = calc_bearing(rValat1, rValon1, rValat2, rValon2);

      sqlite3_result_double(context, brng); // always return positive value between 0 and 359
      break;
    }
  }
}

static void
mx_get_center_between_2_points(sqlite3_context* context, int argc, sqlite3_value** argv)
{

  //i64 iVal = 0;
  assert(argc >= 4 && "mx_get_center_between_2_points() function received less than 4 variables");
  //double planet_great_circle = 3440; // nautical miles

  switch (sqlite3_value_type(argv[0]))
  {
    case SQLITE_NULL:
    {
      sqlite3_result_null(context);
      break;
    }
    default:
    {
//      if (argc > 4)
//        planet_great_circle = sqlite3_value_double(argv[4]); // great circle custom value

      double rValat1 = sqlite3_value_double(argv[0]); // lat1
      double rValon1 = sqlite3_value_double(argv[1]); // lon1
      double rValat2 = sqlite3_value_double(argv[2]); // lat2
      double rValon2 = sqlite3_value_double(argv[3]); // lon2

      double outLat1 = 0.0, outLon1 = 0.0;
      char   charray[30];

      const float brng = (float) calc_bearing(rValat1, rValon1, rValat2, rValon2);
      double distnace_nm = calc_distance_between_2_points_nm(rValat1, rValon1, rValat2, rValon2);
      mxUtils::mxCalcPointBasedOnDistanceAndBearing_2DPlane(outLat1, outLon1, rValat1, rValon1, brng, ((distnace_nm * 0.5) * missionx::nm2meter));

#ifdef IBM
      sprintf(charray, "%4.8f,%4.8f", outLat1, outLon1);
#else
      snprintf(charray, sizeof(charray)-1, "%4.8f,%4.8f", outLat1, outLon1);
#endif
      sqlite3_result_text(context, charray, -1, SQLITE_TRANSIENT);
      break;
    }
  }
} // mx_get_center_between_2_points



static void
mx_get_point_based_on_bearing_and_length_in_meters(sqlite3_context* context, int argc, sqlite3_value** argv)
{

  // i64 iVal = 0;
  assert(argc >= 4 && "mx_get_point_based_on_bearing_and_length_in_meters() function received less than 4 variables");
  //double planet_great_circle = EARTH_RADIUS_M; // great circle in meters

  switch (sqlite3_value_type(argv[0]))
  {
    case SQLITE_NULL:
    {
      sqlite3_result_null(context);
      break;
    }
    default:
    {
      double rValat1 = sqlite3_value_double(argv[0]); // lat1
      double rValon1 = sqlite3_value_double(argv[1]); // lon1

      double outLat1 = 0.0, outLon1 = 0.0;
      char   charray[30];

      float  brng        = (float)sqlite3_value_double(argv[2]); // bearing
      double distnace_mt = sqlite3_value_double(argv[3]); // distance
      mxUtils::mxCalcPointBasedOnDistanceAndBearing_2DPlane(outLat1, outLon1, rValat1, rValon1, brng, distnace_mt, missionx::EARTH_RADIUS_M);

#ifdef IBM
      sprintf(charray, "%4.8f,%4.8f", outLat1, outLon1);
#else
      snprintf(charray, sizeof(charray)-1, "%4.8f,%4.8f", outLat1, outLon1);
#endif
      
      sqlite3_result_text(context, charray, -1, SQLITE_TRANSIENT);
      break;
    }
  }
} // mx_get_center_between_2_points


/*
** missionx distance in nm
* select distinct t2.id, round( 3440 * acos(   cos(radians(?1)) * cos(radians(t1.lat)) * cos(radians(?2) - radians(t1.lon)) + sin(radians(?3) ) * sin(radians(t1.lat))) ) as distance_nm 
from way_street_node_data t1, way_tag_data t2 where t2.key_attrib = 'highway' 
and t2.val_attrib in('primary', 'secondary', 'tertiary', 'residential', 'service', 'living_street', 'track') 
and t1.id = t2.id and t1.lat between ?4 and ?5 
and t1.lon between ?6 and ?7
* 
*/

static void mx_calc_distance(sqlite3_context* context, int argc, sqlite3_value** argv) 
{
  //i64 iVal = 0;
  assert(argc >= 4 && "mx_calc_distance() function received less than 4 variables");
  double planet_great_circle = 3440.0; // Earth great circle length in Nautical Miles

  switch (sqlite3_value_type(argv[0])) {
    case SQLITE_NULL: {
      sqlite3_result_null(context);
      break;
    }
    default: {

      double rValat1 = sqlite3_value_double(argv[0]); // lat1 - plane
      double rValon1 = sqlite3_value_double(argv[1]); // lon1 - plane
      double rValat2 = sqlite3_value_double(argv[2]); // lat2 - target
      double rValon2 = sqlite3_value_double(argv[3]); // lon2 - target
      if (argc > 4)
        planet_great_circle = sqlite3_value_double(argv[4]); // great circle custom value

      const double teta1 = deg2rad(rValat1);
      const double teta2 = deg2rad(rValon1);
      const double teta3 = deg2rad(rValat2);
      const double teta4 = deg2rad(rValon2);


      //const double calc_distance =  round (planet_great_circle * acos(  cos(teta1) * cos(teta3) * cos(teta2 - teta4) 
      //                                                                  + ( sin(teta1) * sin(teta3) ) 
      //                                                               ) 
      //                                    );
      const double calc_distance = planet_great_circle * acos(cos(teta1) * cos(teta3) * cos(teta2 - teta4) + (sin(teta1) * sin(teta3))); 

      //const double calc_distance = planet_great_circle * acos(cos(teta1) * cos(teta3) * cos(teta2 - teta4) + (sin(teta1) * sin(teta3)));

      sqlite3_result_double(context, calc_distance); // always return positive value in nautical miles
      break;
    }
  }
}

/*
    Function: mx_plane_on_which_rw_and_how_center_it_is
    Parameers: Plane lat, plane lon, rw_center_lat_1, rw_center_lon_1, rw_center_lat_2, rw_center_lon2, rw_width_in_meters
    What needs to calculate: Is plane coordinates in the runway box and how far from center line,

    return a string in the format: 1,{plane center from center line}
*/

static void
mx_is_plane_in_rw_area(sqlite3_context* context, int argc, sqlite3_value** argv)
{ 
  assert(argc >= 7 && "mx_plane_on_which_rw_and_how_center_it_is() function received less than 7 variables");

  double planet_great_circle = 3440.0; // Earth great circle length in Nautical Miles

  switch (sqlite3_value_type(argv[0]))
  {
    case SQLITE_NULL:
    {
      sqlite3_result_null(context);
      break;
    }
    default:
    {

      double dPlaneLat1 = sqlite3_value_double(argv[0]); // lat1 - plane
      double dPlaneLon1 = sqlite3_value_double(argv[1]); // lon1 - plane
      double dRunwayCenterEdgeLat1 = sqlite3_value_double(argv[2]); // lat1 - rw center edge lat 1
      double dRunwayCenterEdgeLon1 = sqlite3_value_double(argv[3]); // lon1 - rw center edge lon 1
      double dRunwayCenterEdgeLat2 = sqlite3_value_double(argv[4]); // lat2 - rw center edge lat 2
      double dRunwayCenterEdgeLon2 = sqlite3_value_double(argv[5]); // lon2 - rw center edge lon 2
      double dRunwayWidthInMeters  = sqlite3_value_double(argv[6]); // Runway width in meters
      if (argc <= 7)
        planet_great_circle = missionx::EARTH_RADIUS_M; // Earth great circle 
      else if (argc > 7)
        planet_great_circle = sqlite3_value_double(argv[7]); // great circle custom value in meters


      // Convert lat/lon to Radians
      //const double       rPlaneLat1            = deg2rad(dPlaneLat1); // lat1 - plane
      //const double       rPlaneLon1            = deg2rad(dPlaneLon1); // lon1 - plane
      //const double       rRunwayCenterEdgeLat1 = deg2rad(dRunwayCenterEdgeLat1); // lat1 - rw center edge lat 1
      //const double       rRunwayCenterEdgeLon1 = deg2rad(dRunwayCenterEdgeLon1); // lon1 - rw center edge lon 1
      //const double       rRunwayCenterEdgeLat2 = deg2rad(dRunwayCenterEdgeLat2); // lat2 - rw center edge lat 2
      //const double       rRunwayCenterEdgeLon2 = deg2rad(dRunwayCenterEdgeLon2); // lon2 - rw center edge lon 2

      missionx::mxVec2d planePos(dPlaneLat1, dPlaneLon1);
      missionx::mxVec2d rwCenterEdge1(dRunwayCenterEdgeLat1, dRunwayCenterEdgeLon1);
      missionx::mxVec2d rwCenterEdge2(dRunwayCenterEdgeLat2, dRunwayCenterEdgeLon2);

      float bearing_between_rw_edges = (float)mxUtils::mxCalcBearingBetween2Points(rwCenterEdge1.lat, rwCenterEdge1.lon, rwCenterEdge2.lat, rwCenterEdge2.lon);

      missionx::mxVec2d rwTopLeftPos, rwBottomLeftPos, rwTopRightPos, rwBottomRightPos;

      // TL                    TR
      // +---------------------+
      // |       Runway        |
      // +---------------------+
      // BL                    BR

      // calculate Top Left
      mxUtils::mxCalcPointBasedOnDistanceAndBearing_2DPlane(rwTopLeftPos.lat, rwTopLeftPos.lon, rwCenterEdge1.lat, rwCenterEdge1.lon, bearing_between_rw_edges + -90.0f, dRunwayWidthInMeters / 2.0, planet_great_circle);
      // calculate Bottom Left
      mxUtils::mxCalcPointBasedOnDistanceAndBearing_2DPlane(rwBottomLeftPos.lat, rwBottomLeftPos.lon, rwCenterEdge1.lat, rwCenterEdge1.lon, bearing_between_rw_edges - -90.0f, dRunwayWidthInMeters / 2.0, planet_great_circle);

      // now calculate second runway edge with oposite bearings
      bearing_between_rw_edges -= 180.0;
      if (bearing_between_rw_edges < 0.0)
        bearing_between_rw_edges += 360.0;

      // calculate Top Right
      mxUtils::mxCalcPointBasedOnDistanceAndBearing_2DPlane(rwTopRightPos.lat, rwTopRightPos.lon, rwCenterEdge2.lat, rwCenterEdge2.lon, bearing_between_rw_edges - -90.0f, dRunwayWidthInMeters / 2.0, planet_great_circle);
      // Calculate Bottom Right
      mxUtils::mxCalcPointBasedOnDistanceAndBearing_2DPlane(rwBottomRightPos.lat, rwBottomRightPos.lon, rwCenterEdge2.lat, rwCenterEdge2.lon, bearing_between_rw_edges + -90.0f, dRunwayWidthInMeters / 2.0, planet_great_circle);

      // check if position is inside the runway area
      const std::vector<missionx::mxVec2d> vecRunwayEdges = { rwTopLeftPos, rwTopRightPos, rwBottomRightPos, rwBottomLeftPos };
      const int result = mxUtils::mxIsPointInPolyArea(vecRunwayEdges, planePos);

      //sqlite3_result_text(context, )
      sqlite3_result_int(context, result);
      break;
    }
  }
}

/*
@ mx_get_shortest_distance_to_rw_vector() function receive 3 coordinatates:
First: lat,lon: Plane touchdown position
Second coordinates: Runway Center Edge Start
Third coordinates: Runway Center Edge End
Optional: planet radius in meters, default is EARTH radiues

Overall, minimum parameters to provide are: 6.
*/
static void
mx_get_shortest_distance_to_rw_vector(sqlite3_context* context, int argc, sqlite3_value** argv)
{
  assert(argc >= 6 && "mx_get_shortest_dist_to_vector() function received less than 6 variables");

//  double planet_great_circle = EARTH_RADIUS_M;
  switch (sqlite3_value_type(argv[0]))
  {
    case SQLITE_NULL:
    {
      sqlite3_result_null(context);
      break;
    }
    default:
    {

      double dPlaneLat1            = sqlite3_value_double(argv[0]); // lat1 - plane
      double dPlaneLon1            = sqlite3_value_double(argv[1]); // lon1 - plane
      double dRunwayCenterEdgeLat1 = sqlite3_value_double(argv[2]); // lat1 - rw center edge lat 1
      double dRunwayCenterEdgeLon1 = sqlite3_value_double(argv[3]); // lon1 - rw center edge lon 1
      double dRunwayCenterEdgeLat2 = sqlite3_value_double(argv[4]); // lat2 - rw center edge lat 2
      double dRunwayCenterEdgeLon2 = sqlite3_value_double(argv[5]); // lon2 - rw center edge lon 2
//      if (argc > 6)
//        planet_great_circle = sqlite3_value_double(argv[6]); // great planet circle, custom value in meters


      missionx::mxVec2d planePos (dPlaneLat1, dPlaneLon1);
      missionx::mxVec2d rwCenterEdge1(dRunwayCenterEdgeLat1, dRunwayCenterEdgeLon1);
      missionx::mxVec2d rwCenterEdge2(dRunwayCenterEdgeLat2, dRunwayCenterEdgeLon2);


      const float bearing_between_rw_edges                 = (float)mxUtils::mxCalcBearingBetween2Points(rwCenterEdge1.lat, rwCenterEdge1.lon, rwCenterEdge2.lat, rwCenterEdge2.lon);
      const float bearing_between_Edge1_and_touchdownPoint = (float)mxUtils::mxCalcBearingBetween2Points(rwCenterEdge1.lat, rwCenterEdge1.lon, planePos.lat, planePos.lon);
      const float angle_between_the_2_bearings             = fabsf(bearing_between_rw_edges - bearing_between_Edge1_and_touchdownPoint);

      // Calculate shortest distance using pythagorian theorm.
      //
      //                           C
      //                           +
      //                          /|
      //                         / |
      //                        /  |
      //                     A +---+ B (90deg)
      //
      // The idea is, that the shortest line to the AB vector is a strait line that creates 90deg angle.
      // We can caluclate the length of the height (BC) using: sin(a) = BC/AC  =>  sin(a) * AC = BC
      //
//#ifndef RELEASE
//      double AC_length_meters = mxUtils::mxCalcDistanceBetween2Points (rwCenterEdge1.lat, rwCenterEdge1.lon, planePos.lat, planePos.lon, missionx::mx_units_of_measure::meter, missionx::EARTH_RADIUS_M);
//      double sinRsult         = sin(deg2rad(angle_between_the_2_bearings));
//      double AB_length_meter  = sinRsult * AC_length_meters;
//#endif

      // -1.0 means outside of runway boundaries. Since we always pick the "left" center the plane can be to the right of it, so the angles can be 0 and 180 at worst cases. You should test if plane is inside the RW boundaries first and only then check the touchdown relative to center.
      double result = -1.0;
      if (angle_between_the_2_bearings < 180.0f)
        result = sin(deg2rad(angle_between_the_2_bearings)) * mxUtils::mxCalcDistanceBetween2Points(rwCenterEdge1.lat, rwCenterEdge1.lon, planePos.lat, planePos.lon, missionx::mx_units_of_measure::meter, missionx::EARTH_RADIUS_M);


      // sqlite3_result_text(context, )
      sqlite3_result_double(context, result);
      break;
    }
  }
}





/*
@ mx_is_plane_in_airport_boundary() function receives plane coordinate and the airport boundary string (set off nodes delimated by '|'):
First: lat,lon: Plane position
Second string: airport boundary string (set off nodes delimated by '|')

returns 0 if plane is not in boundary and 1 if yes.
*/
static void
mx_is_plane_in_airport_boundary(sqlite3_context* context, int argc, sqlite3_value** argv)
{
  assert(argc >= 3 && "mx_is_plane_in_airport_boundary() function received less than 3 variables");
  int result = 0;

//  double planet_great_circle = EARTH_RADIUS_M;
  switch (sqlite3_value_type(argv[0]))
  {
    case SQLITE_NULL:
    {
      sqlite3_result_null(context);
      break;
    }
    default:
    {

      double dPlaneLat1 = sqlite3_value_double(argv[0]); // lat1 - plane
      double dPlaneLon1 = sqlite3_value_double(argv[1]); // lon1 - plane
      const unsigned char*  argv2_const      = sqlite3_value_text(argv[2]);   // set of lat, lon | lat, lon | ...

      std::string argv2 = std::string(reinterpret_cast<const char*> (argv2_const));


      if ( ! argv2.empty() )
      {

        missionx::mxVec2d planePos; // (dPlaneLat1, dPlaneLon1);

        planePos.lat = dPlaneLat1;
        planePos.lon = dPlaneLon1;

        // http://www.martinbroadhurst.com/split-a-string-in-c.html
        //dynarray* arrayOfCoordinates = dynarray_create(0);
        std::vector<missionx::mxVec2d> vecCoordinates;
        std::vector<std::string> vecSplitNodes = mxUtils::split(argv2, '|');
        for (const auto& node : vecSplitNodes)
        {
          std::vector<std::string> vecOf2 = mxUtils::split(node, ',');
          if (vecOf2.size() > 1)
          {
            const missionx::mxVec2d vec2(mxUtils::stringToNumber<double>(vecOf2.at(0)), mxUtils::stringToNumber<double>(vecOf2.at(1)));
            vecCoordinates.emplace_back(vec2);
          }
          else
            continue;
        }

        result = mxUtils::mxIsPointInPolyArea(vecCoordinates, planePos);

      } // if bounds is not NULL


      // return result 0 or 1
      sqlite3_result_int(context, result);
      break;
    }
  }
}

















/*
** Given a string (s) in the first argument and an integer (n) in the second returns the
** string that constains s contatenated n times
*/
static void replicateFunc(sqlite3_context* context, int argc, sqlite3_value** argv) {
  unsigned char* z;        /* input string */
  unsigned char* zo;       /* result string */
  i64 iCount;              /* times to repeat */
  i64 nLen;                /* length of the input string (no multibyte considerations) */
  i64 nTLen;               /* length of the result string (no multibyte considerations) */
  i64 i = 0;

  if (argc != 2 || SQLITE_NULL == sqlite3_value_type(argv[0]))
    return;

  iCount = sqlite3_value_int64(argv[1]);

  if (iCount < 0) {
    sqlite3_result_error(context, "domain error", -1);
  }
  else {

    nLen = sqlite3_value_bytes(argv[0]);
    nTLen = nLen * iCount;
    z = (unsigned char*) sqlite3_malloc( (int)(nTLen + 1)); // missionx
    zo = (unsigned char*)sqlite3_malloc( (int)(nLen + 1 )); // missionx
    if (!z || !zo) {
      sqlite3_result_error_nomem(context);
      if (z) sqlite3_free(z);
      if (zo) sqlite3_free(zo);
      return;
    }
    strcpy((char*)zo, (char*)sqlite3_value_text(argv[0]));

    for (i = 0; i < iCount; ++i) {
      strcpy((char*)(z + i * nLen), (char*)zo);
    }

    sqlite3_result_text(context, (char*)z, -1, SQLITE_TRANSIENT);
    sqlite3_free(z);
    sqlite3_free(zo);
  }
}

/*
** Some systems (win32 among others) don't have an isblank function, this will emulate it.
** This function is not UFT-8 safe since it only analyses a byte character.
*/
#ifndef HAVE_ISBLANK
int isblank(char c) {
  return(' ' == c || '\t' == c);
}
#endif

static void properFunc(sqlite3_context* context, int argc, sqlite3_value** argv) {
  const unsigned char* z;     /* input string */
  unsigned char* zo;          /* output string */
  unsigned char* zt;          /* iterator */
  char r;
  int c = 1;

  assert(argc == 1);
  if (SQLITE_NULL == sqlite3_value_type(argv[0])) {
    sqlite3_result_null(context);
    return;
  }

  z = sqlite3_value_text(argv[0]);
  zo = (unsigned char*)sqlite3StrDup((char*)z);
  if (!zo) {
    sqlite3_result_error_nomem(context);
    return;
  }
  zt = zo;

  while ((r = *(z++)) != 0) {
    if (isblank(r)) {
      c = 1;
    }
    else {
      if (c == 1) {
        r = toupper(r);
      }
      else {
        r = tolower(r);
      }
      c = 0;
    }
    *(zt++) = r;
  }
  *zt = '\0';

  sqlite3_result_text(context, (char*)zo, -1, SQLITE_TRANSIENT);
  sqlite3_free(zo);
}

/*
** given an input string (s) and an integer (n) adds spaces at the begining of  s
** until it has a length of n characters.
** When s has a length >=n it's a NOP
** padl(NULL) = NULL
*/
static void padlFunc(sqlite3_context* context, int argc, sqlite3_value** argv) {
  i64 ilen;          /* length to pad to */
  i64 zl;            /* length of the input string (UTF-8 chars) */
  int i = 0;
  const char* zi;    /* input string */
  char* zo;          /* output string */
  char* zt;

  assert(argc == 2);

  if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
    sqlite3_result_null(context);
  }
  else {
    zi = (char*)sqlite3_value_text(argv[0]);
    ilen = sqlite3_value_int64(argv[1]);
    /* check domain */
    if (ilen < 0) {
      sqlite3_result_error(context, "domain error", -1);
      return;
    }
    zl = sqlite3Utf8CharLen(zi, -1);
    if (zl >= ilen) {
      /* string is longer than the requested pad length, return the same string (dup it) */
      zo = sqlite3StrDup(zi);
      if (!zo) {
        sqlite3_result_error_nomem(context);
        return;
      }
      sqlite3_result_text(context, zo, -1, SQLITE_TRANSIENT);
    }
    else {
      zo = (char*) sqlite3_malloc( (int)( strlen(zi) + ilen - zl + 1 ));
      if (!zo) {
        sqlite3_result_error_nomem(context);
        return;
      }
      zt = zo;
      for (i = 1; i + zl <= ilen; ++i) {
        *(zt++) = ' ';
      }
      /* no need to take UTF-8 into consideration here */
      strcpy(zt, zi);
    }
    sqlite3_result_text(context, zo, -1, SQLITE_TRANSIENT);
    sqlite3_free(zo);
  }
}

/*
** given an input string (s) and an integer (n) appends spaces at the end of  s
** until it has a length of n characters.
** When s has a length >=n it's a NOP
** padl(NULL) = NULL
*/
static void padrFunc(sqlite3_context* context, int argc, sqlite3_value** argv) {
  i64 ilen;          /* length to pad to */
  i64 zl;            /* length of the input string (UTF-8 chars) */
  i64 zll;           /* length of the input string (bytes) */
  int i = 0;
  const char* zi;    /* input string */
  char* zo;          /* output string */
  char* zt;

  assert(argc == 2);

  if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
    sqlite3_result_null(context);
  }
  else {
    zi = (char*)sqlite3_value_text(argv[0]);
    ilen = sqlite3_value_int64(argv[1]);
    /* check domain */
    if (ilen < 0) {
      sqlite3_result_error(context, "domain error", -1);
      return;
    }
    zl = sqlite3Utf8CharLen(zi, -1);
    if (zl >= ilen) {
      /* string is longer than the requested pad length, return the same string (dup it) */
      zo = sqlite3StrDup(zi);
      if (!zo) {
        sqlite3_result_error_nomem(context);
        return;
      }
      sqlite3_result_text(context, zo, -1, SQLITE_TRANSIENT);
    }
    else {
      zll = strlen(zi);
      zo = (char *)sqlite3_malloc( (int)(zll + ilen - zl + 1));
      if (!zo) {
        sqlite3_result_error_nomem(context);
        return;
      }
      zt = strcpy(zo, zi) + zll;
      for (i = 1; i + zl <= ilen; ++i) {
        *(zt++) = ' ';
      }
      *zt = '\0';
    }
    sqlite3_result_text(context, zo, -1, SQLITE_TRANSIENT);
    sqlite3_free(zo);
  }
}

/*
** given an input string (s) and an integer (n) appends spaces at the end of  s
** and adds spaces at the begining of s until it has a length of n characters.
** Tries to add has many characters at the left as at the right.
** When s has a length >=n it's a NOP
** padl(NULL) = NULL
*/
static void padcFunc(sqlite3_context* context, int argc, sqlite3_value** argv) {
  i64 ilen;           /* length to pad to */
  i64 zl;             /* length of the input string (UTF-8 chars) */
  i64 zll;            /* length of the input string (bytes) */
  int i = 0;
  const char* zi;     /* input string */
  char* zo;           /* output string */
  char* zt;

  assert(argc == 2);

  if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
    sqlite3_result_null(context);
  }
  else {
    zi = (char*)sqlite3_value_text(argv[0]);
    ilen = sqlite3_value_int64(argv[1]);
    /* check domain */
    if (ilen < 0) {
      sqlite3_result_error(context, "domain error", -1);
      return;
    }
    zl = sqlite3Utf8CharLen(zi, -1);
    if (zl >= ilen) {
      /* string is longer than the requested pad length, return the same string (dup it) */
      zo = sqlite3StrDup(zi);
      if (!zo) {
        sqlite3_result_error_nomem(context);
        return;
      }
      sqlite3_result_text(context, zo, -1, SQLITE_TRANSIENT);
    }
    else {
      zll = strlen(zi);
      zo = (char *)sqlite3_malloc((int)(zll + ilen - zl + 1));
      if (!zo) {
        sqlite3_result_error_nomem(context);
        return;
      }
      zt = zo;
      for (i = 1; 2 * i + zl <= ilen; ++i) {
        *(zt++) = ' ';
      }
      strcpy(zt, zi);
      zt += zll;
      for (; i + zl <= ilen; ++i) {
        *(zt++) = ' ';
      }
      *zt = '\0';
    }
    sqlite3_result_text(context, zo, -1, SQLITE_TRANSIENT);
    sqlite3_free(zo);
  }
}

/*
** given 2 string (s1,s2) returns the string s1 with the characters NOT in s2 removed
** assumes strings are UTF-8 encoded
*/
static void strfilterFunc(sqlite3_context* context, int argc, sqlite3_value** argv) {
  const char* zi1;        /* first parameter string (searched string) */
  const char* zi2;        /* second parameter string (vcontains valid characters) */
  const char* z1;
  const char* z21;
  const char* z22;
  char* zo;               /* output string */
  char* zot;
  int c1 = 0;
  int c2 = 0;

  assert(argc == 2);

  if (sqlite3_value_type(argv[0]) == SQLITE_NULL || sqlite3_value_type(argv[1]) == SQLITE_NULL) {
    sqlite3_result_null(context);
  }
  else {
    zi1 = (char*)sqlite3_value_text(argv[0]);
    zi2 = (char*)sqlite3_value_text(argv[1]);
    /*
    ** maybe I could allocate less, but that would imply 2 passes, rather waste
    ** (possibly) some memory
    */
    zo = (char *)sqlite3_malloc( (int)(strlen(zi1) + 1) );
    if (!zo) {
      sqlite3_result_error_nomem(context);
      return;
    }
    zot = zo;
    z1 = zi1;
    while ((c1 = sqliteCharVal((unsigned char*)z1)) != 0) {
      z21 = zi2;
      while ((c2 = sqliteCharVal((unsigned char*)z21)) != 0 && c2 != c1) {
        sqliteNextChar(z21);
      }
      if (c2 != 0) {
        z22 = z21;
        sqliteNextChar(z22);
        strncpy(zot, z21, z22 - z21);
        zot += z22 - z21;
      }
      sqliteNextChar(z1);
    }
    *zot = '\0';

    sqlite3_result_text(context, zo, -1, SQLITE_TRANSIENT);
    sqlite3_free(zo);
  }
}

/*
** Given a string z1, retutns the (0 based) index of it's first occurence
** in z2 after the first s characters.
** Returns -1 when there isn't a match.
** updates p to point to the character where the match occured.
** This is an auxiliary function.
*/
static int _substr(const char* z1, const char* z2, int s, const char** p) {
  int c = 0;
  int rVal = -1;
  const char* zt1;
  const char* zt2;
  int c1, c2;

  if ('\0' == *z1) {
    return -1;
  }

  while ((sqliteCharVal((unsigned char*)z2) != 0) && (c++) < s) {
    sqliteNextChar(z2);
  }

  c = 0;
  while ((sqliteCharVal((unsigned char*)z2)) != 0) {
    zt1 = z1;
    zt2 = z2;

    do {
      c1 = sqliteCharVal((unsigned char*)zt1);
      c2 = sqliteCharVal((unsigned char*)zt2);
      sqliteNextChar(zt1);
      sqliteNextChar(zt2);
    } while (c1 == c2 && c1 != 0 && c2 != 0);

    if (c1 == 0) {
      rVal = c;
      break;
    }

    sqliteNextChar(z2);
    ++c;
  }
  if (p) {
    *p = z2;
  }
  return rVal >= 0 ? rVal + s : rVal;
}

/*
** given 2 input strings (s1,s2) and an integer (n) searches from the nth character
** for the string s1. Returns the position where the match occured.
** Characters are counted from 1.
** 0 is returned when no match occurs.
*/

static void charindexFunc(sqlite3_context* context, int argc, sqlite3_value** argv) {
  const u8* z1;          /* s1 string */
  u8* z2;                /* s2 string */
  int s = 0;
  int rVal = 0;

  assert(argc == 3 || argc == 2);

  if (SQLITE_NULL == sqlite3_value_type(argv[0]) || SQLITE_NULL == sqlite3_value_type(argv[1])) {
    sqlite3_result_null(context);
    return;
  }

  z1 = sqlite3_value_text(argv[0]);
  if (z1 == 0) return;
  z2 = (u8*)sqlite3_value_text(argv[1]);
  if (argc == 3) {
    s = sqlite3_value_int(argv[2]) - 1;
    if (s < 0) {
      s = 0;
    }
  }
  else {
    s = 0;
  }

  rVal = _substr((char*)z1, (char*)z2, s, NULL);
  sqlite3_result_int(context, rVal + 1);
}

/*
** given a string (s) and an integer (n) returns the n leftmost (UTF-8) characters
** if the string has a length<=n or is NULL this function is NOP
*/
static void leftFunc(sqlite3_context* context, int argc, sqlite3_value** argv) {
  int c = 0;
  int cc = 0;
  int l = 0;
  const unsigned char* z;       /* input string */
  const unsigned char* zt;
  unsigned char* rz;            /* output string */

  assert(argc == 2);

  if (SQLITE_NULL == sqlite3_value_type(argv[0]) || SQLITE_NULL == sqlite3_value_type(argv[1])) {
    sqlite3_result_null(context);
    return;
  }

  z = sqlite3_value_text(argv[0]);
  l = sqlite3_value_int(argv[1]);
  zt = z;

  while (sqliteCharVal(zt) && c++ < l)
    sqliteNextChar(zt);

  cc = (int)( zt - z ); // missionx

  rz = (unsigned char*)sqlite3_malloc((int)(zt - z + 1)); // missionx
  if (!rz) {
    sqlite3_result_error_nomem(context);
    return;
  }
  strncpy((char*)rz, (char*)z, zt - z);
  *(rz + cc) = '\0';
  sqlite3_result_text(context, (char*)rz, -1, SQLITE_TRANSIENT);
  sqlite3_free(rz);
}

/*
** given a string (s) and an integer (n) returns the n rightmost (UTF-8) characters
** if the string has a length<=n or is NULL this function is NOP
*/
static void rightFunc(sqlite3_context* context, int argc, sqlite3_value** argv) {
  int l = 0;
  int c = 0;
  int cc = 0;
  const char* z;
  const char* zt;
  const char* ze;
  char* rz;

  assert(argc == 2);

  if (SQLITE_NULL == sqlite3_value_type(argv[0]) || SQLITE_NULL == sqlite3_value_type(argv[1])) {
    sqlite3_result_null(context);
    return;
  }

  z = (char*)sqlite3_value_text(argv[0]);
  l = sqlite3_value_int(argv[1]);
  zt = z;

  while (sqliteCharVal((unsigned char*)zt) != 0) {
    sqliteNextChar(zt);
    ++c;
  }

  ze = zt;
  zt = z;

  cc = c - l;
  if (cc < 0)
    cc = 0;

  while (cc-- > 0) {
    sqliteNextChar(zt);
  }

  rz = (char*)sqlite3_malloc((int)(ze - zt + 1)); // missionx
  if (!rz) {
    sqlite3_result_error_nomem(context);
    return;
  }
  strcpy((char*)rz, (char*)(zt));
  sqlite3_result_text(context, (char*)rz, -1, SQLITE_TRANSIENT);
  sqlite3_free(rz);
}

#ifndef HAVE_TRIM
/*
** removes the whitespaces at the begining of a string.
*/
const char* ltrim(const char* s) {
  while (*s == ' ')
    ++s;
  return s;
}

/*
** removes the whitespaces at the end of a string.
** !mutates the input string!
*/
void rtrim(char* s) {
  char* ss = s + strlen(s) - 1;
  while (ss >= s && *ss == ' ')
    --ss;
  *(ss + 1) = '\0';
}

/*
**  Removes the whitespace at the begining of a string
*/
static void ltrimFunc(sqlite3_context* context, int argc, sqlite3_value** argv) {
  const char* z;

  assert(argc == 1);

  if (SQLITE_NULL == sqlite3_value_type(argv[0])) {
    sqlite3_result_null(context);
    return;
  }
  z = sqlite3_value_text(argv[0]);
  sqlite3_result_text(context, ltrim(z), -1, SQLITE_TRANSIENT);
}

/*
**  Removes the whitespace at the end of a string
*/
static void rtrimFunc(sqlite3_context* context, int argc, sqlite3_value** argv) {
  const char* z;
  char* rz;
  /* try not to change data in argv */

  assert(argc == 1);

  if (SQLITE_NULL == sqlite3_value_type(argv[0])) {
    sqlite3_result_null(context);
    return;
  }
  z = sqlite3_value_text(argv[0]);
  rz = sqlite3StrDup(z);
  rtrim(rz);
  sqlite3_result_text(context, rz, -1, SQLITE_TRANSIENT);
  sqlite3_free(rz);
}

/*
**  Removes the whitespace at the begining and end of a string
*/
static void trimFunc(sqlite3_context* context, int argc, sqlite3_value** argv) {
  const char* z;
  char* rz;
  /* try not to change data in argv */

  assert(argc == 1);

  if (SQLITE_NULL == sqlite3_value_type(argv[0])) {
    sqlite3_result_null(context);
    return;
  }
  z = sqlite3_value_text(argv[0]);
  rz = sqlite3StrDup(z);
  rtrim(rz);
  sqlite3_result_text(context, ltrim(rz), -1, SQLITE_TRANSIENT);
  sqlite3_free(rz);
}
#endif

/*
** given a pointer to a string s1, the length of that string (l1), a new string (s2)
** and it's length (l2) appends s2 to s1.
** All lengths in bytes.
** This is just an auxiliary function
*/
// static void _append(char **s1, int l1, const char *s2, int l2){
//   *s1 = realloc(*s1, (l1+l2+1)*sizeof(char));
//   strncpy((*s1)+l1, s2, l2);
//   *(*(s1)+l1+l2) = '\0';
// }

#ifndef HAVE_TRIM

/*
** given strings s, s1 and s2 replaces occurrences of s1 in s by s2
*/
static void replaceFunc(sqlite3_context* context, int argc, sqlite3_value** argv) {
  const char* z1;     /* string s (first parameter) */
  const char* z2;     /* string s1 (second parameter) string to look for */
  const char* z3;     /* string s2 (third parameter) string to replace occurrences of s1 with */
  int lz1;
  int lz2;
  int lz3;
  int lzo = 0;
  char* zo = 0;
  int ret = 0;
  const char* zt1;
  const char* zt2;

  assert(3 == argc);

  if (SQLITE_NULL == sqlite3_value_type(argv[0])) {
    sqlite3_result_null(context);
    return;
  }

  z1 = sqlite3_value_text(argv[0]);
  z2 = sqlite3_value_text(argv[1]);
  z3 = sqlite3_value_text(argv[2]);
  /* handle possible null values */
  if (0 == z2) {
    z2 = "";
  }
  if (0 == z3) {
    z3 = "";
  }

  lz1 = strlen(z1);
  lz2 = strlen(z2);
  lz3 = strlen(z3);

#if 0
  /* special case when z2 is empty (or null) nothing will be changed */
  if (0 == lz2) {
    sqlite3_result_text(context, z1, -1, SQLITE_TRANSIENT);
    return;
  }
#endif

  zt1 = z1;
  zt2 = z1;

  while (1) {
    ret = _substr(z2, zt1, 0, &zt2);

    if (ret < 0)
      break;

    _append(&zo, lzo, zt1, zt2 - zt1);
    lzo += zt2 - zt1;
    _append(&zo, lzo, z3, lz3);
    lzo += lz3;

    zt1 = zt2 + lz2;
  }
  _append(&zo, lzo, zt1, lz1 - (zt1 - z1));
  sqlite3_result_text(context, zo, -1, SQLITE_TRANSIENT);
  sqlite3_free(zo);
}
#endif

/*
** given a string returns the same string but with the characters in reverse order
*/
static void reverseFunc(sqlite3_context* context, int argc, sqlite3_value** argv) {
  const char* z;
  const char* zt;
  char* rz;
  char* rzt;
  int l = 0;
  int i = 0;

  assert(1 == argc);

  if (SQLITE_NULL == sqlite3_value_type(argv[0])) {
    sqlite3_result_null(context);
    return;
  }
  z = (char*)sqlite3_value_text(argv[0]);
  l = (int)strlen(z);
  rz = (char*)sqlite3_malloc(l + 1);
  if (!rz) {
    sqlite3_result_error_nomem(context);
    return;
  }
  rzt = rz + l;
  *(rzt--) = '\0';

  zt = z;
  while (sqliteCharVal((unsigned char*)zt) != 0) {
    z = zt;
    sqliteNextChar(zt);
    for (i = 1; zt - i >= z; ++i) {
      *(rzt--) = *(zt - i);
    }
  }

  sqlite3_result_text(context, rz, -1, SQLITE_TRANSIENT);
  sqlite3_free(rz);
}

/*
** An instance of the following structure holds the context of a
** stdev() or variance() aggregate computation.
** implementaion of http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance#Algorithm_II
** less prone to rounding errors
*/
typedef struct StdevCtx StdevCtx;
struct StdevCtx {
  double rM;
  double rS;
  i64 cnt;          /* number of elements */
};

/*
** An instance of the following structure holds the context of a
** mode() or median() aggregate computation.
** Depends on structures defined in xx_map.c (see xx_map & xx_map)
** These aggregate functions only work for integers and floats although
** they could be made to work for strings. This is usually considered meaningless.
** Only usuall order (for median), no use of collation functions (would this even make sense?)
*/
typedef struct ModeCtx ModeCtx;
struct ModeCtx {
  i64 riM;            /* integer value found so far */
  double rdM;         /* double value found so far */
  i64 cnt;            /* number of elements so far */
  double pcnt;        /* number of elements smaller than a percentile */
  i64 mcnt;           /* maximum number of occurrences (for mode) */
  i64 mn;             /* number of occurrences (for mode and percentiles) */
  i64 is_double;      /* whether the computation is being done for doubles (>0) or integers (=0) */
  xx_map* m;             /* xx_map structure used for the computation */
  int done;           /* whether the answer has been found */
};

/*
** called for each value received during a calculation of stdev or variance
*/
static void varianceStep(sqlite3_context* context, int argc, sqlite3_value** argv) {
  StdevCtx* p;

  double delta;
  double x;

  assert(argc == 1);
  p = (StdevCtx*)sqlite3_aggregate_context(context, sizeof(*p));
  /* only consider non-null values */
  if (SQLITE_NULL != sqlite3_value_numeric_type(argv[0])) {
    p->cnt++;
    x = sqlite3_value_double(argv[0]);
    delta = (x - p->rM);
    p->rM += delta / p->cnt;
    p->rS += delta * (x - p->rM);
  }
}

/*
** called for each value received during a calculation of mode of median
*/
static void modeStep(sqlite3_context* context, int argc, sqlite3_value** argv) {
  ModeCtx* p;
  i64 xi = 0;
  double xd = 0.0;
  i64* iptr;
  double* dptr;
  int type;

  assert(argc == 1);
  type = sqlite3_value_numeric_type(argv[0]);

  if (type == SQLITE_NULL)
    return;

  p = (ModeCtx * )sqlite3_aggregate_context(context, sizeof(*p));

  if (0 == (p->m)) {
    p->m = (xx_map *)calloc(1, sizeof(xx_map));
    if (type == SQLITE_INTEGER) {
      /* xx_map will be used for integers */
      *(p->m) = map_make(int_cmp);
      p->is_double = 0;
    }
    else {
      p->is_double = 1;
      /* xx_map will be used for doubles */
      *(p->m) = map_make(double_cmp);
    }
  }

  ++(p->cnt);

  if (0 == p->is_double) {
    xi = sqlite3_value_int64(argv[0]);
    iptr = (i64*)calloc(1, sizeof(i64));
    *iptr = xi;
    map_insert(p->m, iptr);
  }
  else {
    xd = sqlite3_value_double(argv[0]);
    dptr = (double*)calloc(1, sizeof(double));
    *dptr = xd;
    map_insert(p->m, dptr);
  }
}

/*
**  Auxiliary function that iterates all elements in a xx_map and finds the mode
**  (most frequent value)
*/
static void modeIterate(void* e, i64 c, void* pp) {
  i64 ei;
  double ed;
  ModeCtx* p = (ModeCtx*)pp;

  if (0 == p->is_double) {
    ei = *(int*)(e);

    if (p->mcnt == c) {
      ++p->mn;
    }
    else if (p->mcnt < c) {
      p->riM = ei;
      p->mcnt = c;
      p->mn = 1;
    }
  }
  else {
    ed = *(double*)(e);

    if (p->mcnt == c) {
      ++p->mn;
    }
    else if (p->mcnt < c) {
      p->rdM = ed;
      p->mcnt = c;
      p->mn = 1;
    }
  }
}

/*
**  Auxiliary function that iterates all elements in a xx_map and finds the median
**  (the value such that the number of elements smaller is equal the the number of
**  elements larger)
*/
static void medianIterate(void* e, i64 c, void* pp) {
  i64 ei;
  double ed;
  double iL;
  double iR;
  int il;
  int ir;
  ModeCtx* p = (ModeCtx*)pp;

  if (p->done > 0)
    return;

  iL = p->pcnt;
  iR = p->cnt - p->pcnt;
  il = (int)(p->mcnt + c); // missionx
  ir = (int)(p->cnt - p->mcnt); // missionx

  if (il >= iL) {
    if (ir >= iR) {
      ++p->mn;
      if (0 == p->is_double) {
        ei = *(int*)(e);
        p->riM += ei;
      }
      else {
        ed = *(double*)(e);
        p->rdM += ed;
      }
    }
    else {
      p->done = 1;
    }
  }
  p->mcnt += c;
}

/*
** Returns the mode value
*/
static void modeFinalize(sqlite3_context* context) {
  ModeCtx* p;
  p = (ModeCtx * )sqlite3_aggregate_context(context, 0);
  if (p && p->m) {
    map_iterate(p->m, modeIterate, p);
    map_destroy(p->m);
    free(p->m);

    if (1 == p->mn) {
      if (0 == p->is_double)
        sqlite3_result_int64(context, p->riM);
      else
        sqlite3_result_double(context, p->rdM);
    }
  }
}

/*
** auxiliary function for percentiles
*/
static void _medianFinalize(sqlite3_context* context) {
  ModeCtx* p;
  p = (ModeCtx*)sqlite3_aggregate_context(context, 0);
  if (p && p->m) {
    p->done = 0;
    map_iterate(p->m, medianIterate, p);
    map_destroy(p->m);
    free(p->m);

    if (0 == p->is_double)
      if (1 == p->mn)
        sqlite3_result_int64(context, p->riM);
      else
        sqlite3_result_double(context, p->riM * 1.0 / p->mn);
    else
      sqlite3_result_double(context, p->rdM / p->mn);
  }
}

/*
** Returns the median value
*/
static void medianFinalize(sqlite3_context* context) {
  ModeCtx* p;
  p = (ModeCtx*)sqlite3_aggregate_context(context, 0);
  if (p != 0) {
    p->pcnt = (p->cnt) / 2.0;
    _medianFinalize(context);
  }
}

/*
** Returns the lower_quartile value
*/
static void lower_quartileFinalize(sqlite3_context* context) {
  ModeCtx* p;
  p = (ModeCtx*)sqlite3_aggregate_context(context, 0);
  if (p != 0) {
    p->pcnt = (p->cnt) / 4.0;
    _medianFinalize(context);
  }
}

/*
** Returns the upper_quartile value
*/
static void upper_quartileFinalize(sqlite3_context* context) {
  ModeCtx* p;
  p = (ModeCtx*)sqlite3_aggregate_context(context, 0);
  if (p != 0) {
    p->pcnt = (p->cnt) * 3 / 4.0;
    _medianFinalize(context);
  }
}

/*
** Returns the stdev value
*/
static void stdevFinalize(sqlite3_context* context) {
  StdevCtx* p;
  p = (StdevCtx * )sqlite3_aggregate_context(context, 0);
  if (p && p->cnt > 1) {
    sqlite3_result_double(context, sqrt(p->rS / (p->cnt - 1)));
  }
  else {
    sqlite3_result_double(context, 0.0);
  }
}

/*
** Returns the variance value
*/
static void varianceFinalize(sqlite3_context* context) {
  StdevCtx* p;
  p = (StdevCtx * )sqlite3_aggregate_context(context, 0);
  if (p && p->cnt > 1) {
    sqlite3_result_double(context, p->rS / (p->cnt - 1));
  }
  else {
    sqlite3_result_double(context, 0.0);
  }
}

#ifdef SQLITE_SOUNDEX

/* relicoder factored code */
/*
** Calculates the soundex value of a string
*/

static void soundex(const u8* zIn, char* zResult) {
  int i, j;
  static const unsigned char iCode[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 2, 3, 0, 1, 2, 0, 0, 2, 2, 4, 5, 5, 0,
    1, 2, 6, 2, 3, 0, 1, 0, 2, 0, 2, 0, 0, 0, 0, 0,
    0, 0, 1, 2, 3, 0, 1, 2, 0, 0, 2, 2, 4, 5, 5, 0,
    1, 2, 6, 2, 3, 0, 1, 0, 2, 0, 2, 0, 0, 0, 0, 0,
  };

  for (i = 0; zIn[i] && !isalpha(zIn[i]); i++) {}
  if (zIn[i]) {
    zResult[0] = toupper(zIn[i]);
    for (j = 1; j < 4 && zIn[i]; i++) {
      int code = iCode[zIn[i] & 0x7f];
      if (code > 0) {
        zResult[j++] = code + '0';
      }
    }
    while (j < 4) {
      zResult[j++] = '0';
    }
    zResult[j] = 0;
  }
  else {
    strcpy(zResult, "?000");
  }
}

/*
** computes the number of different characters between the soundex value fo 2 strings
*/
static void differenceFunc(sqlite3_context* context, int argc, sqlite3_value** argv) {
  char zResult1[8];
  char zResult2[8];
  char* zR1 = zResult1;
  char* zR2 = zResult2;
  int rVal = 0;
  int i = 0;
  const u8* zIn1;
  const u8* zIn2;

  assert(argc == 2);

  if (sqlite3_value_type(argv[0]) == SQLITE_NULL || sqlite3_value_type(argv[1]) == SQLITE_NULL) {
    sqlite3_result_null(context);
    return;
  }

  zIn1 = (u8*)sqlite3_value_text(argv[0]);
  zIn2 = (u8*)sqlite3_value_text(argv[1]);

  soundex(zIn1, zR1);
  soundex(zIn2, zR2);

  for (i = 0; i < 4; ++i) {
    if (sqliteCharVal((unsigned char*)zR1) == sqliteCharVal((unsigned char*)zR2))
      ++rVal;
    sqliteNextChar(zR1);
    sqliteNextChar(zR2);
  }
  sqlite3_result_int(context, rVal);
}
#endif

/*
** This function registered all of the above C functions as SQL
** functions.  This should be the only routine in this file with
** external linkage.
*/
int mx_RegisterExtensionFunctions(sqlite3* db) {
  static const struct FuncDef {
    char* zName;
    signed char nArg;
    u8 argType;           /* 0: none.  1: db  2: (-1) */
    u8 eTextRep;          /* 1: UTF-16.  0: UTF-8 */
    u8 needCollSeq;
    void (*xFunc)(sqlite3_context*, int, sqlite3_value**);
  } aFuncs[] = {
    /* math.h */
    //{ (char *)"acos",               1, 0, SQLITE_UTF8,    0, acosFunc  }, // deprecated using: SQLITE_ENABLE_MATH_FUNCTIONS
    //{ (char *)"asin",               1, 0, SQLITE_UTF8,    0, asinFunc  }, // deprecated using: SQLITE_ENABLE_MATH_FUNCTIONS
    //{ (char *)"atan",               1, 0, SQLITE_UTF8,    0, atanFunc  }, // deprecated using: SQLITE_ENABLE_MATH_FUNCTIONS
    { (char *)"atn2",               2, 0, SQLITE_UTF8,    0, atn2Func  },
    /* XXX alias */
    //{ (char *)"atan2",              2, 0, SQLITE_UTF8,    0, atn2Func  },  // deprecated using: SQLITE_ENABLE_MATH_FUNCTIONS
    //{ (char *)"acosh",              1, 0, SQLITE_UTF8,    0, acoshFunc  }, // deprecated using: SQLITE_ENABLE_MATH_FUNCTIONS
    //{ (char *)"asinh",              1, 0, SQLITE_UTF8,    0, asinhFunc  }, // deprecated using: SQLITE_ENABLE_MATH_FUNCTIONS
    //{ (char *)"atanh",              1, 0, SQLITE_UTF8,    0, atanhFunc  }, // deprecated using: SQLITE_ENABLE_MATH_FUNCTIONS

    { (char *)"difference",         2, 0, SQLITE_UTF8,    0, differenceFunc},
    //{ (char *)"degrees",            1, 0, SQLITE_UTF8,    0, rad2degFunc  },  // deprecated using: SQLITE_ENABLE_MATH_FUNCTIONS
    //{ (char *)"radians",            1, 0, SQLITE_UTF8,    0, deg2radFunc  },  // deprecated using: SQLITE_ENABLE_MATH_FUNCTIONS

    //{ (char *)"cos",                1, 0, SQLITE_UTF8,    0, cosFunc  },   // deprecated using: SQLITE_ENABLE_MATH_FUNCTIONS
    //{ (char *)"sin",                1, 0, SQLITE_UTF8,    0, sinFunc },    // deprecated using: SQLITE_ENABLE_MATH_FUNCTIONS
    //{ (char *)"tan",                1, 0, SQLITE_UTF8,    0, tanFunc },    // deprecated using: SQLITE_ENABLE_MATH_FUNCTIONS
    { (char *)"cot",                1, 0, SQLITE_UTF8,    0, cotFunc },
    { (char *)"cosh",               1, 0, SQLITE_UTF8,    0, coshFunc  },
    //{ (char *)"sinh",               1, 0, SQLITE_UTF8,    0, sinhFunc },   // deprecated using: SQLITE_ENABLE_MATH_FUNCTIONS
    //{ (char *)"tanh",               1, 0, SQLITE_UTF8,    0, tanhFunc },   // deprecated using: SQLITE_ENABLE_MATH_FUNCTIONS
    { (char *)"coth",               1, 0, SQLITE_UTF8,    0, cothFunc },

    //{ (char *)"exp",                1, 0, SQLITE_UTF8,    0, expFunc  },     // deprecated using: SQLITE_ENABLE_MATH_FUNCTIONS
    //{ (char *)"log",                1, 0, SQLITE_UTF8,    0, logFunc  },     // deprecated using: SQLITE_ENABLE_MATH_FUNCTIONS
    //{ (char *)"log10",              1, 0, SQLITE_UTF8,    0, log10Func  },   // deprecated using: SQLITE_ENABLE_MATH_FUNCTIONS
    //{ (char *)"power",              2, 0, SQLITE_UTF8,    0, powerFunc  },   // deprecated using: SQLITE_ENABLE_MATH_FUNCTIONS
    { (char *)"sign",               1, 0, SQLITE_UTF8,    0, signFunc },
    //{ (char *)"sqrt",               1, 0, SQLITE_UTF8,    0, sqrtFunc },
    { (char *)"square",             1, 0, SQLITE_UTF8,    0, squareFunc },

    //{ (char *)"ceil",               1, 0, SQLITE_UTF8,    0, ceilFunc },     // deprecated using: SQLITE_ENABLE_MATH_FUNCTIONS
    //{ (char *)"floor",              1, 0, SQLITE_UTF8,    0, floorFunc },    // deprecated using: SQLITE_ENABLE_MATH_FUNCTIONS
    { (char *)"mx_bearing",         4, 0, SQLITE_UTF8,    0, mx_bearing }, // v3.0.253.13
    { (char *)"mx_calc_distance",     5, 0, SQLITE_UTF8,    0, mx_calc_distance }, // v3.0.255.2
    { (char*)"mx_is_plane_in_rw_area", 7, 0, SQLITE_UTF8, 0, mx_is_plane_in_rw_area }, // v3.0.255.2
    // v3.0.303.8.2 can use it for landing accuracy stats. Runway Vector is the center of the runway, so we can test the plane distance from it
    { (char*)"mx_get_shortest_distance_to_rw_vector", 6, 0, SQLITE_UTF8, 0, mx_get_shortest_distance_to_rw_vector }, // v3.303.8.3
    { (char*)"mx_is_plane_in_airport_boundary"      , 3, 0, SQLITE_UTF8, 0, mx_is_plane_in_airport_boundary }, // v3.303.8.3
    { (char*)"mx_get_center_between_2_points", 4, 0, SQLITE_UTF8, 0, mx_get_center_between_2_points },               // v3.303.12_r2
    { (char*)"mx_get_point_based_on_bearing_and_length_in_meters", 4, 0, SQLITE_UTF8, 0, mx_get_point_based_on_bearing_and_length_in_meters }, // v3.303.12_r2

    //{ (char*)"pi",                  0, 0, SQLITE_UTF8,    1, piFunc },        // deprecated using: SQLITE_ENABLE_MATH_FUNCTIONS


    /* string */
    { (char *)"replicate",          2, 0, SQLITE_UTF8,    0, replicateFunc },
    { (char *)"charindex",          2, 0, SQLITE_UTF8,    0, charindexFunc },
    { (char *)"charindex",          3, 0, SQLITE_UTF8,    0, charindexFunc },
    { (char *)"leftstr",            2, 0, SQLITE_UTF8,    0, leftFunc },
    { (char *)"rightstr",           2, 0, SQLITE_UTF8,    0, rightFunc },
#ifndef HAVE_TRIM
    { (char *)"ltrim",              1, 0, SQLITE_UTF8,    0, ltrimFunc },
    { (char *)"rtrim",              1, 0, SQLITE_UTF8,    0, rtrimFunc },
    { (char *)"trim",               1, 0, SQLITE_UTF8,    0, trimFunc },
    { (char *)"replace",            3, 0, SQLITE_UTF8,    0, replaceFunc },
#endif
    { (char *)"reverse",            1, 0, SQLITE_UTF8,    0, reverseFunc },
    { (char *)"proper",             1, 0, SQLITE_UTF8,    0, properFunc },
    { (char *)"padl",               2, 0, SQLITE_UTF8,    0, padlFunc },
    { (char *)"padr",               2, 0, SQLITE_UTF8,    0, padrFunc },
    { (char *)"padc",               2, 0, SQLITE_UTF8,    0, padcFunc },
    { (char *)"strfilter",          2, 0, SQLITE_UTF8,    0, strfilterFunc },

  };
  /* Aggregate functions */
  static const struct FuncDefAgg {
    char* zName;
    signed char nArg;
    u8 argType;
    u8 needCollSeq;
    void (*xStep)(sqlite3_context*, int, sqlite3_value**);
    void (*xFinalize)(sqlite3_context*);
  } aAggs[] = {
    { (char *)"stdev",            1, 0, 0, varianceStep, stdevFinalize  },
    { (char *)"variance",         1, 0, 0, varianceStep, varianceFinalize  },
    { (char *)"mode",             1, 0, 0, modeStep,     modeFinalize  },
    { (char *)"median",           1, 0, 0, modeStep,     medianFinalize  },
    { (char *)"lower_quartile",   1, 0, 0, modeStep,     lower_quartileFinalize  },
    { (char *)"upper_quartile",   1, 0, 0, modeStep,     upper_quartileFinalize  },
  };
  long unsigned int i;

  for (i = 0; i < sizeof(aFuncs) / sizeof(aFuncs[0]); i++) {
    void* pArg = 0;
    switch (aFuncs[i].argType) {
    case 1: pArg = db; break;
    case 2: pArg = (void*)(-1); break;
    }
    //sqlite3CreateFunc
    /* LMH no error checking */
    sqlite3_create_function(db, aFuncs[i].zName, aFuncs[i].nArg,
      aFuncs[i].eTextRep, pArg, aFuncs[i].xFunc, 0, 0);
#if 0
    if (aFuncs[i].needCollSeq) {
      struct FuncDef* pFunc = sqlite3FindFunction(db, aFuncs[i].zName,
        strlen(aFuncs[i].zName), aFuncs[i].nArg, aFuncs[i].eTextRep, 0);
      if (pFunc && aFuncs[i].needCollSeq) {
        pFunc->needCollSeq = 1;
      }
    }
#endif
  }

  for (i = 0; i < sizeof(aAggs) / sizeof(aAggs[0]); i++) {
    void* pArg = 0;
    switch (aAggs[i].argType) {
    case 1: pArg = db; break;
    case 2: pArg = (void*)(-1); break;
    }
    //sqlite3CreateFunc
    /* LMH no error checking */
    sqlite3_create_function(db, aAggs[i].zName, aAggs[i].nArg, SQLITE_UTF8,
      pArg, 0, aAggs[i].xStep, aAggs[i].xFinalize);
#if 0
    if (aAggs[i].needCollSeq) {
      struct FuncDefAgg* pFunc = sqlite3FindFunction(db, aAggs[i].zName,
        strlen(aAggs[i].zName), aAggs[i].nArg, SQLITE_UTF8, 0);
      if (pFunc && aAggs[i].needCollSeq) {
        pFunc->needCollSeq = 1;
      }
    }
#endif
  }
  return 0;
}

#ifdef COMPILE_SQLITE_EXTENSIONS_AS_LOADABLE_MODULE
int sqlite3_extension_init(
  sqlite3* db, char** pzErrMsg, const sqlite3_api_routines* pApi) {
  SQLITE_EXTENSION_INIT2(pApi);
  RegisterExtensionFunctions(db);
  return 0;
}
#endif /* COMPILE_SQLITE_EXTENSIONS_AS_LOADABLE_MODULE */

xx_map map_make(cmp_func cmp) {
  xx_map r;
  r.cmp = cmp;
  r.base = 0;

  return r;
}

void* xcalloc(size_t nmemb, size_t size, char* s) {
  void* ret = calloc(nmemb, size);
  return ret;
}

void xfree(void* p) {
  free(p);
}

void node_insert(xx_node** n, cmp_func cmp, void* e) {
  int c;
  xx_node* nn;
  if (*n == 0) {
    nn = (xx_node*)xcalloc(1, sizeof(xx_node), (char*)"for node");
    nn->data = e;
    nn->count = 1;
    *n = nn;
  }
  else {
    c = cmp((*n)->data, e);
    if (0 == c) {
      ++((*n)->count);
      xfree(e);
    }
    else if (c > 0) {
      /* put it right here */
      node_insert(&((*n)->l), cmp, e);
    }
    else {
      node_insert(&((*n)->r), cmp, e);
    }
  }
}

void map_insert(xx_map* m, void* e) {
  node_insert(&(m->base), m->cmp, e);
}

void node_iterate(xx_node* n, map_iterator iter, void* p) {
  if (n) {
    if (n->l)
      node_iterate(n->l, iter, p);
    iter(n->data, n->count, p);
    if (n->r)
      node_iterate(n->r, iter, p);
  }
}

void map_iterate(xx_map* m, map_iterator iter, void* p) {
  node_iterate(m->base, iter, p);
}

void node_destroy(xx_node* n) {
  if (0 != n) {
    xfree(n->data);
    if (n->l)
      node_destroy(n->l);
    if (n->r)
      node_destroy(n->r);

    xfree(n);
  }
}

void map_destroy(xx_map* m) {
  node_destroy(m->base);
}

int int_cmp(const void* a, const void* b) {
  int64_t aa = *(int64_t*)(a);
  int64_t bb = *(int64_t*)(b);
  /* printf("cmp %d <=> %d\n",aa,bb); */
  if (aa == bb)
    return 0;
  else if (aa < bb)
    return -1;
  else
    return 1;
}

int double_cmp(const void* a, const void* b) {
  double aa = *(double*)(a);
  double bb = *(double*)(b);
  /* printf("cmp %d <=> %d\n",aa,bb); */
  if (aa == bb)
    return 0;
  else if (aa < bb)
    return -1;
  else
    return 1;
}

void print_elem(void* e, int64_t c, void* p) {
  int ee = *(int*)(e);
  //printf("%d => %ld\n", ee, c);

  //  https://stackoverflow.com/questions/9225567/how-to-portably-print-a-int64-t-type-in-c
  printf("%d => %" PRId64 "\n", ee, c);
}


} // end of namespace

#endif // !EXTENSIONFUNCTION_H_
