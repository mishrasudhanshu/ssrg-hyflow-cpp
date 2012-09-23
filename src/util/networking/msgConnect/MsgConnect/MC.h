//====================================================
//                                                    
//  EldoS MsgConnect                                 
//                                                    
//   Copyright (c) 2001-2010, EldoS                   
//                                                    
//====================================================
#ifndef __MC__
#define __MC__

#ifndef OMIT_NAMESPACE
#	if !defined(__BORLANDC__) && !defined(__APPLE__)
#		define USE_NAMESPACE
#	else
#		undef USE_NAMESPACE
#	endif
#else
#	undef USE_NAMESPACE
#endif

#ifdef __BORLANDC__
#	ifndef WIN32
#		define WIN32
#	endif
#endif

#define _CRT_SECURE_NO_WARNINGS

#if defined(WIN32) && !defined(_WIN32_WCE)
#	if(_WIN32_WINNT < 0x0400)
#		define _WIN32_WINNT 0x0400
#	endif
#	if (_MSC_VER >= 1300)
#   	include <winsock.h>
#	endif
#   include <windows.h>
#   include <tchar.h>
//#	include <ws2tcpip.h>
#endif

#ifdef _WIN32_WCE
#   include <winsock.h>
#	include <tchar.h>
#endif

#if !defined(_WIN32) && !defined(__APPLE__) && defined(__GNUC__)
#	include <boost/thread.hpp>
#endif

#if !defined(_WIN32) 
#	include <inttypes.h>
#	include <stdint.h>
#endif

#if !defined(_WIN32) && defined(__GNUC__)
	typedef int_least32_t mcInt32;
	typedef uint_least32_t mcUInt32;
	typedef uintptr_t mcPtrInt;
#else
	typedef long mcInt32;
	typedef unsigned long mcUInt32;
	#ifdef _WIN64
		typedef unsigned long long mcPtrInt;
	#else
		typedef unsigned long mcPtrInt;
	#endif
#endif

#if defined(_WIN32_WCE) || defined(UNICODE)
#   include <ctype.h>
#	include <windows.h>
	wchar_t* s2w(const char* s); // use operator delete to release memory after string using
#endif
    

#ifndef SD_BOTH
	const int SD_BOTH = 2;
#endif

#if defined(__GNUC__)
#	define USE_CPPEXCEPTIONS
#endif

#if defined(_WIN32) && defined(_WIN32_WCE)
#   ifdef _WIN32_WCE
#       undef USE_CPPEXCEPTIONS
#   else
#		define USE_CPPEXCEPTIONS
#	endif
#else
#   define USE_CPPEXCEPTIONS
#endif

//#define USE_CPPEXCEPTIONS
#define NO_CHECK_DISPATCHTHREAD

//define some macros for handling SEH & C++ exceptions in one way 
#ifndef ELDOS_CPP_EXCEPTIONS
#	ifndef USE_CPPEXCEPTIONS //use the SEH instead
#   	define MCERRORCODEBASE	1000
#   	define MCRETURNCODE MCERRORCODEBASE+1
#   	define MCRETURN		MCERRORCODEBASE+2
//#   define FINALLY(z)	__finally { z }
#   	define try __try
#   	define TRY_BLOCK __try
#   	define CATCH_EVERY __except(1)
#   	define RETURN_EXCEPT RaiseException(MCRETURNCODE, 0, 0, NULL)
#   	define RETURN RaiseException(MCRETURN, 0, 0, NULL);
#   	define CATCH_RETURN } __except(((mcInt32)((_EXCEPTION_POINTERS*)GetExceptionInformation())->ExceptionRecord->ExceptionInformation) != 1) {}
//#   undef FINALLY
#   	define FINALLY(z)	__finally{z}
#   	define THROW_ERROR(x) RaiseException(MCERRORCODEBASE+x+2, 0, 0, NULL)
#  		define CATCH_MCERROR __except( ((GetExceptionCode() >= MCERRORCODEBASE+2) && (GetExceptionCode() <= MCERRORCODEBASE+2+1000)) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH )
# 	else
#   	define TRY_BLOCK try
#   	define CATCH_EVERY catch(...)
#   	define RETURN_EXCEPT throw 1
#   	define RETURN throw((mcInt32)1);
#   	define CATCH_RETURN } catch(mcInt32) {}
#   	define FINALLY(z)	catch(...) { z throw; } z
#   	define THROW_ERROR(x) throw EMCError(x)
#   	define CATCH_MCERROR catch(EMCError&/* x*/)
# 	endif
# 	define ELDOS_CPP_EXCEPTIONS
#endif

