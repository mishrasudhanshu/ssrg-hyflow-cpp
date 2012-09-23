//====================================================
//                                                    
//  EldoS MsgConnect                                 
//                                                    
//   Copyright (c) 2001-2010, EldoS                   
//                                                    
//====================================================

#ifndef __MCTHREADS__
#define __MCTHREADS__

#include "MC.h"
#include "MCUtils.h"
#include "MCSyncs.h"
#include "MCLists.h"
 
#ifdef USE_NAMESPACE
namespace MsgConnect
{
#endif

#ifndef __APPLE__
unsigned long IntGetCurrentThreadID(void);
MCHandle GetCurrentThreadHandle(void);
#endif

class EMCThread: public EMCError {
public:
	EMCThread(long l):EMCError(l) {}
};

typedef void(*MCThreadMethod)(void* resv);
typedef void(*MCThreadProc)(void *resv, void *sender);

#ifdef _WIN32
typedef enum {tpIdle = 0, tpLowest, tpLower, tpNormal, tpHigher, tpHighest,
	tpTimeCritical} MCThreadPriority;
#endif

class MCThread : public MCObject {
private:
	MCHandle FHandle;
public:
#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
	unsigned FThreadID;
#else
#if defined(__APPLE__)
	pthread_t FThreadID;
#else
	// ** FThreadID is not TMCHandle in Linux **
	unsigned long FThreadID;
#endif
	TSemaphore FCreateSuspendedSem;
	bool FInitialSuspendDone;
#endif
protected:
//friend long ThreadProc(MCThread* Thread);
//friend int __fastcall ThreadProc(void* p);
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
friend long ThreadProc(void* p);
#else
#endif
	bool            FCreateSuspended;
	bool            FTerminated;
	bool            FSuspended;
	bool            FFreeOnTerminate;
	bool            FFinished;
	mcInt32         FReturnValue;
	MCThreadProc    FOnTerminate;
	MCThreadMethod  FMethod;
	EMCError*       FSynchronizeException;
	EMCError*       FFatalException;
    char*           FThreadName;
    
	void                CheckThreadError(mcInt32 ErrCode);
	void                CheckThreadError(bool Success);

protected:
	virtual void        Free(void);
	virtual void        Execute(void) PURE_VIRTUAL;
	virtual bool		CanExecute(void);
	mcInt32             getReturnValue(void);
	void                setReturnValue(mcInt32 value);
public:
	MCThread(bool CreateSuspended, int Priority = -1);
	virtual ~MCThread();

	virtual void        AfterConstruction(void);
	bool                getTerminated(void);
	void                Resume(void);
	void                Suspend(void);
	virtual void        Terminate(void);
	unsigned long		WaitFor(void);
	EMCError*           getFatalException(void);
	bool                getFreeOnTerminate(void);
	void                setFreeOnTerminate(bool value);
	MCHandle             getHandle(void);
    void                setThreadName(const char* threadName);
    const char*         getThreadName(void);
#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
#else
	// ** Priority is an Integer value in Linux
	long                getPriority(void);
	void                setPriority(long Value);
	long                getPolicy(void);
	void                setPolicy(long Value);
#ifdef __APPLE__
	pthread_t           getThreadID(void);
#else
	unsigned long	    getThreadID(void);
#endif
#endif
	bool                getSuspended(void);
	void                setSuspended(bool Value);
};

//typedef int __fastcall (*MCThreadFunc)(void* Parameter);
#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
#else
typedef long (*MCThreadFunc)(void*);
#endif

typedef struct sThreadRec {
	MCThreadFunc Func;
	void* Parameter;
} ThreadRec;

#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
#elif defined(__APPLE__)
pthread_t GetCurrentThreadID(void);
int BeginThread(pthread_attr_t* SecurityAttributes, MCThreadFunc ThreadFunc,
    void* Parameter, pthread_t &ThreadId);
void EndThread(int ExitCode);
#else
unsigned long GetCurrentThreadID(void);
int BeginThread(pthread_attr_t* SecurityAttributes, MCThreadFunc ThreadFunc,
    void* Parameter, unsigned long &ThreadId);
void EndThread(int ExitCode);
#endif

void InitializeThreads(void);

//----------- Thread pool interface declaration ------
//#define THREADINFO

class MCWorkerThread;

class MCThreadFactory
{
public:
    virtual MCWorkerThread* CreateThread(void) PURE_VIRTUAL;
};

class MCJobErrorHandler
{
public:
#ifdef USE_CPPEXCEPTIONS
    virtual void HandleError(const EMCError& Error) PURE_VIRTUAL;
#else
    virtual void HandleError(unsigned int ErrorCode) PURE_VIRTUAL;
#endif
};


class MCThreadJob
{
friend class MCWorkerThread;
protected:
    MCWorkerThread*     FThread;
    char*               FJobName;

public:
    MCThreadJob(void);
    virtual ~MCThreadJob();
    virtual void Run(void) PURE_VIRTUAL;
    bool getTerminated(void);
    void setJobName(const char* jobName);
    const char* getJobName(void);
};


class MCThreadPool;

class MCWorkerThread: public MCThread
{
friend class MCThreadPool;
protected:
    MCThreadPool*   FPool;
    MCCriticalSection FGuard;
    MCThreadJob*    FJob;
	virtual void	KickThread(bool MessageFlag) {};
public:
    MCWorkerThread(MCThreadPool* Pool, int Priority = -1);
    virtual ~MCWorkerThread();
    virtual void    Execute(void);
	virtual bool	CanExecute(void);
    virtual void    Terminate(void);
};


class MCThreadPool
{
friend class MCWorkerThread;
protected:
    unsigned int        FSize;
    MCList              FThreadList;
    MCList              FJobList;

#ifndef _WIN32
    pthread_mutex_t	FGuard;
    pthread_cond_t	FNonEmpty;
#else
#endif
    MCThreadFactory*    FFactory;
    MCJobErrorHandler*  FErrorHandler;
    unsigned int        FInactivityIdle;    
    unsigned int        FMaxSize;
    unsigned int        FJobsRan;
    unsigned int        FBusyCount;
public:
    MCThreadPool(MCThreadFactory* Factory, MCJobErrorHandler* ErrorHandler);
    virtual ~MCThreadPool();

    void    PostJob(MCThreadJob* Job);
	void    FinalizeJob(MCThreadJob* Job);
    void    Clear(void);
    
    unsigned int 
            getSize(void);
    void    setSize(unsigned int size);
    unsigned int
            getMaxSize(void);
    void    setMaxSize(unsigned int size);
    unsigned int 
            getInactivityIdle(void);
    void    setInactivityIdle(unsigned int Idle);
    unsigned int 
            getJobsRan(void);
    void    setJobsRan(unsigned int jobsRan);
    unsigned int
            getBusyCount(void);
};


#ifdef USE_NAMESPACE
}
#endif


#endif
