//====================================================
//                                                    
//  EldoS MsgConnect                                  
//                                                    
//   Copyright (c) 2001-2010, EldoS                   
//                                                    
//==================================================== 

#include "MC.h"
#include "MCUtils.h"
#include "MCBase.h"
#include "MCThreads.h"
#include <string.h>
#include <stdio.h>
#ifndef USE_CPPEXCEPTIONS
#	include <Excpt.h>
#endif
 

//#define MC_FORCE_EVAL

#ifdef USE_NAMESPACE
namespace MsgConnect
{
#endif

#ifdef MC_NO_PURE_VIRTUAL
void MC_pure_virtual(void)
{
}
#endif

mcInt32 GlobalSeed;

#if defined(__MINGW32__) || defined(__MINGW64__)
MCList* DirectTransports = NULL;
#else
static MCList* DirectTransports = NULL;
#endif
static bool DirectTransportForeign = false;
static MCCriticalSection* DirectTransportCS = NULL;



//thanks to Claudio Fiorani for replacing wrong #define
#if defined(_UNICODE) || defined(UNICODE)
wchar_t* s2w(const char* s)
{
	if(s) 
    {
		mcInt32 l = strlen(s) + 1;
		wchar_t* w=new wchar_t[l];
		mbstowcs(w,s,l);
		return(w);
	}
	else
		return(NULL);
}
#endif

#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
mcUInt32 GetTickCount(void)
{
    char buf[1024];
    mcInt32 n, fd = -1;
    double up, idle;
    fd = open("/proc/uptime", O_RDONLY);
    if (fd == -1)
		return(0);

    lseek(fd, 0L, SEEK_SET);
    if ((n = read(fd, buf, sizeof buf - 1)) < 0) 
    {
		close(fd);
		fd = -1;
		return(0);
    }
    close(fd);

    buf[n] = '\0';
    if (sscanf(buf, "%lf %lf", &up, &idle) < 2)
	    return 0;
    return (mcInt32)(up*1000);
}
#endif


void DefineOS(void)
{
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
	IsLinux = true;
#endif
}

char* sstr2pchar(mcInt32 l,char*s)
{
	static char z[256];
	strncpy(z, s, l);
	return(&z[0]);
}
/*


void* MCMemAlloc(mcInt32 Size)
{
	if(Size > 0)
    {
        void* result = (void*)malloc(Size);
        if (NULL == result)
            THROW_ERROR(MCError_NotEnoughMemory);
        return result;
    }
	else
		return NULL;
}

void MCMemFree(void* Block)
{
	if (NULL != Block)
        free(Block);
}

void MCFreeNull(void*& Block)
{
	MCMemFree(Block);
	Block = NULL;
}

*/

#ifdef _WIN32_WCE
#ifndef strdup
char* strdup(const char* s)
{
  if(!s) return(NULL);
  char* p = (char*)MCMemAlloc(strlen(s)+1);
  strcpy(p, s);
  return p;
}
#endif
#endif

bool IsLeapYear(mcInt32 Year)
{
	return((Year % 4 == 0) && ((Year % 100 != 0) || (Year % 400 == 0)));
}

const mcInt32 MSecsPerDay = 24 * 60 * 60 * 1000;

double EncodeTime(mcInt32 Hour, mcInt32 Min, mcInt32 Sec, mcInt32 MSec)
{
	double r = 0.0;

	if((Hour < 24) && (Min < 60) && (Sec < 60) && (MSec < 1000))
		r = ((double)Hour * 3600000 + (double)Min * 60000 + (double)Sec * 1000 + MSec) / MSecsPerDay;
	return(r);
}

double EncodeDate(mcInt32 Year, mcInt32 Month, mcInt32 Day)
{
	byte days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	days[1] = 28 + IsLeapYear(Year);
	double r = 0.0;
	if((Year >= 1) && (Year <= 9999) && (Month >= 1) && (Month <= 12) &&
		(Day >= 1) && (Day <= days[Month-1]))
	{
		mcInt32 i;
		for(i = 1;i < Month;i++)
			Day+=days[i-1];
		i = Year - 1;
		r = i * 365 + i / 4 - i / 100 + i / 400 + Day - 693594; // DateDelta
	}
	return r;
}

double Now(void)
{
	double r;
#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
#else
	time_t t;
	struct timeval tv;
	struct tm ut;
	
	gettimeofday(&tv, NULL);
	t = tv.tv_sec;
	gmtime_r(&t, &ut);
	//systemtime_r(&t, &ut);
	r = EncodeDate(ut.tm_year + 1900, ut.tm_mon + 1, ut.tm_mday) +
		EncodeTime(ut.tm_hour, ut.tm_min, ut.tm_sec, tv.tv_usec / 1000);
#endif
	return r;
}

double Time(void)
{
	double r;
#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
#else
	time_t t;
	struct timeval tv;
	struct tm ut;

	gettimeofday(&tv, NULL);
	t = tv.tv_sec;
	localtime_r(&t, &ut);
	r = EncodeTime(ut.tm_hour, ut.tm_min, ut.tm_sec, tv.tv_usec / 1000);
#endif
	return r;
}

const mcInt32 PSTR_MAX = 255;

void equPstr(char* p, char* s, mcInt32 max)
{
	mcInt32 l = strlen(s);
	if(l > max)
		l = max;
	if(l > PSTR_MAX)
		l = PSTR_MAX;
	p[0] = (char)l;
	strncpy(&p[1], s, l);
}

char* Pstr2c(char* p)
{
  char* t = (char*)MCMemAlloc(p[0]+1);
  strncpy(t, &p[1], p[0]);
  t[p[0]] = 0;
  return t;
}

char* strCopy(const char* s, mcInt32 f, mcInt32 l)
{
	mcInt32 ll = l;
	mcInt32 sl = strlen(s);
	if(!s)
		return(NULL);
	if(f < 0)
		return(NULL);
	if(ll < 0)
		ll = sl - f;
	if(ll < 0)
		return(NULL);
	if(f >= sl)
		return(NULL);
	if(f + ll > sl)
		ll = sl - f;
	char* t = (char*)MCMemAlloc(ll+1);
	strncpy(t, &s[f], ll);
	t[ll] = 0;
	return t;
}

mcInt32 strPos(char c, const char* s)
{
	char* t = strchr((char*) s, c);
	if(!t)
		return -1;
	return t - s;
}

// ******************************* MCMessageInfo *******************************

MCMessageInfo::MCMessageInfo(MCMessage* Msg)
: Time(0), OrigID(0), IsSendMsg(0), MCError(0), State(imsQueued), ReplyFlag(NULL),
    Transport(NULL), UserData(0), NotifyProc(NULL), TimeoutProc(NULL), ErrorProc(NULL), 
	SMessenger(NULL), DMessenger(NULL), Sent(false),
    URL(NULL), StartTime(0), Timeout(0), Track(NULL), ConnID(0), ClientID(0), OldState(imsNone)
{
    memset(DQueue, 0, sizeof(DQueue));
    memset(&Credentials, 0, sizeof(Credentials));
    memset(&Message, 0, sizeof(Message));

    MCMessenger::SetMessageID(this);

	Time = Now();
	Msg->MsgID = Message.MsgID;
	memmove(&Message, Msg, sizeof(MCMessage));
	Message.Data = NULL;
	if(Message.DataSize > 0 && Msg->Data != NULL)
	{
		Message.Data = MCMemAlloc(Message.DataSize);
		memmove(Message.Data, Msg->Data, Message.DataSize);
	}
}

MCMessageInfo::~MCMessageInfo()
{
	if(Message.Data)
        MCFreeNull(Message.Data);
    delete ReplyFlag; ReplyFlag = NULL;
    if (SMessenger)
		MCMemFree(SMessenger); SMessenger = NULL;
	if (DMessenger)
		MCMemFree(DMessenger); DMessenger = NULL;
    MCMemFree(URL); URL = NULL;
    //if (this->State == imsEmpty)
    //    OutputDebugString("~MCMessageInfo\n");
}

void MCMessageInfo::WriteToStream(MCStream* Stream)
{
	Stream->Write(&Time, sizeof(Time));
    Stream->Write(&OrigID, sizeof(OrigID));
    Stream->Write(&IsSendMsg, sizeof(IsSendMsg));
    Stream->Write(DQueue, sizeof(DQueue));
    Stream->Write(&Credentials, sizeof(Credentials));
	Stream->Write(&MCError, sizeof(MCError));
    mcUInt32 stub = (mcUInt32)State;
    Stream->Write(&stub, sizeof(stub));
    //Stream->Write(&stub, sizeof(stub));
	signed char flag = (signed char) Message.TransCmd;
	Stream->Write(&flag, sizeof(flag));
	Stream->Write(&Message.TransID, sizeof(Message.TransID));
    Stream->Write(&Message.Priority, sizeof(Message.Priority));

    Stream->Write(&Message.MsgID, sizeof(Message.MsgID));
    Stream->Write(&Message.MsgCode, sizeof(Message.MsgCode));
    Stream->Write(&Message.Result, sizeof(Message.Result));
    Stream->Write(&Message.Param1, sizeof(Message.Param1));
    Stream->Write(&Message.Param2, sizeof(Message.Param2));
    stub = Message.DataType;
    Stream->Write(&stub, sizeof(stub));
    Stream->Write(&Message.DataSize, sizeof(Message.DataSize));
}

void MCMessageInfo::LoadFromStream(MCStream* Stream)
{
	Stream->Read(&Time, sizeof(Time));
    Stream->Read(&OrigID, sizeof(OrigID));
    Stream->Read(&IsSendMsg, sizeof(IsSendMsg));
    Stream->Read(DQueue, sizeof(DQueue));
    Stream->Read(&Credentials, sizeof(Credentials));
	Stream->Read(&MCError, sizeof(MCError));
	mcUInt32 stub = 0; 
    Stream->Read(&stub, sizeof(stub));
    State = (MCMessageState)stub;
    // Stream->Read(&stub, sizeof(stub)); //read fake value for compatibility with earlier versions
	signed char flag;
	Stream->Read(&flag, sizeof(flag));
	Message.TransCmd = (MCTransactCmd) flag;
	Stream->Read(&Message.TransID, sizeof(Message.TransID));
	Stream->Read(&Message.Priority, sizeof(Message.Priority));

    Stream->Read(&Message.MsgID, sizeof(Message.MsgID));
    Stream->Read(&Message.MsgCode, sizeof(Message.MsgCode));
    Stream->Read(&Message.Result, sizeof(Message.Result));
    Stream->Read(&Message.Param1, sizeof(Message.Param1));
    Stream->Read(&Message.Param2, sizeof(Message.Param2));
    Stream->Read(&stub, sizeof(stub));
    Message.DataType = MCBinDataType(stub);
    Stream->Read(&Message.DataSize, sizeof(Message.DataSize));
}

void MCMessageInfo::setSMessenger(char* SMsg)
{
    MCMemFree(SMessenger); SMessenger = NULL;
    if (NULL != SMsg)
        SMessenger = _strdup(SMsg);
    else
        SMessenger = NULL;
}

void MCMessageInfo::setDMessenger(char* DMsg)
{
    MCMemFree(DMessenger); DMessenger = NULL;
    if (NULL != DMsg)
        DMessenger = _strdup(DMsg);
    else
        DMessenger = NULL;
}

void MCMessageInfo::setURL(char* URLStr)
{
    MCMemFree(URL); URL = NULL;
    if (NULL != URLStr)
        URL = _strdup(URLStr);
    else
        URL = NULL;
}


void MCMessageInfo::CopyMessage(MCMessageInfo *Dest)
{
	memmove(&Dest->Message.MsgID,  &Message.MsgID, sizeof(MCMessage));
}

// ***************************** MCBaseTransformer ***************************

#ifdef MC_NO_PURE_VIRTUAL
mcInt32 MCBaseTransformer::GetID()
{
	MC_pure_virtual();
	return 0;
}
#endif

// ***************************** MCBaseCompression ***************************

#ifdef MC_NO_PURE_VIRTUAL
void MCBaseCompression::Compress(char* Remote, void* InData, mcInt32 InDataSize,
		void*& OutData, mcInt32& OutDataSize,bool& Success)
{
	MC_pure_virtual();
}

void MCBaseCompression::Decompress(char* Remote, void* InData, mcInt32 InDataSize,
		void*& OutData, mcInt32& OutDataSize, mcInt32 TransformID,
		bool& Success)
{
	MC_pure_virtual();
}
#endif

// ***************************** MCCBaseEncryption ***************************

#ifdef MC_NO_PURE_VIRTUAL
void MCCBaseEncryption::Encrypt(char* Remote, void* InData, mcInt32 InDataSize,
		void*& OutData, mcInt32& OutDataSize, bool& Success)
{
	MC_pure_virtual();
}

void MCCBaseEncryption::Decrypt(char* Remote, void* InData, mcInt32 InDataSize,
		void*& OutData, mcInt32& OutDataSize, mcInt32 TransformID, bool& Success)
{
	MC_pure_virtual();
}
#endif

// ******************************* MCBaseSealing *****************************

#ifdef MC_NO_PURE_VIRTUAL
void MCBaseSealing::Seal(char* Remote, void* InData, mcInt32 InDataSize,
		void*& OutData, mcInt32& OutDataSize,bool& Success)
{
	MC_pure_virtual();
}

void MCBaseSealing::UnSeal(char* Remote, void* InData, mcInt32 InDataSize,
		void*& OutData, mcInt32& OutDataSize, mcInt32 TransformID,
		bool& Success)
{
	MC_pure_virtual();
}
#endif

// ******************************* MCMessenger *******************************

MCMessenger::MCMessenger()
{
	FThreadID = GetCurrentThreadID();
	FIncomingCriticalSection = new MCCriticalSection();
	FCompleteCriticalSection = new MCCriticalSection();
	FForwardCriticalSection  = new MCCriticalSection();
  
	FQueues = new MCList();
	FForwardQueue = new MCList();
	FIncomingQueue = new MCList();
	FCompleteQueue = new MCList();
	FIncomingEvent = new MCEvent(NULL, false, false, NULL);
	FCompleteEvent = new MCEvent(NULL, false, false, NULL);
	FMaxTimeout = INFINITE;
	FLastCleanup = 0;
	FCleanupInterval = 0;//10000;
	
	
	FTransportList = new MCList();

	FOnValidateCredentials = NULL;
	FOnValidateCredentialsData = NULL;
}

MCMessenger::~MCMessenger()
{
	MCMessageInfo* Info;
  
	FIncomingCriticalSection->Enter();
	while(FIncomingQueue->Length() > 0)
	{
		Info = (MCMessageInfo*)(*FIncomingQueue)[0];

		// remove the message from the queue of pending messages
		FIncomingQueue->Del((mcInt32)0);

		// adjust message state
		Info->State = imsFailed;
		if(Info->IsSendMsg && (FTransportList->Find(Info->Transport) > -1))
			Info->Transport->MessageProcessed(Info);
		else
        {
			delete Info; 
            Info = NULL;
        }
	}
	FIncomingCriticalSection->Leave();
	DispatchCompleteMessages();
  
	  // Cleanup ForwardQueue
	FForwardCriticalSection->Enter();
	while (FForwardQueue->Length() > 0)
	{
		Info = (MCMessageInfo*)(*FForwardQueue)[0];

		// remove the message from the queue of pending messages
		FForwardQueue->Del((mcInt32)0);
		delete Info; Info = NULL;
	}
	FForwardCriticalSection->Leave();


	delete FCompleteCriticalSection; FCompleteCriticalSection = NULL;
	delete FIncomingCriticalSection; FIncomingCriticalSection = NULL;
	delete FForwardCriticalSection; FForwardCriticalSection = NULL;
	delete FForwardQueue; FForwardQueue = NULL;
  	delete FIncomingQueue; FIncomingQueue = NULL;
	delete FIncomingEvent; FIncomingEvent = NULL;
	delete FCompleteQueue; FCompleteQueue = NULL;
	delete FCompleteEvent; FCompleteEvent = NULL;
	// FTransportList->DeleteItem = TransportDelete;
	delete FTransportList; FTransportList = NULL;
    delete FQueues; FQueues = NULL;
}

#ifndef __APPLE__
mcUInt32 MCMessenger::getThreadID(void)
#else
pthread_t MCMessenger::getThreadID(void)
#endif
{
	return FThreadID;
}

#ifndef __APPLE__
void MCMessenger::setThreadID(mcUInt32 Thr)
#else
void MCMessenger::setThreadID(pthread_t Thr)
#endif
{
    FThreadID = Thr;
}

mcUInt32 MCMessenger::getMaxTimeout(void)
{
	return FMaxTimeout;
}

mcUInt32 MCMessenger::getCleanupInterval(void)
{
	return FCleanupInterval;
}


MCValidateCredentialsEvent MCMessenger::getOnValidateCredentials(void* *UserData)
{
	if (UserData)
		*UserData = FOnValidateCredentialsData;
	
	return FOnValidateCredentials;
}

void MCMessenger::setOnValidateCredentials(MCValidateCredentialsEvent Value, void* UserData)
{
	FOnValidateCredentialsData = UserData;
	FOnValidateCredentials = Value;
}

void MCMessenger::AddQueue(MCQueue* AQueue)
{
	if(FQueues->Find(AQueue) <= -1)
		FQueues->Add(AQueue);
}

void MCMessenger::AddTransport(MCBaseTransport* ATransport)
{
	if(FTransportList->Find(ATransport) <= -1)
		FTransportList->Add(ATransport);
}

void MCMessenger::CleanupForwardQueue()
{
	mcInt32 i;
    MCMessageInfo* Info;
    mcUInt32 TC;
    bool me;
    
	FForwardCriticalSection->Enter();

	TRY_BLOCK
    {
		i = 0;
		TC = GetTickCount();
		while (i < FForwardQueue->Length()) 
		{    
			Info = (MCMessageInfo*)((*FForwardQueue)[i]);

			if (Info->Timeout == 0)
			{
				i++;
				continue;
			}

			if (TC < Info->StartTime)
			{
				me = 0xFFFFFFFF - Info->StartTime + TC >= Info->Timeout;
			}
			else
			{
				me = TC - Info->StartTime >= Info->Timeout;
			}
			if (me)
				FForwardQueue->Del(Info);
			else
				i++;
		}
	} 
    FINALLY (
		FForwardCriticalSection->Leave();
	)

}

void MCMessenger::CleanupQueues()
{
	mcUInt32 TC;
	bool dc;
	mcInt32 i;

	TC = GetTickCount();
	if (TC < FLastCleanup)
	{
		dc = 0xFFFFFFFF - FLastCleanup + TC >= FCleanupInterval;
	}
	else
	{
		dc = TC - FLastCleanup >= FCleanupInterval;
	}
	if (dc)
	{
		FLastCleanup = TC;
		for (i = 0; i < FTransportList->Length(); i++)
		{
			((MCBaseTransport*)((*FTransportList)[i]))->CleanupMessages();
		}

		CleanupForwardQueue();
	}

}

bool MCMessenger::CancelMessage(__int64 MsgID)
{
	mcInt32 i;
	for (i = 0; i < FTransportList->Length(); i++)
	{
		if (((MCBaseTransport*)((*FTransportList)[i]))->CancelMessageByID(MsgID, true))
			return true;
	}
	return false;
}


bool MCMessenger::CancelMessageEx(__int64 MsgID, bool CancelComplete)
{
	mcInt32 i;
	for (i = 0; i < FTransportList->Length(); i++)
	{
		if (((MCBaseTransport*)((*FTransportList)[i]))->CancelMessageByID(MsgID, true))
		{
			return true;
		}
	}
	if (CancelComplete) 
	{
		bool res = false;

		FCompleteCriticalSection->Enter();
		
		MCMessageInfo* Info = NULL;

		for (i = 0; i < FCompleteQueue->Length(); i++)
		{
			Info = ((MCMessageInfo*)(*FCompleteQueue)[i]);
			if (Info->Message.MsgID == MsgID)
			{
				FCompleteQueue->Del(i);
				delete Info; Info = NULL;
				res = true;
				break;
			}
		}
		FCompleteCriticalSection->Leave();
		return res;
	}
	else
	  return false;
}

void MCMessenger::DispatchCompleteMessages(void)
{
	MCMessageInfo* Info;
	MCMessageInfo* LocalInfo;
	mcInt32 i = 0, i2 = 0;
	bool ProcessInfo;

	if (FCleanupInterval > 0)
		CleanupQueues();

	FCompleteCriticalSection->Enter();
	TRY_BLOCK
	{
		while(i < FCompleteQueue->Length())
		{
			Info = (MCMessageInfo*)(*FCompleteQueue)[i];
			ProcessInfo = false;

			FCompleteQueue->Del(i);
				
			if(!Info->ReplyFlag)
				ProcessInfo = true;
			
			LocalInfo = NULL;
			
			FForwardCriticalSection->Enter();

			i2 = FindMessageInQueue(Info->OrigID, FForwardQueue);
			if (i2 != -1)
				LocalInfo = (MCMessageInfo *)((*FForwardQueue)[i2]);

			if (LocalInfo != NULL)
			{
				FForwardQueue->Del(LocalInfo);
				FForwardCriticalSection->Leave();
				
				LocalInfo->MCError = Info->MCError;
				
				Info->CopyMessage(LocalInfo);
				Info->Message.DataSize = 0;
				Info->Message.Data = NULL;

				DoMessageProcessed(LocalInfo);				
			}
			else
			{
				FForwardCriticalSection->Leave();

				// only notify about complete (not abandoned) events
				if(Info->State == imsComplete)
				{
					if(Info->NotifyProc)
					{
						FCompleteCriticalSection->Leave();
						Info->NotifyProc(Info->UserData, Info->Message);
						FCompleteCriticalSection->Enter();
					}
				}
				else
				if (Info->State == imsExpired)
				{
					if(Info->TimeoutProc)
					{
						FCompleteCriticalSection->Leave();
						Info->TimeoutProc(Info->UserData, Info->Message);
						FCompleteCriticalSection->Enter();
					}
				}
				else
				if (Info->State == imsFailed)
				{
					if(Info->ErrorProc)
					{
						FCompleteCriticalSection->Leave();
						Info->ErrorProc(Info->UserData, Info->Message, Info->MCError);
						FCompleteCriticalSection->Enter();
					}
				}
			}
			
			if (ProcessInfo)
			{
				delete Info; Info = NULL;
			}
		}
	}
	FINALLY
	(
		FCompleteEvent->resetEvent();
		FCompleteCriticalSection->Leave();
	)
}

void MCMessenger::MCDispatchMessages(void)
{
	MCQueue* AQueue;
	MCMessageInfo* Info;
	mcInt32 i;

/*	if(FThreadID != GetCurrentThreadID())
        THROW_ERROR(MCError_WrongReceiverThread);
*/
    
    DispatchCompleteMessages();
	if(NULL == FIncomingCriticalSection) 
        return;
	FIncomingCriticalSection->Enter();
	TRY_BLOCK 
    {
		i = 0;
		while(i < FIncomingQueue->Length())
		{
			Info = (MCMessageInfo*)(*FIncomingQueue)[i];
			if(Info->State == imsProcessing)
			{
				/*
				char tmp[128];
                sprintf(tmp, "Missing message number %d due to its imsProcessing state\n", Info->Message.Param1);
                OutputDebugString(tmp);
				*/
                i++;
				continue;
			}
			Info->State = imsProcessing;

            char* q = _strdup(Info->DQueue);
			AQueue = FindQueue(q);
			MCMemFree(q);
            
			//mcInt32 Len = FIncomingQueue->Length();
            if(AQueue)
			{
				FIncomingCriticalSection->Leave();
                AQueue->ProcessMessage(Info);
				FIncomingCriticalSection->Enter();
			}
			else
			{
				Info->MCError = MCError_BadDestinationName;
				FIncomingCriticalSection->Leave();
                MessageProcessed(&Info->Message);
				FIncomingCriticalSection->Enter();                
			}
			/*
			if(FIncomingQueue->Find(Info) != -1)
                i++;
			*/

		}
	} FINALLY (
		FIncomingCriticalSection->Leave();
	)
}

void MCMessenger::DoSendMessage(const char* Destination, MCMessageInfo* Info)
{
	bool b = false;
	char* destCopy = strCopy(Destination, 0, strlen(Destination));

	
	MCBaseTransport* Transport = NULL;
	
	for(mcInt32 i = 0; i < FTransportList->Length();i++)
	{
		Transport = (MCBaseTransport*)((*FTransportList)[i]);

		if (Transport->AddressValid(destCopy, false))
		{
			if(!Transport->getActive())
			{
				THROW_ERROR(MCError_TransportInactive);
			}
			else
			if (Transport->DeliverMessage(destCopy, Info))
			{
				b = true;
				break;
			}
		}
	}
	MCMemFree(destCopy);
	if(!b)
        THROW_ERROR(MCError_BadTransport);
}

mcInt32 MCMessenger::FindMessageInQueue(__int64 MessageID, MCList* Queue)
{
	MCMessageInfo* Info = NULL;

	for(mcInt32 i = 0; i < Queue->Length(); i++)
	{
		Info = (MCMessageInfo*)(*Queue)[i];
		if(Info->Message.MsgID == MessageID)
			return i ;
	}
	return -1;
}

MCQueue* MCMessenger::FindQueue(char* QueueName)
{
//	QueueName := Uppercase(QueueName);
	for(mcInt32 i = 0;i <= FQueues->Length() - 1;i++)
	{
		if (((MCQueue*)(*FQueues)[i])->getQueueName() != NULL)
#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__) || defined(QNX)
			if(stricmp(((MCQueue*)(*FQueues)[i])->getQueueName(), QueueName)==0)
#else
			if(strcasecmp(((MCQueue*)(*FQueues)[i])->getQueueName(), QueueName)==0)
#endif
				return((MCQueue*)(*FQueues)[i]);
	}
	return NULL;
}

/*
Gets new message from the message queue. Doesn't return until any message 
is available.
*/
void MCMessenger::MCGetMessage(MCMessage* Message)
{
	mcInt32 i = -1;
	bool cq = false;
	bool canBreak = false;
	MCHandle HandleArray[2];
	MCMessageInfo* Info;

/*	if(FThreadID != GetCurrentThreadID())
        THROW_ERROR(MCError_WrongReceiverThread);
*/
	DispatchCompleteMessages();
	while (canBreak == false) 
	{
		FIncomingCriticalSection->Enter();
		i = HasPendingMessages(FIncomingQueue);
		if(i == -1)
		{
			FIncomingCriticalSection->Leave();
			do {
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
				if(FIncomingEvent->WaitFor(0) == wrSignaled)
					cq = true;
				if(FCompleteEvent->WaitFor(0) == wrSignaled)
					DispatchCompleteMessages();
				Sleep(10);
#else
#endif
			} while(!cq);
			DispatchCompleteMessages();
			FIncomingCriticalSection->Enter();
		}		
		
		if (i == -1)
			i = HasPendingMessages(FIncomingQueue);

		if(i != -1)
		{
			Info = (MCMessageInfo*)((*FIncomingQueue)[i]);
			Info->State = imsDispatching;
			Info->Message.copyTo(Message);
			FIncomingEvent->WaitFor(0);
			canBreak = true;
		}
		FIncomingCriticalSection->Leave();
	}
}

mcInt32 MCMessenger::HasPendingMessages(MCList* Queue)
{
	MCMessageInfo* Info = NULL;

	for(mcInt32 i = 0;i <= Queue->Length() - 1;i++)
	{
		Info = (MCMessageInfo*)(*Queue)[i];
		if(Info->State == imsWaiting)
			return i;
	}
	return -1;
}

void MCMessenger::MessageProcessed(MCMessage* Message)
{
	mcInt32 i;
	MCMessageInfo* Info;
/*	if(FThreadID != GetCurrentThreadID())
        THROW_ERROR(MCError_WrongReceiverThread);
*/        
	FIncomingCriticalSection->Enter();
	i = FindMessageInQueue(Message->MsgID, FIncomingQueue);
	if(i != -1)
	{
		Info = (MCMessageInfo*)(*FIncomingQueue)[i];

		//copy the 'fresh' message :)
		Message->copyTo(&Info->Message);
    
		// remove the message from the queue of pending messages

		FIncomingQueue->Del(i);
		FIncomingCriticalSection->Leave();

		DoMessageProcessed(Info);		
	}
	else
		FIncomingCriticalSection->Leave();
}


mcUInt32 MCMessenger::MsgWaitForMultipleObjects(mcUInt32 nCount,void* pHandles,
        bool fWaitAll, mcUInt32 dwMilliseconds)
{
        MCHandle* Handles = NULL;
        mcUInt32 i = 0, r = 0;
//#ifndef NO_CHECK_DISPATCHTHREAD
//        if(FThreadID != IntGetCurrentThreadID())
//            THROW_ERROR(MCError_WrongReceiverThread);
//#endif

		mcInt32 ToWait = (mcInt32) dwMilliseconds;
		mcUInt32 StTime = 0;
		mcUInt32 Elapsed = 0;
	
        DispatchCompleteMessages();
        FIncomingCriticalSection->Enter();
        if(HasPendingMessages(FIncomingQueue) == -1)
        {
			FIncomingCriticalSection->Leave();
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
//error No Linux support yet
#else
#endif
        }
        else
        {
                FIncomingCriticalSection->Leave();
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
//error No Linux support yet
#else
                r = WAIT_OBJECT_0 + nCount;
#endif
        }
        return r;
}

/*
Gets new message from the message queue. Return immediately even when no 
message is available.
Returns true if the message has been picked and false otherwise.
*/

bool MCMessenger::MCPeekMessage(MCMessage* Message)
{
	mcInt32 i;
	bool r = true;
	//if(FThreadID != GetCurrentThreadID())
    //    THROW_ERROR(MCError_WrongReceiverThread);
	
	MCMessageInfo * Info;

    DispatchCompleteMessages();
	FIncomingCriticalSection->Enter();
	try {
		i = HasPendingMessages(FIncomingQueue);
		if(i != -1)
		{
			Info = (MCMessageInfo*)(*FIncomingQueue)[i];
			Info->Message.copyTo(Message);			
			FIncomingEvent->WaitFor(0);
		}
		else
			r = false;
	} FINALLY (
		FIncomingCriticalSection->Leave();
	)
	return r;
}

void MCMessenger::MCPostMessage(const char* Destination, MCMessage* Message,
	MCMessageCredentials* Credentials)
{
	SendMessageTimeoutCallback(Destination, Message, FMaxTimeout, NULL, NULL, NULL, 0, Credentials);
}

void MCMessenger::RemoveQueue(MCQueue* AQueue)
{
	mcInt32 i = FQueues->Find(AQueue);
	if(i != -1)
		FQueues->Del(i);
}

void MCMessenger::RemoveTransport(MCBaseTransport* ATransport)
{
	mcInt32 i = FTransportList->Find(ATransport);
	if(i != -1)
		FTransportList->Del(i);
}

/*
Gets new message from the message queue. Doesn't return until any message 
is available.
*/
mcInt32 MCMessenger::MCSendMessage(const char* Destination, MCMessage* Message,
	MCMessageCredentials* Credentials)
{
	mcInt32 r;
	if (MCSendMessageTimeout(Destination, Message, FMaxTimeout, false, Credentials, r))
		return r;
	else
		THROW_ERROR(MCError_TransportTimeout);	
	return 0; //stupid compiler...
}

void MCMessenger::MCSendMessageCallback(const char* Destination, MCMessage* Message, 
	MCNotifyProc CompletionProc, mcPtrInt UserData, MCMessageCredentials* Credentials)
{
	SendMessageTimeoutCallback(Destination, Message, FMaxTimeout, CompletionProc, NULL, NULL, UserData, Credentials);
}

void MCMessenger::SendMessageTimeoutCallback(const char* Destination, MCMessage* Message,
		                        mcUInt32 Timeout, MCNotifyProc CompletionProc, 
								MCNotifyProc TimeoutProc, MCErrorProc ErrorProc, 
								mcPtrInt UserData, MCMessageCredentials* Credentials)
{

#ifndef MC_FORCE_EVAL
  if (false)
  {
#endif
#ifndef MC_FORCE_EVAL
  }
#endif

	MCMessageInfo* Info;
	Info = new MCMessageInfo(Message);

	// set OrigID
	MCMessenger::SetOrigID(Info);
	
	Info->Timeout = Timeout;
	Info->StartTime = GetTickCount();
	Info->NotifyProc = CompletionProc;
	Info->TimeoutProc = TimeoutProc;
	Info->ErrorProc = ErrorProc;
	Info->UserData = UserData;
	Info->ReplyFlag = NULL;
	Info->IsSendMsg = CompletionProc != NULL;
	if(Credentials)
		memmove(&Info->Credentials, Credentials, sizeof(*Credentials));
	else
		memset(&Info->Credentials, 0, sizeof(Info->Credentials));

	mcInt32 MCErrorCode = 0;
    TRY_BLOCK
    {
		DoSendMessage(Destination, Info);
    }
    CATCH_MCERROR
    {
        delete Info; Info = NULL;
        THROW_ERROR(MCErrorCode);
    }
}

bool MCMessenger::MCSendMessageTimeout(const char* Destination, MCMessage* Message,
	mcUInt32 Timeout, bool Block, MCMessageCredentials* Credentials,
	mcInt32& MsgResult)
{
	MCMessageInfo* Info;
	mcInt32 ToWait;
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
	mcInt32 wtime;
#endif
	mcUInt32 StTime;
	mcUInt32 Elapsed;
	bool b,r;
	MCHandle HandleArray[3];

#ifndef MC_FORCE_EVAL
  if (false)
  {
#endif
#ifndef MC_FORCE_EVAL
  }
#endif

	r = false;
	Info = new MCMessageInfo(Message);
	// set OrigID
	MCMessenger::SetOrigID(Info);

	Info->ReplyFlag = new MCEvent(NULL, false, false, NULL);
	Info->IsSendMsg = true;
	if(Credentials)
		memmove(&Info->Credentials, Credentials, sizeof(*Credentials));
	else
		memset(&Info->Credentials, 0, sizeof(*Credentials));
    mcInt32 errorCode = 0;
	TRY_BLOCK 
    {
		DoSendMessage(Destination, Info);
    }
    CATCH_EVERY
    {
        delete Info; Info = NULL;
#ifdef USE_CPPEXCEPTIONS
        throw;
#else
        RaiseException(GetExceptionCode(), 0, 0, NULL);
#endif
    }

	ToWait = (mcInt32)Timeout;
	b = Timeout == INFINITE;
	do {
		StTime = GetTickCount();

#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
		mcInt32 l_wait = ToWait * 100;
		while(l_wait > 0)
		{
			if(Info->ReplyFlag->WaitFor(0) == wrSignaled)
			{
				r = true;
				break;
			}
			if(FThreadID != GetCurrentThreadID())
			{
				if(FIncomingEvent->WaitFor(0) == wrSignaled)
					break;
			}
			Sleep(10);
			l_wait--;
		}
		if(r)
			break;
#else
#endif
		// if timeout elapsed, break the loop
		if (!r && (FThreadID != GetCurrentThreadID()))
			break;

		Elapsed = GetTickCount();
		if(StTime > Elapsed)
			Elapsed = Elapsed + ((mcUInt32)-1) - StTime;
		else
			Elapsed = Elapsed - StTime;
		ToWait = ToWait - ((mcInt32)Elapsed);

		StTime = GetTickCount();
		DispatchCompleteMessages();
		
		if(!Block)
		{
			DispatchMessages();
		}

		Elapsed = GetTickCount();
		if(StTime > Elapsed)
			Elapsed = Elapsed + ((mcUInt32)-1) - StTime;
		else
			Elapsed = Elapsed - StTime;
		ToWait = ToWait - ((mcInt32)Elapsed);
	} while (b || (ToWait > 0));
	// if it was, set result
	if(r)
	{
		FCompleteCriticalSection->Enter();
		TRY_BLOCK
        {
			if(Info->State == imsFailed)
			{
                THROW_ERROR(Info->MCError);
			}
			else
				{
					MsgResult = Info->Message.Result;
					Message->Result = MsgResult;
                    if (bdtVar == Message->DataType)
                    {//copy the fresh message value
                        Message->DataSize = Info->Message.DataSize;
                        MCFreeNull(Message->Data);
						if (Message->DataSize)
						{
							Message->Data = MCMemAlloc(Message->DataSize);
							memmove(Message->Data, Info->Message.Data, Message->DataSize);
						}
						else
							Message->Data = NULL;
                    }
				}
		} FINALLY (
			FCompleteQueue->Del(Info);
			delete Info; Info = NULL;
			FCompleteCriticalSection->Leave();
		)
	}
	else
	{
		CancelMessageEx(Info->Message.MsgID, true);        
	}
	return r;
}

void MCMessenger::setMaxTimeout(mcUInt32 Value)
{
	if(FMaxTimeout != Value)
	{
		FMaxTimeout = Value;
		for (mcInt32 i = 0; i < FTransportList->Length(); i++)
			((MCBaseTransport*)((*FTransportList)[i]))->CleanupMessages();
	}
}

void MCMessenger::setCleanupInterval(mcUInt32 Value)
{
	if (FCleanupInterval != Value)
	{
		if (FCleanupInterval == 0 && Value > 0)
			FLastCleanup = GetTickCount();
		FCleanupInterval = Value;
	}
}    

void MCMessenger::SetMessageID(MCMessageInfo* MessageInfo)
{
	// create unique ID
	mcInt32 i = mcInt32(floor(Time() * 86400000));
	memmove(&MessageInfo->Message.MsgID, &i, sizeof(i));
	i = GetTickCount()
#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
#else
	+ GlobalSeed++;
#endif
	;
	memmove((((char*)(&MessageInfo->Message.MsgID)) + sizeof(i)), &i, sizeof(i));
}

void MCMessenger::SetOrigID(MCMessageInfo* MessageInfo)
{
	// create unique ID
	mcInt32 i = mcInt32(floor(Time() * 86400000));
	memmove(&MessageInfo->OrigID, &i, sizeof(i));
	i = GetTickCount()
#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
#else
	+ GlobalSeed++;
#endif
	;
	memmove((((char*)(&MessageInfo->OrigID)) + sizeof(i)), &i, sizeof(i));
}

bool MCMessenger::ValidateCredentials(MCMessageInfo* Info)
{
	bool r = false;

	void* UserData;

	if(getOnValidateCredentials(&UserData))
		FOnValidateCredentials(UserData, this, &Info->Message, &Info->Credentials, r);
	else
		r = true;
	
	return(r);
}

/*
Waits for any message to appear in the message queue. 
Returns only when any message appears.
*/
void MCMessenger::MCWaitMessage(void)
{
	MCWaitMessageEx(INFINITE);
/*    THandle HandleArray[2];
	bool cq = false;
	mcInt32 i;

//	if(FThreadID != GetCurrentThreadID())
//        THROW_ERROR(MCError_WrongReceiverThread);

    DispatchCompleteMessages();
	FIncomingCriticalSection->Enter();
	i = HasPendingMessages(FIncomingQueue);
	if(i == -1)
	{
		FIncomingCriticalSection->Leave();
		do {
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
			if(FIncomingEvent->WaitFor(0) == wrSignaled)
				cq = true;
			if(FCompleteEvent->WaitFor(0) == wrSignaled)
				DispatchCompleteMessages();
			Sleep(10);
#else
#endif
		} while(!cq);
	}
	else
		FIncomingCriticalSection->Leave();
    DispatchCompleteMessages();
    */
}

//Timeout pointed in milliseconds
bool MCMessenger::MCWaitMessageEx(mcUInt32 Timeout)
{
	MCHandle HandleArray[2];
	bool cq = false;
	mcInt32 i;

    double startTime = Now();
    
    DispatchCompleteMessages();
	FIncomingCriticalSection->Enter();
	i = HasPendingMessages(FIncomingQueue);
	if(i == -1)
	{
		FIncomingCriticalSection->Leave();
		do {
            double currentTime = Now();
            //to consider possible floating-point errors...
            DWORD elapsedTime = (currentTime > startTime + 0.0000001) ? (DWORD)((currentTime - startTime) * 86400000 + 0.5) : 0;
			//timeout expired?
            if (elapsedTime > Timeout && INFINITE != Timeout)
                return false;

#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))

            if(FIncomingEvent->WaitFor(0) == wrSignaled)
				cq = true;
			if(FCompleteEvent->WaitFor(0) == wrSignaled)
				DispatchCompleteMessages();
			Sleep(10);
#else
#endif
		} while(!cq);
	}
	else
		FIncomingCriticalSection->Leave();
    /*
    if (cq || i != -1)
    {
        //walk through FIncomingQueue
        FIncomingCriticalSection->Enter();
        for (mcInt32 j=0; j<FIncomingQueue->Length(); j++)
        {
            MCMessageInfo* pInfo = (MCMessageInfo*)FIncomingQueue->At(j);
            if (imsWaiting == pInfo->State)
            {
                char tmp[128];
                sprintf(tmp, "Message number %d arrived\n", pInfo->Message.Param1);
                OutputDebugString(tmp);
            }
        }
        FIncomingCriticalSection->Leave();
    }
    */
    DispatchCompleteMessages();
    return true;
}

void MCMessenger::GetMessageSource(const MCMessage* Message, char* Buffer, mcInt32* BufferSize)
{
//    if (GetCurrentThreadID() != FThreadID)
//        THROW_ERROR(MCError_WrongReceiverThread);

    FIncomingCriticalSection->Enter();
    mcInt32 i = FindMessageInQueue(Message->MsgID, FIncomingQueue);
    if (-1 != i)
    {
        MCMessageInfo* Info = (MCMessageInfo*)(*FIncomingQueue)[i];
        FIncomingCriticalSection->Leave();
        if ((NULL != Info) && (FTransportList->Find(Info->Transport) != -1))
        {
            TRY_BLOCK
            {
                Info->Transport->GetMessageSource(Info, Buffer, BufferSize);
            }
            CATCH_EVERY
            {
                ;
            }
        } 
        else
        {
            if (NULL != Buffer)
                Buffer[0] = 0;
            if (NULL != BufferSize)
                *BufferSize = 0;
        }
    }
    else
    {
        FIncomingCriticalSection->Leave();
        if (NULL != Buffer)
            Buffer[0] = 0;
        if (NULL != BufferSize)
            *BufferSize = 0;
    }
}


void MCMessenger::DoMessageProcessed(MCMessageInfo* Info)
{	
    // adjust message state
	if(Info->MCError == 0)
		Info->State = imsComplete;
	else
		Info->State = imsFailed;

	strncpy(Info->DQueue, "intREPLY\0", sizeof(Info->DQueue) - 1);

	if(Info->Message.DataType == bdtConst)
	{
		if((Info->Message.DataSize != 0) && Info->Message.Data)
			MCFreeNull(Info->Message.Data);
		else
		if (Info->Message.Data)
		{
			Info->Message.Data = NULL;
		}
		Info->Message.Data = NULL;
		Info->Message.DataSize = 0;
	}

	// if the message was posted, no need to return it back
	if(Info->IsSendMsg && (FTransportList->Find(Info->Transport) != -1))
	{
		TRY_BLOCK
        {
            Info->StartTime = GetTickCount();
			Info->Transport->MessageProcessed(Info);
        }
        CATCH_EVERY
        {

        }
	}
	else
	{
		delete Info; Info = NULL;//DestroyMessageInfo(Info);
	}
}

bool MCMessenger::ForwardMessage(const char* Destination, MCMessage* Message,
		                        mcUInt32 Timeout, MCMessageCredentials* Credentials)
{
	bool res = false;
	FIncomingCriticalSection->Enter();
	TRY_BLOCK
	{
		MCMessageInfo* Info = NULL;
		mcInt32 i = FindMessageInQueue(Message->MsgID, FForwardQueue);
		if (i != -1)
			Info = (MCMessageInfo *) ((*FForwardQueue)[i]);

		// only forward the message that has not been forwarded yet
		if (Info == NULL)
		{
			Info = NULL;
			i = FindMessageInQueue(Message->MsgID, FIncomingQueue);
			if (i != -1)
				Info = (MCMessageInfo *) ((*FIncomingQueue)[i]);

			if (Info != NULL)
			{
				MCMessageInfo* NewInfo = new MCMessageInfo(Message);
				NewInfo->OrigID = Message->MsgID;

				NewInfo->Message.MsgID = Message->MsgID;

				/*
				if (Message->Data != NULL)
				{
					NewInfo->Message.Data = MCMemAlloc(Message->DataSize);
					NewInfo->Message.DataSize = Message->DataSize;
					memmove(NewInfo->Message.Data, Message->Data, Message->DataSize);
				}
				*/

				NewInfo->Timeout     = Timeout;
				NewInfo->StartTime   = GetTickCount();
				NewInfo->NotifyProc  = NULL;
				NewInfo->TimeoutProc = NULL;
				NewInfo->ErrorProc   = NULL;
				NewInfo->UserData    = 0;
				NewInfo->ReplyFlag   = NULL;
				NewInfo->IsSendMsg   = true;
				if(Credentials)
					memmove(&NewInfo->Credentials, Credentials, sizeof(*Credentials));
				else
					memset(&NewInfo->Credentials, 0, sizeof(Info->Credentials));

				mcInt32 MCErrorCode = 0;
				TRY_BLOCK
				{
					DoSendMessage(Destination, NewInfo);

					FIncomingQueue->Del(Info);
					FForwardCriticalSection->Enter();
					FForwardQueue->Add(Info);
					FForwardCriticalSection->Leave();
			
					res = true;
				}
				CATCH_MCERROR
				{
					delete NewInfo; NewInfo = NULL;
#ifndef USE_CPPEXCEPTIONS
					THROW_ERROR(MCErrorCode);
#else
					throw;
#endif
				}
			}
		}
	}
	FINALLY 
	(
		FIncomingCriticalSection->Leave();
	)
	return res;
}

void MCMessenger::GetQueueStatus(const char* QueueName, mcInt32 MsgCodeLow, mcInt32 MsgCodeHigh, 
								mcInt32 &MsgCount, mcInt32 &MsgTotalSize, mcInt32 &MsgMaxSize)
{
	mcInt32 i;
    MCMessageInfo* Info;
    mcInt32 MS;

	MsgCount = 0;
	MsgMaxSize = 0;
	MsgTotalSize = 0;
	
	char* q = (char *) QueueName;
	if (q != NULL && strlen(q) == 0) 
		q = NULL;
	

	FIncomingCriticalSection->Enter();
	TRY_BLOCK
	{
		for (i = 0; i < FIncomingQueue->Length(); i++)
		{
			Info = (MCMessageInfo *) ((*FIncomingQueue)[i]);
			if (q) 
			{
#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__) || defined(QNX)
				if(stricmp(Info->DQueue, q)==0)
#else
				if(strcasecmp(Info->DQueue, q)==0)
#endif
					continue;
			}

			if (((MsgCodeLow != 0) || (MsgCodeHigh != 0)) &&
				((Info->Message.MsgCode < MsgCodeLow) || (Info->Message.MsgCode > MsgCodeHigh)))
				continue;

			MS = sizeof(MCMessage) - sizeof(Info->Message.Data) + Info->Message.DataSize;
			MsgTotalSize += MS;
			if (MS > MsgMaxSize)
				MsgMaxSize = MS;
			MsgCount++;
		}
	}
	FINALLY
	(
		FIncomingCriticalSection->Leave();
	);	
}

bool MCMessenger::GetMessageSent(__int64 MsgID)
{
	MCMessageInfo *Info;

	bool res = true;

	for (mcInt32 i = 0; i < FTransportList->Length(); i++)
	{
		Info = ((MCBaseTransport*)((*FTransportList)[i]))->GetMessageByID(MsgID);
		if (Info)
		{
			res = Info->State == imsWaiting;
			break;
		}
		else
			res = true;
	}
	return res;
}

void MCMessenger::CreateMessageSimple(mcInt32 MsgCode, mcInt32 Param1, mcInt32 Param2, MCMessage& Message)
{
	memset(&Message, 0, sizeof(MCMessage));
	Message.MsgCode = MsgCode;
	Message.Param1 = Param1;
	Message.Param2 = Param2;
}

void MCMessenger::CreateMessageFromBinary(mcInt32 MsgCode, mcInt32 Param1, mcInt32 Param2, void* Binary, mcInt32 BinarySize, bool IsVarData, MCMessage& Message)
{
	memset(&Message, 0, sizeof(MCMessage));
	Message.MsgCode = MsgCode;
	Message.Param1 = Param1;
	Message.Param2 = Param2;

	Message.DataSize =  BinarySize;

	if (BinarySize > 0)
	{
		Message.Data = MCMemAlloc(BinarySize);
		memmove(Message.Data, Binary, BinarySize);
	}
	else
		Message.Data = NULL;

	Message.DataType = IsVarData ? bdtVar : bdtConst;
}

void MCMessenger::CreateMessageFromText(mcInt32 MsgCode, mcInt32 Param1, mcInt32 Param2, const char* Text, bool IsVarData, MCMessage& Message)
{
	memset(&Message, 0, sizeof(MCMessage));
	Message.MsgCode = MsgCode;
	Message.Param1 = Param1;
	Message.Param2 = Param2;

	if (Text == NULL)
	{
		Message.DataSize = 0;
		Message.Data = NULL;
	}
	else
	{
		Message.DataSize = strlen(Text) + 1;
		Message.Data = MCMemAlloc(Message.DataSize);
		memmove(Message.Data, Text, Message.DataSize);
	}

	Message.DataType = IsVarData ? bdtVar : bdtConst;
}

void* MCMessenger::getDirectTransportList()
{
	if (DirectTransportCS == NULL)
			DirectTransportCS = new MCCriticalSection();

	void* result;

	DirectTransportCS->Enter();
	TRY_BLOCK
	{
		if (DirectTransports == NULL)
		{
			DirectTransports = new MCList();
		}	
		result = DirectTransports;
	}
	FINALLY (
		DirectTransportCS->Leave();
	)	
	return result;
}

void MCMessenger::setDirectTransportList(void* value)
{
	if (DirectTransports != value)
	{
		if (DirectTransportCS == NULL)
			DirectTransportCS = new MCCriticalSection();

		DirectTransportCS->Enter();
		if (DirectTransports != NULL)
		{
			delete DirectTransports;
		}
		DirectTransports = (MCList *) value;
		DirectTransportForeign = true;
		DirectTransportCS->Leave();
	}
}


/*const char* MCMessenger::GetMessageDestQueue(__int64 MsgID)
{
	MCMessageInfo *Info = NULL;

	for (mcInt32 i = 0; i < FTransportList->Length(); i++)
	{
		Info = ((MCBaseTransport*)((*FTransportList)[i]))->GetMessageByID(MsgID);
		if (Info)
		    return Info->DQueue;
	}
	return NULL;
}*/

// **************************** TMCMessageHandler *****************************

MCMessageHandler::MCMessageHandler(MCMessageHandlers* Collection)
{
	FCollection = Collection;
	FEnabled = true;
	FMsgCodeHigh = 4294967295U;
	FMsgCodeLow = 0;
	FOnMessage = NULL;
	FOnMessageData = NULL;
}

void MCMessageHandler::Assign(MCMessageHandler& Source)
{
	FEnabled = Source.getEnabled();
	FMsgCodeHigh = Source.getMsgCodeHigh();
	FMsgCodeLow = Source.getMsgCodeLow();
	FOnMessage = Source.getOnMessage();
	FOnMessageData = Source.FOnMessageData;
}

bool MCMessageHandler::getEnabled(void)
{
	return(FEnabled);
}

void MCMessageHandler::setEnabled(bool v)
{
	FEnabled = v;
}

mcInt32 MCMessageHandler::getMsgCodeHigh(void)
{
	return(FMsgCodeHigh);
}

void MCMessageHandler::setMsgCodeHigh(mcInt32 v)
{
	FMsgCodeHigh = v;
}

mcInt32 MCMessageHandler::getMsgCodeLow(void)
{
	return(FMsgCodeLow);
}

void MCMessageHandler::setMsgCodeLow(mcInt32 v)
{
	FMsgCodeLow = v;
}

MCHandleMessageEvent MCMessageHandler::getOnMessage(void**UserData)
{
	if (UserData)
		*UserData = FOnMessageData;
	
	return(FOnMessage);
}

void MCMessageHandler::setOnMessage(MCHandleMessageEvent v, void* UserData)
{
	FOnMessageData = UserData;
	FOnMessage = v;
}

// **************************** TMCMessageHandlers ****************************

void MCMessageHandlers::MCMessageHandlerDelete(void* /*sender*/, void* v)
{
	delete (MCMessageHandler*)v;
}

MCMessageHandlers::MCMessageHandlers(MCQueue* Queue)
{
	FOwner = Queue;
	DeleteItem = MCMessageHandlers::MCMessageHandlerDelete;
}

/*
Adds a message handler to the list and returns the reference to this 
handler.
*/
MCMessageHandler* MCMessageHandlers::Add(void)
{
    MCMessageHandler* mh =  new MCMessageHandler(this);
    MCList::Add(mh);
    return mh;    
}

MCMessageHandler* MCMessageHandlers::getItems(mcInt32 Index)
{
	return((MCMessageHandler*)((*this)[Index]));
}

MCQueue* MCMessageHandlers::GetOwner(void)
{
	return FOwner;
}

void MCMessageHandlers::setItems(mcInt32 Index, const MCMessageHandler* Value)
{
	this->Data[Index] = (void*)Value;
}

// ********************************* TMCQueue *********************************

MCQueue::MCQueue()
{
	FHandlers = new MCMessageHandlers(this);
	FMessenger = NULL;
	FOnUnhandledMessage = NULL;
	FOnUnhandledMessageData = NULL;
	FQueueName = NULL;
}

MCQueue::~MCQueue()
{
	FMessenger = NULL;
	delete FHandlers; FHandlers = NULL;
	if (NULL != FQueueName)
        free(FQueueName);
    FQueueName = NULL;
}

void MCQueue::ProcessMessage(MCMessageInfo* Info)
{
	mcInt32 i;
	bool Handled = false;

	if(!FMessenger) return;
	i = 0;
	
	while(i < FHandlers->Length())
	{
		Handled = false;
		void* UserData;
		if(FHandlers->getItems(i)->getOnMessage(&UserData))
		{
			if((FHandlers->getItems(i)->getMsgCodeHigh() >= Info->Message.MsgCode)
				&& (FHandlers->getItems(i)->getMsgCodeLow() <= Info->Message.MsgCode)
				&& (FHandlers->getItems(i)->getEnabled()))
			{				
				FHandlers->getItems(i)->getOnMessage()(UserData, this, Info->Message, Handled);
			}
		}
		if(Handled)
			break;
		i++;
	}
	if(!Handled)
	{
		void* UserData;
		if(getOnUnhandledMessage(&UserData))
			getOnUnhandledMessage()(UserData, this, Info->Message, Handled);
		if(!Handled)
			Info->Message.Result = 0;
	}
	FMessenger->MessageProcessed(&Info->Message);
}

MCMessageHandlers* MCQueue::getHandlers(void)
{
	return(FHandlers);
}

void MCQueue::setHandlers(MCMessageHandlers* Value)
{
//	FHandlers->copyof(Value);
	FHandlers = Value;
}

MCMessenger* MCQueue::getMessenger(void)
{
	return(FMessenger);
}

void MCQueue::setMessenger(MCMessenger* Value)
{
	if(FMessenger != Value)
	{
		if(FMessenger)
		{
			FMessenger->RemoveQueue(this);
		}
		FMessenger = Value;
		if(FMessenger)
		{
			FMessenger->AddQueue(this);
		}
	}
}

MCHandleMessageEvent MCQueue::getOnUnhandledMessage(void**UserData)
{
	if (UserData)
		*UserData = FOnUnhandledMessageData;
	
	return(FOnUnhandledMessage);
}

void MCQueue::setOnUnhandledMessage(MCHandleMessageEvent Value, void* UserData)
{
	FOnUnhandledMessageData = UserData;
	FOnUnhandledMessage = Value;
}

char* MCQueue::getQueueName(void)
{
	return FQueueName;
}

void MCQueue::setQueueName(char* Value)
{
	FQueueName = _strdup(Value);
}

// ***************************** TMCBaseTransport *****************************

MCBaseTransport::MCBaseTransport()
{
	FLinkCriticalSection = new MCCriticalSection();
	FCriticalSection = new MCCriticalSection();
	
	FMaxTimeout = INFINITE;
	FMaxMsgSize = 0x7FFFDFFF;
	FThreadID = GetCurrentThreadID();
	FActive = false;
	FCompressor = NULL;
	FEncryptor = NULL;
	FMessenger = NULL;
	FSealer = NULL;
	FName = NULL;
	FDefaultTransportName = NULL;
	FIgnoreIncomingPriorities = false;
	FDiscardUnsentMessages = false;
}

MCBaseTransport::~MCBaseTransport()
{
	setActive(false);
	FLinkCriticalSection->Enter();
	try {
		setMessenger(NULL);
		setCompressor(NULL);
		setEncryptor(NULL);
		setSealer(NULL);
	} FINALLY (
		FLinkCriticalSection->Leave();
		delete FLinkCriticalSection;
		FLinkCriticalSection = NULL;
	)
	delete FCriticalSection;
	FCriticalSection = NULL;
	MCMemFree(FName);
}


void MCBaseTransport::InsertInfoIntoQueueWithPriorities(MCList* Queue, MCMessageInfo* Info, bool IgnoreMode)
{
	mcInt32 Cnt, j;
	MCMessageInfo* TmpInfo;
	Cnt = Queue->Length();
	j = Cnt - 1;
	while (j >= -1)
	{
		if (j == -1)
		{
			Queue->Insert(0, Info);
			if (Cnt > 1)
			{
				TmpInfo = (MCMessageInfo *)((*Queue)[1]);				
			}
		}
		else
		{
			TmpInfo = (MCMessageInfo *)((*Queue)[j]);
			if ((TmpInfo != NULL) && (TmpInfo->IsSendMsg && (!Info->IsSendMsg)) ||
				((TmpInfo->IsSendMsg == Info->IsSendMsg) && (IgnoreMode || 
				(TmpInfo->Message.Priority >= Info->Message.Priority))) )
			{
				Queue->Insert(j + 1, Info);
				break;
			}
		}
		j--;
	}
}

bool MCBaseTransport::getActive(void)
{
	return(FActive);
}

MCBaseCompression* MCBaseTransport::getCompressor(void)
{
	return(FCompressor);
}

MCCBaseEncryption* MCBaseTransport::getEncryptor(void)
{
	return FEncryptor;
}

mcInt32 MCBaseTransport::getMaxMsgSize(void)
{
	return FMaxMsgSize;
}

void MCBaseTransport::setMaxMsgSize(mcInt32 v)
{
	FMaxMsgSize = v;
}

mcUInt32 MCBaseTransport::getMaxTimeout(void)
{
	return FMaxTimeout;
}

void MCBaseTransport::setMaxTimeout(mcUInt32 v)
{
	FMaxTimeout = v;
}

MCMessenger* MCBaseTransport::getMessenger(void)
{
	return FMessenger;
}

MCBaseSealing* MCBaseTransport::getSealer(void)
{
	return FSealer;
}

bool MCBaseTransport::AddressValid(char* Address, bool ChangeAddress)
{
	char* S = NULL;
	mcInt32 l = 0;
	bool r = false;

	if(!FName)
	{
		l = strlen(FDefaultTransportName) + 2;
		S = (char*)MCMemAlloc(l);
		strcpy(S, FDefaultTransportName);
	}
	else
	{
		l = strlen(FName) + 2;
		S = (char*)MCMemAlloc(l);
		strcpy(S,FName);
	}
	S[l - 2] = ':';
	S[l - 1] = 0;
#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__) || defined(QNX)
	r = strnicmp(Address, S, l - 1) == 0;
#else
	r = strncasecmp(Address, S, l - 1) == 0;
#endif
	MCMemFree(S); S = NULL;
    if(true == r && true == ChangeAddress)
    {
		memmove(Address, &Address[l - 1], strlen(Address) - l + 1);
        Address[strlen(Address) - l + 1] = 0;
    }
	return r;
}

void MCBaseTransport::DoSetActive(void)
{
  // intentionally left blank
}

void MCBaseTransport::Loaded(void)
{
	if(FActive)
	{
		FActive = false;
		setActive(true);
	}
}

/*
	PrepareDataBlock takes some buffer and prepares it for delivery. After the 
	call to this method the Data block is considered to be invalid (
	PrepareDataBlock most likely will dispose of it).
*/

void MCBaseTransport::PrepareDataBlock(char* Remote,void* Data, mcInt32 DataSize, void*& OutputData,
	mcInt32& OutputSize, mcInt32& EncryptID, mcInt32& CompressID, mcInt32& SealID)
{
	bool b;

	FLinkCriticalSection->Enter();
	try {
		OutputData = Data;
		OutputSize = DataSize;
  
		// compress
		if(CompressID > 0)
		{
			if(FCompressor && (FCompressor->GetID() == CompressID))
      		{
				b = true;
				FCompressor->Compress(Remote, Data, DataSize, OutputData, OutputSize, b);
				if(b)
				{
					Data = OutputData;
					DataSize = OutputSize;
				}
				else
					CompressID = 0;
			}
			else
				CompressID = 0;
		}
		else
			CompressID = 0;
  
		// seal
		if(SealID != 0)
		{
			if(FSealer && (FSealer->GetID() == SealID))
			{
				b = true;
				FSealer->Seal(Remote, Data, DataSize, OutputData, OutputSize, b);
				if(b)
				{
					Data = OutputData;
					DataSize = OutputSize;
				}
				else
					SealID = 0;
			}
			else
				SealID = 0;
		}
		else
			SealID = 0;
  
		// encrypt
		if(EncryptID != 0)
		{
			if(FEncryptor && (FEncryptor->GetID() == EncryptID))
			{
				b = true;
				FEncryptor->Encrypt(Remote, Data, DataSize, OutputData, OutputSize, b);
				if(b)
				{
					/*Data = OutputData;*/
					/*DataSize = OutputSize;*/
				}
				else
					EncryptID = 0;
			}
			else
				EncryptID = 0;
		}
		else
			EncryptID = 0;
	} FINALLY (
		FLinkCriticalSection->Leave();
	)
}

void MCBaseTransport::setActive(bool Value)
{
	if(FActive != Value)
	{
		FActive = Value;
		try {
			if (FMessenger)
				DoSetActive();
#ifdef USE_CPPEXCEPTIONS
		} catch(...) {
#else
		} __except(1) {
#endif
			FActive = false;
#ifdef USE_CPPEXCEPTIONS
			throw;
#else
			RaiseException(1,0,0,0);
#endif
		}
	}
}

void MCBaseTransport::setCompressor(MCBaseCompression* Value)
{
	bool b;

	if(FCompressor != Value)
	{
//		FLinkCriticalSection->Enter();
//		TRY_BLOCK {
			b = getActive();
			setActive(false);
			FCompressor = Value;
			setActive(b);
//		} FINALLY (
//			FLinkCriticalSection->Leave();
//		)
	}
}

void MCBaseTransport::setEncryptor(MCCBaseEncryption* Value)
{
	bool b;

	if(FEncryptor != Value)
	{
//		FLinkCriticalSection->Enter();
//		TRY_BLOCK {
			b = getActive();
			setActive(false);
			FEncryptor = Value;
			setActive(b);
//		} FINALLY (
//			FLinkCriticalSection->Leave();
//		)
	}
}

void MCBaseTransport::setMessenger(MCMessenger* Value)
{
	bool b, b1;

	if(FMessenger != Value)
	{
		b1 = getActive();
		setActive(false);
			
		FLinkCriticalSection->Enter();
		try {
			b = FMessenger != NULL;
			if(b)
				FMessenger->RemoveTransport(this);
			FMessenger = Value;
			if(FMessenger)
			{
				FMessenger->AddTransport(this);
//				FMessenger->FreeNotification(this);
			}
		} FINALLY (
			FLinkCriticalSection->Leave();
		)
		setActive(b1);
	}
}

void MCBaseTransport::setSealer(MCBaseSealing* Value)
{
	bool b;

	if(FSealer != Value)
	{
//		FLinkCriticalSection->Enter();
//		TRY_BLOCK {
			b = getActive();
			setActive(false);
			FSealer = Value;
			setActive(b);
//		} FINALLY (
//			FLinkCriticalSection->Leave();
//		)
	}
}

/*
	PrepareDataBlock takes some buffer and prepares it for delivery. After the 
	call to this method the Data block is considered to be invalid (
	PrepareDataBlock most likely will dispose of it).
*/

bool MCBaseTransport::UnprepareDataBlock(char* Remote,void* Data, mcInt32 DataSize, void*& OutputData,
	mcInt32& OutputSize, mcInt32 EncryptID, mcInt32 CompressID, mcInt32 SealID)
{
	bool b, b1, b2, b3;

	FLinkCriticalSection->Enter();
	TRY_BLOCK
    {
		OutputData = Data;
		OutputSize = DataSize;
		b1 = false;
		b2 = false;
		b3 = false;
  
		// compress
		if(EncryptID != 0)
		{
			if(FEncryptor && (FEncryptor->GetID() == EncryptID))
			{
				b = true;
				FEncryptor->Decrypt(Remote, Data, DataSize, OutputData, OutputSize, EncryptID, b);
				if(b)
				{
					Data = OutputData;
					DataSize = OutputSize;
					b3 = true;
				}
			}
		}
		else
		if(EncryptID == 0)
			b3 = true;

		// seal
		if(b3)
		{
			if(SealID != 0)
			{
				if(FSealer && (FSealer->GetID() == SealID))
				{
					b = true;
					FSealer->UnSeal(Remote, Data, DataSize, OutputData, OutputSize, SealID, b);
					if(b)
					{
						Data = OutputData;
						DataSize = OutputSize;
						b2 = true;
					}
				}
			}
			else
			if(SealID == 0)
				b2 = true;
		}
  
		// encrypt
		if(b2)
		{
			if(CompressID > 0)
			{
				if(FCompressor && (FCompressor->GetID() == CompressID))
				{
					b = true;
					FCompressor->Decompress(Remote, Data, DataSize, OutputData, OutputSize, CompressID, b);
					if(b)
					{
						//Data = OutputData;
						//DataSize = OutputSize;
						b1 = true;
					}
				}
			}
			else
			if(CompressID == 0)
				b1 = true;
		}
	} FINALLY (
		FLinkCriticalSection->Leave();
	)
	return b1;
}

void MCBaseTransport::GetMessageSource(MCMessageInfo* Info, char* Buffer, mcInt32* BufferSize)
{
    bool EmptySource = false;
    EmptySource = (NULL == FName) && (NULL == FDefaultTransportName);
    if (false == EmptySource)
        EmptySource = FName ? (FName[0] == 0) : (FDefaultTransportName[0] == 0);

    if (true == EmptySource)
    {
        if (NULL != Buffer)
            Buffer[0] = 0;
        if (NULL != BufferSize)
            *BufferSize = 0;
    }
    else
    {
        //char temp[16]; sprintf(temp, "%d", Info->ConnID);
		if (NULL != BufferSize)
        {
            if (NULL != FName)
                *BufferSize = strlen(FName) + 1 + strlen(Info->SMessenger)/* + 1 + strlen(temp)*/;
            else
                *BufferSize = strlen(FDefaultTransportName) + 1 + strlen(Info->SMessenger) /*+ 1 + strlen(temp)*/;
        }
        if (NULL != Buffer)
        {
            strcpy(Buffer, (NULL != FName) ? FName : FDefaultTransportName);
            strcat(Buffer, ":"); 
			//strcat(Buffer, temp);
			//strcat(Buffer, "#"); 
			strcat(Buffer, Info->SMessenger);
        }
    }
            
}



#ifdef MC_NO_PURE_VIRTUAL
void MCBaseTransport::CancelMessage(MCList* Queue, MCMessageInfo* Info)
{
	MC_pure_virtual();
}

bool MCBaseTransport::DeliverMessage(char* DestAddress, MCMessageInfo* Info)
{
	MC_pure_virtual();
	return false;
}

void MCBaseTransport::MessageProcessed(MCMessageInfo* Info)
{
	MC_pure_virtual();
}

void MCBaseTransport::CleanupMessages(void)
{
	MC_pure_virtual();
}

void MCBaseTransport::CleanOutgoingQueue(void)
{
	MC_pure_virtual();
}

mcInt32 MCBaseTransport::GetOutgoingMessagesCount()
{
	MC_pure_virtual();
	return 0;
}
#endif

char* MCBaseTransport::RealTransportName()
{
	if (FName)
		return FName;
	else
		return FDefaultTransportName;
}

char* MCBaseTransport::getName(void)
{
	return FName;
}

void MCBaseTransport::setName(char* Value)
{
	MCMemFree(FName);
	FName = strCopy(Value, 0);
}

bool MCBaseTransport::CancelMessageByID(__int64 MsgID, mcUInt32 ErrorCode, bool Discard)
{
	bool res = false;
	FCriticalSection->Enter();
	TRY_BLOCK
	{
		MCMessageInfo *Info = GetMessageByID(MsgID);
		if (Info)
		{
			CancelMessage(NULL, Info);
			Info->State = imsFailed;
			Info->MCError = ErrorCode;

			res = true;

			if (Discard == false)
			{
				FLinkCriticalSection->Enter();
				TRY_BLOCK
				{
				
					FMessenger->FCompleteCriticalSection->Enter();
					//TRY_BLOCK {
						FMessenger->FCompleteQueue->Add(Info);
						FMessenger->FCompleteEvent->setEvent();
						if (Info->ReplyFlag)
							Info->ReplyFlag->setEvent();
					//} FINALLY (
						FMessenger->FCompleteCriticalSection->Leave();
					//)		
				}
				FINALLY (
					FLinkCriticalSection->Leave();
				)
			}
			else
			{
				delete Info;
				Info = NULL;
			}				
		}
	}
	FINALLY
	(
		FCriticalSection->Leave();
	)
	return res;
}

MCMessageInfo* MCBaseTransport::GetMessageByID(__int64 MsgID)
{
	return NULL;
}
	
bool MCBaseTransport::getIgnoreIncomingPriorities(void)
{
	return FIgnoreIncomingPriorities;
}

void MCBaseTransport::setIgnoreIncomingPriorities(bool Value)
{
	FIgnoreIncomingPriorities = Value;
}

void MCBaseTransport::Shutdown(bool DiscardUnsentMessages)
{
	if (getActive())
	{
		FDiscardUnsentMessages = DiscardUnsentMessages;
		setActive(false);
		FDiscardUnsentMessages = false;
	}
	else
	if (FDiscardUnsentMessages)
		CleanOutgoingQueue();
}

// **************************** TMCDirectTransport ****************************

MCDirectTransport* MCDirectTransport::FindTransportByID(char* ID)
{
	MCDirectTransport* Transport = NULL;
	if (DirectTransports == NULL)
		return NULL;
	for (mcInt32 i = 0; i < DirectTransports->Length(); i++)
	{
		Transport = ((MCDirectTransport*)(*DirectTransports)[i]);
#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__) || defined(QNX)
		if (stricmp(ID, Transport->getTransportID()) == 0)
#else
		if (strcasecmp(ID, Transport->getTransportID()) == 0)
#endif
		{
			return (MCDirectTransport*)(*DirectTransports)[i];
		}
	}
	return NULL;
}

MCDirectTransport::MCDirectTransport()
:MCBaseTransport()
{
	FDefaultTransportName = "LOCAL";

	FTransportID = (char *) MCMemAlloc(11);
	sprintf(FTransportID, "%d", (int)(mcPtrInt) this); 

	FOutgoingQueue = new MCList();

	if (DirectTransportCS == NULL)
	{
		DirectTransportCS = new MCCriticalSection();	
		DirectTransports = new MCList();
	}	
}

MCDirectTransport::~MCDirectTransport()
{
	setActive(false);
	delete FOutgoingQueue;

	MCMemFree(FTransportID); FTransportID = NULL;
	DirectTransportCS->Enter();
	if (DirectTransports != NULL && DirectTransports->Length() == 0)
	{
		delete DirectTransports; DirectTransports = NULL;
	}
	DirectTransportCS->Leave();
}

mcInt32 MCDirectTransport::GetOutgoingMessagesCount()
{
	return FOutgoingQueue->Length();
}

void MCDirectTransport::DoSetActive(void)
{
	if (getActive())
	{
		DirectTransportCS->Enter();
		if (DirectTransports == NULL)
		{
			DirectTransports = new MCList();
		}
		DirectTransports->Add(this);
		DirectTransportCS->Leave();
	}
	else
	{
		DirectTransportCS->Enter();
		DirectTransports->Del(this);
		DirectTransportCS->Leave();

		if (FOutgoingQueue != NULL)
		{
			while (FOutgoingQueue->Length() > 0)
			{
				CancelMessageByID(((MCMessageInfo*)(*FOutgoingQueue)[0])->Message.MsgID, MCError_Shutdown);
			}
		}
	}
}

void MCDirectTransport::CancelMessage(MCList* Queue, MCMessageInfo* Info)
{
	if (Queue == NULL)
	{
		FCriticalSection->Enter();
		TRY_BLOCK
		{
			FOutgoingQueue->Del(Info);
		} FINALLY (
			FCriticalSection->Leave();
		)
	}
}

bool MCDirectTransport::DeliverMessage(char* DestAddress, MCMessageInfo* Info)
{
	mcInt32 p;
	char *sep, *dm;
	MCDirectTransport* DestTransport = NULL;

	if(DestAddress && AddressValid(DestAddress))
	{
		sep = strchr(DestAddress, '|');
		
		if (sep != NULL)
		{
			p = sep - DestAddress;
						
			dm = (char *) MCMemAlloc(p + 1);
			strncpy(dm, DestAddress, p);
			dm[p] = 0;
			
			DestTransport = FindTransportByID(dm);
			if (DestTransport == NULL)
			{
				MCMemFree(dm);
				THROW_ERROR(MCError_BadDestinationName);
			}
		}
		else
		{
			dm = _strdup(FTransportID);
			DestTransport = this;
		}
			
		__int64 saveID = Info->Message.MsgID;

		MCMessageInfo* NewInfo = new MCMessageInfo(&Info->Message);
		Info->Message.MsgID = saveID;
		Info->Transport = this;

		NewInfo->OrigID = Info->OrigID;

		NewInfo->setDMessenger(dm);
		NewInfo->setSMessenger(FTransportID);
		
		sep++;
		strncpy(NewInfo->DQueue, sep, DQueueMaxLength - 1);
		NewInfo->DQueue[DQueueMaxLength - 1] = 0;
		
		NewInfo->IsSendMsg = Info->IsSendMsg;
  
		memmove(&NewInfo->Credentials, &Info->Credentials, sizeof(Info->Credentials));

		/*
		if (Info->Message.Data != NULL)
		{
			NewInfo->Message.Data = MCMemAlloc(Info->Message.DataSize);
			memmove(NewInfo->Message.Data, Info->Message.Data, Info->Message.DataSize);
			NewInfo->Message.DataSize = Info->Message.DataSize;
		}
		*/

		if(Info->IsSendMsg)
		{
			FCriticalSection->Enter();
			try
			{
				FOutgoingQueue->Add(Info);
				DestTransport->MessageReceived(NewInfo);
			}
			FINALLY 
			(
				FCriticalSection->Leave();
			)
		}
		else
		{
			delete Info; Info = NULL;
			DestTransport->MessageReceived(NewInfo);
		}

		MCMemFree(dm);

		return true;
	}
	else
		return false;
}

void MCDirectTransport::MessageReceived(MCMessageInfo* Info)
{
	Info->Transport = this;
	Info->State = imsWaiting;

	FMessenger->FIncomingCriticalSection->Enter();
	try 
	{
		InsertInfoIntoQueueWithPriorities(FMessenger->FIncomingQueue, Info, FIgnoreIncomingPriorities);
		FMessenger->FIncomingEvent->setEvent();
	} FINALLY (
		FMessenger->FIncomingCriticalSection->Leave();
	)
}

void MCDirectTransport::ReplyReceived(MCMessageInfo* Info)
{
	MCMessageInfo* CurInfo = NULL;
	
	FCriticalSection->Enter();
	try
	{
		for (mcInt32 i = 0; i < FOutgoingQueue->Length(); i++)
		{
			if (((MCMessageInfo*)((*FOutgoingQueue)[i]))->OrigID == Info->OrigID)
			{
				CurInfo = (MCMessageInfo*)((*FOutgoingQueue)[i]);
				FOutgoingQueue->Del(i);
				break;
			}
		}
	}
	FINALLY
	(
		FCriticalSection->Leave();
	)
	if (CurInfo != NULL)
	{	
		if (CurInfo->Message.Data != NULL)	
		{
			MCMemFree(CurInfo->Message.Data);
			CurInfo->Message.Data = NULL;
		}
		Info->CopyMessage(CurInfo);
		Info->Message.DataSize = 0;
		Info->Message.Data = NULL;

		CurInfo->MCError = Info->MCError;
		CurInfo->State = Info->State;

		DoMessageProcessed(CurInfo);
	}
}

void MCDirectTransport::DoMessageProcessed(MCMessageInfo* Info)
{
	FMessenger->FCompleteCriticalSection->Enter();
	try {
		FMessenger->FCompleteQueue->Add(Info);
		FMessenger->FCompleteEvent->setEvent();
		if(Info->ReplyFlag)
			Info->ReplyFlag->setEvent();
	} FINALLY (
		FMessenger->FCompleteCriticalSection->Leave();
	)
}

void MCDirectTransport::MessageProcessed(MCMessageInfo* Info)
{
	MCDirectTransport *DestTransport = FindTransportByID(Info->SMessenger);
	if (DestTransport != NULL)
	{
		DestTransport->ReplyReceived(Info);
		delete Info; Info = NULL;
	}
}

void MCDirectTransport::CleanOutgoingQueue(void)
{
	FCriticalSection->Enter();
	TRY_BLOCK
	{
		if (FOutgoingQueue != NULL)
		{
			while (FOutgoingQueue->Length() > 0)
			{
				CancelMessageByID(((MCMessageInfo*)(*FOutgoingQueue)[0])->Message.MsgID, MCError_Shutdown);
			}
		}
	}
	FINALLY
	(
		FCriticalSection->Leave();
	)
}

void MCDirectTransport::CleanupMessages()
{
	mcInt32 i;
    MCMessageInfo* Info;
    mcUInt32 TC;
    bool me;

	FCriticalSection->Enter();
	TRY_BLOCK
    {
		i = 0;
		TC = GetTickCount();
		while (i < FOutgoingQueue->Length()) 
		{    
			Info = (MCMessageInfo*)((*FOutgoingQueue)[i]);

			if (Info->Timeout == 0)
			{
				i++;
				continue;
			}

			if (TC < Info->StartTime)
			{
				me = 0xFFFFFFFF - Info->StartTime + TC >= Info->Timeout;
			}
			else
			{
				me = TC - Info->StartTime >= Info->Timeout;
			}
			if (me)
			{
				CancelMessage(NULL, Info);
				Info->State = imsExpired;
				FLinkCriticalSection->Enter();
				TRY_BLOCK
				{
				
					FMessenger->FCompleteCriticalSection->Enter();
					TRY_BLOCK {
						FMessenger->FCompleteQueue->Add(Info);
						FMessenger->FCompleteEvent->setEvent();
						if (Info->ReplyFlag)
							Info->ReplyFlag->setEvent();
					} FINALLY (
						FMessenger->FCompleteCriticalSection->Leave();
					)		
				}
				FINALLY (
					FLinkCriticalSection->Leave();
				)
			}
			else
				i++;
		}
	} 
    FINALLY (
		FCriticalSection->Leave();
	)
}

MCMessageInfo* MCDirectTransport::GetMessageByID(__int64 MsgID)
{
	mcInt32 i;
    MCMessageInfo* Info;
    MCMessageInfo* res = NULL;

	FCriticalSection->Enter();
	TRY_BLOCK
    {
		i = 0;
		while (i < FOutgoingQueue->Length()) 
		{    
			Info = (MCMessageInfo*)((*FOutgoingQueue)[i]);
			
			if (Info->Message.MsgID == MsgID)
			{
				res = Info;
				break;
			}
			else
				i++;
		}
	} 
    FINALLY (
		FCriticalSection->Leave();
	)
	return res;
}

// **************************** TMCSimpleTransport ****************************
MCSimpleTransport::MCSimpleTransport()
{
	FOutgoingQueue = new MCList();
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
#if !defined(QNX) && !defined(__APPLE__)
	FOutgoingCount = new MCSemaphore(0, IPC_PRIVATE, true);
#else
	FOutgoingCount = new MCSemaphore(0, NULL, true);
#endif
#else
#endif
}

MCSimpleTransport::~MCSimpleTransport()
{
	delete FOutgoingQueue;
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
	delete FOutgoingCount;
#else
#endif	
}

void MCSimpleTransport::CleanOutgoingQueue(void)
{
	FCriticalSection->Enter();
	TRY_BLOCK
	{
		if (FOutgoingQueue != NULL)
		{
			while (FOutgoingQueue->Length() > 0)
			{
				CancelMessageByID(((MCMessageInfo*)(*FOutgoingQueue)[0])->Message.MsgID, MCError_Shutdown);
			}
		}
	}
	FINALLY
	(
		FCriticalSection->Leave();
	)
}

void MCSimpleTransport::CleanupMessages()
{
	mcInt32 i;
    MCMessageInfo* Info;
    mcUInt32 TC;
    bool me;

	FCriticalSection->Enter();
	TRY_BLOCK
    {
		i = 0;
		TC = GetTickCount();
		while (i < FOutgoingQueue->Length()) 
		{    
			Info = (MCMessageInfo*)((*FOutgoingQueue)[i]);

			if (Info->Timeout == 0)
			{
				i++;
				continue;
			}

			if (TC < Info->StartTime)
			{
				me = 0xFFFFFFFF - Info->StartTime + TC >= Info->Timeout;
			}
			else
			{
				me = TC - Info->StartTime >= Info->Timeout;
			}
			if (me)
			{
				CancelMessage(NULL, Info);
				Info->State = imsExpired;
				FLinkCriticalSection->Enter();
				TRY_BLOCK
				{
				
					FMessenger->FCompleteCriticalSection->Enter();
					TRY_BLOCK {
						FMessenger->FCompleteQueue->Add(Info);
						FMessenger->FCompleteEvent->setEvent();
						if (Info->ReplyFlag)
							Info->ReplyFlag->setEvent();
					} FINALLY (
						FMessenger->FCompleteCriticalSection->Leave();
					)		
				}
				FINALLY (
					FLinkCriticalSection->Leave();
				)
			}
			else
				i++;
		}
	} 
    FINALLY (
		FCriticalSection->Leave();
	)
}

MCMessageInfo* MCSimpleTransport::GetMessageByID(__int64 MsgID)
{
	mcInt32 i;
    MCMessageInfo* Info;
    MCMessageInfo* res = NULL;

	FCriticalSection->Enter();
	TRY_BLOCK
    {
		i = 0;
		while (i < FOutgoingQueue->Length()) 
		{    
			Info = (MCMessageInfo*)((*FOutgoingQueue)[i]);
			
			if (Info->Message.MsgID == MsgID)
			{
				res = Info;
				break;
			}
			else
				i++;
		}
	} 
    FINALLY (
		FCriticalSection->Leave();
	)
	return res;
}

mcInt32 MCSimpleTransport::GetOutgoingMessagesCount()
{
	return FOutgoingQueue->Length();
}

void MCSimpleTransport::MessageReceived(MCMessageInfo* Info)
{
	// add to incoming queue
	FLinkCriticalSection->Enter();
	TRY_BLOCK
    {
		if(FMessenger)
		{
			// check message credentials
			if(!FMessenger->ValidateCredentials(Info))
			{
				Info->MCError = MCError_WrongCredentials;
				Info->State = imsFailed;
				// if the message was posted, no need to return it back
				if(Info->IsSendMsg)
				{
					FLinkCriticalSection->Leave();
					MessageProcessed(Info);
					FLinkCriticalSection->Enter();
				}
				else
                {
					delete Info; Info = NULL;//MCMessenger::DestroyMessageInfo(Info);
                }
			}
			else
			{
				Info->MCError = 0;
				Info->Transport = this;
				Info->State = imsWaiting;

				FMessenger->FIncomingCriticalSection->Enter();
				TRY_BLOCK
                {
					InsertInfoIntoQueueWithPriorities(FMessenger->FIncomingQueue, Info, FIgnoreIncomingPriorities);
					
					FMessenger->FIncomingEvent->setEvent();
					// Begin platform-specific code (Windows)
//					PostThreadMessage(FMessenger->getThreadID(), WM_NULL, 0, 0);
					// End platform-specific code (Windows)
				} FINALLY (
					FMessenger->FIncomingCriticalSection->Leave();
				)
			}
		}
		else
        {
			delete Info; Info = NULL;//MCMessenger::DestroyMessageInfo(Info);
        }
	} FINALLY (
		FLinkCriticalSection->Leave();
	)
}

void MCSimpleTransport::ReplyReceived(MCMessageInfo* Info)
{
	// add to complete queue
	FLinkCriticalSection->Enter();
	TRY_BLOCK
    {
		if(FMessenger)
		{
			FMessenger->FCompleteCriticalSection->Enter();
			TRY_BLOCK
            {
				FMessenger->FCompleteQueue->Add(Info);
				FMessenger->FCompleteEvent->setEvent();
				if(Info->ReplyFlag)
					Info->ReplyFlag->setEvent();
//				else
//					return;
			} FINALLY (
				FMessenger->FCompleteCriticalSection->Leave();
			)
		}
		else
        {
			delete Info; Info = NULL;
            //MCMessenger::DestroyMessageInfo(Info);
        }
	} FINALLY (
		FLinkCriticalSection->Leave();
	)
}

void MCSimpleTransport::DeliveryFailed(MCMessageInfo* Info)
{
	FLinkCriticalSection->Enter();
	TRY_BLOCK
    {
		if(FMessenger && (Info->State == imsDispatching || Info->State == imsWaiting) && Info->IsSendMsg)
		{
			FMessenger->FCompleteCriticalSection->Enter();
			TRY_BLOCK
            {
				Info->State = imsFailed;
				FMessenger->FCompleteQueue->Add(Info);
				FMessenger->FCompleteEvent->setEvent();
				if(Info->ReplyFlag)
					Info->ReplyFlag->setEvent();
			} FINALLY (
				FMessenger->FCompleteCriticalSection->Leave();
			)
		}
		else
        {
			delete Info; Info = NULL;
        }
	} FINALLY (
		FLinkCriticalSection->Leave();
	)
}

void MCSimpleTransport::MessageProcessed(MCMessageInfo* Info)
{
	// Info->DQueue[0] = 0;
	MCMemFree(Info->DMessenger);
	Info->DMessenger = _strdup(Info->SMessenger);
	MCMemFree(Info->SMessenger);
	Info->SMessenger = NULL;
	SetInfoSMessenger(Info);	
	FCriticalSection->Enter();
	TRY_BLOCK
    {
		InsertInfoIntoQueueWithPriorities(FOutgoingQueue, Info, FIgnoreIncomingPriorities);
		KickSender(true);
	} FINALLY (
		FCriticalSection->Leave();
	)
}

void MCSimpleTransport::CancelMessage(MCList* Queue, MCMessageInfo* Info)
{
	if (Queue == NULL)
	{
		FCriticalSection->Enter();
		TRY_BLOCK
		{
			CancelMessageInSender(Info);
			FOutgoingQueue->Del(Info);
		} FINALLY (
			FCriticalSection->Leave();
		)
	}
}

void MCSimpleTransport::KickSender(bool MessageFlag)
{
	// default implementation does nothing
}

void MCSimpleTransport::CancelMessageInSender(MCMessageInfo* Info)
{
	// default implementation does nothing
}

bool MCSimpleTransport::WasCancelled(MCMessageInfo* Info)
{
	// default implementation does nothing
	return false;
}

void MCSimpleTransport::SetInfoSMessenger(MCMessageInfo* Info)
{
	// default implementation does nothing
}


static volatile mcInt32 UniqueID = 0;
MCCriticalSection UniqueIDCS;

mcInt32 CreateUniqueID(void)
{
    UniqueIDCS.Enter();
    mcInt32 result = ++UniqueID;
    UniqueIDCS.Leave();
    return result;
}

void MCBaseInitialization(void)
{
	DefineOS();
#if (defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))) || defined(_WIN32_WCE)
	GlobalSeed = (mcUInt32)(((__int64)0x7FFFFFFFul)*Random()/(RAND_MAX+1));
#else
#if !defined(_MSC_VER) && !(defined(__MINGW32__) || defined(__MINGW64__))
	GlobalSeed = random(0x7FFFFFFFul);
#else
	GlobalSeed = rand();
#endif
#endif
	InitializeThreads();
	MCMainThreadID = GetCurrentThreadID();
}

static class MsgConnectInitializer
{
public:    
    MsgConnectInitializer(void)
    {
        MCBaseInitialization();
    }
} InitMsgConnectObject;



#ifdef USE_NAMESPACE
}
#endif

