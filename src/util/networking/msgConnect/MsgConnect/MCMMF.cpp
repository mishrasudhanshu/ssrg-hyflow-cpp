//====================================================
//                                                    
//  EldoS MsgConnect                                 
//                                                    
//   Copyright (c) 2001-2010, EldoS                   
//                                                    
//====================================================
#include "MC.h"
#include "MCUtils.h"
#include "MCMMF.h"
#ifdef _WIN32_WCE
#	include <Excpt.h>
#endif
 
#ifdef _WIN32
#   include <stdlib.h>
#endif

#ifdef USE_NAMESPACE
namespace MsgConnect
{
#endif


const mcInt32 MMF_FRAME_SIZE = 4096;
const mcInt32 MMF_CLEAN_SIZE = 4096 - sizeof(mcInt32);

  
// ***************************** TMCMMFTransport ******************************

MCMMFTransport::MCMMFTransport(void):MCSimpleTransport()
{
	FDefaultTransportName = "MMF";
	FMessengerName = NULL;
	FReceiverThread = NULL;
	FSenderThread = NULL;
}

MCMMFTransport::~MCMMFTransport(void)
{
	setActive(false);
	
	MCMemFree(FMessengerName);
	delete FReceiverThread;
	delete FSenderThread;
}

bool MCMMFTransport::DeliverMessage(char* DestAddress,MCMessageInfo* Info)
{
	bool r;
	char *p, *p1;
	char *MMFName, *QueueName;
	//mcInt32 i;

	if(DestAddress && AddressValid(DestAddress))
	{
		Info->Transport = this;

		if(!strchr(DestAddress, '|'))
		{
            THROW_ERROR(MCError_BadDestinationName);
		}

        p = DestAddress;
		p1 = p;
		while(*p && (*p != '|'))
			p++;
		if(!*p)
            THROW_ERROR(MCError_BadDestinationName);

        // get MMF name
		if(p - p1 > 0)
			MMFName = strCopy(p1, 0, (mcInt32)(p - p1));
		else
            THROW_ERROR(MCError_BadDestinationName);

        p++;
		// get queue name
		p1 = p;
		while(*p)
			p++;
		if(p - p1 > 0)
			QueueName = strCopy(p1, 0, (mcInt32)(p - p1));
		else
            THROW_ERROR(MCError_BadDestinationName);
  
		// setup message info properties

		strncpy(Info->DQueue, QueueName, DQueueMaxLength - 1);
        MCMemFree(QueueName); QueueName = NULL;
        
		Info->DQueue[DQueueMaxLength - 1] = 0;
		Info->DMessenger = MMFName;
		
		Info->SMessenger = strdup(FMessengerName);
  
		Info->State = imsDispatching;
		FCriticalSection->Enter();
  
		TRY_BLOCK
		{
			InsertInfoIntoQueueWithPriorities(FOutgoingQueue, Info, false);

			KickSender(true);
		} FINALLY (
			FCriticalSection->Leave();
		)
		r = true;
	}
	else
		r = false;
	return r;
}

MCMMFSenderThread* MCMMFTransport::CreateSenderThread(bool x)
{
    return new MCMMFSenderThread(x);
}

MCMMFReceiverThread* MCMMFTransport::CreateReceiverThread(bool x)
{
    return new MCMMFReceiverThread(x);
}

void MCMMFTransport::DoSetActive(void)
{
	bool initFailed = false;
//	MCHandle HandleArray[2];

	if(!FMessenger)
		return;

	if(getActive())
	{
		if(!getMessengerName())
            THROW_ERROR(MCError_NoMessengerName);
		FSenderThread = NULL;

		// initialize receiver
		FReceiverThread = CreateReceiverThread(true);//new MCMMFReceiverThread(true);
		TRY_BLOCK
        {
			FReceiverThread->Initialize(this);
        }
		CATCH_EVERY
        {
			delete FReceiverThread; FReceiverThread = NULL;
			initFailed = true;
		}
		
		if (true == initFailed)
#ifndef USE_CPPEXCEPTIONS
			RaiseException(1, 0, 0, NULL);
#else
			throw EMCError(0);
#endif	
        
//				RaiseException(((EXCEPTION_POINTERS*)_exception_info)->ExceptionRecord->ExceptionRecord->ExceptionCode,
//                            ((EXCEPTION_POINTERS*)_exception_info)->ExceptionRecord->ExceptionFlags,
//                            ((EXCEPTION_POINTERS*)_exception_info)->ExceptionRecord->NumberParameters,
//                            ((EXCEPTION_POINTERS*)_exception_info)->ExceptionRecord->ExceptionInformation);

		FReceiverThread->setFreeOnTerminate(false);

		// initialize sender
		FSenderThread = CreateSenderThread(true);//new MCMMFSenderThread(true);
		TRY_BLOCK
        {
			FSenderThread->Initialize(this);
		}
        CATCH_EVERY
        {
			delete FSenderThread;
			FSenderThread = NULL;
			delete FReceiverThread;
			FReceiverThread = NULL;
			initFailed = true;
		}
		if (true == initFailed)
#ifndef USE_CPPEXCEPTIONS
			RaiseException(1, 0, 0, NULL);
#else
			throw EMCError(0);
#endif	
        FSenderThread->setFreeOnTerminate(false);
		FReceiverThread->Resume();
		FSenderThread->Resume();
	}
	else
	{
		if (NULL != FSenderThread)
        {
            FSenderThread->Terminate();
		    FSenderThread->FStopEvent->setEvent();
        }
		    
        if (NULL != FReceiverThread)
        {
            FReceiverThread->Terminate();
		    FReceiverThread->FStopEvent->setEvent();
        }
        
        if ((NULL != FSenderThread) && (NULL != FReceiverThread))
        {
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
		    FSenderThread->WaitFor();
		    FReceiverThread->WaitFor();
#else
#endif
        }
		delete FSenderThread; FSenderThread = NULL;
		delete FReceiverThread; FReceiverThread = NULL;

		if (FOutgoingQueue != NULL)
		{
			while (FOutgoingQueue->Length() > 0)
			{
				CancelMessageByID(((MCMessageInfo*)(*FOutgoingQueue)[0])->Message.MsgID, MCError_Shutdown);
			}
		}
	}
}

char* MCMMFTransport::getMessengerName(void)
{
	return(FMessengerName);
}

void MCMMFTransport::setMessengerName(const char* Value)
{
	bool b;

	if(!FMessengerName && !Value)
		return;
	if(!FMessengerName || !Value || (strcmp(FMessengerName, Value) != 0))
	{
		b = getActive();
		setActive(false);
		if(FMessengerName)
			MCMemFree(FMessengerName);
		FMessengerName = strdup(Value);
		setActive(b);
	}
}

void MCMMFTransport::KickSender(bool MessageFlag)
{
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
	FOutgoingCount->Release();
#else
#endif
}

void MCMMFTransport::CancelMessageInSender(MCMessageInfo* Info)
{
	if(FSenderThread != NULL)
	{
		FSenderThread->FCancelID = Info->Message.MsgID;
		while (FSenderThread->FSendingInfo == Info)
			Sleep(100);
	}
}

bool MCMMFTransport::WasCancelled(MCMessageInfo* Info)
{
	return (FSenderThread && (FSenderThread->FCancelID == Info->Message.MsgID));
}

void MCMMFTransport::SetInfoSMessenger(MCMessageInfo* Info)
{
	Info->SMessenger = strdup(FMessengerName);
}

// **************************** TMCMMFSenderThread ****************************

MCMMFSenderThread::~MCMMFSenderThread(void)
{
	delete FStopEvent;
}

bool MCMMFSenderThread::CanExecute(void)
{
	return true;
}

void MCMMFSenderThread::Execute(void)
{
    FCancelID = 0;
#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
#endif

	while(!getTerminated())
	{
		TRY_BLOCK
        {
			// wait for messages to be delivered
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
			if(FOwner->FOutgoingCount->WaitFor(0) == wrSignaled)
				SendMessage();
			if(FStopEvent->WaitFor(0) == wrSignaled)
				break;
			Sleep(10);
#else
#endif
			if(getTerminated())
				return;
        }
        CATCH_EVERY
        {
		}
	}
}

#ifdef _WIN32_WCE
#ifndef OpenEvent
HANDLE OpenEvent(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCTSTR lpName)
{
	HANDLE res = CreateEvent(NULL, false, false, lpName);
	if (res != NULL)
	{
		DWORD lastErr = GetLastError();
		if (lastErr == ERROR_ALREADY_EXISTS)
			return res;
		else
		{
			CloseHandle(res);
			SetLastError(ERROR_FILE_NOT_FOUND);
			return NULL;
		}
	}
	else
		return NULL;
}
#endif
#endif

void MCMMFSenderThread::Initialize(MCMMFTransport* Owner)
{
	FOwner = Owner;
	FStopEvent = new MCEvent(NULL, true , false, NULL);
}

void MCMMFSenderThread::SendMessage(void)
{
	MCMessageInfo* Info;	
	MCMessageInfo* tmpInfo;
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
	MCEvent *RFreeMutex, *RReplyEvent, *RRequestEvent, *RReceiptEvent;
	mcInt32 RMMF;
#else
#endif
	MCMMFHeader* RMMFPtr;
	
	char* DMessenger;
	mcInt32 i;
	//char* AQueue;
	__int64 StartTime, EndTime;
	mcUInt32 ToWait;
	char* p;
	__int64 GTC;
	MCStream* AStream = NULL;

	Info = NULL;
	
	FOwner->FCriticalSection->Enter();
	TRY_BLOCK
    {
		
        for(i = 0;i <= FOwner->FOutgoingQueue->Length() - 1;i++)
		{
			tmpInfo = (MCMessageInfo*)((*FOwner->FOutgoingQueue)[i]);
			
			MCMessageState aState = tmpInfo->State;			
			if(aState == imsDispatching || aState == imsComplete || aState == imsFailed)
			{
				Info = tmpInfo;
				FOwner->FOutgoingQueue->Del(i);
				break;
			}
		}
	} FINALLY (
		FOwner->FCriticalSection->Leave();
	)
  
	if(!Info)
		return;
  
	FSendingInfo = Info;
	
	if (FCancelID == Info->Message.MsgID)
	{
		FSendingInfo = NULL;
		return;
	}

	//	AQueue = Pstr2c(Info->DQueue);
	//	AQueue = strdup(Info->DQueue);
		DMessenger = Info->DMessenger;
		// get destination messenger name
		if(!DMessenger)
		{
			Info->MCError = MCError_GenTransportFailure;
			FOwner->DeliveryFailed(Info);
			FSendingInfo = NULL;
			return;
		}
  
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
		char* ttmpdir = getenv("TEMP");
		char tmpdir[1024];
		if(ttmpdir)
		{
			strcpy(tmpdir, ttmpdir);
			strcat(tmpdir, "/");
		}
		else
			strcpy(tmpdir, "/tmp/");
		p = (char*)MCMemAlloc(strlen(tmpdir) + strlen(DMessenger) + strlen("_Mapping") + 1);
		strcpy(p, tmpdir);
		strcat(p, DMessenger);
		strcat(p, "_Mapping");
		RMMF = open(p, O_RDWR|O_SYNC, DEFFILEMODE);
		MCMemFree(p);
		if(RMMF < 0)
#else
#endif
		{
			Info->MCError = MCError_MMFCreationFailed;
			FOwner->DeliveryFailed(Info);
			FSendingInfo = NULL;
			return;
		}

#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
		RMMFPtr = (MCMMFHeader*)mmap(0, MMF_FRAME_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, RMMF, 0);
		if(-1 == (intptr_t) RMMFPtr)
#else
#endif
		{
			Info->MCError = MCError_MMFCreationFailed;
			FOwner->DeliveryFailed(Info);
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
			close(RMMF);
#else
#endif
			FSendingInfo = NULL;
			return;
		}

		// create necessary events
		RFreeMutex = NULL;
		RReceiptEvent = NULL;
		RRequestEvent = NULL;
		RReplyEvent = NULL;

		TRY_BLOCK
		{
			TRY_BLOCK
			{
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
				p = (char*)MCMemAlloc(strlen(DMessenger) + strlen("_MMFFreeMutex") + 1);
				strcpy(p, DMessenger);
				strcat(p, "_MMFFreeMutex");
				RFreeMutex		= new MCEvent(NULL, false, false, p, true);
      			MCMemFree(p);

				p = (char*)MCMemAlloc(strlen(DMessenger) + strlen("_ReceiptEvent") + 1);
				strcpy(p, DMessenger);
				strcat(p, "_ReceiptEvent");
				RReceiptEvent	= new MCEvent(NULL, false, false, p, true);
				MCMemFree(p);

				p = (char*)MCMemAlloc(strlen(DMessenger) + strlen("_RequestEvent") + 1);
				strcpy(p, DMessenger);
				strcat(p, "_RequestEvent");
				RRequestEvent	= new MCEvent(NULL, false, false, p, true);
				MCMemFree(p);

				p = (char*)MCMemAlloc(strlen(DMessenger) + strlen("_ReplyEvent") + 1);
				strcpy(p, DMessenger);
				strcat(p, "_ReplyEvent");
				RReplyEvent		= new MCEvent(NULL, false, false, p, true);
				MCMemFree(p);
#else
#endif

				if((RFreeMutex == 0) ||
					(RReceiptEvent == 0) ||
					(RRequestEvent == 0) ||
					(RReplyEvent == 0))
					THROW_ERROR(MCError_MutexCreationFailed);
  
				// write transferrable data to the buffer
				AStream = new MCMemStream();
				i = strlen(Info->SMessenger);
				if(i > 0)
					AStream->Write(&i, sizeof(i));
  
				AStream->Write(Info->SMessenger, i);
				Info->WriteToStream(AStream);
				if(Info->Message.DataSize > 0)
					AStream->Write((char*)(Info->Message.Data), Info->Message.DataSize);
				AStream->SetPos(0);
  
				// initialize time counters
				StartTime = GetTickCount();
				ToWait = FOwner->getMaxTimeout();
				EndTime = StartTime + ToWait;
	
				if (FCancelID != Info->Message.MsgID)
				{

#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
					mcInt32 l_wait = ToWait*100;
					while(true)
					{
						if(FStopEvent->WaitFor(0) == wrSignaled)
							throw EMCError(MCError_TransportTimeout);
						if(RFreeMutex->WaitFor(0) == wrSignaled)
							break;
						Sleep(10);
						l_wait--;
						if(l_wait == 0)
						{
							printf("timeout when waiting for signal\n");
							throw EMCError(MCError_TransportTimeout);
						}
					}
#else
#endif
  
					// begin transfer operation
					RMMFPtr->dwDataSize = AStream->Len();
  
					while(AStream->Pos() < AStream->Len())
					{
						RMMFPtr->bIsFirst = AStream->Pos() == 0;
						RMMFPtr->dwErrorCode = 0;
						RMMFPtr->dwBlockStart = AStream->Pos();
						if((RMMFPtr->dwMMFSize - sizeof(MCMMFHeader)) < (mcUInt32)(AStream->Len() - AStream->Pos()))
							RMMFPtr->dwBlockSize = RMMFPtr->dwMMFSize - sizeof(MCMMFHeader);
						else
							RMMFPtr->dwBlockSize = AStream->Len() - AStream->Pos();
  
						p = (char*)&RMMFPtr->dwBlockSize;
						p += 4;
						AStream->Read(p, RMMFPtr->dwBlockSize);
  
						// flag the availability of the data
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
						msync(RMMFPtr, MMF_FRAME_SIZE, MS_SYNC);
						RRequestEvent->setEvent();
#else
#endif
  
						GTC = GetTickCount();
						if(GTC < StartTime)
#if defined(__GNUC__)
							GTC = GTC + 0x100000000ll;
#else
#endif
						if((GTC > EndTime) && (ToWait != INFINITE))
							THROW_ERROR(MCError_TransportTimeout);
						
						if(ToWait != INFINITE)
#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
#else
							ToWait = EndTime - GTC;
#endif

#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
						mcInt32 l_wait = ToWait*100;
						while(true)
						{
							if(FStopEvent->WaitFor(0) == wrSignaled)
								throw EMCError(MCError_TransportTimeout);
							if(RReplyEvent->WaitFor(0) == wrSignaled)
								break;
							Sleep(10);
							l_wait--;
							if(l_wait == 0)
							{
								printf("timeout when waiting for signal\n");
								throw EMCError(MCError_TransportTimeout);
							}
						}
#else
#endif
						if(AStream->Pos() < AStream->Len())
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
							RReceiptEvent->setEvent();
#else
#endif
					}

					if((Info->State == imsDispatching) && Info->IsSendMsg && (Info->Message.MsgID != FCancelID))
					{
						FOwner->FCriticalSection->Enter();
						TRY_BLOCK
						{
							Info->State = imsWaiting;
							FOwner->FOutgoingQueue->Add(Info);
						} FINALLY (
							FOwner->FCriticalSection->Leave();
						)
					}
					else
					{
						if (Info->Message.MsgID != FCancelID)
						{
							delete Info; 
							Info = NULL; //MCMessenger::DestroyMessageInfo(Info);
						}
						else
						{
							FCancelID = 0;
						}
					}
				}
				else
				{
					FCancelID = 0;
				}

#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
				RReceiptEvent->setEvent();
				RFreeMutex->setEvent();
#else
#endif
#ifdef USE_CPPEXCEPTIONS
			} catch(EMCError* E) {
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
				RFreeMutex->setEvent();
#else
#endif
				Info->MCError = E->ErrorCode();
				FOwner->DeliveryFailed(Info);
	//			return;
			}
#else
			} __except(1) {
				ReleaseMutex(RFreeMutex);
				Info->MCError = _exception_code();
				FOwner->DeliveryFailed(Info);
	//			return;
			}
#endif
		  
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
		} 
		FINALLY 
		(
			FSendingInfo = NULL;
			delete AStream;
			munmap(RMMFPtr, MMF_FRAME_SIZE);
			close(RMMF);
			delete RFreeMutex;
			delete RReceiptEvent;
			delete RRequestEvent;
			delete RReplyEvent;
		)
#else
#endif
}

