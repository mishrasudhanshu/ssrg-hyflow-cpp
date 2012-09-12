//====================================================
//                                                    
//  EldoS MsgConnect                                 
//                                                    
//   Copyright (c) 2001-2010, EldoS                   
//                                                    
//====================================================
 
#ifndef __MCPLATFORMS__
#define __MCPLATFORMS__

#include "MC.h"
#ifdef USE_NAMESPACE
namespace MsgConnect
{
#endif

// =----------------- ! user-configurable part begins here ! -----------------=

//#define LINUX
//#define WINFULL
//#define WINCE
//#define PALM_CW
//#define PALM_GNU

// =------------------ ! user-configurable part ends here ! ------------------=

#ifdef _WIN32
#  include <windows.h>
#endif

#ifdef QNX
#include <stddef.h>
#endif

#if !defined(_WIN32) && defined(__GNUC__) && !defined(QNX)
#if !defined(wchar_t) 
#   define wchar_t short int
#endif
#endif

int wstrlen(wchar_t*);
wchar_t* wstrcat(wchar_t*, wchar_t*);
long wstrtol(wchar_t*, wchar_t**, int);
double wstrtod(wchar_t*, wchar_t**);
wchar_t* wstrdup(wchar_t*);
wchar_t* str2wstr(char*, unsigned int);
char* wstr2str(wchar_t*, unsigned int);

//disable _snprintf
#undef _SNPRINTF_ENABLED_

#if defined(PALM_CW) || defined(PALM_GNU)
#  define M68K
#  define PALM
#endif

#if defined(_WIN32) || defined(__GNUC__)
#   define _SNPRINTF_ENABLED_
#   ifndef _WIN32_WCE
#		define WINFULL
#   else
#		ifndef WINCE
#			define WINCE
#		endif
#   endif
#endif

#ifdef __GNUC__
#define _snprintf snprintf
#endif

#ifdef PALM_CW
#  include <string.h>
#  include <stdlib.h>
#  ifndef NULL
#    define NULL 0L
#  endif
#else
#  include <stdio.h>
#  include <string.h>
#  ifndef PALM_GNU
#    include <ctype.h>
#    include <stdlib.h>
#  endif
#endif

#ifndef _WIN32
typedef char* LPSTR;
#endif

typedef LPSTR* PLPSTR;

#ifndef CP_ACP
#define CP_ACP 0
#endif

#ifdef _WIN32_WCE
#   define strdup _strdup
double strtod(char *, char **);
#endif

#ifdef PALM_GNU
#  define DEFSECT __attribute__ ((section ("DEFSECT")))
#else
#  define DEFSECT /**/
#endif

DEFSECT double	TurnDouble(double d);
DEFSECT mcInt32	TurnLong(mcInt32 d);

#ifdef PALM
DEFSECT char*	strdup(const char*);
DEFSECT double 	strtod(char *, char **);
DEFSECT char*	strrchr(const char*,char);
DEFSECT int		isspace(char);
DEFSECT void	MakeHexByteAt(char* P, char C);
DEFSECT void	MakeLongAt(char* P, mcInt32 L, int N);
DEFSECT void	MakeDoubleAt(char* P, double D, int Ni, int Nf);
#endif

#ifdef PALM_GNU
extern "C" {
     void *malloc (size_t __size);
     void free (void *__ptr);
     void *realloc (void *__ptr, size_t __size);
}
#  undef NULL
#  define NULL 0L
#endif

#ifdef PALM_CW
//void*	malloc(mcInt32);
//void	free(void*);
long	strtol(char * , char ** , int);
int		_strncmp(const char*,const char*,long);
//char*	strncpy(char*,const char*,long);
//char*	strcat(char*,const char*);
char*	strchr(const char*,char);
#else
#define _strncmp strncmp
#endif

typedef void** PVOIDLIST;

long  DEFSECT UpperTrimCmp(char* S1, char* S2);
LPSTR DEFSECT CharNstr(char C, char* S, char* D);
LPSTR DEFSECT CharNstr(char C, char* S);
LPSTR DEFSECT StrNchar(char* S, char C, char* D);
LPSTR DEFSECT StrNchar(char* S, char C);
LPSTR DEFSECT Char2Str(char C, char* S);
LPSTR DEFSECT AddStr(char* S1, char* S2);

#ifdef USE_NAMESPACE
}
#endif

#ifndef _strdup 
#define _strdup strdup
#endif

#endif
