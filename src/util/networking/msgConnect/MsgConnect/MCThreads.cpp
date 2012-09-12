//====================================================
//                                                    
//  EldoS MsgConnect                                 
//                                                    
//   Copyright (c) 2001-2010, EldoS                   
//                                                    
//====================================================
#include "MC.h"
#include "MCThreads.h"
 
#ifdef USE_NAMESPACE
namespace MsgConnect
{
#endif

//#define THREADINFO
    
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
typedef pthread_cond_t* MCCondVar;
#   define InitializeCriticalSection(x) pthread_mutex_init(x, NULL)
#   define DeleteCriticalSection(x) pthread_mutex_destroy(x)
#   define EnterCriticalSection(x) pthread_mutex_lock(x)
#   define LeaveCriticalSection(x) pthread_mutex_unlock(x)
#endif

typedef struct {
	MCThread* Thread;
#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
#else
	MCCondVar Signal;
#endif
} MCSyncProc;

bool ProcPosted;
MCList* SyncList = NULL;
//TRTLCriticalSection ThreadLock;
long ThreadCount;

void InitializeThreads(void)
{
        //InitializeCriticalSection(&ThreadLock);
}

#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__) || defined(__APPLE__))
unsigned long GetCurrentThreadID(void)
{
	return(pthread_self());
}
#elif defined(__APPLE__)
pthread_t GetCurrentThreadID(void)
{
	return pthread_self();
}
#endif

unsigned long IntGetCurrentThreadID(void)
{
#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
#else
	return getpid();
#endif
}

MCHandle GetCurrentThreadHandle(void)
{
	MCHandle DstHandle;
#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
#else
	DstHandle = (void*)pthread_self();
#endif
	return(DstHandle);
}

void AddThread(void)
{
}

void RemoveThread(void)
{
}

#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
long ThreadProc(void* p)
#else
#endif
{
	bool FreeThread = false;
	long r = 0;
    MCThread* Thread = (MCThread*)p;
    Thread->FReturnValue = 0;
    //suspend if needed
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
	if(Thread->FSuspended)
		sem_wait(&Thread->FCreateSuspendedSem);
	else
		Thread->FInitialSuspendDone = true;
#endif
	if(!Thread->getTerminated())
	TRY_BLOCK 
    {
	    while (!Thread->CanExecute() && (r < 1000))
		{
			r++;
			Sleep(1);
		}
		r = 0;
		if (Thread->CanExecute())
			Thread->Execute();
		else
			r = 0;
    }
    CATCH_EVERY
    {
	    Thread->FFatalException = NULL;
    }
    FreeThread = Thread->FFreeOnTerminate;
	r = Thread->FReturnValue;
	Thread->FFinished = true;
	//Thread->DoTerminate();
	if(FreeThread)
	    delete Thread;
    return r;
}

MCThread::MCThread(bool CreateSuspended, int Priority)
{
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
	long ErrCode;
	FInitialSuspendDone = false;
#endif
	FTerminated = false;
	//AddThread();
	FSuspended = CreateSuspended;
	FCreateSuspended = CreateSuspended;
	FFatalException = NULL;
	FThreadName = NULL;
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
	if(!FSuspended)
		FInitialSuspendDone = true;
#endif

#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
#else
	//FCreateSuspendedSem = 0;
	sem_init(&FCreateSuspendedSem, false, 0);
	ErrCode = BeginThread(NULL, &ThreadProc, this, FThreadID);
#endif
	//AfterConstruction();
}

MCThread::~MCThread()
{
	
    Free();
}

void MCThread::Free(void)
{
	if (NULL != FThreadName)
        free(FThreadName);
    
    if((FThreadID != 0) && !FFinished)
	{
		Terminate();
		if(FCreateSuspended)
			Resume();
		WaitFor();
	}
#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
#else
	// This final check is to ensure that even if the thread was never waited on
	// its resources will be freed.
	if(FThreadID != 0)
		pthread_detach(FThreadID);
	sem_destroy(&FCreateSuspendedSem);
#endif
    if(FFatalException)
		delete FFatalException;
	RemoveThread();
}

mcInt32 MCThread::getReturnValue(void)
{
	return FReturnValue;
}

void MCThread::setReturnValue(mcInt32 value)
{
	FReturnValue = value;
}

bool MCThread::getTerminated(void)
{
	return FTerminated;
}

EMCError* MCThread::getFatalException(void)
{
	return FFatalException;
}

bool MCThread::getFreeOnTerminate(void)
{
	return FFreeOnTerminate;
}

void MCThread::setFreeOnTerminate(bool value)
{
	FFreeOnTerminate = value;
}

MCHandle MCThread::getHandle(void)
{
	return FHandle;
}

bool MCThread::getSuspended(void)
{
	return FSuspended;
}

void MCThread::setThreadName(const char* threadName)
{
    if (NULL != FThreadName)
    {
        free(FThreadName);
        FThreadName = NULL;
    }
    if (NULL != threadName)
    {
        FThreadName = (char*)malloc(strlen(threadName)+1);
        strcpy(FThreadName, threadName);
    }
}

const char* MCThread::getThreadName(void)
{
    return FThreadName;
}

#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
#elif defined(__APPLE__)
pthread_t
#else
unsigned long
#endif
MCThread::getThreadID(void)
{
	return FThreadID;
}
 
/*
MCThreadProc MCThread::getOnTerminate(void)
{
	return(FOnTerminate);
}

void MCThread::setOnTerminate(MCThreadProc value)
{
	FOnTerminate = value;
}
*/

void MCThread::AfterConstruction(void)
{
	if(!FCreateSuspended)
		Resume();
}

void MCThread::CheckThreadError(mcInt32 ErrCode)
{
	if(ErrCode != 0)
        THROW_ERROR(ErrCode);
}

void MCThread::CheckThreadError(bool Success)
{
	if(!Success)
		CheckThreadError((mcInt32)MCError_ThreadError);
}

/*
void MCThread::CallOnTerminate(void)
{
	if(FOnTerminate)
		FOnTerminate(NULL,this);
}

void MCThread::DoTerminate(void)
{
}
*/

#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
#else
long MCThread::getPriority(void)
{
	int p;
	sched_param j;

	/*
    Linux Priority is based on the Schedule policy.
    There are 3 different kinds of policy.  See SetPolicy.

        Policy          Type         Priority
      ----------      --------       --------
      SCHED_RR        RealTime         0-99
      SCHED_FIFO      RealTime         0-99
      SCHED_OTHER     Regular           0

    SCHED_RR and SCHED_FIFO can only be set by root.
	*/
	CheckThreadError((mcInt32)pthread_getschedparam(FThreadID, &p, &j));
	return(j.sched_priority);
}

/*
  Note that to fully utilize Linux Scheduling, see SetPolicy.
*/

void MCThread::setPriority(long Value)
{
	sched_param p;

	if(Value != getPriority())
	{
		p.sched_priority = Value;
		CheckThreadError((mcInt32)pthread_setschedparam(FThreadID, getPolicy(), &p));
	}
}

long MCThread::getPolicy(void)
{
	sched_param j;
	int p;

	CheckThreadError((mcInt32)pthread_getschedparam(FThreadID, &p, &j));
	return(p);
}

/*
  Note that to fully utilize Linux Scheduling, SetPolicy needs to
  be used as well.  See SetPriority for the relationship between these
  methods.
*/

void MCThread::setPolicy(long Value)
{
	sched_param p;

	if(Value != getPolicy())
	{
		p.sched_priority = getPriority();
		CheckThreadError((mcInt32)pthread_setschedparam(FThreadID, Value, &p));
	}
}
#endif


void MCThread::setSuspended(bool Value)
{
	if(Value != FSuspended)
		if(Value)
			Suspend();
		else
			Resume();
}

#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
#else
/*
  About Suspend and Resume. POSIX does not support suspending/resuming a thread.
  Suspending a thread is considerd dangerous since it is not guaranteed where the
  thread would be suspend. It might be holding a lock, mutex or it might be inside
  a critical section.  In order to simulate it in Linux we've used signals. To
  suspend, a thread SIGSTOP is sent and to resume, SIGCONT is sent. Note that this
  is Linux only i.e. according to POSIX if a thread receives SIGSTOP then the
  entire process is stopped. However Linux doesn't entirely exhibit the POSIX-mandated
  behaviour. If and when it fully complies with the POSIX standard then suspend
  and resume won't work.
*/

void MCThread::Suspend(void)
{
	FSuspended = true;
	CheckThreadError((mcInt32)pthread_kill(FThreadID, SIGSTOP));
}

void MCThread::Resume(void)
{
	if(!FInitialSuspendDone)
	{
		FInitialSuspendDone = true;
		sem_post(&FCreateSuspendedSem);
	} else
		CheckThreadError((mcInt32)pthread_kill(FThreadID, SIGCONT));
	FSuspended = false;
}
#endif

void MCThread::Terminate(void)
{
	FTerminated = true;
}

unsigned long MCThread::WaitFor(void)
#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
#else
{
	void* x;
	unsigned long r = 0;
#ifdef __APPLE__
	pthread_t ID = FThreadID;
#else
	unsigned long ID = FThreadID;
#endif

	if(GetCurrentThreadID() == MCMainThreadID)
		while(!FFinished)
		{
			sleep(10);
			//CheckSynchronize();
		}
	FThreadID = 0;
	x = &r;
	if (pthread_join(ID, &x) == 0)
		return(r);
	else
		return 0;
}
#endif

bool MCThread::CanExecute(void)
{
	return true;
}

#ifdef MC_NO_PURE_VIRTUAL
void MCThread::Execute(void)
{
	MC_pure_virtual();
}
#endif


#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
#elif defined(__APPLE__)

int BeginThread(pthread_attr_t* SecurityAttributes, MCThreadFunc ThreadFunc,
    void* Parameter, pthread_t &ThreadId)
{
//	if(BeginThreadProc)
//		return(BeginThreadProc(Attribute, ThreadFunc, Parameter, ThreadId))
//	else
	    return(pthread_create((pthread_t*)&ThreadId, SecurityAttributes, (void *(*) (void *))ThreadFunc, Parameter));
}

void EndThread(int ExitCode)
{
//	pthread_detach(GetCurrentThreadID);
	pthread_exit((void*)ExitCode);
}

#else

int BeginThread(pthread_attr_t* SecurityAttributes, MCThreadFunc ThreadFunc,
    void* Parameter, unsigned long &ThreadId)
{
//	if(BeginThreadProc)
//		return(BeginThreadProc(Attribute, ThreadFunc, Parameter, ThreadId))
//	else
	    return(pthread_create((pthread_t*)&ThreadId, SecurityAttributes, (void *(*) (void *))ThreadFunc, Parameter));
}

void EndThread(int ExitCode)
{
//	pthread_detach(GetCurrentThreadID);
	pthread_exit((void*)ExitCode);
}
#endif

// ----------------MCThreadPool -------------------------------

MCThreadJob::MCThreadJob(void)
:FThread(NULL)
, FJobName(NULL)
#ifdef _TESTPOOL
, FJobRunning(NULL)
#endif
{
       
}

MCThreadJob::~MCThreadJob()
{
    if (NULL != FJobName)
        free(FJobName);
}

bool MCThreadJob::getTerminated(void)
{
	if (FThread)
		return FThread->getTerminated();
	else
		return true;
}

void MCThreadJob::setJobName(const char* jobName)
{
    if (NULL != FJobName)
    {
        free(FJobName);
        FJobName = NULL;
    }
    if (NULL != jobName)
    {
        FJobName = (char*)malloc(strlen(jobName)+1);
        strcpy(FJobName, jobName);
    }
}

const char* MCThreadJob::getJobName(void)
{
    return FJobName;
}

#ifdef _TESTPOOL
void MCThreadJob::setJobRunning(HANDLE handle)
{
    FJobRunning = handle;
}

HANDLE MCThreadJob::getJobRunning(void)
{
    return FJobRunning;
}
#endif

#ifdef MC_NO_PURE_VIRTUAL
void MCThreadJob::Run(void)
{
	MC_pure_virtual();
}
#endif

//--------------- MCThreadPool ------------

MCThreadPool::MCThreadPool(MCThreadFactory* Factory, MCJobErrorHandler* ErrorHandler)
:FSize(0), FFactory(Factory), FErrorHandler(ErrorHandler), 
FInactivityIdle(1), FMaxSize(0), FJobsRan(0), FBusyCount(0)
{

#ifndef _WIN32
    pthread_mutex_init(&FGuard, NULL);
    pthread_cond_init(&FNonEmpty, NULL);
#else
#endif
}

MCThreadPool::~MCThreadPool()
{
    TRY_BLOCK
    {
        Clear();
    }
    CATCH_EVERY
    {
    }
#ifndef _WIN32
    pthread_cond_destroy(&FNonEmpty);
    pthread_mutex_destroy(&FGuard);
#else
#endif
}

#ifdef _TESTPOOL

void MCThreadPool::PostJob(MCThreadJob* Job)
{
    bool assigned = false; HANDLE runningSignal = NULL;
    FGuard.Enter();
    while (!assigned)
    {
        //seek for free thread
        bool thrExited = false;
        if (FRestThreads.Length() > 0)
        {
            MCWorkerThread* thr = (MCWorkerThread*)FRestThreads[0];
            FRestThreads.Del((mcInt32)0);
            thr->FJob = Job;
            thr->FThrExit = &thrExited;
            runningSignal = CreateSemaphore(NULL, 0, 0x7FFFFFFF, NULL);
            thr->FJob->setJobRunning(runningSignal);
            ReleaseSemaphore(thr->FJobAssigned, 1, NULL);
            FGuard.Leave();
            
            assigned = WaitForSingleObject(runningSignal, 5000) != WAIT_TIMEOUT;
            FGuard.Enter();
            assigned = false == thrExited;
        }
        else
        {//create new thread
            MCWorkerThread* Thread = FFactory->CreateThread();
            //OutputDebugString(TEXT("New worker thread is created\n"));
            Thread->FJob = Job;
            Thread->FThrExit = NULL;
            Thread->Resume();
            assigned = true;
        }
    }
    FGuard.Leave();
    //LeaveCriticalSection(&FGuard);
}

#else

void MCThreadPool::PostJob(MCThreadJob* Job)
{
#ifndef _WIN32
    pthread_mutex_lock(&FGuard);
#else
#endif
    bool notCreate = false;
    
    //iterate all threads and look for waiting worker thread
    
    //There is possible free threads
    int thrAvail = FThreadList.Length() - FBusyCount - FJobList.Length();

    if (thrAvail > 0)
        notCreate = true;
    if (false == notCreate)
    {
        if ((FMaxSize > (unsigned int)FThreadList.Length()) || (0 == FMaxSize))
        {//there is some slots for new threads
            MCWorkerThread* Thread = FFactory->CreateThread();
            FThreadList.Add(Thread);
            //OutputDebugString(TEXT("New worker thread is created\n"));
            Thread->Resume();
        }
        else
            notCreate = true;
    }
//    if (notCreate)
//        OutputDebugString(TEXT("There is no need to create yet another thread\n"));
    
    FJobList.Add(Job);

#ifndef _WIN32
    pthread_mutex_unlock(&FGuard);
    pthread_cond_signal(&FNonEmpty);
#else
#endif

}

#endif

unsigned int MCThreadPool::getSize(void)
{
    return FSize;
}

void MCThreadPool::setSize(unsigned int Size)
{
    FSize = Size;
}

unsigned int MCThreadPool::getMaxSize(void)
{
    return FMaxSize;
}

void MCThreadPool::setMaxSize(unsigned int size)
{
    FMaxSize = size;
}

unsigned int MCThreadPool::getInactivityIdle(void)
{
    return FInactivityIdle;
}

void MCThreadPool::setInactivityIdle(unsigned int Idle)
{
    if (Idle > 0)
    	FInactivityIdle = Idle;
}

#ifdef _TESTPOOL

void MCThreadPool::Clear(void)
{
    FGuard.Enter();
    volatile int ThrLen = 0;
    ThrLen = FBusyThreads.Length();
    int i = 0;
    for (i = 0; i<ThrLen; i++)
    {
        ((MCWorkerThread*)FBusyThreads[0])->Terminate();        
    }
	for (i = 0; i<ThrLen; i++)
    {
        ((MCWorkerThread*)FBusyThreads[0])->KickThread(false);
    }
    FGuard.Leave();
}

#else


void MCThreadPool::Clear(void)
{
    volatile unsigned int ThrLen = 0;
#ifdef _WIN32
#else
    pthread_mutex_lock(&FGuard);
#endif    
    FJobList.DeleteAll();
    unsigned int i = 0;
    unsigned int ThrCount = FThreadList.Length();
    
    for (i=0; i<ThrCount; i++)
        ((MCWorkerThread*)FThreadList[i])->Terminate();
#ifdef _WIN32
#else
    pthread_cond_broadcast(&FNonEmpty);
    pthread_mutex_unlock(&FGuard);
#endif
    for (ThrLen = FThreadList.Length(); ThrLen != 0; ThrLen = FThreadList.Length())
    {
        Sleep(10);
    }
    FBusyCount = 0;
}

#endif

void MCThreadPool::FinalizeJob(MCThreadJob* Job)
{
#ifdef _WIN32
#else
    pthread_mutex_lock(&FGuard);
#endif    
	int i;
	MCWorkerThread* Thread = NULL;
	for (i = 0; i < FThreadList.Length(); i++)
	{
		if (((MCWorkerThread*)FThreadList[i])->FJob == Job)
		{
			Thread = (MCWorkerThread*)FThreadList[i];
			break;
		}
	}
	if (Thread != NULL)
	{
		Thread->Terminate();
		Thread->KickThread(false);
#ifdef _WIN32
#else
		pthread_cond_signal(&FNonEmpty);
#endif  
	}
#ifdef _WIN32
#else
    pthread_mutex_unlock(&FGuard);
#endif
	if (Thread != NULL)
	{
		Thread->WaitFor();
		FJobList.Del(Job);
	}
}

unsigned int MCThreadPool::getJobsRan(void)
{
    return FJobsRan;
}

void MCThreadPool::setJobsRan(unsigned int jobsRan)
{
    FJobsRan = jobsRan;
}

unsigned int MCThreadPool::getBusyCount(void)
{
    return FBusyCount;
}

//---------------- MCThreadFactory ------------

#ifdef MC_NO_PURE_VIRTUAL
MCWorkerThread* MCThreadFactory::CreateThread(void)
{
	MC_pure_virtual();
	return NULL;
}
#endif

//-------------- MCJobErrorHandler ------------

#ifdef MC_NO_PURE_VIRTUAL
#ifdef USE_CPPEXCEPTIONS
void MCJobErrorHandler::HandleError(const EMCError& Error)
#else
void MCJobErrorHandler::HandleError(unsigned int ErrorCode)
#endif
{
	MC_pure_virtual();
}
#endif

//----------------- MCWorkerThread ------------

MCWorkerThread::MCWorkerThread(MCThreadPool* Pool, int Priority)
:MCThread(true, Priority), FPool(Pool), FJob(NULL)
{
    FFreeOnTerminate = true;	
}

MCWorkerThread::~MCWorkerThread()
{
    FFreeOnTerminate = true;
    ;
}


#ifdef _WIN32

#else


bool MCWorkerThread::CanExecute(void)
{
	return true;
}

void MCWorkerThread::Execute(void)
{
    bool timeout = false;

    while (getTerminated() == false)
    {
        TRY_BLOCK
        {
            //FGuard.Enter();
	        pthread_mutex_lock(&FPool->FGuard);
	        if (FPool->FJobList.Length() == 0)
	        {//wait for the job
		        timespec abst;
		        abst.tv_sec = time(NULL) + FPool->FInactivityIdle;
		        abst.tv_nsec = 0;
		        pthread_cond_timedwait(&FPool->FNonEmpty, &FPool->FGuard, &abst);
	        }
	        //ok, should we add itself to the thread's list?
            if (FPool->FThreadList.Find(this) < 0)
                FPool->FThreadList.Add(this);

            if ((FPool->FJobList.Length() != 0) && (false == getTerminated()))
            {//well, we have a job for thread!
                
                FJob = (MCThreadJob*)FPool->FJobList[0];
                FPool->FJobList.Del(FJob);
                FPool->FBusyCount++;
                FPool->FJobsRan++;
		        pthread_mutex_unlock(&FPool->FGuard);
                TRY_BLOCK
                {
                    //FGuard.Leave();
                    FJob->FThread = this;
                    FJob->Run();
                }
                CATCH_EVERY
                {
                    
                }
                
                pthread_mutex_lock(&FPool->FGuard);
				if (FPool->FBusyCount > 0)
					FPool->FBusyCount--; 
                pthread_mutex_unlock(&FPool->FGuard);
                delete FJob; FJob = NULL;
            }
            else
            {
                //inactivity idle happens or time to terminate :(
                bool toExit = (FPool->FSize < (unsigned int)FPool->FThreadList.Length()) || getTerminated();
		        pthread_mutex_unlock(&FPool->FGuard);
                if (true == toExit)
                    this->Terminate();
                else
                    continue;
            }                                                                          
        }
        CATCH_EVERY
        {
            
        }
    }
    delete FJob; FJob = NULL;
    pthread_mutex_lock(&FPool->FGuard);
    FPool->FThreadList.Del(this);
    pthread_mutex_unlock(&FPool->FGuard);

}

#endif

void MCWorkerThread::Terminate(void)
{
    MCThread::Terminate();
}

#ifdef USE_NAMESPACE
}
#endif

