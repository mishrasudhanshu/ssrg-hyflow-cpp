//====================================================
//                                                    
//  EldoS MsgConnect                                 
//                                                    
//   Copyright (c) 2001-2010, EldoS                   
//                                                    
//====================================================
#include "MCBase.h"
#include "MCSyncs.h"
#ifndef _WIN32
#   include <pthread.h>
#endif

#ifdef _WIN32
#   include <stdlib.h>
#endif
 
#ifdef USE_NAMESPACE
namespace MsgConnect
{
#endif


// --------- MCSynchroObject --------------------------------------------------

MCSynchroObject::MCSynchroObject()
{
#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
	FHandle = INVALID_HANDLE_VALUE;
#else
	FOrig = false;
#if !defined(QNX)
	FId = 0;
	FEvent = -1;
	FMaxCount = 1;
#endif
#endif
}

#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
#if !defined(QNX)
//**********************UNIX*******************************************
void MCSynchroObject::Initialize(int InitialState, key_t Id, bool Orig, unsigned long MaxCount)
{int fd;
	FId = Id;
	FEvent = -1;
	if (Orig)
		FEvent = semget(FId, 1, IPC_CREAT | IPC_EXCL | 0666);
	if (FEvent < 0) 
	{
		if ((not Orig) || (errno == EEXIST)) 
		{
			FEvent = semget(FId, 1, IPC_CREAT | 0666);
			if (Orig && (FEvent >=0))
				errno = EEXIST;
		}
	}
	if (FEvent < 0)
		THROW_ERROR(MCError_SynchroObjectFailed);

	FMaxCount = MaxCount;
	FOrig = Orig;

	if (FOrig)
	{
		semctl(FEvent, 0, SETVAL, 0);
		if (InitialState > 0)
			Release(InitialState);
	}
}

void MCSynchroObject::Release(int Num)
{
	sembuf b0;
	sembuf b;
	int err;	
	memset(&b0, 0, sizeof(b0));
	memset(&b, 0, sizeof(b));

	b0.sem_flg = IPC_NOWAIT;
	b.sem_op = 1;

	if (FMaxCount > 1)
	{
		if (Num > 1)
		  b.sem_op = Num;
		err = semop(FEvent, &b, 1);
	}
	else
	{
		if (semop(FEvent, &b0, 1) >= 0)
			err = semop(FEvent, &b, 1);
		else
			err = 0;
	};
	if ((err < 0) && (errno != EIDRM))
		THROW_ERROR(MCError_SynchroObjectFailed);
}

#else
//**********************QNX*******************************************
void MCSynchroObject::Initialize(int InitialState, char *FileName, bool Orig, unsigned long MaxCount)
{
	int fd, err;
	FEvent = -1;
	void *addr;
	if (FileName)
	{
		if (Orig)
		{
			sem = sem_open(FileName, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0);
		}
		if (sem == NULL) 
		{
			if ((!Orig) || (errno == EEXIST)) 
			{
				sem = sem_open(FileName, 0, 0);
			}
		}
		else
			FEvent = 0;
	}
	else
	{
		FEvent = sem_init(sem, false, 0);
		err = errno;
	}
	if (FEvent < 0)
		THROW_ERROR(MCError_SynchroObjectFailed);

	FMaxCount = MaxCount;
	FOrig = Orig;

	if (FOrig && (FileName != NULL))
	{
		FEvent = sem_init(sem, 1, 0);
//		if (InitialState > 0)
//			Release(InitialState);
	}
}

void MCSynchroObject::Release(int Num)
{
	int err;	
//	if (FMaxCount > 1)
//		err=sem_wait(sem);		
//	else
		err=sem_post(sem);	
	if ((err < 0) && (errno != EIDRM))
		THROW_ERROR(MCError_SynchroObjectFailed);
}
#endif //QNX
#endif //__GNUC__


MCWaitResult MCSynchroObject::WaitFor(unsigned long Timeout)
{
  MCWaitResult Result;
#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)

#else
	
	if ((Timeout == 0xFFFFFFFFL) || (Timeout == 0)) 
	{
#if !defined(QNX)
		sembuf b;

		memset(&b, 0, sizeof(b));

		if (Timeout == 0)
		  b.sem_flg = IPC_NOWAIT;
		else
		  b.sem_flg = 0;

		//if num >= 1 then
		b.sem_op = -1;

		if (semop(FEvent, &b, 1) >= 0)
		  Result = wrSignaled;
		else if (errno == EIDRM)
		  Result = wrAbandoned;
		else if (errno == EAGAIN)
		  Result = wrTimeout;
		else
		  THROW_ERROR(MCError_SynchroObjectFailed);
		  //result := wrError;
#else
		if(sem_wait(sem))
			Result = wrSignaled;
		else if (errno == EIDRM)
			Result = wrAbandoned;
		else if (errno == EAGAIN)
			Result = wrTimeout;
		else
			THROW_ERROR(MCError_SynchroObjectFailed);
#endif
	}
	else
		Result = wrError;
#endif
	return(Result);
}


MCSynchroObject::~MCSynchroObject()
{
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
	if (FOrig)
#if !defined(QNX)
		semctl(FEvent, 0, IPC_RMID, 0);
#else
		sem_close(sem);
#endif
		
#else
#endif
}

#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
#endif

// --------- MCEvent ----------------------------------------------------------

#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
void MCEvent::Initialize(void* EventAttributes, bool ManualReset, bool InitialState, bool InitialOwner, const char* Name)
{
	char *name;
	bool IO;
	FManualReset = ManualReset;
	name = (char*)Name;

#if !defined(QNX)
	key_t token;
	if (name == NULL || *name == 0)
	{
		token = IPC_PRIVATE;
		FFileName = NULL;
		IO = true;
	}
#else
	if (name == NULL || *name == 0)
	{
#if defined(QNX)
		FFileName = "/SEM";
		IO = true;
#elif defined(__APPLE__)
		char *templ = "/tmp/MC.XXXXXX";
		name = (char*)MCMemAlloc(strlen(templ) + 1);
		strcpy(name, templ);
		templ = mktemp(name);
		IO = true;
#endif
	}
#endif
	if ((name != NULL) && (*name != 0))
	{
		IO = InitialOwner;

		if (*name != '/')
		{
			char* ttmpdir = getenv("TEMP");
			char tmpdir[1024];
			if(ttmpdir)
			{
	  			strcpy(tmpdir, ttmpdir);
	  			strcat(tmpdir, "/");
	  		}
	  		else
		  		strcpy(tmpdir, "/tmp/");

			FFileName = (char*) MCMemAlloc(strlen(tmpdir) + strlen(name) + 1);
			strcpy(FFileName, tmpdir);
			strcat(FFileName, Name);
		}
		else
		{
			FFileName = (char*) MCMemAlloc(strlen(name) + 1);
			strcpy(FFileName, name);
		}

		FFileObj = -1;
		if (IO)
		{
			FFileObj = open(FFileName, O_CREAT | O_RDWR | O_EXCL | O_SYNC, DEFFILEMODE);
			if (FFileObj < 0)
			{
				if (errno != EEXIST)
					THROW_ERROR(MCError_SynchroObjectFailed);
  				IO = false;
			}
		}
#if !defined(QNX)
		token = ftok(FFileName, 'E');
#endif
	}
	
#if !defined(QNX)
	MCSynchroObject::Initialize(InitialState ? 1 : 0, token, IO, 1);
#else
	MCSynchroObject::Initialize(InitialState ? 1 : 0, FFileName, IO, 1);
#endif
	if (name != Name)
	{
		MCMemFree(name);
	}
}

#endif

MCEvent::MCEvent(void* EventAttributes, bool ManualReset, bool InitialState,
	const char* Name, bool OpenOnly) : MCSynchroObject()
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
#if !defined(QNX)
	, FManualReset(false), FFileName(NULL), FFileObj(-1)
#endif
#endif
{	
#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
#else
	Initialize(EventAttributes, ManualReset, InitialState, !OpenOnly, Name);
#endif
}


MCEvent::~MCEvent()
{
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
	if (FFileObj >= 0)
	{
		close(FFileObj);
		remove(FFileName);
	}
	MCMemFree(FFileName);
#endif
}

MCWaitResult MCEvent::WaitFor(unsigned long Timeout)
{
  MCWaitResult Result;
#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
#else
	Result = MCSynchroObject::WaitFor(Timeout);
	if ((FManualReset) && (Result == wrSignaled)) 
		setEvent();
#endif
	return(Result);
}

void MCEvent::setEvent(void)
{
#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
#else
	MCSynchroObject::Release(1);
#endif
}

void MCEvent::resetEvent(void)
{
#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
#else
	while (MCSynchroObject::WaitFor(0) != wrTimeout) {};
#endif
}

//-------------- MCCriticalSection --------------
MCCriticalSection::MCCriticalSection(void)
//:startWaiting(0), sumWaiting(0)
{
#ifdef _WIN32
#else

  if (pthread_mutexattr_init(&FMutexAttr) != 0)
		THROW_ERROR(MCError_SynchroObjectFailed);
	try {
    	if (pthread_mutexattr_settype(&FMutexAttr, PTHREAD_MUTEX_RECURSIVE)!=0)
			THROW_ERROR(MCError_SynchroObjectFailed);
	
		pthread_mutex_init(&FMutex, &FMutexAttr);
		
	}
	FINALLY(
		pthread_mutexattr_destroy(&FMutexAttr);
	)
#endif
}

MCCriticalSection::~MCCriticalSection()
{
#ifdef _WIN32
#else
    pthread_mutex_destroy(&FMutex);
#endif
}

void MCCriticalSection::Enter(void)
{
    
#ifdef _WIN32
#else
    pthread_mutex_lock(&FMutex);
#endif
}

void MCCriticalSection::Leave(void)
{
#ifdef _WIN32
#else
    pthread_mutex_unlock(&FMutex);
#endif
}

/*

//-------------- MCSemaphore - pthread's version -----
#ifndef _WIN32
MCSemaphore::MCSemaphore(void)
:FCount(0)
{
    pthread_cond_init(&FCond, NULL);
    pthread_mutex_init(&FMutex, NULL);
    
}

MCSemaphore::~MCSemaphore(void)
{
    pthread_cond_destroy(&FCond);
    pthread_mutex_destroy(&FMutex);
}

void MCSemaphore::Release(void)
{
    pthread_mutex_lock(&FMutex);    
    FCount++;
    pthread_mutex_unlock(&FMutex);
    pthread_cond_signal(&FCond);
}

bool MCSemaphore::Wait(unsigned int TimeOut)
{
    timespec abst;
    bool res = false;

    pthread_mutex_lock(&FMutex);
    if (FCount > 0)
    {
	FCount--;
    }
    else
    {
	abst.tv_sec = time(NULL) + TimeOut / 1000;
	abst.tv_nsec = (TimeOut % 1000) * 1000000;
	res = pthread_cond_timedwait(&FCond, &FMutex, &abst) == 0;
    }
    pthread_mutex_unlock(&FMutex);
    return res;
}

void MCSemaphore::Wait(void)
{
    Wait(0);
}

#else

#endif

*/

#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
#if !defined(QNX)
MCSemaphore::MCSemaphore(int InitialState, key_t Id, bool Orig, unsigned long MaxCount)
{
	Initialize(InitialState, Id, Orig, MaxCount);
}
#else
MCSemaphore::MCSemaphore(int InitialState, char* FileName, bool Orig, unsigned long MaxCount)
{
	Initialize(InitialState, FileName, Orig, MaxCount);
}

#endif
#endif

#ifdef USE_NAMESPACE
}
#endif