// *************************** TMCMMFReceiverThread ***************************
MCMMFReceiverThread::MCMMFReceiverThread(bool CreateSuspended)
:MCThread(CreateSuspended), FStopEvent(NULL), FReceiptEvent(NULL), FRequestEvent(NULL),
    FReplyEvent(NULL)
{
#ifdef _WIN32
    FMapping = NULL;
    FMMFFreeMutex = NULL;
#endif
}


MCMMFReceiverThread::~MCMMFReceiverThread(void)
{
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
	close(FMapping);
	unlink(FMn);
	MCMemFree(FMn);
	delete FMMFFreeMutex;
#else
#endif
	delete FStopEvent;
	delete FReceiptEvent;
	delete FRequestEvent;
	delete FReplyEvent;
}

bool MCMMFReceiverThread::CanExecute(void)
{
	return true;
}

void MCMMFReceiverThread::Execute(void)
{
//	MCHandle HandleArray[2];

	while(!getTerminated())
	{
		TRY_BLOCK
        {
		// wait for messages to be delivered
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
			if(FRequestEvent->WaitFor(0) == wrSignaled)
			{
				ReceiveMessage();
				FMMFFreeMutex->setEvent();
			}
			else
			if(FStopEvent->WaitFor(0) == wrSignaled)
				break;
			Sleep(10);
#else
#endif
			if(getTerminated())
				return;
        }
        CATCH_EVERY
        {
        }
	}
}