#define TRY_RETURN try {

#ifdef _WIN32_WCE
#   define try __try
#   define stricmp _stricmp
#   define strncmpi _strnicmp
#   include <winbase.h>
#   include <stdlib.h>
//int     __cdecl _stricmp(const char *, const char *);
//int     __cdecl _strnicmp(const char *, const char *, size_t);

#   define assert(p) if(!p) exit(1);
#   include <wtypes.h>
    void DebugWrite(LPSTR s);
    char* strdup(const char*);
#   define snprintf _snprintf
#else
#   include <assert.h>
typedef unsigned char byte;
#endif

#ifndef _WIN32
typedef unsigned long DWORD;
#endif

typedef unsigned long Cardinal;

#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
#ifndef __USE_UNIX98
#   define __USE_UNIX98
#endif

    typedef long long __int64;
    typedef unsigned char byte;

#	define UNALIGNED
#	define TCHAR char
#	define __T(x)      x
#	define _T(x)       __T(x)
#	define TEXT(x)    __T(x)
#	define _TEXT(x)    __T(x)
#	define LPTSTR		char *
#	define LPCTSTR		const char *

#   define Sleep(s) usleep((mcInt32)s*1000)
#   define Random random
#   define closesocket(s) close(s)
#   ifndef __APPLE__
#       define GetCurrentThreadId IntGetCurrentThreadID
#       define GetCurrentThread IntGetCurrentThreadID
#   endif
#   include <sys/ioctl.h>
#   include <sys/socket.h>
#   include <sys/time.h>
#   include <sys/types.h>
#   include <netinet/in.h>
#   include <arpa/inet.h>
#   include <netdb.h>
#   include <fcntl.h>
#   include <unistd.h>
#   include <pthread.h>
#   include <signal.h>
#   include <errno.h>
#   define __stdcall
#else
//#if defined(__MINGW32__) || defined(__MINGW64__)
//	typedef long long __int64;
//#endif
#   include <windows.h>
#endif
    
#if defined(_MSC_VER) && !defined(_WIN32_WCE)
#   define snprintf _snprintf
#   define strncmpi _strnicmp
#endif

#ifdef _WIN32
#	define LAST_ERROR WSAGetLastError()
#else
#	define LAST_ERROR errno
#endif


//Socket section
#define SOCKETBUF_INITIAL_SIZE  512

//#ifndef _WIN32_WCE
#	include <math.h>
#	include <limits.h>
#	include <stdlib.h>
#	include <stdio.h>
#	include <string.h>
//#endif

#ifndef _WIN32_WCE
#   define MCASSERT(x) assert(x)
#else
#   define MCASSERT(x)
#endif

#ifdef _WIN32
#	ifndef _WINDOWS
#		define _WINDOWS
#	endif
#endif


#define MC_NO_PURE_VIRTUAL

#ifdef USE_NAMESPACE
namespace MsgConnect
{
#endif

const unsigned long INFINITE = 0xFFFFFFFFul; // Infinite timeout
const int INVALID_SOCKET = -1;
const int SOCKET_ERROR = -1;
mcUInt32 GetTickCount(void);

#ifndef __APPLE__
extern mcUInt32  MCMainThreadID;
#else
extern pthread_t MCMainThreadID;
#endif

#ifdef MC_NO_PURE_VIRTUAL

#define PURE_VIRTUAL

void MC_pure_virtual(void);

#else
#define PURE_VIRTUAL = 0
#endif

#ifdef USE_NAMESPACE
}
#endif

#endif
