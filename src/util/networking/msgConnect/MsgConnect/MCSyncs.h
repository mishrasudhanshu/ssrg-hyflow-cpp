//====================================================
//                                                    
//  EldoS MsgConnect                                 
//                                                    
//   Copyright (c) 2001-2010, EldoS                   
//                                                    
//====================================================
 
#ifndef __MCSYNCS__
#define __MCSYNCS__

#include "MC.h"

#ifdef USE_NAMESPACE
namespace MsgConnect
{
#endif

#ifndef _WINDOWS
typedef void* MCHandle;
#else
typedef HANDLE MCHandle;
#endif

typedef enum {
		wrSignaled, 
		wrTimeout, 
		wrAbandoned, 
		wrError
} MCWaitResult;

#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
#if !defined(QNX)
#   include <sys/types.h>
#   include <sys/ipc.h>
#   include <sys/sem.h>
#else
#	include <sys/mman.h> 
#	include <sys/stat.h>
#endif
#	define	DEFFILEMODE	(S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)
#	include <semaphore.h>
typedef sem_t TSemaphore;
#endif


class MCSynchroObject {
#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
#else
protected:
#if !defined(QNX)
    key_t FId;
#else
	TSemaphore *sem;
#endif
    int FEvent;
    int FOrig;
    unsigned long FMaxCount;
public:
#if !defined(QNX)
	void Initialize(int InitialState, key_t Id, bool Orig, unsigned long MaxCount);
#else
	void Initialize(int InitialState, char* FileName, bool Orig, unsigned long MaxCount);
#endif
	void Release(int Num = 1);
#endif
public:
	MCSynchroObject();
	virtual MCWaitResult WaitFor(unsigned long Timeout);
    virtual ~MCSynchroObject();
};


class MCEvent: public MCSynchroObject {
#ifdef __GNUC__
private:
	char*		FFileName;
    int			FFileObj;
	bool		FManualReset;
public:
	void		Initialize(void* EventAttributes, bool ManualReset, bool InitialState, bool InitialOwner, const char* Name);
#endif
public:
				MCEvent(void* EventAttributes, bool ManualReset, bool InitialState, const char* Name, bool OpenOnly = false);
	virtual		~MCEvent();
	virtual	    MCWaitResult WaitFor(unsigned long Timeout);
    void        setEvent(void);
    void        resetEvent(void);
};


#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
//#define __USE_UNIX98
#if !defined(QNX)
#if !defined(__APPLE__)
#include <bits/pthreadtypes.h>
#endif
#endif
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

typedef pthread_mutex_t TRTLCriticalSection;

long InitializeCriticalSection(TRTLCriticalSection* lpCriticalSection);
long EnterCriticalSection(TRTLCriticalSection* lpCriticalSection);
long LeaveCriticalSection(TRTLCriticalSection* lpCriticalSection);
bool TryEnterCriticalSection(TRTLCriticalSection* lpCriticalSection);
long DeleteCriticalSection(TRTLCriticalSection* lpCriticalSection);

#else
#include <windows.h>
#ifndef _WIN32_WCE
#include <io.h>
#endif

typedef CRITICAL_SECTION TRTLCriticalSection;

#endif

class MCCriticalSection
{
protected:
#ifdef _WIN32
#else
    pthread_mutex_t FMutex;
    pthread_mutexattr_t FMutexAttr;
#endif
    //unsigned int startWaiting;
    //unsigned int sumWaiting;
public:
    MCCriticalSection(void);
    ~MCCriticalSection();
    void Enter(void);
    void Leave(void);
};

#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
class MCSemaphore : public MCSynchroObject
{
public:
#if !defined(QNX)
	MCSemaphore(int InitialState, key_t Id, bool Orig, unsigned long MaxCount = 0xFFFFFFFF);
#else
	MCSemaphore(int InitialState, char* FileName, bool Orig, unsigned long MaxCount = 0xFFE);
#endif
};
#endif


#ifdef USE_NAMESPACE
}
#endif


#endif