void MCMMFReceiverThread::Initialize(MCMMFTransport* Owner)
{
#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
#endif
//	mcInt32 i;
	MCMMFHeader* MMFPtr;
	char* p;

	FOwner = Owner;
#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
#else
#define pSA NULL
#endif
  
	FStopEvent = new MCEvent(NULL, true , false, NULL);
#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
#endif
  
#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
#endif
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
	char* ttmpdir = getenv("TEMP");
	char tmpdir[1024];
	if(ttmpdir)
	{
		strcpy(tmpdir, ttmpdir);
		strcat(tmpdir, "/");
	}
	else
		strcpy(tmpdir, "/tmp/");
	FMn = (char*)MCMemAlloc(strlen(tmpdir) + strlen(Owner->getMessengerName()) + strlen("_Mapping") + 1);
	strcpy(FMn, tmpdir);
	strcat(FMn, Owner->getMessengerName());
	strcat(FMn, "_Mapping");
	FMapping = open(FMn, O_CREAT|O_EXCL|O_RDWR|O_SYNC, 00666);
	if(FMapping < 0)
	    THROW_ERROR(MCError_MMFCreationFailed);

	void *c;
  	lseek(FMapping, sizeof(mcInt32), SEEK_SET);
  	c = malloc(MMF_CLEAN_SIZE);
  	memset(c, 0, MMF_CLEAN_SIZE);
  	write(FMapping, c, MMF_CLEAN_SIZE);
  	free(c);
  	lseek(FMapping, 0, SEEK_SET);

	MMFPtr = (MCMMFHeader*)mmap(0, MMF_FRAME_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, FMapping, 0);
	if(MMFPtr == MAP_FAILED)
	    THROW_ERROR(MCError_MMFCreationFailed);
#else
	p = (char*)MCMemAlloc(strlen(Owner->getMessengerName()) + strlen("_Mapping") + 1);
	strcpy(p, Owner->getMessengerName());
	strcat(p, "_Mapping");
#if defined(_WIN32_WCE) || defined(UNICODE)
	w = s2w(p);
	SetLastError(0);
	FMapping = CreateFileMapping((void*)0xfffffffful, pSA, PAGE_READWRITE, 0, MMF_FRAME_SIZE, w);
	delete (void*)w;
#else
    SetLastError(0);
	FMapping = CreateFileMapping(/*(void*)0xfffffffful*/INVALID_HANDLE_VALUE, pSA, PAGE_READWRITE, 0, MMF_FRAME_SIZE, p);
#endif
	MCMemFree(p);
	i = GetLastError();
	if(((FMapping != 0) && (i == ERROR_ALREADY_EXISTS)) || ((FMapping == 0) && (i != 0)))
	    THROW_ERROR(MCError_MMFCreationFailed);
	MMFPtr = (MCMMFHeader*)MapViewOfFile(FMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if(!MMFPtr)
    	    THROW_ERROR(MCError_MMFCreationFailed);
#endif
 
	MMFPtr->dwMMFSize = MMF_FRAME_SIZE;

#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))

	p = (char*)MCMemAlloc(strlen(Owner->getMessengerName()) + strlen("_MMFFreeMutex") + 1);
	strcpy(p, Owner->getMessengerName());
	strcat(p, "_MMFFreeMutex");
	FMMFFreeMutex	= new MCEvent(pSA, false, false, p);
	MCMemFree(p);

	p = (char*)MCMemAlloc(strlen(Owner->getMessengerName()) + strlen("_ReceiptEvent") + 1);
	strcpy(p, Owner->getMessengerName());
	strcat(p, "_ReceiptEvent");
	FReceiptEvent = new MCEvent(pSA, false, false, p);
	MCMemFree(p);

	p = (char*)MCMemAlloc(strlen(Owner->getMessengerName()) + strlen("_RequestEvent") + 1);
	strcpy(p, Owner->getMessengerName());
	strcat(p, "_RequestEvent");
	FRequestEvent = new MCEvent(pSA, false, false, p);
	MCMemFree(p);

	p = (char*)MCMemAlloc(strlen(Owner->getMessengerName()) + strlen("_ReplyEvent") + 1);
	strcpy(p, Owner->getMessengerName());
	strcat(p, "_ReplyEvent");
	FReplyEvent   = new MCEvent(pSA, false, false, p);
	MCMemFree(p);
  	
	if(!FMMFFreeMutex)
	    THROW_ERROR(MCError_MutexCreationFailed);
	
#endif

#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
	munmap(MMFPtr, MMF_FRAME_SIZE);
	FMMFFreeMutex->setEvent();
#else
#endif
}

#ifdef _WIN32_WCE
//suppress C4509
#pragma warning( disable : 4509)
#endif

void MCMMFReceiverThread::ReceiveMessage(void)
{
	MCMMFHeader* MMFPtr;
	MCMessageInfo *Info, *CurInfo;
	__int64 StartTime, EndTime;
	mcUInt32 ToWait;
	MCMemStream* AStream;
	mcInt32 TotalSize;
//	MCHandle HandleArray[2];
	__int64 GTC;
	MCMessage Message;
	mcInt32 i;
	char* s;

	TRY_BLOCK
    {
		AStream = new MCMemStream();
		try {
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
			MMFPtr = (MCMMFHeader*)mmap(0, MMF_FRAME_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, FMapping, 0);
			if (((void*) MAP_FAILED) == MMFPtr)
#else
#endif
//				return; // what else can we do?
				RETURN

			try {
#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
#else
				FReceiptEvent->resetEvent();
				FReplyEvent->resetEvent();
#endif
				// initialize time counters
				StartTime = GetTickCount();
				ToWait = FOwner->getMaxTimeout();
				EndTime = StartTime + ToWait;
  
				TotalSize = MMFPtr->dwDataSize;
				do {
					if(MMFPtr->dwBlockStart != AStream->Pos())
					{
						AStream->SetPos(MMFPtr->dwBlockStart);
					}
					AStream->Write((char*)((char *)MMFPtr + sizeof(MCMMFHeader)), MMFPtr->dwBlockSize);
  
					memset(((unsigned char*)MMFPtr + sizeof(mcUInt32)), 0, MMF_CLEAN_SIZE);
	
					FReplyEvent->setEvent();
  
					GTC = GetTickCount();
					if(GTC < StartTime)
#if defined(__GNUC__)
						GTC = GTC + 0x100000000ll;
#else
#endif
					if(GTC > EndTime)
                        THROW_ERROR(MCError_TransportTimeout);

						if(ToWait != INFINITE)
#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
#else
							ToWait = EndTime - GTC;
#endif


#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
					mcInt32 l_wait = ToWait*100;
					while(true)
					{
						if(FStopEvent->WaitFor(0) == wrSignaled)
							RETURN
						if(FReceiptEvent->WaitFor(0) == wrSignaled)
							break;
						Sleep(10);
						l_wait--;
						if(l_wait == 0)
							RETURN
					}
#else
#endif
					// ResetEvent(FReplyEvent.Handle);
					if(AStream->Len() < TotalSize)
					{
						GTC = GetTickCount();
						if(GTC < StartTime)
#if defined(__GNUC__)
							GTC = GTC + 0x100000000ll;
#else
#endif
						if(GTC > EndTime)
                            THROW_ERROR(MCError_TransportTimeout);

                        if(ToWait != INFINITE)
#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
#else
							ToWait = EndTime - GTC;
#endif

#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
						l_wait = ToWait*100;
						while(true)
						{
							if(FStopEvent->WaitFor(0) == wrSignaled)
								RETURN
							if(FRequestEvent->WaitFor(0) == wrSignaled)
								break;
							Sleep(10);
							l_wait--;
							if(l_wait == 0)
								RETURN
						}
#else
#endif
					}
				} while(AStream->Len() < TotalSize);
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
			} FINALLY (
				munmap(MMFPtr, MMF_FRAME_SIZE);
			)
#else
#endif
			// message received. Load it
			Info = NULL;
			AStream->SetPos(0);
			if(AStream->Len() < (mcInt32)sizeof(mcInt32))
//				return;
				RETURN
			AStream->Read(&i, sizeof(i));
			//if(AStream->Len() < i + ((char*)(&Info->Message.Data) - (char*)(Info)))
//				return;
			//	RETURN
			s = NULL;
			if(i > 0)
			{
        		s = (char*)MCMemAlloc(i + 1);
				AStream->Read(s, i);
				s[i] = 0;
			}
  
			Message.DataSize = 0;
			Info = new MCMessageInfo(&Message);
			Info->SMessenger = s;
			Info->LoadFromStream(AStream); //AStream->Read(Info, (mcInt32)((char*)(&Info->Message.Data) - (char*)(Info)));
			if(Info->State == imsDispatching) // we have received an original message
			{
				MCMessenger::SetMessageID(Info);
				if(AStream->Len() - AStream->Pos() < Info->Message.DataSize)
				{
					if(Info->IsSendMsg && (!getTerminated()))
					{
						// decline the message as the data portion is corrupt
						Info->MCError = MCError_GenTransportFailure;
						Info->State = imsFailed;
						FOwner->FCriticalSection->Enter();
						try {
							FOwner->FOutgoingQueue->Add(Info);
						} FINALLY (
							FOwner->FCriticalSection->Leave();
						)
					}
					else
                    {
						delete Info; Info = NULL;//MCMessenger::DestroyMessageInfo(Info);
                    }
					RETURN
				}
				else
				if(Info->Message.DataSize > 0)
				{
					Info->Message.Data = MCMemAlloc(Info->Message.DataSize);
					AStream->Read(Info->Message.Data, Info->Message.DataSize);
				}
				else
					Info->Message.Data = NULL;
				FOwner->MessageReceived(Info);
			}
			else // we have received a reply
			{
				// remove the message from the list of pending messages
				FOwner->FCriticalSection->Enter();
				try {
					CurInfo = NULL;
					for(i = 0;i < FOwner->FOutgoingQueue->Length();i++)
						if(((MCMessageInfo*)((*FOwner->FOutgoingQueue)[i]))->OrigID == Info->OrigID)
						{
							CurInfo = (MCMessageInfo*)(*FOwner->FOutgoingQueue)[i];
							FOwner->FOutgoingQueue->Del(i);
							break;
						}
				} FINALLY (
					FOwner->FCriticalSection->Leave();
				)
  
				if(!CurInfo) // the message was cancelled
                {
					delete Info; Info = NULL;//MCMessenger::DestroyMessageInfo(Info);
                }
				else
				{
					// we have to copy result data and signal about reply
					if (bdtConst != CurInfo->Message.DataType)
					{
						if(NULL != CurInfo->Message.Data)
						{//free old (request) data
							MCMemFree(CurInfo->Message.Data); 
							CurInfo->Message.Data = NULL;
							CurInfo->Message.DataSize = 0;
						}
						// read the resulting data from the stream
						CurInfo->Message.DataSize = Info->Message.DataSize;
						if(CurInfo->Message.DataSize > 0)
						{
							CurInfo->Message.Data = MCMemAlloc(CurInfo->Message.DataSize);
							AStream->Read(CurInfo->Message.Data, Info->Message.DataSize);
						}
						else
							CurInfo->Message.Data = NULL;
  
						// reset fields in Info
						Info->Message.Data = NULL;
						Info->Message.DataSize = 0;
					}

					CurInfo->Message.Result = Info->Message.Result;
					CurInfo->MCError = Info->MCError;
					CurInfo->State = Info->State;
  
					delete Info; Info = NULL;//MCMessenger::DestroyMessageInfo(Info);
					FOwner->ReplyReceived(CurInfo);
				}
			}
		} FINALLY (
			delete AStream;
		)
    }
	CATCH_EVERY
    {
    }
}

#ifdef USE_NAMESPACE
}
#endif

