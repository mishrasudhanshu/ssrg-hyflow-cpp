//====================================================
//                                                    
//  EldoS MsgConnect                                 
//                                                    
//   Copyright (c) 2001-2010, EldoS                   
//                                                    
//====================================================
#include <iostream>
#include "MC.h"
#include "MCUtils.h"
#include "MCInetTransport.h"
#include "MCThreads.h"


#ifdef USE_NAMESPACE
namespace MsgConnect
{
#endif

#ifdef __GNUC__
	void OutputDebugString(char *text)
{
	
}

//#define TEXT(x) x

#endif

#ifdef _WIN32
#include <stdio.h>
#include <stdarg.h>

void ShowDebugInfo(const char* format,...)
{
#ifdef _SHOW_DEBUG_INFO
	char log[256];
	va_list lst;
	va_start(lst, format);
	vsprintf(log, format, lst);
	va_end(lst);
	//OutputDebugString(log);
#endif
}

#else
void ShowDebugInfo(const char* format,...)
{
}
#endif
//mcInt32 MCInetTransport::FConnectionID = 0;

#ifdef _WIN32_WCE
//suppress C4509
#pragma warning( disable : 4509)
#endif
//#pragma warning( disable : 4355)

inline mcInt32 Min(mcInt32 A, mcInt32 B)
{
  return A < B ? A : B;
};

//yes, I know about warning. But 'this' pointer is not used during construction of MCThreadPool, so I'm able to code such construction
MCInetTransport::MCInetTransport()
:FThreadPool(this, this) 
{
	FDefaultSocketFactory = new MCStdSocketFactory();
	FSocketFactory = NULL;

	char empty[20] = "";
	FDefaultTransportName = empty;
	FOutgoingQueue = new MCList();
	FAttemptsToConnect = 0;
    FAttemptsInterval = 1;
	FConnectionTimeout = 10000;
	FConnections = new MCList();
	FActive = false;

	FListenerJob = NULL;
	FBandwidthPolicy = bpStrict;
	FNoTransformerFallback = false;
	//FAttemptsInterval = 0;
	FClientThreadLimit = 0;
	FFailOnInactive = false;
	FInactivityTime = 0;
	FIncomingMemoryThreshold = 0;
	FMessengerAddress = (char*) MCMemAlloc(strlen("0.0.0.0") + 1);
	strcpy(FMessengerAddress, "0.0.0.0");
	FMessengerPort = 0;
	FOutgoingMemoryThreshold = 0;
	FTempFilesFolder = NULL;
	FThreadPoolSize = 0;
	FTransportMode = (MCInetTransportMode)0;
	FUseTempFilesForIncoming = false;
	FUseTempFilesForOutgoing = false;
    FReqTime = 5;
    FTransformerFailProc = NULL;
    FMsgTimeout = 0;
	FIncomingBufferSize = 8192;
	FOutgoingBufferSize = 8192;
	FIncomingSpeedLimit = 0;
	FOutgoingSpeedLimit = 0;

	FNoUDPNotify = false;


    //  Web tunneling support
    FUseWebTunneling = false;
    FWebTunnelAddress = NULL;
    FWebTunnelPort = 4080;
    FWebTunnelAuthentication = wtaNoAuthentication;
    FWebTunnelUserId = NULL;
    FWebTunnelPassword = NULL;
	
	FReuseServerPort = true;

	this->FOnConnected = NULL;
	this->FOnConnectedData = 0;
	this->FOnDisconnected = NULL;
	this->FOnDisconnectedData = 0;
}


MCInetTransport::~MCInetTransport()
{
	setActive(false);
	delete FOutgoingQueue; FOutgoingQueue = NULL;
    delete FConnections; FConnections = NULL;
    delete FDefaultSocketFactory; FDefaultSocketFactory = NULL;

	if (NULL != FMessengerAddress)
    {
        free(FMessengerAddress);
        FMessengerAddress = NULL;
    }
    if (NULL != FTempFilesFolder)
    {
        free(FTempFilesFolder);
        FTempFilesFolder = NULL;
    }

    //  Web tunneling support
    if (NULL != FWebTunnelAddress)
    {
        free(FWebTunnelAddress);
        FWebTunnelAddress = NULL;
    }
    if (NULL != FWebTunnelUserId)
    {
        free(FWebTunnelUserId);
        FWebTunnelUserId = NULL;
    }
    if (NULL != FWebTunnelPassword)
    {
        free(FWebTunnelPassword);
        FWebTunnelPassword = NULL;
    }
}

class MCSocketThread: public MCWorkerThread
{
public:
    MCSocketThread(MCThreadPool* Pool, mcInt32 Priority = -1): MCWorkerThread(Pool, Priority) {};
    virtual void Terminate(void);
	
	virtual void KickThread(bool MessageFlag);
};

void MCSocketThread::Terminate()
{
    MCWorkerThread::Terminate();
	KickThread(false);
}

void MCSocketThread::KickThread(bool MessageFlag)
{
	bool NoNotify;

	if (FJob != NULL)
	{
#ifndef _WIN32_WCE
		if (typeid(*FJob) == typeid(MCInetTransportJob))
		{
			NoNotify = ((MCInetTransportJob*)(FJob))->getOwner()->getNoUDPNotify();
			if (NoNotify)
			{
				if (MessageFlag)
					((MCInetTransportJob*)(FJob))->FNotifyMsg = true;
				else
					((MCInetTransportJob*)(FJob))->FNotifyStop = true;
			}
		}
		else
		if (typeid(*FJob) == typeid(MCInetListenerJob))
		{
			NoNotify = ((MCInetListenerJob*)(FJob))->getOwner()->getNoUDPNotify();
			if (NoNotify)
			{
				if (MessageFlag)
					((MCInetListenerJob*)(FJob))->FNotifyMsg = true;
				else
					((MCInetListenerJob*)(FJob))->FNotifyStop = true;
			}
		}
		else
#endif
			NoNotify = false;

		if (!NoNotify)
		{
			char c = (char) MessageFlag;
			mcInt32 i = 0;
			MCSocketJob* SocketJob = (MCSocketJob*)FJob;
			//if (NULL != SocketJob)
			{	
				//printf("Kicking socket...\n");
				char localHost[20] = "127.0.0.1";
				SocketJob->getSocket()->SendTo(&c, 1, i, localHost, SocketJob->getSocket()->getLocalPort());
			}
		}
	}
}

MCWorkerThread* MCInetTransport::CreateThread(void)
{
    return new MCSocketThread(&FThreadPool);
}

#ifdef USE_CPPEXCEPTIONS
void MCInetTransport::HandleError(const EMCError& Error)
{
    
}
#else
void MCInetTransport::HandleError(mcUInt32 ErrorCode)
{
}
#endif


void MCInetTransport::CancelMessage(MCList* Queue, MCMessageInfo* Info)
{
	MCInetConnectionEntry* Entry = NULL;
	char* SocketName = NULL;

	FCriticalSection->Enter();
	TRY_BLOCK 
    {
		SocketName = Info->DMessenger;
		Entry = FindEntry(SocketName, Info);
		if(NULL != Entry)
		{
			Entry->getCS()->Enter();
			if(Entry->getInfo() == Info)
            {
				//this message is being sent now
				Entry->setInfo(NULL);
                //delete Entry->getOutgoingStream(); Entry->setOutgoingStream(NULL);
                
				if (NULL != Entry->getProcessingJob())
                    Entry->getProcessingJob()->setIsSending(false);
				
				//else
				//	ShowDebugInfo("Job for the message being cancelled is not found!");

            }
            if (NULL != Entry->getProcessingJob())
			{
				Entry->getProcessingJob()->getCriticalSection()->Enter();
				Entry->getProcessingJob()->getOutgoingQueue()->Del(Info);
				Entry->getProcessingJob()->getCriticalSection()->Leave();
			}
			Entry->getCS()->Leave();
		}
		// remove from outgoing queue
		if (Queue != NULL)
		{
			Queue->Del(Info);
		}
		else
		{
			mcInt32 j;
			for(j = 0; j < FConnections->Length(); j++)
			{
				Entry = (MCInetConnectionEntry*)(*FConnections)[j];
				Entry->getCS()->Enter();
				TRY_BLOCK
				{
					MCInetTransportJob *job = Entry->getProcessingJob();
					if (NULL != job)
					{
						job->getCriticalSection()->Enter();
						TRY_BLOCK
						{
							mcInt32 idx = job->getOutgoingQueue()->Find(Info);
							if (idx != -1)
							{
								job->getOutgoingQueue()->Del(idx);
	#ifndef _WIN32_WCE
								break;
	#endif
							}
						}
						FINALLY (
							job->getCriticalSection()->Leave();
						)				
					}
				}
				FINALLY (
					Entry->getCS()->Leave();
				)
			}
			FOutgoingQueue->Del(Info);
		}
	} FINALLY (
		FCriticalSection->Leave();
	)
}

char* CreateSMessenger(mcInt32 ClientID, mcInt32 ConnID, const char* RemAddr, mcInt32 Port)
{
	char buf[128];
	//char idbuf[64];
	
	if (NULL != RemAddr)
        sprintf(buf, "%d:%d:%s:%d", ClientID, ConnID, RemAddr, Port);
    else
        sprintf(buf, "%d:%d", ClientID, ConnID);

    char* result = (char*)MCMemAlloc(strlen(buf)+1);
	strcpy(result, buf);
	return result;

}

MCSocket* MCInetTransport::CreateClientSocket(void)
{
	
	MCSocket* s;
	if (FSocketFactory != NULL)
		s = FSocketFactory->CreateClientSocket();
	else
		s = FDefaultSocketFactory->CreateClientSocket();

	s->Init(istStream);
	return s;
}


MCSocket* MCInetTransport::CreateServerSocket(void)
{
	
	MCSocket* s;
	if (FSocketFactory != NULL)
		s = FSocketFactory->CreateServerSocket();
	else
		s = FDefaultSocketFactory->CreateServerSocket();

	return s;
}

MCInetListenerJob* MCInetTransport::CreateListenerJob(void)
{
    MCInetListenerJob* t = new MCInetListenerJob();
    t->setJobName("Listener");
	t->setOwner(this);
	t->setNoUDPNotify(FNoUDPNotify);
	t->Initialize(CreateServerSocket());
	return t;
}

const char* ExtractConnID(const char* SocketName, mcInt32* ClientID, mcInt32* ConnID)
{
	MCASSERT(NULL != SocketName);
	MCASSERT(NULL != ConnID);
	MCASSERT(NULL != ClientID);
	
	mcInt32 Port = -1; char* IP = NULL;
    MCInetTransportJob::ParseRemoteAddr(SocketName, ClientID, ConnID, &IP, &Port);
    MCMemFree(IP); IP = NULL;
    if (*ClientID != 0 || *ConnID != 0)
	{		
		char* p = strchr((char*) SocketName, ':');
		return p ? p + 1 : NULL;
	}	
	else
	{
		return SocketName;
	}
}

void MCInetTransport::PutMessageToOutgoing(const char* SocketName, MCMessageInfo* Info)
{
	//FLogger->PutMessage(llTrivial, "PutMessageToOutgoing is started.", 0, "MCInetTransport::PutMessageToOutgoing");
    MCInetConnectionEntry* Entry = NULL;
	//mcInt32 i = 0;
    //bool foundEntry = false;
	// place the message to the queue
    FCriticalSection->Enter();
	
	TRY_BLOCK
    {
		// find out whether we have a connection with this host
		// or we need to initiate one
		Entry = FindEntry((char*)SocketName, Info);
		if (NULL != Entry)
		{
			Entry->getCS()->Enter();
			Info->ConnID = Entry->getConnID();
			Info->ClientID = Entry->getClientID();
			
			Entry->getProcessingJob()->getCriticalSection()->Enter();
			TRY_BLOCK
			{
				InsertInfoIntoQueueWithPriorities(Entry->getProcessingJob()->getOutgoingQueue(), Info, false);
			}
			FINALLY (
				Entry->getProcessingJob()->getCriticalSection()->Leave();
			)
			NotifyJob(Entry->getProcessingJob(), true);
			Entry->getCS()->Leave();
		}
		else
		{
			//OutputDebugString("Client connection initiated for ");
			//OutputDebugString(SocketName);
			//OutputDebugString("\n");
			InsertInfoIntoQueueWithPriorities(FOutgoingQueue, Info, false);

			if (NeedLiveConnectionForDelivery() == false)
			{
				if (IsImmediateDelivery(SocketName) == true)
				{
					if (Info->State == imsDispatching)
						InitiateDelivery(NULL, NULL);
				}
			}
			else
				InitiateDelivery(NULL, NULL);
		}
	} FINALLY (
		FCriticalSection->Leave();
	)
}

bool MCInetTransport::IsImmediateDelivery(const char* RemAddr)
{
	char* p1 = strchr((char*) RemAddr, ':'); //is there any address component or just ID?
	if (p1 != RemAddr)
		return p1 != RemAddr + (strlen(RemAddr) - 1);
	else
		return false;
}

bool MCInetTransport::DeliverMessage(char* DestAddress, MCMessageInfo* Info)
{
    bool r = false;

	char *p = NULL, *p1 = NULL;
	char *SocketName = NULL, *QueueName = NULL, *URLName = NULL;

	if(DestAddress && AddressValid(DestAddress))
	{
	    Info->Transport = this;

		// extract names
		if(!strchr(DestAddress, '|'))
            THROW_ERROR(MCError_BadDestinationName);
		
        p = p1 = DestAddress;

        while((*p != '|') && (*p != '/') && *p)
			p++;
		if(!*p)
            THROW_ERROR(MCError_BadDestinationName);
            
        // get socket name
		if(p - p1 > 0)
			SocketName = strCopy(p1, 0, p - p1);
		else
            THROW_ERROR(MCError_BadDestinationName);
        //p++;
        
        //get URL if such available
        p1 = p;
        while ((*p != '|') && *p)
            p++;
        if (!*p)
            THROW_ERROR(MCError_BadDestinationName); // there is not queue name
        
        if (*p == '|')
            URLName = strCopy(p1, 0, p - p1);
        p++;

        // get queue name
		p1 = p;
		while(*p)
			p++;
		if(p - p1 > 0)
			QueueName = strCopy(p1, 0, p - p1);
		else
            THROW_ERROR(MCError_BadDestinationName);
        

		// set message info properties
		strncpy(Info->DQueue, QueueName, DQueueMaxLength - 1);
		Info->DQueue[DQueueMaxLength - 1] = 0;
		
		// update Info->ConnID from SocketName
		const char* realName = ExtractConnID(SocketName, &Info->ClientID, &Info->ConnID);
		if (NULL != realName)
		{
			mcInt32 len = strlen(realName);
			memmove(SocketName, realName, strlen(realName));
			SocketName[len] = 0;
		}

		Info->DMessenger = SocketName;
        Info->URL = URLName;

		Info->State = imsDispatching;
		MCMemFree(QueueName); QueueName = NULL;
		//FCriticalSection->Enter();
		//TRY_BLOCK
		//{
			PutMessageToOutgoing(SocketName, Info);
		//}
		//FINALLY(
		//	FCriticalSection->Leave();
		//)
		r = true;
	}
	else
		r = false;
	return r;
}

void MCInetTransport::DeliveryFailed(MCMessageInfo* Info)
{
/*	FCriticalSection->Enter();
	TRY_BLOCK
    {
		FOutgoingQueue->Del(Info);
	} 
    FINALLY (
		FCriticalSection->Leave();
	) */

  
	FLinkCriticalSection->Enter();
	TRY_BLOCK
	{
		if(FMessenger && (Info->State == imsDispatching || Info->State == imsWaiting) && Info->IsSendMsg
			
			)
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
			//MCMessenger::DestroyMessageInfo(Info);
		}
	}
	FINALLY (
		FLinkCriticalSection->Leave();			
	)
}

void MCInetTransport::CleanOutgoingQueue(void)
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

void MCInetTransport::CleanupMessages()
{
	mcInt32 i = 0;
    MCMessageInfo* Info = NULL;
    mcUInt32 TC = 0;
    bool me = false;

	FCriticalSection->Enter();
	TRY_BLOCK
    {
		TC = GetTickCount();
		
		i = 0;
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
				i++;
		}

		MCInetConnectionEntry* Entry = NULL;
		mcInt32 j;
		for(j = 0; j < FConnections->Length(); j++)
		{
			Entry = (MCInetConnectionEntry*)(*FConnections)[j];
			Entry->getCS()->Enter();
			TRY_BLOCK
            {
                if (NULL != Entry->getProcessingJob())
			    {
				    MCInetTransportJob *job = Entry->getProcessingJob();
				    if (job != NULL)
				    {
						job->getCriticalSection()->Enter();
						TRY_BLOCK
						{			
							i = 0;
							while (i < job->getOutgoingQueue()->Length()) 
							{    
								Info = (MCMessageInfo*)((*(job->getOutgoingQueue()))[i]);

								if (Info->Timeout == 0)
								{
									i++;
									continue;
								}

								if (TC < Info->StartTime)
									me = 0xFFFFFFFF - Info->StartTime + TC >= Info->Timeout;
								else
									me = TC - Info->StartTime >= Info->Timeout;
    							if (me)
	    						{
		    						CancelMessage(job->getOutgoingQueue(), Info);
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
							job->getCriticalSection()->Leave();
						)			
					}	
				}
			}
            FINALLY (
			    Entry->getCS()->Leave();
            )
		}
	} 
    FINALLY (
		FCriticalSection->Leave();
	)
}

MCMessageInfo* MCInetTransport::GetMessageByID(__int64 MsgID)
{
	mcInt32 i = 0;
    MCMessageInfo* Info = NULL;
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

		if (res == NULL)
		{
		
			MCInetConnectionEntry* Entry = NULL;
			mcInt32 j;
			for(j = 0; j < FConnections->Length(); j++)
			{
				Entry = (MCInetConnectionEntry*)(*FConnections)[j];
				Entry->getCS()->Enter();
				if (NULL != Entry->getProcessingJob())
				{
					MCInetTransportJob *job = Entry->getProcessingJob();
					if (job != NULL)
					{
						i = 0;
						job->getCriticalSection()->Enter();
						TRY_BLOCK
						{
							while (i < job->getOutgoingQueue()->Length()) 
							{    
								Info = (MCMessageInfo*)((*(job->getOutgoingQueue()))[i]);

								if (Info->Message.MsgID == MsgID)
								{
									res = Info;
									break;
								}
								else
									i++;
							}	
						}
						FINALLY
						(
							job->getCriticalSection()->Leave();
						)

					}
				}
				Entry->getCS()->Leave();
			}
		}
	} 
    FINALLY (
		FCriticalSection->Leave();
	)
	return res;
}

void MCInetTransport::DoSetActive(void)
{
	if(!FMessenger)
		return;

	if(getActive())
	{
		FThreadPool.setJobsRan(0);
        if((getTransportMode() == stmP2P) || (getTransportMode() == stmServer))
        {
			FListenerJob = CreateListenerJob();
            FThreadPool.PostJob(FListenerJob); //run listener
            while (FThreadPool.getJobsRan() == 0)
                Sleep(1);
        }
	}
	else
	{
		if (FListenerJob)
			FThreadPool.FinalizeJob(FListenerJob);
		
		FListenerJob = NULL;
        FThreadPool.Clear(); //stop all threads/jobs
		
		CleanOutgoingQueue();
	}
}

#ifdef MC_NO_PURE_VIRTUAL
MCInetConnectionEntry* MCInetTransport::CreateConnectionEntry(void)
{
	MC_pure_virtual();
	return NULL;
}

MCInetTransportJob* MCInetTransport::CreateTransportJob(void)
{
	MC_pure_virtual();
	return NULL;
}

void MCInetTransport::NotifyJob(MCInetTransportJob* Job, bool MessageFlag)
{
	MC_pure_virtual();
}

bool MCInetTransport::NeedLiveConnectionForDelivery(void)
{
	MC_pure_virtual();
	return false;
}

mcUInt32 MCInetTransport::getDeliveryInterval(void)
{
	MC_pure_virtual();
	return 0;
}
#endif

#ifndef _WIN32
#ifndef QNX
#	define stricmp strcasecmp
#endif
#endif

bool MCInetTransport::CompareAddress(const char* Addr1, const char* Addr2, bool Strict)
{
    mcInt32 ClientID1 = 0, ClientID2 = 0, ConnID1 = 0, ConnID2 = 0; mcInt32 Port1 = 0, Port2 = 0;
    char *IP1 = NULL, *IP2 = NULL;
    bool res = false;

    if ((NULL == Addr1) || (NULL == Addr2))
        return false;
    if ((0 == Addr1[0]) || (0 == Addr2[0]))
        return false;

    MCInetTransportJob::ParseRemoteAddr(Addr1, &ClientID1, &ConnID1, &IP1, &Port1);
    MCInetTransportJob::ParseRemoteAddr(Addr2, &ClientID2, &ConnID2, &IP2, &Port2);
    if (Strict == false)
        res = (Port1 == Port2) && (stricmp(IP1, IP2) == 0);
    else
        res = (ClientID1 == ClientID2) && (Port1 == Port2) && (stricmp(IP1, IP2) == 0);
    
    MCMemFree(IP1); MCMemFree(IP2);
    return res;
}

MCInetConnectionEntry* MCInetTransport::FindEntry(char* SocketName, MCMessageInfo* Info)
{
	mcInt32 i = 0;
	MCInetConnectionEntry* Entry = NULL;
	
	if (NULL != Info)
	{
		for (i = 0; i < FConnections->Length(); i++)
		{
			Entry = (MCInetConnectionEntry*)(*FConnections)[i];
			if (Entry)
			{
				Entry->getCS()->Enter();
				if (Entry->getConnID() == Info->ConnID)
				{		
					Entry->getCS()->Leave();
					return Entry;
				}
				else
					Entry->getCS()->Leave();
			}
		}
	}
	
	if (Info->ClientID == 0)
	{
		//connID is not found so seek by RemoteAddress
		for(i = 0;i < FConnections->Length();i++)
		{
			Entry = (MCInetConnectionEntry*)(*FConnections)[i];
			if (Entry != NULL)
			{
				Entry->getCS()->Enter();
				if (CompareAddress(Entry->getRemoteAddress(), SocketName, false)
					|| CompareAddress(Entry->getRemoteSAddress(), SocketName, false))
				{
					Entry->getCS()->Leave();
					return Entry;
				}
				Entry->getCS()->Leave();
			}
		}
	}
	else
	{
		for(i = 0;  i < FConnections->Length();i++)
		{
			Entry = (MCInetConnectionEntry*)(*FConnections)[i];
			if (Entry != NULL)
			{
				Entry->getCS()->Enter();

				if (Entry->getProcessingJob()->getTransporter()->getDirection() == isdIncoming)
				{
					if (Info->ClientID == Entry->getClientID())
					{
						Entry->getCS()->Leave();
						return Entry;
					}
				}
				else
				{

					if (Info->ClientID == Entry->getClientID() &&
						(CompareAddress(Entry->getRemoteAddress(), SocketName, false) ||
						 CompareAddress(Entry->getRemoteSAddress(), SocketName, false)))
					{
						Entry->getCS()->Leave();
						return Entry;
					}
				}

				
				Entry->getCS()->Leave();
			}
		}
	}
	return NULL;
}

bool MCInetTransport::InitiateDelivery(MCInetTransportJob* Job,
	MCInetConnectionEntry* Entry)
{
	bool r = false;
	MCSocket* Socket = NULL;
	MCMessageInfo* Info = NULL;
	//mcInt32 i = 0;
//    MCInetConnectionEntry* NewEntry = NULL;
    //lock transport
    //FCriticalSection->Enter();

    bool IsMessage = FOutgoingQueue->Length() > 0;
    bool IsClientMode = (getTransportMode() == stmP2P) || (getTransportMode() == stmClient);
    bool ShouldCreateJob = true;//(FThreadPool.getBusyCount() < FThreadPool.getMaxSize()) || (FThreadPool.getMaxSize() == 0);
    if (IsMessage && IsClientMode && ShouldCreateJob) //can we try to establish client connection?
	{
		//TRY_BLOCK
        // {
            //FLogger->PutMessage(llTrivial, "New transport job is creating.", 0, "MCInetTransport::InitiateDelivery");
            MCInetConnectionEntry* NewEntry = this->CreateConnectionEntry();
            if (NULL != FCompressor)
                NewEntry->setCompressID(FCompressor->GetID());
            if (NULL != FEncryptor)
                NewEntry->setEncryptID(FEncryptor->GetID());
            if (NULL != FSealer)
                NewEntry->setSealID(FSealer->GetID());
                
            MCInetTransportJob* NewJob = this->CreateTransportJob();
            NewJob->setJobName("Client worker job");
			
            Info = (MCMessageInfo*)(*FOutgoingQueue)[0];
            char* S = Info->DMessenger;
            //move existing Info's to job's queue
//            mcInt32 removedItems = 0;
            mcInt32 i = 0;
            while (i < FOutgoingQueue->Length())
            {
                MCMessageInfo* CandidateInfo = (MCMessageInfo*)(*FOutgoingQueue)[i];
                if (true == CompareAddress(S, CandidateInfo->DMessenger, false))
                {
                    CandidateInfo->ConnID = NewEntry->getConnID();
					CandidateInfo->ClientID = NewEntry->getClientID();
					NewJob->getOutgoingQueue()->Add(CandidateInfo);
                    FOutgoingQueue->Del(CandidateInfo);
                }
                else
                    i++;
            }

            Socket = CreateClientSocket(); //create new endpoint
            NewJob->setOwner(this);
			NewJob->setNoUDPNotify(FNoUDPNotify);
            NewJob->Initialize2(Socket, NewEntry);
            NewEntry->setProcessingJob(NewJob);
			//NewEntry->setSocket(Socket);
			MCASSERT(Info->DMessenger);
            NewEntry->setRemoteAddress(Info->DMessenger);
            
            NewEntry->setRemoteSAddress(Info->DMessenger);

            FConnections->Add(NewEntry);
            FThreadPool.PostJob(NewJob); //run the job
            r = true;
        //}
		//FINALLY (
		//	FCriticalSection->Leave();
		//)
	}
    //else
    //    FCriticalSection->Leave();
	return r;
}

void MCInetTransport::KickEntrySocket(MCInetConnectionEntry* Entry, bool MessageFlag)
{
	char c = (char) MessageFlag;
	mcInt32 i = 0;

	char localHost[20] = "127.0.0.1";
	Entry->getProcessingJob()->FKickSocket->SendTo(&c, 1, i, localHost,
		Entry->getProcessingJob()->getSocket()->getLocalPort());
}

void MCInetTransport::MessageProcessed(MCMessageInfo* Info)
{
	//FLogger->PutMessage(llTrivial, "MessageProcessed is started.\n", 0, "MCInetTransport::MessageProcessed");
    char* SocketName;
	
	if (!NeedLiveConnectionForDelivery())
    {
        //modify DMessenger/SMessenger
        mcInt32 ClientID = 0; mcInt32 ConnID = 0; mcInt32 Port = -1; char* IP = NULL;
        MCInetTransportJob::ParseRemoteAddr(Info->DMessenger, &ClientID, &ConnID, &IP, &Port);
        MCMemFree(IP); IP = NULL;
        SocketName = CreateSMessenger(ClientID, ConnID, NULL, -1);
		//char* SMessenger = CreateSMessenger(ClientID, NULL, -1);
        //Info->setDMessenger(SMessenger);
        //Info->setSMessenger(SMessenger);
		//MCMemFree(SMessenger); SMessenger = NULL;
    }
	else
		SocketName = _strdup(Info->DMessenger);
        
	/* Moved to Messenger
	Info->DQueue[0] = 0;
	if(Info->Message.DataType == bdtConst)
	{
		if((Info->Message.DataSize != 0) && Info->Message.Data)
			MCMemFree(Info->Message.Data);
		Info->Message.Data = NULL;
		Info->Message.DataSize = 0;
	}
    */
	this->PutMessageToOutgoing(SocketName, Info);
	MCMemFree(SocketName);
}



void MCInetTransport::MessageReceived(MCMessageInfo* Info)
{
	// Add to incoming queue
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
                    delete Info;
                    Info = NULL;
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
				} FINALLY (
					FMessenger->FIncomingCriticalSection->Leave();
				)
			}
		}
		else
        {
            delete Info; 
            Info = NULL;
        }
	} FINALLY (
		FLinkCriticalSection->Leave();
	)
}

void MCInetTransport::ReplyReceived(MCMessageInfo* Info)
{
	// Add to complete queue
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
			} FINALLY (
				FMessenger->FCompleteCriticalSection->Leave();
			)
		}
		else
        {
            delete Info;
            Info = NULL;
        }
	} FINALLY (
		FLinkCriticalSection->Leave();
	)
}



mcUInt32 MCInetTransport::getThreadPoolSize(void)
{
	return FThreadPool.getSize() - 1;
}

mcUInt32 MCInetTransport::getAttemptsInterval(void)
{
	return FAttemptsInterval;
}

void MCInetTransport::setAttemptsInterval(mcUInt32 Value)
{
	FAttemptsInterval = Value;
}

mcUInt32 MCInetTransport::getAttemptsToConnect(void)
{
	return FAttemptsToConnect;
}

void MCInetTransport::setAttemptsToConnect(mcUInt32 Value)
{
	FAttemptsToConnect = Value;
}

mcUInt32 MCInetTransport::getClientThreadLimit(void)
{
	return FThreadPool.getMaxSize();
}

void MCInetTransport::setClientThreadLimit(mcUInt32 Value)
{
	FThreadPool.setMaxSize(Value);
}

mcUInt32 MCInetTransport::getConnectionTimeout(void)
{
	return FConnectionTimeout;
}

void MCInetTransport::setConnectionTimeout(mcUInt32 Value)
{
	FConnectionTimeout = Value;
}

void MCInetTransport::setFailOnInactive(bool Value)
{
	if(FFailOnInactive != Value)
		FFailOnInactive = Value;
}

bool MCInetTransport::getFailOnInactive(void)
{
	return FFailOnInactive;
}

void MCInetTransport::setInactivityTime(mcUInt32 Value)
{
	FInactivityTime = Value;
}

mcUInt32 MCInetTransport::getInactivityTime(void)
{
	return FInactivityTime;
}

void MCInetTransport::setIncomingMemoryThreshold(mcUInt32 Value)
{
	if(FIncomingMemoryThreshold != Value)
	{
		if(getActive())
			FCriticalSection->Enter();
		FIncomingMemoryThreshold = Value;
		if(getActive())
			FCriticalSection->Leave();
	}
}

mcUInt32 MCInetTransport::getIncomingMemoryThreshold(void)
{
	return FIncomingMemoryThreshold;
}

void MCInetTransport::setMessengerAddress(char* Value)
{
	bool b = false;
	mcUInt32 i = 0;

	if(!FMessengerAddress && !Value)
		return;
	if(!FMessengerAddress || !Value || (strcmp(FMessengerAddress, Value) != 0))
	{
		if(FMessengerAddress)
			MCMemFree(FMessengerAddress);
		FMessengerAddress = NULL;
		b = getActive();
		setActive(false);
		if(Value && strcmp(Value, "255.255.255.255") != 0)
		{
			i = inet_addr(Value);
			if(i == INADDR_NONE)
                THROW_ERROR(MCError_InvalidAddress);
		}
		FMessengerAddress = _strdup(Value);
		setActive(b);
	}
}

char* MCInetTransport::getMessengerAddress(void)
{
	return FMessengerAddress;
}

void MCInetTransport::setMessengerPort(mcUInt32 Value)
{
	bool b;

	if(FMessengerPort != Value)
	{
		b = getActive();
		setActive(false);
		FMessengerPort = Value;
		setActive(b);
	}
}

mcUInt32 MCInetTransport::getMessengerPortBound(void)
{
	if (getActive() == false || FListenerJob == NULL)
		return 0;
	else
		return FListenerJob->getListener()->getLocalPort();
}

mcUInt32 MCInetTransport::getMessengerPort(void)
{
	return FMessengerPort;
}

#ifndef _WIN32_WCE
void MCInetTransport::setNoUDPNotify(bool Value)
{
	FNoUDPNotify = Value;
}

bool MCInetTransport::getNoUDPNotify(void)
{
	return FNoUDPNotify;
}
#endif

void MCInetTransport::setOutgoingMemoryThreshold(mcUInt32 Value)
{
	if(FOutgoingMemoryThreshold != Value)
	{
		if(getActive())
			FCriticalSection->Enter();
		FOutgoingMemoryThreshold = Value;
		if(getActive())
			FCriticalSection->Leave();
	}
}

mcUInt32 MCInetTransport::getOutgoingMemoryThreshold(void)
{
	return FOutgoingMemoryThreshold;
}

void MCInetTransport::setTempFilesFolder(char* Value)
{
	if(FTempFilesFolder)
		MCMemFree(FTempFilesFolder);
	if(Value)
		FTempFilesFolder = _strdup(Value);
	else
		FTempFilesFolder = Value;
}

char* MCInetTransport::getTempFilesFolder(void)
{
	return FTempFilesFolder;
}

void MCInetTransport::setTransportMode(MCInetTransportMode Value)
{
	FTransportMode = Value;
	if(getActive())
	{
		setActive(false);
		FTransportMode = Value;
		setActive(true);
	}
	else
		FTransportMode = Value;
}

MCInetTransportMode MCInetTransport::getTransportMode(void)
{
	return FTransportMode;
}

void MCInetTransport::setUseTempFilesForIncoming(bool Value)
{
	if(FUseTempFilesForIncoming != Value)
	{
		if(getActive())
			FCriticalSection->Enter();
		FUseTempFilesForIncoming = Value;
		if(getActive())
			FCriticalSection->Leave();
	}
}

bool MCInetTransport::getUseTempFilesForIncoming(void)
{
	return FUseTempFilesForIncoming;
}

void MCInetTransport::setUseTempFilesForOutgoing(bool Value)
{
	if(FUseTempFilesForOutgoing != Value)
	{
		if(getActive())
			FCriticalSection->Enter();
		FUseTempFilesForOutgoing = Value;
		if(getActive())
			FCriticalSection->Leave();
	}
}

bool MCInetTransport::getUseTempFilesForOutgoing(void)
{
	return FUseTempFilesForOutgoing;
}

void MCInetTransport::setThreadPoolSize(mcUInt32 Value)
{
    FThreadPool.setSize(Value+1);
}


void MCInetTransport::setTransformerFailingHandler(MCTransformerFailProc Handler)
{
    this->FTransformerFailProc = Handler;
}

MCTransformerFailProc MCInetTransport::getTransformerFailingHandler(void)
{
    return this->FTransformerFailProc;
}

bool MCInetTransport::getNoTransformerFallback()
{
	return FNoTransformerFallback;
}

void MCInetTransport::setNoTransformerFallback(bool Value)
{
	FNoTransformerFallback = Value;
}

void MCInetTransport::setIncomingBufferSize(mcUInt32 Value)
{
	if (Value != FIncomingBufferSize && Value >= 1024)
	{
		FIncomingBufferSize = Value;
	}
}

mcUInt32 MCInetTransport::getIncomingBufferSize()
{
	return FIncomingBufferSize;
}

void MCInetTransport::setOutgoingBufferSize(mcUInt32 Value)
{
	if (Value != FOutgoingBufferSize && Value >= 1024)
	{
		FOutgoingBufferSize = Value;
	}
}

mcUInt32 MCInetTransport::getOutgoingBufferSize()
{
	return FOutgoingBufferSize;
}



void MCInetTransport::setIncomingSpeedLimit(mcUInt32 Value)
{
	if (Value != FIncomingSpeedLimit)
	{
		FIncomingSpeedLimit = Value;
	}
}

mcUInt32 MCInetTransport::getIncomingSpeedLimit()
{
	return FIncomingSpeedLimit;
}

void MCInetTransport::setOutgoingSpeedLimit(mcUInt32 Value)
{
	if (Value != FOutgoingSpeedLimit)
	{
		FOutgoingSpeedLimit = Value;
	}
}

mcUInt32 MCInetTransport::getOutgoingSpeedLimit()
{
	return FOutgoingSpeedLimit;
}

int	MCInetTransport::GetIncomingConnectionCount()
{
	mcInt32 res = 0;
	MCInetConnectionEntry* Entry = NULL;
	MCInetTransportJob *job = NULL;

	FCriticalSection->Enter();
	TRY_BLOCK
	{
		mcInt32 i = 0;

		for (i = 0; i < FConnections->Length(); i++)
		{
		
			Entry = (MCInetConnectionEntry*)(*FConnections)[i];
			Entry->getCS()->Enter();
			TRY_BLOCK
			{
				if (NULL != Entry->getProcessingJob())
				{
					job = Entry->getProcessingJob();
					if (job != NULL && job->getSocket()->getDirection() == isdIncoming)
						res ++;
				}
			}
			FINALLY (
				Entry->getCS()->Leave();
			)
		}
	}
	FINALLY
	(
		FCriticalSection->Leave();
	)
	return res;
}

int	MCInetTransport::GetOutgoingConnectionCount()
{
	mcInt32 res = 0;
	MCInetConnectionEntry* Entry = NULL;
	MCInetTransportJob *job = NULL;

	FCriticalSection->Enter();
	TRY_BLOCK
	{
		mcInt32 i = 0;

		for (i = 0; i < FConnections->Length(); i++)
		{
		
			Entry = (MCInetConnectionEntry*)(*FConnections)[i];
			Entry->getCS()->Enter();
			TRY_BLOCK
			{
				if (NULL != Entry->getProcessingJob())
				{
					job = Entry->getProcessingJob();
					if (job != NULL && job->getSocket()->getDirection() == isdOutgoing)
						res ++;
				}
			}
			FINALLY (
				Entry->getCS()->Leave();
			)
		}
	}
	FINALLY
	(
		FCriticalSection->Leave();
	)
	return res;
}

void MCInetTransport::GetMessageSource(MCMessageInfo* Info, char* Buffer, mcInt32* BufferSize)
{

	//return MCBaseTransport::GetMessageSource(Info, Buffer, BufferSize);

	char* SMessenger;

	MCInetConnectionEntry *entry = FindEntry(Info->DMessenger, Info);
	if (entry != NULL)
	{
		SMessenger = entry->getRemoteAddress();
	}
	else
	{
		SMessenger = Info->SMessenger;
	}

    bool EmptySource = false;
    EmptySource = (NULL == FName) && (NULL == FDefaultTransportName);
    if (false == EmptySource)
        EmptySource = FName ? (FName[0] == 0) : (FDefaultTransportName[0] == 0);

    if (true == EmptySource)
    {
        if (NULL != Buffer && NULL != BufferSize && *BufferSize > 0)
            Buffer[0] = 0;
        if (NULL != BufferSize)
            *BufferSize = 0;
    }
    else
    {
		if (NULL != BufferSize)
        {
			mcInt32 newsize; 
            if (NULL != FName)
                newsize = strlen(FName) + 1 + strlen(SMessenger);
            else
                newsize = strlen(FDefaultTransportName) + 1 + strlen(SMessenger);
			
			if (*BufferSize < newsize)
			{
				if (*BufferSize > 0)
					Buffer[0] = 0;
				*BufferSize = newsize;
				return;
			}
			*BufferSize = newsize;
        }
        if (NULL != Buffer)
        {
            strcpy(Buffer, (NULL != FName) ? FName : FDefaultTransportName);
            strcat(Buffer, ":"); strcat(Buffer, SMessenger);
        }
    }
            

}



void MCInetTransport::setReuseServerPort(bool Value)
{
	FReuseServerPort = Value;
}

bool MCInetTransport::getReuseServerPort(void)
{
	return FReuseServerPort;
}

void MCInetTransport::setBandwidthPolicy(MCBandwidthPolicy Value)
{
	FBandwidthPolicy = Value;
}

MCBandwidthPolicy MCInetTransport::getBandwidthPolicy()
{
	return FBandwidthPolicy;
}


//  Web tunneling support
void MCInetTransport::setUseWebTunneling(bool Value)
{
    FUseWebTunneling = Value;
}

bool MCInetTransport::getUseWebTunneling()
{
    return FUseWebTunneling;
}

void MCInetTransport::setWebTunnelAddress(const char *Value)
{
    if (FWebTunnelAddress)
        free(FWebTunnelAddress);
    FWebTunnelAddress = _strdup(Value);
}

const char* MCInetTransport::getWebTunnelAddress()
{
    return FWebTunnelAddress;
}

void MCInetTransport::setWebTunnelPort(unsigned short Value)
{
    FWebTunnelPort = Value;
}

unsigned short MCInetTransport::getWebTunnelPort()
{
    return FWebTunnelPort;
}

MCWebTunnelAuthentication MCInetTransport::getWebTunnelAuthentication()
{
    return FWebTunnelAuthentication;
}

void MCInetTransport::setWebTunnelAuthentication(MCWebTunnelAuthentication Value)
{
    FWebTunnelAuthentication = Value;
}

void MCInetTransport::setWebTunnelUserId(const char *Value)
{
    if (FWebTunnelUserId)
        free(FWebTunnelUserId);
    FWebTunnelUserId = _strdup(Value);
}

const char* MCInetTransport::getWebTunnelUserId()
{
    return FWebTunnelUserId;
}

void MCInetTransport::setWebTunnelPassword(const char *Value)
{
    if (FWebTunnelPassword)
        free(FWebTunnelPassword);
    FWebTunnelPassword = _strdup(Value);
}

const char* MCInetTransport::getWebTunnelPassword()
{
    return FWebTunnelPassword;
}

void MCInetTransport::setSocketFactory(MCSocketFactory* Value)
{
	FSocketFactory = Value;
}

MCSocketFactory* MCInetTransport::getSocketFactory()
{
	return FSocketFactory;
}

MCSocketConnectedEvent MCInetTransport::getOnConnected(void* *UserData)
{
	if (UserData)
		*UserData = FOnConnectedData;
	return FOnConnected;
}

void MCInetTransport::setOnConnected(MCSocketConnectedEvent Value, void* UserData)
{
	FOnConnected = Value;
	FOnConnectedData = UserData;
}

MCSocketDisconnectedEvent MCInetTransport::getOnDisconnected(void* *UserData)
{
	if (UserData)
		*UserData = FOnDisconnectedData;
	return FOnDisconnected;
}

void MCInetTransport::setOnDisconnected(MCSocketDisconnectedEvent Value, void* UserData)
{
	FOnDisconnected = Value;
	FOnDisconnectedData = UserData;
}

mcInt32 MCInetTransport::GetOutgoingMessagesCount()
{
	mcInt32 res = 0;
	MCInetConnectionEntry* Entry = NULL;
	MCInetTransportJob *job = NULL;
	FCriticalSection->Enter();
	TRY_BLOCK
	{
		res += FOutgoingQueue->Length();
		mcInt32 i = 0;
		for (i = 0; i < FConnections->Length(); i++)
		{
			Entry = (MCInetConnectionEntry*)(*FConnections)[i];
			Entry->getCS()->Enter();
			TRY_BLOCK
            {
                if (NULL != Entry->getProcessingJob())
			    {
				    job = Entry->getProcessingJob();
				    if (job != NULL)
				        res += job->getOutgoingQueue()->Length();
				}
			}
            FINALLY (
			    Entry->getCS()->Leave();
            )
		}
	}
	FINALLY
	(
		FCriticalSection->Leave();
	)
	return res;
}

void MCInetTransport::DebugBreakConnections(void)
{
    FCriticalSection->Enter();
    TRY_BLOCK
    {
        for (mcInt32 i=0; i<FConnections->Length(); i++)
        {
            MCInetConnectionEntry* entry = (MCInetConnectionEntry*)FConnections->At(i);
            entry->getCS()->Enter();
            MCInetTransportJob* job = entry->getProcessingJob();
            //closesocket(job->getTransporter()->getSocket());
			job->getTransporter()->Close(true);
            entry->getCS()->Leave();
        }
    }
    CATCH_EVERY
    {
    }
    FCriticalSection->Leave();
}

// ************************** MCInetTransportJob *************************

MCInetTransportJob::MCInetTransportJob(void)
{
	FEntry = NULL;
	FHandshakeWaitingReply = false;
	FTransporter = NULL;
	FInitialized = false;
	FIsReceiving = false;
	FIsSending = false;

	FIncomingSpeedLimit = 0;
    FOutgoingSpeedLimit = 0;

    FblInSessionStartTime = 0;
    FblOutSessionStartTime = 0;

    FblInMsgStartTime = 0;
    FblOutMsgStartTime = 0;

    FblInSessionTransferred = 0;
    FblOutSessionTransferred = 0;

    FblInMsgTransferred = 0;
    FblOutMsgTransferred = 0;

	FblInSecTransferred = 0;
    FblOutSecTransferred = 0;

	FblInNextTime = 0;
    FblOutNextTime = 0;

    FblToRecv = 0;
	FblToSend = 0;

	FWasConnected = false;

	FIncomingBuffer = NULL;
	FOutgoingBuffer = NULL;
	
	FIncomingBufferSize = 0;
	FOutgoingBufferSize = 0;

	AdjustBufferSize(8192, 8192);

	FServerRequestTime = 0;

	FOwner = NULL;
	FCriticalSection = new MCCriticalSection();
}

MCInetTransportJob::~MCInetTransportJob(void)
{
	delete FCriticalSection;
	delete FTransporter;
	FTransporter = NULL;
	if (FIncomingBuffer)
		MCMemFree(FIncomingBuffer);
	FIncomingBuffer = NULL;

	if (FOutgoingBuffer)
		MCMemFree(FOutgoingBuffer);
	FOutgoingBuffer = NULL;
}



void MCInetTransportJob::WaitForSignal(mcUInt32 ms)
{
    fd_set FDRecvSet;
    FD_ZERO(&FDRecvSet);
    FD_SET(FSocket->getSocket(), &FDRecvSet);
    timeval tv;
    tv.tv_sec = ms / 1000;
    tv.tv_usec = (ms % 1000) * 1000;
    //printf("Waiting for signal...\n");
    mcInt32 resCode = select(FSocket->getSocket(), &FDRecvSet, NULL, NULL, &tv);
    
    if (resCode > 0)
    {
        char c; 
		sockaddr_in fromAddr; 
		int fromAddrLen = sizeof(fromAddr);
    #ifdef _WIN32
	#else
		recvfrom(FSocket->getSocket(), &c, 1, 0, (sockaddr*)&fromAddr, (socklen_t*)&fromAddrLen);
	#endif
	
    }
    //printf("Waiting for signal finished\n");
}

bool MCInetTransportJob::ClientConnect(void)
{

	bool result = false;

    //ok, trying to connect!
    
    //set transporter address (maybe proxy in HTTP transport)
    SetTransporterAddress(FEntry, FTransporter);

    //wait while connect
    TRY_BLOCK
    {
		//  Web tunneling support
        FTransporter->setUseWebTunneling(FOwner->getUseWebTunneling());
		if (FOwner->getUseWebTunneling())
		{
			FTransporter->setWebTunnelAddress(FOwner->getWebTunnelAddress());
			FTransporter->setWebTunnelPort(FOwner->getWebTunnelPort());
			FTransporter->setWebTunnelAuthentication(FOwner->getWebTunnelAuthentication());
			FTransporter->setWebTunnelUserId(FOwner->getWebTunnelUserId());
			FTransporter->setWebTunnelPassword(FOwner->getWebTunnelPassword());
		}

		mcInt32 timeout = FOwner->getConnectionTimeout();
		if (timeout == 0)
			timeout = 30000;

		FTransporter->ExtConnect(FSocket, timeout);

		if(FTransporter->getState() == issConnected)
		{
			if (FTransporter->AfterConnection(FSocket, FOwner->getConnectionTimeout()) != 0)
				FTransporter->Close(true);
		}
		if (FTransporter->getState() == issConnected)
		{ //yes, we are connected
			if (!InitializeConnection())
				FTransporter->Close(true);
			else
				result = true;
		}
    
    }
    CATCH_EVERY
    {//cannot connect due to bad host name etc. Just ignore it for now
    }
	return result;
}

bool MCInetTransportJob::SetMessageForSending(void)
{
    bool bRet = false;
    FEntry->getCS()->Enter();
    TRY_BLOCK
    {

        if (FEntry->getInfo() == NULL)
        {
            MCMessageInfo *Info = GetMessageForDelivery();
			FEntry->setInfo(Info);
            if (NULL != Info)
            {
                Info->OldState = Info->State;
				//FEntry->setInfoState(FEntry->getInfo()->State);
                Info->State =  imsWaiting; //mark message as 'waiting' delivery
            }
        }
    
        bRet = FEntry->getInfo() != NULL;
    }
    FINALLY (
        FEntry->getCS()->Leave();
    )
    
    return bRet;
}

bool MCInetTransportJob::AreRequestsInQueue(void)
{
	FCriticalSection->Enter();
	bool res = false;
	TRY_BLOCK
	{
		for (mcInt32 i=0; i<FOutgoingQueue.Length(); i++)
		{
			MCMessageInfo* info = (MCMessageInfo*)FOutgoingQueue[i];
			if (false == info->Sent)
			{
				res = true;
				break;
			}
		}
	}
	FINALLY 
	(
		FCriticalSection->Leave();
	)
    return res;
}

bool MCInetTransportJob::HandleIncomingData(void)
{

	if ((FEntry->FIncomingStream == NULL) || (FEntry->FIncomingStream->Pos() == 0))
	{
		FblInMsgStartTime = GetTickCount();
		FblInMsgTransferred = 0;
	}

    if (ReceiveData())
	{
        MessageReceivedCompletely2();
        return true;
    }
    else
        return false;
}

bool MCInetTransportJob::HandleOutgoingData()
{
   
	if ((FEntry->FOutgoingStream == NULL) || (FEntry->FOutgoingStream->Pos() == 0)) 
	{
		FblOutMsgStartTime = GetTickCount();
		FblOutMsgTransferred = 0;
	}

	if (SendData())
	{
        MessageSentCompletely();
        return true;
    } 
    else
        return false;
}

/*
void checkForTimeOut(mcInt32 select_res, bool failOnInactive, bool& timeout, bool& errorFlag) 
{
    if (0 == select_res)
    {//timeout!
        if (failOnInactive)
            timeout = true;
        else
            errorFlag = true;
    }
    else
    {
        errorFlag = true;
    }
}
*/

bool MCInetTransportJob::PerformRecvSend(bool& errorFlag, bool& closeConnection, bool& timeout)
{
    fd_set FDSendSet, FDRecvSet;
    timeval TimeVal, *PTV;

	mcInt32 TimeoutV;
	mcUInt32 InactTime = 0;

	bool IsBWLimit;
    bool SendAllowed, RecvAllowed;

	mcUInt32 CurTicks;
    mcUInt32 x1;

	CurTicks = GetTickCount();
	FblInMsgStartTime = CurTicks;
	FblOutMsgStartTime = CurTicks;
	
    while ((false == errorFlag) &&
           (false == closeConnection) &&
           (false == timeout) && 
           (false == getTerminated()))
    {
        TRY_BLOCK
        {
			//ShowDebugInfo("ThreadID=%d, ConnID=%d, ClientID=%d, Dir=%d, Timestamp=%d, \"IsSending is %d.\";\n", 
			//	GetCurrentThreadId(), FEntry->getConnID(), FEntry->getClientID(), FTransporter->getDirection() == isdIncoming, 
			//	GetTickCount(), (mcInt32)getIsSending());

			if (false == getIsSending())
            {//nothing sending now
                if (SetMessageForSending())
				{
                    PrepareMessageForSending();
				}
            }
            
			if (IsRequestNeeded())
			{
				FCriticalSection->Enter();
				FOutgoingQueue.Insert(0, MakeEmptyPacket(imsEmptyRequest));
				FCriticalSection->Leave();
				continue;
			}
						
            UpdateConnectionContext();
                
			/*
            if (((MCStdSocket*)FTransporter)->IsCachedData())
            {
                if (!HandleIncomingData())
                    errorFlag = true;
                continue;
            }
			*/

			// adjust speed limitations and define amount of data to transfer

			AdjustSpeedLimits(FOwner->FIncomingSpeedLimit, FOwner->FOutgoingSpeedLimit);
			
			IsBWLimit = (FIncomingSpeedLimit > 0) || (FOutgoingSpeedLimit > 0);

			CurTicks = GetTickCount() + 1000;
			
			RecvAllowed = true;
			FblToRecv = FIncomingBufferSize;
			if (FIncomingSpeedLimit > 0)
			{

				if (FOwner->getBandwidthPolicy() == bpStrict)
				{
					if (CurTicks > FblInNextTime)
					{
						FblInNextTime = (CurTicks / 1000 + 1) * 1000;
						FblInSecTransferred = 0;
					}
					if (FblInSecTransferred < FIncomingSpeedLimit) 
					{
						FblToRecv = FIncomingSpeedLimit;

						if (FIncomingBufferSize < FblToRecv)
							FblToRecv = FIncomingBufferSize;

						if (FblToRecv == 0)
						  RecvAllowed = false;
					}
					else
						RecvAllowed = false;
				}
				else
				{
					if ((FblInSessionTransferred > ((__int64)(CurTicks - FblInSessionStartTime)) * ((__int64)FIncomingSpeedLimit) / 1000) ||
						(FblInMsgTransferred > ((__int64)(CurTicks - FblInMsgStartTime)) * ((__int64)FIncomingSpeedLimit) / 1000))
					{
						RecvAllowed = false;
					}
					else
					{
						FblToRecv = (mcUInt32)(((__int64)CurTicks - (__int64)FblInSessionStartTime) * (__int64)FIncomingSpeedLimit / 1000 - FblInSessionTransferred);
						x1 = (mcUInt32)(((__int64)CurTicks - (__int64)FblInMsgStartTime) * (__int64)FIncomingSpeedLimit / 1000 - FblInMsgTransferred);

						if (x1 < FblToRecv)
							FblToRecv = x1;

						if (FIncomingBufferSize < FblToRecv)
							FblToRecv = FIncomingBufferSize;
						if (FblToRecv > FIncomingSpeedLimit)
							FblToRecv = FIncomingSpeedLimit;

						if (FblToRecv == 0)
							RecvAllowed = false;
					}
				}
			}

			SendAllowed = true;
			FblToSend = FOutgoingBufferSize;
			
			if (getIsSending() && (FOutgoingSpeedLimit > 0))
			{
				if (FOwner->getBandwidthPolicy() == bpStrict)
				{
					if (CurTicks > FblOutNextTime)
					{
						FblOutNextTime = (CurTicks / 1000 + 1) * 1000;
						FblOutSecTransferred = 0;
					}
					if (FblOutSecTransferred < FOutgoingSpeedLimit) 
					{
						FblToSend = FOutgoingSpeedLimit;

						if (FOutgoingBufferSize < FblToSend)
							FblToSend = FOutgoingBufferSize;

						if (FblToSend == 0)
						  SendAllowed = false;
					}
					else
						SendAllowed = false;
				}
				else
				{
					if ((FblOutSessionTransferred > ((__int64)(CurTicks - FblOutSessionStartTime)) * ((__int64)FOutgoingSpeedLimit) / 1000) ||
						(FblOutMsgTransferred > ((__int64)(CurTicks - FblOutMsgStartTime)) * ((__int64)FOutgoingSpeedLimit) / 1000))
					{
						SendAllowed = false;
					}
					else
					{
						FblToSend = (mcUInt32)(((__int64)CurTicks - (__int64)FblOutSessionStartTime) * (__int64)FOutgoingSpeedLimit / 1000 - FblOutSessionTransferred);
						x1 = (mcUInt32)(((__int64)CurTicks - (__int64)FblOutMsgStartTime) * (__int64)FOutgoingSpeedLimit / 1000 - FblOutMsgTransferred);

						if (x1 < FblToSend)
							FblToSend = x1;

						if (FOutgoingBufferSize < FblToSend)
							FblToSend = FOutgoingBufferSize;
						if (FblToSend > FOutgoingSpeedLimit)
							FblToSend = FOutgoingSpeedLimit;

						if (FblToSend == 0)
							SendAllowed = false;
						
					}
				}
			}

			//prepare select's parameters for send/receive/signal
            // initialize sets for select
			FD_ZERO(&FDSendSet);
			FD_ZERO(&FDRecvSet);
			
			if (!FNoUDPNotify)
				FD_SET(FSocket->getSocket(), &FDRecvSet);
			
			mcUInt32 highSocketHandle;

			if (RecvAllowed)
			{
				FD_SET(FTransporter->getSocket(), &FDRecvSet);
            
				highSocketHandle = 
						(FSocket->getSocket() > FTransporter->getSocket() ? FSocket->getSocket() 
																		  : FTransporter->getSocket());
			}
			else
			{
				highSocketHandle = FSocket->getSocket();
			}


			// highSocketHandle++; // moved to select() call
			bool b = getIsSending();
                
			InactTime = FOwner->getInactivityTime();

			//determine select's timeout
			if (FNoUDPNotify)
				TimeoutV = 10;
			else
            if (IsBWLimit && ((InactTime == 0) || (InactTime > 1000)))
				TimeoutV = 1000;
			else
			if (FServerRequestTime != 0)
				TimeoutV = FServerRequestTime < 1000 ? FServerRequestTime : 1000;
			else
				TimeoutV = InactTime;	
			
			if(TimeoutV > 0)
			{
			    TimeVal.tv_sec = TimeoutV / 1000;
				TimeVal.tv_usec = (TimeoutV % 1000) * 1000;
				PTV = &TimeVal;
		    }
			else
			    PTV = NULL;
                
            mcInt32 select_res = 0;
            //well, do select
            // printf("Select starts...\n");
			
			if (RecvAllowed && FTransporter->HasBufferedIncomingData())
			{
				if (HandleIncomingData())
					ChangeLastRecvTime(FLastMsgRecvState);
				else
					errorFlag = true;
			}
			else
			{		
				if((b || FTransporter->HasBufferedOutgoingData())  && SendAllowed)
				{
					FD_SET(FTransporter->getSocket(), &FDSendSet);
					select_res = select(highSocketHandle + 1, &FDRecvSet, &FDSendSet, NULL, PTV);
				}
				else
				if (RecvAllowed || !FNoUDPNotify)
        			select_res = select(highSocketHandle + 1, &FDRecvSet, NULL, NULL, PTV);
				else
				{
					Sleep(TimeoutV);
					select_res = 0;
				}
				if (FNoUDPNotify)
				{
					if (FNotifyStop)
					{
						FNotifyStop = false;
						errorFlag = true;
						continue;
					}
				}
        
				//FOwner->FLogger->PutMessage(llTrivial, "Select finished\n", 0, "MCInetTransportJob::PerformRecvSend");
				if ((0 != select_res) && (-1 != select_res))
				{
					if(FD_ISSET(FSocket->getSocket(), &FDRecvSet))
					{//thread was kicked
						if(!getTerminated())
						{
							errorFlag = (MCSocket::PopFromCtlSocket(FSocket) == false);
							continue;
						}
					}
						
					if(FD_ISSET(FTransporter->getSocket(), &FDRecvSet))
					{
						if (FTransporter->PreprocessIncomingData())
						{
							if (HandleIncomingData())
								ChangeLastRecvTime(FLastMsgRecvState);
							else
							{
								errorFlag = true;
								continue;
							}
						}
					}
	                
					if (FD_ISSET(FTransporter->getSocket(), &FDSendSet))
					{
						if ((!getIsSending()) || HandleOutgoingData())
						{
							if (FTransporter->PostprocessOutgoingData() && getIsSending())
							{
								ChangeLastSendTime(FLastMsgSendState);
							}
						}
                        else
						{
							errorFlag = true;
							continue;
						}
						// continue;
					}
				}
				else
				if (select_res == 0)
				{
					if (IsBWLimit || (FServerRequestTime != 0) || FNoUDPNotify)
					{
						//ShowDebugInfo("ThreadID=%d, ConnID=%d, ClientID=%d, Dir=%d, Timestamp=%d, \"Bandwidth limit is found.\";\n", 
						//	GetCurrentThreadId(), FEntry->getConnID(), FEntry->getClientID(), FTransporter->getDirection() == isdIncoming, GetTickCount());

						if (FOwner->getInactivityTime() > 0)
							timeout = GetTickCount() - FActivityTime > FOwner->getInactivityTime();
						else
							timeout = false;
					}
					else
						timeout = true;
					/*
					if (timeout)
					{
						/
						if (!FOwner->getFailOnInactive())
						{
							//ShowDebugInfo("ThreadID=%d, ConnID=%d, ClientID=%d, Dir=%d, Timestamp=%d, \"Timeout occured. Connection will not be closed.\";\n", 
							//	GetCurrentThreadId(), FEntry->getConnID(), FEntry->getClientID(), FTransporter->getDirection() == isdIncoming, GetTickCount());
							
							timeout = false;
							errorFlag = true;
						}
						//else
							//ShowDebugInfo("ThreadID=%d, ConnID=%d, ClientID=%d, Dir=%d, Timestamp=%d, \"Timeout occured. Connection will be closed.\";\n", 
							//	GetCurrentThreadId(), FEntry->getConnID(), FEntry->getClientID(), FTransporter->getDirection() == isdIncoming, GetTickCount());
					}
					*/
				}
				else
				
				{
					//ShowDebugInfo("ThreadID=%d, ConnID=%d, ClientID=%d, Dir=%d, Timestamp=%d\"Select returned socket_error.\";\n", 
					//	GetCurrentThreadId(), FEntry->getConnID(), FEntry->getClientID(), FTransporter->getDirection() == isdIncoming, GetTickCount());
					errorFlag = true;
					/*
					mcInt32 err = LAST_ERROR;
					{
						char Buffer[100];
					
						if (FTransporter->getDirection() == isdOutgoing)
							sprintf(Buffer, "select() on outgoing socket (in PerformRecvSend) failed with error %d\n", err);
						else
							sprintf(Buffer, "select() on incoming socket (in PerformRecvSend) failed with error %d\n", err);
						//OutputDebugString(Buffer);
					}
					*/
				}
			}
        }
        CATCH_EVERY
        {
            errorFlag = true;
        }
    }
    //if (errorFlag)
	//	ShowDebugInfo("ThreadID=%d, ConnID=%d, ClientID=%d, Dir=%d, Timestamp=%d, \"Select returned socket_error.\";\n", 
	//		GetCurrentThreadId(), FEntry->getConnID(), FEntry->getClientID(), FTransporter->getDirection() == isdIncoming, GetTickCount());
    return errorFlag;
}

MCMessageInfo* MCInetTransportJob::MakeEmptyPacket(MCMessageState state)
{
    MCMessage msg;
    memset(&msg, 0, sizeof(msg));
    MCMessageInfo* dstInfo = new MCMessageInfo(&msg);
    
	//memset(dstInfo, 0, sizeof(*dstInfo));
	MCMessenger::SetOrigID(dstInfo);

    char* SMessenger = CreateSMessenger(FEntry->getClientID(), FEntry->getConnID(), FTransporter->getLocalAddress(), FOwner->getMessengerPort());
    dstInfo->setSMessenger(SMessenger);
    MCMemFree(SMessenger);
    dstInfo->setDMessenger(FEntry->getRemoteAddress());
    dstInfo->MCError = 0;
    dstInfo->State = state;
	dstInfo->StartTime = GetTickCount();
	dstInfo->Timeout = FOwner->FMsgTimeout;

    return dstInfo;
}

void MCInetTransportJob::FinalizeJob(void)
{
    //clean thread
	setIsSending(false);
    setIsReceiving(false);

    if (NULL != FEntry)
    {
        delete FEntry->getOutgoingStream(); FEntry->setOutgoingStream(NULL);
        delete FEntry->getIncomingStream(); FEntry->setIncomingStream(NULL);
        delete FEntry; 
        FEntry = NULL;
    }

    if (NULL != FTransporter)
    {
        delete FTransporter;
        FTransporter = NULL;
    }
}

bool MCInetTransportJob::FailCurrentMessage(mcInt32 ErrorCode)
{
	MCMessageInfo* Info = NULL;
	FEntry->getCS()->Enter();
	
	FCriticalSection->Enter();

	//check first packet
	if (FOutgoingQueue.Length() > 0)
	{
		Info = (MCMessageInfo*)FOutgoingQueue[0];
		if (Info->State == imsEmptyRequest || Info->OldState == imsEmptyRequest)
		{
			FOutgoingQueue.Del((mcInt32)0);
			if (FEntry->getInfo() == Info)
				FEntry->setInfo(NULL);
			delete Info;
		}
	}
		
	mcInt32 index = 0;
	if (FEntry->getInfo() != NULL)
		index = FOutgoingQueue.Find(FEntry->getInfo());
	if (index >= 0 && FOutgoingQueue.Length() > 0)
	{
		Info = (MCMessageInfo*)FOutgoingQueue[index];
		Info->MCError = ErrorCode;
		FOutgoingQueue.Del(index);
		if (FEntry->getInfo() == Info)
			FEntry->setInfo(NULL);

		FCriticalSection->Leave();
		FEntry->getCS()->Leave();
		FOwner->DeliveryFailed(Info);
		FEntry->getCS()->Enter();
		FCriticalSection->Enter();
	}

	bool result = FOutgoingQueue.Length() > 0;
	FCriticalSection->Leave();
	FEntry->getCS()->Leave();
	return result;
}

void MCInetTransportJob::ClientExecuteFinalizeBlock(mcInt32 &errorCode)
{
//ok, we are finished exchanging
    FOwner->FCriticalSection->Enter();
	TRY_BLOCK
	{
		//remove connection from list of connections
		FOwner->FConnections->Del(FEntry);

		FCriticalSection->Enter();
		TRY_BLOCK
		{
			if (FOutgoingQueue.Length() > 0)
			{
				FinalizeJob();

				//move remaining messages to transport's main outgoing queue
				while (0 < FOutgoingQueue.Length())
				{
					MCMessageInfo* Info = (MCMessageInfo*)FOutgoingQueue[0];
					Info->Track = NULL;
					if (Info->State == imsWaiting && !Info->Sent)
						Info->State = Info->OldState; //restore original state
					
					FOutgoingQueue.Del(Info);
					
					if ((Info->State != imsEmptyRequest) && (Info->State != imsEmptyReply))
					{
						if (FOwner->FDiscardUnsentMessages && !Info->ReplyFlag)
							delete Info;
						else
						if ((FOwner->FDiscardUnsentMessages && Info->ReplyFlag) || 
							(getTerminated() && FOwner->getFailOnInactive()))
						{
							Info->MCError = errorCode;
							FCriticalSection->Leave();
							FOwner->FCriticalSection->Leave();
							TRY_BLOCK
							{
								FOwner->DeliveryFailed(Info);
							}
							FINALLY
							(
								FOwner->FCriticalSection->Enter();
								FCriticalSection->Enter();
							)
						}
						else
							FOwner->FOutgoingQueue->Add(Info);
					}
					else
					{
						delete Info;
					}
				}
				//FEntry->getCS()->Leave();
			}
			else
				FinalizeJob();
		}
		FINALLY
		(
			FCriticalSection->Leave();
		)
	}
	FINALLY (
		FOwner->FCriticalSection->Leave();
	)    
}

void MCInetTransportJob::ClientExecute(void)
{
    bool Connected = false;
    bool errorFlag = false, closeConnection = false, timeout = false;
//	MCMessageInfo* Info = NULL;

	mcInt32 errorCode = MCError_InvalidAddress;

	//bool toDel = (strcmp(FEntry->FRemoteAddress, "192.168.0.4:14583") == 0);

	TRY_BLOCK
	{

		while ((false == errorFlag) && (false == closeConnection) && (false == getTerminated()) && (false == timeout) && 
			((FOwner->getAttemptsToConnect() == 0) || (FEntry->getAttempt() <= (mcInt32)FOwner->getAttemptsToConnect())))
		{
			if (false == Connected)
			{			
				// check if we still need to connect
				FCriticalSection->Enter();
				mcInt32 QueueLen = FOutgoingQueue.Length();
				FCriticalSection->Leave();
				if (QueueLen == 0)
					break;
				
				//try to connect
				
				//increase counter of connect's attempts
				FEntry->setAttempt(FEntry->getAttempt() + 1);

				if ((FOwner->getAttemptsToConnect() > 0) && (FEntry->getAttempt() > (mcInt32)FOwner->getAttemptsToConnect()))
				{
					if (FailCurrentMessage(MCError_ConnectionLost))
						FEntry->setAttempt(0);
					else
						errorFlag = true;
					continue;
				}
				if (FEntry->getAttempt() > 1)
					Sleep(0); //avoid 100% CPU during frequent reconnections

				if (true == ClientConnect())
				{
					errorCode = MCError_ConnectionLost;

					//go to PostConnectionStep && look for termination flag
					if (PostConnectionStep())
					{
						Connected = !getTerminated();
						if (Connected && (FEntry->getClientID() == 0) && (FEntry->getInfo() == NULL))
						{
							FCriticalSection->Enter();
							FOutgoingQueue.Insert(0, MakeEmptyPacket(imsEmptyRequest)); //get ClientID from server
							FCriticalSection->Leave();
						}
						continue;
					}
					else
					{
						FinalizeConnection();
						if (FTransporter != NULL)
							FTransporter->Close(true);
					}
				}
				
				{
					//ShowDebugInfo("ThreadID=%d, ConnID=%d, ClientID=%d, Dir=%d, Timestamp=%d, \"Cannot connect.\";\n", 
					//	GetCurrentThreadId(), FEntry->getConnID(), FEntry->getClientID(), FTransporter->getDirection() == isdIncoming, GetTickCount());

					//should we try again?
					if (!timeout && ((FOwner->getAttemptsToConnect() >= (mcUInt32)FEntry->getAttempt()) || 
						(FOwner->getAttemptsToConnect() == 0)))
					{
						//ShowDebugInfo("ThreadID=%d, ConnID=%d, ClientID=%d, Dir=%d, Timestamp=%d, \"Reconnecting.\";\n", 
						//	GetCurrentThreadId(), FEntry->getConnID(), FEntry->getClientID(), FTransporter->getDirection() == isdIncoming, GetTickCount());

						//replace transporter by this way due to strange XP behaviour
						MCSocket* newTransporter = FOwner->CreateClientSocket();
						MCSocket* oldTransporter = FTransporter;
						FTransporter = newTransporter; 
						delete oldTransporter; 
						//MCASSERT(newTransporter->getSocket() != FTransporter->getSocket());
						WaitForSignal(FOwner->getAttemptsInterval());
						continue;
					}
					else
					{
						if (FailCurrentMessage(MCError_ConnectionLost))
							FEntry->setAttempt(0);
						else
							errorFlag = true;
					}
				}
			}
			else
			{
				//DWORD trd = GetCurrentThreadId();
				//ShowDebugInfo("Client %d PerformRecvSend started\n", GetCurrentThreadId());
				PerformRecvSend(errorFlag, closeConnection, timeout);
				FinalizeConnection();
				//ShowDebugInfo("Client PerformRecvSend done\n", GetCurrentThreadId());
			
				if (errorFlag && !getTerminated() && !timeout)
				{
					Connected = false;
            
					bool tryAgain = false;
					
					FEntry->getCS()->Enter();
					TRY_BLOCK
					{
						setIsReceiving(false); setIsSending(false);
						
						//cleanup streams
						if (FEntry->getOutgoingStream() != NULL)
						{
							delete FEntry->getOutgoingStream();
							FEntry->setOutgoingStream(NULL);
						}
						
						if (FEntry->getIncomingStream() != NULL)
						{
							delete FEntry->getIncomingStream();
							FEntry->setIncomingStream(NULL);
						}
					}
					FINALLY(
						FEntry->getCS()->Leave();
					)
					//should try again to deliver remaining messages?
					if (!timeout && ((FOwner->getAttemptsToConnect() > (mcUInt32)FEntry->getAttempt()) || 
						(FOwner->getAttemptsToConnect() == 0)))
						tryAgain = true;
					else
					{
						if (FailCurrentMessage(MCError_ConnectionFailed))
						{
							tryAgain = true;
							FEntry->setAttempt(0);
						}
					}

					if (tryAgain)
					{
						errorFlag = false; closeConnection = false;
						MCSocket* newTransporter = FOwner->CreateClientSocket();
						MCSocket* oldTransporter = FTransporter;
						FTransporter = newTransporter; 
						delete oldTransporter; 
						WaitForSignal(FOwner->getAttemptsInterval());
						FEntry->getCS()->Enter();
						if (!getTerminated() && (FEntry->getInfo() == NULL) && (!AreRequestsInQueue() || (FEntry->getClientID() == 0)))
						{
							FCriticalSection->Enter();
							FOutgoingQueue.Insert(0, MakeEmptyPacket(imsEmptyRequest));
							FCriticalSection->Leave();
						}
						FEntry->getCS()->Leave();
						//ShowDebugInfo("ThreadID=%d, ConnID=%d, ClientID=%d, Dir=%d, Timestamp=%d, \"Connection will be established again.\";\n", 
						//	GetCurrentThreadId(), FEntry->getConnID(), FEntry->getClientID(), FTransporter->getDirection() == isdIncoming, GetTickCount());

						continue;
					}
					//else
					//	ShowDebugInfo("ThreadID=%d, ConnID=%d, ClientID=%d, Dir=%d, Timestamp=%d, \"Connection will NOT be established again.\";\n", 
					//		GetCurrentThreadId(), FEntry->getConnID(), FEntry->getClientID(), FTransporter->getDirection() == isdIncoming, GetTickCount());
                    
				}
			}
		}
		
	/*
	#if defined(__GNUC__) && (!defined(QNX))
		errorCode = ((MCStdSocket*)FTransporter)->GetInAddr()->sin_addr.s_addr == INADDR_NONE ? MCError_InvalidAddress : MCError_ConnectionLost;
	#else
		errorCode = ((MCStdSocket*)FTransporter)->GetInAddr()->sin_addr.S_un.S_addr == INADDR_NONE ? MCError_InvalidAddress : MCError_ConnectionLost;
	#endif
	*/
	}
	FINALLY
	(
		ClientExecuteFinalizeBlock(errorCode);
	)
	
	errorFlag = false;
    Connected = false;
    closeConnection = true; //exit from loop
}


void MCInetTransportJob::ServerExecuteFinalizeBlock(void)
{
	FOwner->FCriticalSection->Enter();
	TRY_BLOCK
	{
		FOwner->FConnections->Del(FEntry);
		FEntry->setAttempt(0);

		FCriticalSection->Enter();
		TRY_BLOCK
		{
			if (FOutgoingQueue.Length() > 0)
			{
				FinalizeJob();

				//move remaining messages to transport's main outgoing queue
				while (0 < FOutgoingQueue.Length())
				{
					MCMessageInfo* Info = (MCMessageInfo*)FOutgoingQueue[0];
					Info->Track = NULL;
					if (Info->State == imsWaiting && !Info->Sent)
						Info->State = Info->OldState; //restore original state
					
					FOutgoingQueue.Del(Info);
					
					if ((Info->State != imsEmptyRequest) && (Info->State != imsEmptyReply))
					{
						if (FOwner->FDiscardUnsentMessages && !Info->ReplyFlag)
							delete Info;
						else
						if ((FOwner->FDiscardUnsentMessages && Info->ReplyFlag) || 
							(getTerminated() && FOwner->getFailOnInactive()))
						{	Info->MCError = MCError_ConnectionLost;
							FCriticalSection->Leave();
							FOwner->FCriticalSection->Leave();
							TRY_BLOCK
							{
								FOwner->DeliveryFailed(Info);
							}
							FINALLY
							(
								FOwner->FCriticalSection->Enter();
								FCriticalSection->Enter();
							)
						}
						else
							FOwner->FOutgoingQueue->Add(Info);
					}
					else
					{
						delete Info;
					}
				}
				//FEntry->getCS()->Leave();
			}
			else
				FinalizeJob();
		}
		FINALLY
		(
			FCriticalSection->Leave();
		)
	}
	FINALLY (
		FOwner->FCriticalSection->Leave();
	)    
}

void MCInetTransportJob::ServerExecute(void)
{
//    MCMessageInfo* InfoCopy = NULL;
    bool /*Connected = false, */errorFlag = false, closeConnection = false, timeout = false;
	TRY_BLOCK
	{
	
		TRY_BLOCK
		{
			errorFlag = FTransporter->AfterConnection(FSocket, FOwner->getConnectionTimeout()) != 0;
		}
		CATCH_EVERY
		{
			errorFlag = true;
		}

		if (!errorFlag)
		{
			if (!InitializeConnection())
				errorFlag = true;
        

			if (false == errorFlag && (false == getTerminated()) && (true == PostConnectionStep()) )
				PerformRecvSend(errorFlag, closeConnection, timeout);
			FinalizeConnection();
		}
	//ShowDebugInfo("Server %d PerformRecvSend done\n", GetCurrentThreadId());
	}
	FINALLY
	(
		ServerExecuteFinalizeBlock();
	)
	errorFlag = false;
    closeConnection = true;
}

void MCInetTransportJob::Run(void)
{
    //FOwner->FLogger->PutMessage(llTrivial, "Job is starting.\n", 0, "MCInetTransportJob::Run");
    TRY_BLOCK
    {
        if (FTransporter->getDirection() == isdOutgoing)
		{
			//ShowDebugInfo("Client thread %d started\n", GetCurrentThreadId());
            ClientExecute();
			//ShowDebugInfo("Client thread %d finalizing\n", GetCurrentThreadId());
		}
        else
		{
			//ShowDebugInfo("Server %d thread started\n", GetCurrentThreadId());
            ServerExecute();
			//ShowDebugInfo("Server %d thread finalizing\n", GetCurrentThreadId());
		}
    }
    CATCH_EVERY
    {
    }
	/*
    FOwner->FCriticalSection->Enter();
	TRY_BLOCK
	{
		FOwner->FConnections->Del(FEntry);
		FinalizeJob();
		delete FTransporter; FTransporter = NULL;
	}
	FINALLY (
		FOwner->FCriticalSection->Leave();
	)
	*/
    //FOwner->FLogger->PutMessage(llTrivial, "Job is finishing.\n", 0, "MCInetTransportJob::Run");
}

MCMessageInfo* MCInetTransportJob::GetMessageForDelivery(void)
{
	mcInt32 i;
    MCMessageInfo *r = NULL, *r1 = NULL;

	FCriticalSection->Enter();
	TRY_BLOCK
    {
        for(i = 0; i < FOutgoingQueue.Length(); i++)
		{
            r = (MCMessageInfo*)FOutgoingQueue[i];
			
			//CHANGES
			if (FEntry->getClientID() != 0)
			{
				if (r->State == imsDispatching || r->State == imsComplete || r->State == imsFailed || r->State == imsEmptyRequest || r->State == imsEmptyReply)
				{
					r1 = r;
					break;
				}
			}
			else
			{
				if (r->State == imsEmptyRequest)
				{
					r1 = r;
					break;
				}
			}
		}
	} FINALLY (
		FCriticalSection->Leave();
	)

    return r1;
}

void MCInetTransportJob::Initialize2(MCSocket* Socket, MCInetConnectionEntry* Entry)
{
    MCSocketJob::Initialize();
	FTransporter = Socket;
	FEntry = Entry;
	FInitialized = true;
}

bool MCInetTransportJob::IsMessageRead(void)
{
    return false;
}


bool MCInetTransportJob::IsMessageFetched(void)
{
    MCStream* inStream = FEntry->getIncomingStream();
    if (NULL != inStream)
        return (FEntry->getMsgSize() > 0) && (inStream->Len() == FEntry->getMsgSize());
    else
        return false;
}


char* ReadStringFromArray(UNALIGNED char* array, char** str)
{
    MCASSERT(NULL != array);
    MCASSERT(NULL != str);

	//UNALIGNED mcInt32* p = (mcInt32 *) array;
	mcInt32 len;
	memmove(&len, array, sizeof(len));
	
	array += sizeof(mcInt32);    
    if (0 != len)
    {	
        *str = (char*)MCMemAlloc(len+1);
        memmove(*str, array, len);
        (*str)[len] = 0;
        array += len;
    }
    return array;
}

bool MCInetTransportJob::ParseRoutingStrings(char* s, char* *outS, char* TransactCmd, char** RouteTo, 
                                             char** RecvPath, char** DQueue)
{
    MCASSERT(NULL != s);
    MCASSERT(NULL != outS);
    MCASSERT(NULL != TransactCmd);
    MCASSERT(NULL != RouteTo);
    MCASSERT(NULL != RecvPath);
    MCASSERT(NULL != DQueue);
    
    //mcUInt32 i = 0;
    char* p = s + strlen(s) + 1;
    
    //get the right SMessenger
    *outS = (char*)MCMemAlloc(strlen(s)+1);
    strcpy(*outS, s);
    
    //read the transact cmd
    *TransactCmd = *p++;

    //read the RecvPath
    p = ReadStringFromArray(p, RecvPath);
    //read the RouteTo
    p = ReadStringFromArray(p, RouteTo);
    //read the DQueue
    p = ReadStringFromArray(p, DQueue);

    return true;
}

bool MCInetTransportJob::ReadHeader(MCInetHeader* Header, char** s, char* TransactCmd, 
                                     char** RouteTo, char** RecvPath, char** DQueue)
{
    MCASSERT(NULL != Header);
    MCASSERT(NULL != s);
    MCASSERT(NULL != TransactCmd);
    MCASSERT(NULL != RouteTo);
    MCASSERT(NULL != RecvPath);
    MCASSERT(NULL != DQueue);

    MCStream* inStream = FEntry->getIncomingStream();

    // read the header and smessenger
	inStream->Read(Header, sizeof(*Header));
	
    mcUInt32 SMsgLen = 0;
    mcUInt32 readSize = inStream->Read(&SMsgLen, sizeof(SMsgLen));
    // read the source messenger name
	if (readSize < sizeof(SMsgLen))
    {
        MCASSERT(0);
    }
        
	if((SMsgLen > 0) && (mcInt32) SMsgLen < (Min(0x7FFFDFFF, FOwner->getMaxMsgSize()) + 8192))
	{
	    *s = (char*)MCMemAlloc(SMsgLen + 1);
		if (inStream->Read(*s, SMsgLen) != (mcInt32)SMsgLen)
        {
            MCASSERT(0);
        }
		(*s)[SMsgLen] = 0;
        
        char* newS = NULL;
        if (strlen(*s) < SMsgLen) //is there routing/transaction info?
        {
            if (false == ParseRoutingStrings(*s, &newS, TransactCmd, RouteTo, RecvPath, DQueue))
                return false;
            MCASSERT(NULL != newS);
            MCMemFree(*s); *s = NULL;
            *s = newS;
        }
        else
            ;
    }
    else
    {
        MCASSERT(0);
        *s = NULL;
    }
    MCASSERT(*s != NULL);
    return true;
}

bool MCInetTransportJob::UpdateEntry(char* *s, char **OriginalSMsg)
{
    MCASSERT(NULL != OriginalSMsg);
    MCASSERT(NULL != s);
    MCASSERT(NULL != *s);

	mcInt32 ClientID = 0, ConnID = 0;
    mcInt32 Port = 0;
    char* IP = NULL;
    // what's this?
	*OriginalSMsg = _strdup(*s); //just store original SMessenger

    //ok, let's parse the incoming SMessenger
    bool IsRemoteAddr = ParseRemoteAddr(*s, &ClientID, &ConnID, &IP, &Port);
    if (ClientID != 0)
    {
		if (FEntry->getClientID() == 0)
		{
			FEntry->setClientID(ClientID);
			//ShowDebugInfo("ThreadID=%d, ConnID=%d, ClientID=%d, Dir=%d, Timestamp=%d, \"Last ClientID is uploaded from incoming packet.\";\n", 
			//	GetCurrentThreadId(), FEntry->getConnID(), FEntry->getClientID(), FTransporter->getDirection() == isdIncoming, GetTickCount());
		}
		/*
        else
        if (FEntry->getClientID() != ClientID)
            THROW_ERROR(MCError_InvalidSignature);
		*/
    }
    else
    {
		if (FEntry->getClientID() == 0)
		{
            FEntry->setClientID(CreateUniqueID());
			//ShowDebugInfo("ThreadID=%d, ConnID=%d, ClientID=%d, Dir=%d, Timestamp=%d, \"New ClientID is generated .\";\n", 
			//	GetCurrentThreadId(), FEntry->getConnID(), FEntry->getClientID(), FTransporter->getDirection() == isdIncoming, GetTickCount());
		}
        else
            ;
    }
    
    MCMemFree(*s); *s = NULL;
    
    //update the RemoteAddress
    if (true == IsRemoteAddr)
    {
	    if (strcmp(IP, "0.0.0.0") != 0)
        {
            *s = CreateSMessenger(FEntry->getClientID(), FEntry->getConnID(), IP, Port);
        }
        else
        {//.NET bug workaround
            char* OldIP = NULL;
			mcInt32 OldClientID = 0;
			mcInt32 OldConnID = 0;
            mcInt32 OldPort = 0;
            ParseRemoteAddr(FEntry->getRemoteAddress(), &OldClientID, &OldConnID, &OldIP, &OldPort);
            *s = CreateSMessenger(FEntry->getClientID(), FEntry->getConnID(), OldIP, Port);
            MCMemFree(OldIP);
        }
    }
    else
	    *s = CreateSMessenger(FEntry->getClientID(), FEntry->getConnID(), NULL, 0);
    MCMemFree(IP); IP = NULL;
    FEntry->setRemoteAddress(*s);
    MCASSERT(*s);
    return true;
}

bool MCInetTransportJob::PrepareRouting(char* s, char* OriginalSMsg, char** RecvPath)
{
    char* tmpPath = NULL;
	mcInt32 npl = 0;
	if (*RecvPath != NULL)
	{
	    npl = strlen(FOwner->RealTransportName()) + 1 + strlen(s) + 1 + strlen(*RecvPath) + 1;
		tmpPath = (char*)MCMemAlloc(npl);
		tmpPath[0] = 0;
		strcpy(tmpPath, FOwner->RealTransportName()); strcat(tmpPath, ":");
		strcat(tmpPath, s); strcat(tmpPath, "|"); strcat(tmpPath, *RecvPath);
		MCMemFree(*RecvPath);
		*RecvPath = tmpPath;
	}
	else
	{
	    npl = strlen(FOwner->RealTransportName()) + 1 + strlen(s) + 1;
		tmpPath = (char*) MCMemAlloc(npl);
		tmpPath[0] = 0;
		strcpy(tmpPath, FOwner->RealTransportName()); strcat(tmpPath, ":");
		strcat(tmpPath, s);
		*RecvPath = tmpPath;
	}

    {
        PutErrorPacket(OriginalSMsg, s, *RecvPath, 0, MCError_RoutingNotAllowed);
        return false;
    }
    return true;
    
}

//don't forget to not free DMessenger and SMessenger
void MCInetTransportJob::PutErrorPacket(char* DMessenger, char* SMessenger, char* RecvPath, __int64 MsgID, mcInt32 Error)
{
    MCMessage Message;
    Message.Data = NULL;
    Message.DataType = bdtConst;
	Message.DataSize = 0;
	Message.Result = 0;
	Message.Param1 = 0;
	Message.Param2 = 0;
	Message.MsgID = 0;

	MCMessageInfo *Info = new MCMessageInfo(&Message);
	Info->setDMessenger(DMessenger); 
	Info->setSMessenger(SMessenger);
	MCMessenger::SetMessageID(Info);
	Info->MCError = Error;
	Info->State = imsFailed;
	Info->StartTime = GetTickCount();
	Info->Timeout = FOwner->FMsgTimeout;
    Info->OrigID = MsgID;
	FCriticalSection->Enter();
	FOutgoingQueue.Add(Info);
	FCriticalSection->Leave();	
}

MCMessageInfo* MCInetTransportJob::LoadAndUnpackMessage(MCInetHeader* Header, void** DataBuf, 
                                                        mcInt32* DataLen, MCMemStream** AStream, 
                                                        char* s, char* SMsg, char TransactCmd,
                                                        char* RecvPath)
{
    MCASSERT(NULL != Header);
    MCASSERT(NULL != DataBuf);
    MCASSERT(NULL != DataLen);
    MCASSERT(NULL != AStream);
    MCASSERT(NULL != s);
    MCASSERT(NULL != SMsg);

    *AStream = NULL; *DataBuf = NULL; *DataLen = 0;

    bool res = false;
    MCMessageInfo* Info = NULL;
	mcInt32 Port;
	char* IP;
	char* s1;
	mcInt32 ClientID;
	mcInt32 ConnID;

	MCStream* inStream = FEntry->getIncomingStream();
    
    if (inStream->Pos() >= inStream->Len())
        return NULL; //there is no real message

	*DataLen = inStream->Len() - inStream->Pos(); //find message's size
	*DataBuf = MCMemAlloc(*DataLen); //alloc mem for message
	//if(NULL == DataBuf)
    //    THROW_ERROR(MCError_NotEnoughMemory);

    TRY_BLOCK
	{
	    // read the data
		inStream->Read(*DataBuf, *DataLen);
        // decode the message
		ParseRemoteAddr(s, &ClientID, &ConnID, &IP, &Port);
		s1 = (char*)MCMemAlloc(strlen(IP) + 20);
		sprintf(s1, "%s:%d", IP, Port);
		MCMemFree(IP);
        if(FOwner->UnprepareDataBlock(s1, *DataBuf, *DataLen, *DataBuf, *DataLen, FEntry->getEncryptID(), FEntry->getCompressID(), FEntry->getSealID()))
		{
            // decoding was successful
			
			*AStream = new MCMemStream();
            (*AStream)->SetPointer((char*)*DataBuf, *DataLen);
			
            MCMessage Message; Message.DataSize = 0;
            Info = new MCMessageInfo(&Message);
            Info->setSMessenger(SMsg);
            Info->setDMessenger(s);
            Info->Track = FEntry;//this;
            Info->Timeout = FOwner->FMsgTimeout;
            Info->LoadFromStream(*AStream);
            res = true;
        }
        else
            res = false;
		MCMemFree(s1);			
    }
    CATCH_EVERY
    {
        res = false;
    }
    if (false == res)
    {
		if (*AStream)
		{
			delete *AStream; *AStream = NULL;
		}
        MCMemFree(*DataBuf); *DataBuf = NULL; *DataLen = 0;
        delete Info; Info = NULL;
    }
    return res ? Info : NULL;
}

bool MCInetTransportJob::HandleEmptyRequest(MCMessageInfo* Info, MCMemStream* AStream)
{
    FEntry->getCS()->Leave();
    FOwner->FCriticalSection->Enter();
    FEntry->getCS()->Enter();
    TRY_BLOCK
    {
		FCriticalSection->Enter();
		TRY_BLOCK
		{
			mcInt32 i = 0;
			while (i<FOwner->FOutgoingQueue->Length())
			{
				MCMessageInfo* pInfo = (MCMessageInfo*)(*FOwner->FOutgoingQueue)[i];
				if ((pInfo->ClientID == FEntry->getClientID()) || ((pInfo->ClientID == 0) && FOwner->CompareAddress(pInfo->SMessenger, FEntry->getRemoteAddress(), false)))
				{
					FOwner->FOutgoingQueue->Del(pInfo);
					FOutgoingQueue.Add(pInfo);
				}
				else
					i++;
			}
			if (FOutgoingQueue.Length() == 0)
				FOutgoingQueue.Add(MakeEmptyPacket(imsEmptyReply));
		}
		FINALLY
		(
			FCriticalSection->Leave();
		)
    }
    FINALLY
    (
        FEntry->getCS()->Leave();
        FOwner->FCriticalSection->Leave();
        FEntry->getCS()->Enter();
        delete Info; Info = NULL;
    )
    
    return true;    
}

bool MCInetTransportJob::HandleEmptyReply(MCMessageInfo* Info, MCMemStream* AStream)
{
    //find request
    // remove the message from the list of pending messages
    /*
	MCMessageInfo* CurInfo = NULL;
    mcInt32 i = -1;

	for(i = 0;i < FOutgoingQueue.Length();i++)
	    if(((MCMessageInfo*)FOutgoingQueue[i])->OrigID == Info->OrigID)
		{
		    CurInfo = (MCMessageInfo*)FOutgoingQueue[i];
			FOutgoingQueue.Del(i);
			break;
		}

    delete CurInfo; CurInfo = NULL;
    */
	delete Info; Info = NULL;

    return true;
}

bool MCInetTransportJob::HandleDispatching(MCMessageInfo* Info, MCMemStream* AStream, char* RecvPath)
{
    MCMessenger::SetMessageID(Info);
    
    if(AStream->Len() - AStream->Pos() < Info->Message.DataSize)
	{//we've got corrupted packet
	    if(Info->IsSendMsg && (!getTerminated()))
		    PutErrorPacket(Info->DMessenger, Info->SMessenger, RecvPath, Info->OrigID, MCError_GenTransportFailure);
		else
        { //just drop this packet
		    delete Info;
            Info = NULL;
        }
    }
    else
    {
        if(Info->Message.DataSize > 0)
		{
    	    Info->Message.Data = MCMemAlloc(Info->Message.DataSize);
			AStream->Read(Info->Message.Data, Info->Message.DataSize);
		}
		else
		    Info->Message.Data = NULL;

    }
    FEntry->getCS()->Leave();
    FOwner->MessageReceived(Info);
    FEntry->getCS()->Enter();
    
    return true;
}

bool MCInetTransportJob::HandleReply(MCMessageInfo* Info, MCMemStream* AStream)
{
    // remove the message from the list of pending messages
    MCMessageInfo* CurInfo = NULL;
	mcInt32 i = -1;

	FCriticalSection->Enter();
	TRY_BLOCK
	{
    for(i = 0;i < FOutgoingQueue.Length();i++)
	    if(((MCMessageInfo*)FOutgoingQueue[i])->OrigID == Info->OrigID)
		{
		    CurInfo = (MCMessageInfo*)FOutgoingQueue[i];
			FOutgoingQueue.Del(i);
			break;
		}
	} FINALLY (
		FCriticalSection->Leave();
	)
        
    if(NULL == CurInfo) 
    {// the message was cancelled
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
		FOwner->ReplyReceived(CurInfo);
    }
    delete Info; Info = NULL;
    
    return true;
}

bool MCInetTransportJob::HandleUnknown(MCMessageInfo* Info, MCMemStream* AStream)
{
    delete Info; Info = NULL;
    return true;
}

void MCInetTransportJob::UpdateRemoteSAddress(MCMessageInfo* Info)
{
    if (NULL != Info->SMessenger)
    {
        mcInt32 ClientID1 = 0, ConnID1;
		mcInt32 Port1 = 0;
        char* IP1 = NULL;

        bool res1 = ParseRemoteAddr(Info->SMessenger, &ClientID1, &ConnID1, &IP1, &Port1);
        char buf[128];
        if (res1)
            sprintf(buf, "%d:%d:%s:%d", FEntry->getClientID(), FEntry->getConnID(), IP1, Port1);
        else
            sprintf(buf, "%d:%d", FEntry->getClientID(), FEntry->getConnID());
        FEntry->setRemoteSAddress(buf);
        Info->setSMessenger(buf);
        MCMemFree(IP1);
    }
    
}

bool MCInetTransportJob::MessageReceivedCompletely2(void)
{
    MCInetHeader Header;
    char* s = NULL, *RouteTo = NULL, *RecvPath = NULL, *DQueue = NULL, *OriginalSMsg = NULL;
    char TransactCmd = 0;
    void *DataBuf = NULL; mcInt32 DataLen = 0; MCMemStream *AStream = NULL;

    bool res = false;                                     
    TRY_BLOCK
    {
		FEntry->getCS()->Enter();
		TRY_BLOCK
		{
			if (IsMessageFetched())
			{
				FEntry->getIncomingStream()->SetPos(0);
				if (ReadHeader(&Header, &s, &TransactCmd, &RouteTo, &RecvPath, &DQueue))
				{
					UpdateEntry(&s, &OriginalSMsg);
					MCASSERT(OriginalSMsg != NULL);

					if (NULL != RouteTo) //should we route this message?
						PrepareRouting(s, OriginalSMsg, &RecvPath);
					else
					{
						if (!IsTransformerOK(&Header))
							PutErrorPacket(OriginalSMsg, s, RecvPath, 0, MCError_NoTransformer);
						FEntry->setSealID(Header.dwSealID);
						FEntry->setCompressID(Header.dwCompressID);
						FEntry->setEncryptID(Header.dwEncryptID);
					}	                
	                
					MCMessageInfo* Info = LoadAndUnpackMessage(&Header, &DataBuf, &DataLen, &AStream, s, OriginalSMsg, TransactCmd, RecvPath);
	                
					//update Entry->RemoteSAddress for server connections
	                

					if (NULL != Info)
					{
						if (FTransporter->getDirection() == isdIncoming)
							UpdateRemoteSAddress(Info);
						
						//ShowDebugInfo("ThreadID=%d, ConnID=%d, ClientID=%d, Dir=%d, Timestamp=%d, \"Incoming message with type %d.\";\n", 
						//	GetCurrentThreadId(), FEntry->getConnID(), FEntry->getClientID(), FTransporter->getDirection() == isdIncoming, 
						//	GetTickCount(), Info->State);

						MCMessageState State = Info->State;			

						TRY_BLOCK
						{
							Info->ConnID = FEntry->getConnID();
							Info->ClientID = FEntry->getClientID();
								
							switch (Info->State)
							{
							case imsEmptyRequest: 
									res = HandleEmptyRequest(Info, AStream);
								break;
	                        
							case imsEmptyReply:
								res = HandleEmptyReply(Info, AStream);
								break;

							case imsDispatching:
								res = HandleDispatching(Info, AStream, RecvPath);
								break;

							case imsComplete:
							case imsFailed:
								res = HandleReply(Info, AStream);
								break;
							default:
								res = HandleUnknown(Info, AStream);
							}
						}
						CATCH_EVERY
						{
							res = false;
						}
						if (res) {
							FLastMsgRecvState = State;
						}
													
						delete AStream; AStream = NULL;
						MCMemFree(DataBuf); DataBuf = NULL; DataLen = 0;
					}
				}
				if (NULL != FEntry)
				{
					delete FEntry->getIncomingStream(); FEntry->setIncomingStream(NULL);
					FEntry->setMsgSize(0);
				}
				setIsReceiving(false);
			}
		}
		FINALLY(
			FEntry->getCS()->Leave();
		)
    }
    CATCH_EVERY
    {
        res = false;
    }

    MCMemFree(s); s = NULL;
    MCMemFree(RouteTo); RouteTo = NULL;
    MCMemFree(RecvPath); RecvPath = NULL;
    MCMemFree(DQueue); DQueue = NULL;
    MCMemFree(OriginalSMsg); OriginalSMsg = NULL;
    
    return res;
}

void WriteStringToStream(MCStream* stream, char* str)
{
    mcInt32 len = NULL != str ? strlen(str) : 0;
    stream->Write(&len, sizeof(len));
    if (0 != len)
        stream->Write(str, len);
}

bool MCInetTransportJob::PrepareMessageForSending(void)
{
	mcInt32 i = 0, j = 0;
	MCMessageInfo* Info = NULL;
	MCMemStream* MemStream = NULL;
	void* DataBuf = NULL;
	mcInt32 DataLen = 0;
	MCInetHeader Header;
	char* s = NULL;
	bool r = false, immExit = false, wrt = false;
//	bool im = false;
	mcInt32 extra = 0;
	unsigned char bt = 0;
	mcInt32 Port;
	char* IP;
	mcInt32 ClientID, ConnID;

	TRY_BLOCK
    {
		FOwner->FCriticalSection->Enter();
		TRY_BLOCK
        {
			FEntry->getCS()->Enter();
			TRY_BLOCK
            {
				if(!FEntry->getInfo())
				{
					FEntry->setInfo(GetMessageForDelivery());
					if(!FEntry->getInfo())
					{
						r = false; immExit = true;
					}
					if (false == immExit)
						FEntry->getInfo()->OldState = FEntry->getInfo()->State;
                        //FEntry->setInfoState(FEntry->getInfo()->State);
				}
  
				if (false == immExit)
                {
                    Info = FEntry->getInfo();
				    s = CreateSMessenger(FEntry->getClientID(), FEntry->getConnID(), FTransporter->getLocalAddress(), FOwner->getMessengerPort());
                    Info->setSMessenger(s);
                    MCMemFree(s); s = NULL;

				    
					FEntry->getInfo()->State = FEntry->getInfo()->OldState;//= FEntry->getInfoState();
				    // write data for conversion
				    MemStream = new MCMemStream();
			
//					im = false;
					{
						Info->WriteToStream(MemStream);
					    if(Info->Message.DataSize > 0)
						    MemStream->Write(Info->Message.Data, Info->Message.DataSize);
					}

				    //FEntry->setInfoState(FEntry->getInfo()->State);
					Info->OldState  = Info->State;
				    Info->State = imsWaiting;
  
					{

						DataLen = MemStream->Len();
						DataBuf = MCMemAlloc(DataLen);
						MemStream->SetPos(0);
						MemStream->Read(DataBuf, DataLen);
						MemStream->SetPointer(NULL, 0);
  
						// prepare data block
						IP = NULL;
						ParseRemoteAddr(Info->DMessenger, &ClientID, &ConnID, &IP, &Port);
						if (IP != NULL)
						{
							s = (char*)MCMemAlloc(strlen(IP) + 20);
							sprintf(s, "%s:%d", IP, Port);
							MCMemFree(IP);
						}
						else
						{
							s = (char*)MCMemAlloc(1);
							s[0] = 0;
						}
						
						FOwner->PrepareDataBlock(s, DataBuf, DataLen, DataBuf, DataLen, FEntry->FEncryptID, FEntry->FCompressID, FEntry->FSealID);
						MCMemFree(s);						
					}				    
					
					Header.dwSignature = 0x50444945ul;
				    
					Header.dwDataSize = DataLen + sizeof(Header) + strlen(Info->SMessenger) + sizeof(mcInt32);
  
					// this will be used for routing and transactions
					extra = 2 + 4 + 4 + 4;

					{
						Header.dwCompressID = FEntry->FCompressID;
					    Header.dwEncryptID = FEntry->FEncryptID;
					    Header.dwSealID = FEntry->FSealID;
					}

					// this will be used for routing and transactions
					Header.dwDataSize += extra;
				    
				    // allocate stream for outgoing data
				    if(FOwner->getUseTempFilesForOutgoing() && (Header.dwDataSize > (mcInt32)FOwner->getOutgoingMemoryThreshold()))
				    {
					    delete MemStream; MemStream = NULL;
					    FEntry->setOutgoingStream(new MCTmpFileStream(FOwner->getTempFilesFolder(), Info->Message.MsgID));
				    }
				    else
					    FEntry->setOutgoingStream(MemStream);
  
					MCStream* writer = FEntry->getOutgoingStream();

				    // write outgoing data
				    // write header
				    writer->Write(&Header, sizeof(Header));
  
				    // write source address
				    i = strlen(Info->SMessenger);
					j = i;
					
					// this will be used for routing and transactions
					i += extra;
                    MCASSERT(i != 0);
					writer->Write(&i, sizeof(i));
				    if(j > 0)
					    writer->Write(Info->SMessenger, j);
					//write zero - end of string
                    bt = 0;
					writer->Write(&bt, sizeof(bt));

					wrt = false;
                    if (!wrt)
                    {
                        //write empty transact command
                        bt = (char)0;
                        writer->Write(&bt, sizeof(bt));
					    WriteStringToStream(writer, NULL);
                        WriteStringToStream(writer, NULL);
                        WriteStringToStream(writer, NULL);
                    }
				    // write actual data
				    writer->Write(DataBuf, DataLen);
				    writer->SetPos(0);
  
				    setIsSending(true);
                }
			}
            FINALLY (
				FEntry->getCS()->Leave();
			)
			if (false == immExit)
            {
                MCMemFree(DataBuf);
			    r = true;
            }
		} FINALLY (
			FOwner->FCriticalSection->Leave();
		)
    }
	CATCH_EVERY
    {
    }
    return r;
}

bool MCInetTransportJob::MessageSentCompletely(void)
{
	bool r = false;
	MCMessageInfo* LocalInfo;

	TRY_BLOCK
    {
		FEntry->getCS()->Enter();
		TRY_BLOCK
        {
            if (FEntry->getOutgoingStream() != NULL)
                r = FEntry->getOutgoingStream()->Len() == FEntry->getOutgoingStream()->Pos();
            else
                r = false;
  
			if(r)
			{
				//ShowDebugInfo("ThreadID=%d, ConnID=%d, ClientID=%d, Dir=%d, Timestamp=%d, \"Message with type %d is sent.\";\n", 
				//	GetCurrentThreadId(), FEntry->getConnID(), FEntry->getClientID(), FTransporter->getDirection() == isdIncoming, 
				//	GetTickCount(), FEntry->getInfo()->OldState);

				
				FEntry->setAttempt(0);

				LocalInfo = FEntry->getInfo();
				FEntry->setInfo(NULL);

				TRY_BLOCK
                {
					if (LocalInfo != NULL)
					{
						LocalInfo->Sent = true;
		
						FLastMsgSendState = LocalInfo->OldState;

						// if the message is to be replied, we do nothing
						if((LocalInfo->OldState == imsDispatching) && LocalInfo->IsSendMsg
							)
						{							
						}
						else
						{							
							// remove the message from the queue
							FCriticalSection->Enter();
							FOutgoingQueue.Del(LocalInfo);
							FCriticalSection->Leave();

							// destroy message info
							delete LocalInfo;
						}
					}
				} 
				FINALLY 
				(
					if (FEntry->getOutgoingStream() != NULL)
					{
						delete FEntry->getOutgoingStream();
						FEntry->setOutgoingStream(NULL);
					}
					setIsSending(false);
				)
			}
		} 
		FINALLY 
		(
			FEntry->getCS()->Leave();
		)
    }
    CATCH_EVERY
    {
    }
	return r;
}

/*
int	MCInetTransportJob::ExtractClientID(const char* S)
{
	mcInt32 res = 0;
	ParseRemoteAddr(S, &res, NULL, NULL);
	return res;
}
*/
/*
char* MCInetTransportJob::UpdateClientID(const char* S, mcInt32 ID)
{
    mcInt32 ClientID = 0, Port = 0;
    char* IP = NULL;
    char* res = (char*)MCMemAlloc(strlen(S) + 20);
    if (ParseRemoteAddr(S, &ClientID, &IP, &Port) == true)
        sprintf(res, "%d:%s:%d", ID, IP, Port);
    else
        sprintf(res, "%d", ID);
    MCMemFree(IP);
    return res;
}
*/
bool MCInetTransportJob::ParseRemoteAddr(const char* S, mcInt32 *ClientID, mcInt32 *ConnID, char** IP, mcInt32* Port)
{
	const mcInt32 BUF_SIZE = 128;

	char cc[BUF_SIZE];
	char pt[8];

    if (NULL == S) return false;

    if (NULL != ClientID) 
        *ClientID = 0;

	if (NULL != ConnID) 
        *ConnID = 0;

	cc[0] = 0;

	char* p2 = NULL;
	char* p1 = strchr((char *) S, ':');
	char *p3 = NULL;
	bool res = false;
	
	if (NULL != p1)
	{
		p2 = strchr(p1+1, ':');
		if (NULL != p2)
		{
			p3 = strchr(p2 + 1, ':');
			if (p3 == NULL)
			{
				p3 = p1;
			}
			else
			{
				p3 = p2;
				p2 = strchr(p2 + 1, ':');
			}
			// set cc
			memset(cc, 0, sizeof(cc));
            strncpy(cc, S, p3-S);
            cc[p3-S] = 0;
			
			if (NULL != IP)
			{
				*IP = (char*)MCMemAlloc(p2 - p3);
                memset(*IP, 0, p2 - p3);
				strncpy(*IP, p3 + 1, p2 - p3 - 1);
                //*IP[p2-p1-1] = 0;
			}
			if (NULL != Port)
			{
				strcpy(pt, p2+1);
				*Port = atoi(pt);				
			}
			res = true;
		} 
		else
		{
			if (p1 != (S + strlen(S)-1))
            {//extract IP
                if (NULL != IP)
                {
                    *IP = (char*)MCMemAlloc(p1 - S + 1);
                    memset(*IP, 0, p1 - S + 1);
                    strncpy(*IP, S, p1 - S);
                }
                if (NULL != Port)
                {
                    strcpy(pt, p1 + 1);
                    *Port = atoi(pt);
                }
                res = true;
            }
            else // extract ClientID
            {
                memset(cc, 0, sizeof(cc));
                strncpy(cc, S, p1-S);
			    res = false;
		    }
		}
	}
	else
	{
		memset(cc, 0, sizeof(cc));
        strcpy(cc, S);
		res = false;
	}
	if (cc[0] != 0)
	{
		p1 = strchr((char *) cc, ':');
		p2 = p1 + 1;
		if (p1 != NULL)
		{
			*p1 = 0;
			if ((p1 - cc) > 0)
			{

				if (ClientID != NULL)
					*ClientID = atoi(cc);
			}
			if (*p2 != 0)
			{
				if (ConnID != NULL)
					*ConnID = atoi(p2);
			}
		}
		else
		{
			p1 = strchr((char *) cc, '\\');
			p2 = p1 + 1;
			if (p1 == NULL)
			{
				if (ClientID != NULL)
					*ClientID = atoi(cc);
			}
			else
			{
				*p1 = 0;
				if ((p1 - cc) > 0)
				{

					if (ClientID != NULL)
						*ClientID = atoi(cc);
				}
				if (*p2 != 0)
				{
					if (ConnID != NULL)
						*ConnID = atoi(p2);
				}
			}
		}
	}
	return res;
}

void MCInetTransportJob::ChangeLastSendTime(MCMessageState state)
{
	FActivityTime = GetTickCount();
	FLastMsgSendTime = FActivityTime; 
}

void MCInetTransportJob::ChangeLastRecvTime(MCMessageState state)
{
	FActivityTime = GetTickCount();
	FLastMsgRecvTime = FActivityTime;
}

bool MCInetTransportJob::IsTransformerOK(MCInetHeader* Header)
{
	if (FTransporter->getDirection() == isdIncoming)
	{
		FOwner->FLinkCriticalSection->Enter();
		FEntry->setCompressID(0);
		FEntry->setSealID(0);
		FEntry->setEncryptID(0);
		FEntry->setRejectedTransformer(false);
		
		if (0 != Header->dwCompressID)
		{
			if (NULL != FOwner->getCompressor())
			{
				if (FOwner->getCompressor()->GetID() == Header->dwCompressID)
					FEntry->setCompressID(Header->dwCompressID);
				else
					FEntry->setRejectedTransformer(true);
			} else
				FEntry->setRejectedTransformer(true);
		}

		if (0 != Header->dwEncryptID)
		{
			if (NULL != FOwner->getEncryptor())
			{
				if (FOwner->getEncryptor()->GetID() == Header->dwEncryptID)
					FEntry->setEncryptID(Header->dwEncryptID);
				else
					FEntry->setRejectedTransformer(true);
			} else
				FEntry->setRejectedTransformer(true);
		}
		
		if (0 != Header->dwSealID)
		{
			if (NULL != FOwner->getSealer())
			{
				if (FOwner->getSealer()->GetID() == Header->dwSealID)
					FEntry->setSealID(Header->dwSealID);
				else
					FEntry->setRejectedTransformer(true);
			} else
				FEntry->setRejectedTransformer(true);
		}
		FOwner->FLinkCriticalSection->Leave();
		return !FEntry->getRejectedTransformer();
	}
	else
		return true;
	
}

bool MCInetTransportJob::getIsReceiving(void)
{
	return FIsReceiving;
}

void MCInetTransportJob::setIsReceiving(bool Value)
{
	FIsReceiving = Value;
	if (Value == false && FEntry != NULL) 
		FEntry->setMsgSize(0);    
}

bool MCInetTransportJob::getIsSending(void)
{
	return FIsSending;
}

void MCInetTransportJob::setIsSending(bool Value)
{
	FIsSending = Value;
}

MCInetConnectionEntry* MCInetTransportJob::getEntry(void)
{
	return FEntry;
}

void MCInetTransportJob::setEntry(MCInetConnectionEntry* Value)
{
	FEntry = Value;
}

MCInetTransport* MCInetTransportJob::getOwner(void)
{
	return FOwner;
}

void MCInetTransportJob::setOwner(MCInetTransport* Value)
{
	FOwner = Value;
}

MCSocket* MCInetTransportJob::getTransporter(void)
{
	return FTransporter;
}

void MCInetTransportJob::setTransporter(MCSocket* Value)
{
	if(FTransporter != Value)
	{
		MCSocket* aTransporter;
		aTransporter = FTransporter;
		FTransporter = Value;
		delete aTransporter;
	}
}

MCCriticalSection* MCInetTransportJob::getCriticalSection(void)
{
	return FCriticalSection;
}

MCList* MCInetTransportJob::getOutgoingQueue(void)
{
    return &FOutgoingQueue;
}

void MCInetTransportJob::AdjustSpeedLimits(mcUInt32 Incoming, mcUInt32 Outgoing)
{
	FIncomingSpeedLimit = Incoming;
	FOutgoingSpeedLimit = Outgoing;
}

void MCInetTransportJob::AdjustBufferSize(mcUInt32 Incoming, mcUInt32 Outgoing)
{
	if (FIncomingBuffer)
		MCMemFree(FIncomingBuffer);
	FIncomingBuffer = NULL;
	FIncomingBufferSize = Incoming;
	FIncomingBuffer = (unsigned char *) MCMemAlloc(Incoming);

	if (FOutgoingBuffer)
		MCMemFree(FOutgoingBuffer);
	FOutgoingBuffer = NULL;
	FOutgoingBufferSize = Outgoing;
	FOutgoingBuffer = (unsigned char *) MCMemAlloc(Outgoing);

	FblToSend = FOutgoingBufferSize;
	FblToRecv = FIncomingBufferSize;
}

bool MCInetTransportJob::InitializeConnection()
{
	FActivityTime = GetTickCount();
	FLastMsgRecvTime = FActivityTime;
	FLastMsgSendTime = FActivityTime;
	FblInSessionStartTime = FActivityTime;
	FblOutSessionStartTime = FActivityTime;

	FblInSessionTransferred = 0;
	FblOutSessionTransferred = 0;

	FblInMsgStartTime = 0;
	FblOutMsgStartTime = 0;
	FblInMsgTransferred = 0;
	FblOutMsgTransferred = 0;

	FServerRequestTime = 0;

	bool result = true;
	void* UserData;
	if (FOwner->getOnConnected(&UserData) != NULL)
		(FOwner->getOnConnected())(UserData, FOwner, FTransporter->getDirection(), FTransporter->getRemoteAddress(), FTransporter->getRemotePort(), result);
	FWasConnected = result;
	return result;
}

void MCInetTransportJob::FinalizeConnection()
{
	void* UserData;
	if (FWasConnected && FOwner->getOnDisconnected(&UserData) != NULL)
		(FOwner->getOnDisconnected())(UserData, FOwner, FTransporter->getDirection(), FTransporter->getRemoteAddress(), FTransporter->getRemotePort());
}

#ifdef MC_NO_PURE_VIRTUAL
bool MCInetTransportJob::ReceiveData(void)
{
	MC_pure_virtual();
	return false;
}

void MCInetTransportJob::UpdateConnectionContext(void)
{
	MC_pure_virtual();
}

bool MCInetTransportJob::SendData(void)
{
	MC_pure_virtual();
	return false;
}

void MCInetTransportJob::SetTransporterAddress(MCInetConnectionEntry* Ent, MCSocket* Transp)
{
	MC_pure_virtual();
}

bool MCInetTransportJob::PostConnectionStep(void)
{
	MC_pure_virtual();
	return false;
}
#endif

// ************************** MCInetListenerJob **************************

MCInetListenerJob::MCInetListenerJob(void)
{
	FInitialized = false;
	FOwner = NULL;
	FListener = NULL;
    FSocket = NULL;
}


MCInetListenerJob::~MCInetListenerJob(void)
{
	if(FListener)
	{
		FListener->Close(false);
		delete FListener;
	}
	FListener = NULL;
	//printf("Listener is destroyed\n");
}

MCSocket* MCInetListenerJob::getListener()
{
	return FListener;
}

bool MCInetListenerJob::AcceptConnection(void)
{
	MCSocket* NewSocket = NULL;
	MCInetConnectionEntry* NewEntry = NULL;
//	MCInetTransportJob* NewJob = NULL;
	char* s = NULL;
	bool r = false;

	assert(FListener != NULL);
	if(FListener->getState() != issListening)
		return false;
  
	NewSocket = FListener->Accept();
	if(NewSocket)
	{
		// Add objects to owner's list
		FOwner->FCriticalSection->Enter();
		TRY_BLOCK
        {
            
            MCInetTransportJob* NewJob = FOwner->CreateTransportJob();
            NewJob->setJobName("Server worker job");
            NewJob->setOwner(FOwner);
			NewJob->setNoUDPNotify(FNoUDPNotify);
            NewJob->Initialize2(NewSocket, FOwner->CreateConnectionEntry());
            NewEntry = NewJob->getEntry();
			//ShowDebugInfo("ThreadID=%d, ConnID=none, ClientID=none, Dir=accepting, Timestamp=%d, \"New connection is accepted.\";\n", 
			//	GetCurrentThreadId(), GetTickCount());
            
			
			//create remote address
            s = (char*)MCMemAlloc(strlen(NewSocket->getRemoteAddress()) + 20);
			sprintf(s, "%s:%d", NewSocket->getRemoteAddress(), NewSocket->getRemotePort());
			NewEntry->setRemoteAddress(s);
            MCASSERT(s);
			
			MCMemFree(s);
			
			
			
			//NewEntry->setSocket(NewSocket);
			NewEntry->setLastActionTime(GetTickCount());
			NewEntry->setProcessingJob(NewJob);
			
			//NewEntry->getCS()->Enter();
			FOwner->FConnections->Add(NewEntry);
            //NewEntry->getCS()->Leave();
            //just for experiment - start
#ifdef _WIN32            
            u_long enableNonBlocking = 1;
            if (0 != ioctlsocket(NewJob->getTransporter()->getSocket(), FIONBIO, &enableNonBlocking))
                RETURN_EXCEPT;
#endif
            //just for experiment - finish
            FOwner->FThreadPool.PostJob(NewJob);
		} FINALLY (
			FOwner->FCriticalSection->Leave();
		)
		r = true;
	}
	else
		r = false;
	return r;
}

#define RETURN_MESSAGE(x) 
//OutputDebugString(x); return

void MCInetListenerJob::Run(void)
{
	fd_set FDRecvSet;
	mcInt32 select_res;
	int num_fds;
	timeval tv;

	while(!getTerminated())
	{
		if (FNoUDPNotify)
		{
			if (FNotifyStop)
			{
				FNotifyStop = false;
				return;
			}
		}
    
		TRY_BLOCK
		{
			FD_ZERO(&FDRecvSet);
			num_fds = 1;
			
			tv.tv_sec = 0;
			tv.tv_usec = 500000;
			if (!FNoUDPNotify)
			{
				FD_SET(FSocket->getSocket(), &FDRecvSet);
				num_fds++;
				tv.tv_usec = 50000; // 10 times less	
			}

			FD_SET(FListener->getSocket(), &FDRecvSet);
			
#ifdef __GNUC__
			select_res = select(FD_SETSIZE, &FDRecvSet, NULL, NULL, &tv);
#else
			select_res = select(num_fds, &FDRecvSet, NULL, NULL, &tv);
#endif
			switch(select_res) 
			{
				case 0: // timeout elapsed? OS error. Something's wrong in the world today...
					continue;
				default:
					if (!getTerminated())
					{
						if (FD_ISSET(FListener->getSocket(), &FDRecvSet))
						{
							if (false == AcceptConnection())
							{
								return; //finish job
							}
							else
								continue;
							}
							else
								return; //finish job
						} else
							return; //finish job
					}
			}		   	
        CATCH_EVERY
        {
        }
	}
}

void MCInetListenerJob::Initialize(MCSocket *ASocket)
{
	MCSocketJob::Initialize();
	FListener = ASocket;
	FListener->setLocalAddress(FOwner->getMessengerAddress());
	FListener->setLocalPort(FOwner->getMessengerPort());
	WinsockErrorCheck(FListener->Init(istStream));
	if (FOwner->getReuseServerPort())
		WinsockErrorCheck(FListener->ReusePort());
	WinsockErrorCheck(FListener->Bind());
	WinsockErrorCheck(FListener->Listen(SOMAXCONN));
	FInitialized = true;
}

MCInetTransport* MCInetListenerJob::getOwner()
{
	return FOwner;
}

void MCInetListenerJob::setOwner(MCInetTransport* Value)
{
	FOwner = Value;
}

// ************************** MCInetConnectionEntry *************************

MCInetConnectionEntry::MCInetConnectionEntry(void)
{
	FCS = new MCCriticalSection();
	FEncryptID = -1;
	FCompressID = -1;
	FSealID = -1;
	FAttempt = 0;
	FCancelID = 0;
	FIncomingStream = NULL;
	FInfo = NULL;
	//FInfoState = (MCMessageState)0;
	FLastActionTime = 0;
	FMsgSize = 0;
	FOutgoingStream = NULL;
	FProcessingThread = NULL;
	FRemoteAddress = NULL;
    FRemoteSAddress = NULL;
    FClientID = 0;
	FConnID = CreateUniqueID();
}

MCInetConnectionEntry::~MCInetConnectionEntry(void)
{
	//FCS->Enter();
	delete FIncomingStream;
	delete FOutgoingStream;
    if (FRemoteAddress)
        MCMemFree(FRemoteAddress);
    FRemoteAddress = NULL;
    if (FRemoteSAddress)
        MCMemFree(FRemoteSAddress);
    FRemoteSAddress = NULL;
	delete FCS; FCS = NULL;
}

void MCInetConnectionEntry::Reset(void)
{
	FCS->Enter();
	delete FIncomingStream;
	FIncomingStream = NULL;
	delete FOutgoingStream;
	FOutgoingStream = NULL;
	FAttempt = 0;
	FCancelID = 0;
	FEncryptID = -1;
	FCompressID = -1;
	FSealID = -1;
	FMsgSize = 0;
	FInfo = NULL;
    if (NULL != FRemoteAddress)
        FRemoteAddress[0] = 0;
    if (NULL != FRemoteSAddress)
        FRemoteSAddress[0] = 0;
	FCS->Leave();
}

void MCInetConnectionEntry::setAttempt(mcInt32 Value)
{
	FAttempt = Value;
}

mcInt32 MCInetConnectionEntry::getAttempt(void)
{
	return FAttempt;
}

void MCInetConnectionEntry::setCancelID(__int64 Value)
{
	FCancelID = Value;
}

__int64 MCInetConnectionEntry::getCancelID(void)
{
	return FCancelID;
}

void MCInetConnectionEntry::setCompressID(mcInt32 Value)
{
	FCompressID = Value;
}

mcInt32 MCInetConnectionEntry::getCompressID(void)
{
	return FCompressID;
}

MCCriticalSection* MCInetConnectionEntry::getCS(void)
{
	return FCS;
}

void MCInetConnectionEntry::setEncryptID(mcInt32 Value)
{
	FEncryptID = Value;
}

mcInt32 MCInetConnectionEntry::getEncryptID(void)
{
	return FEncryptID;
}

void MCInetConnectionEntry::setIncomingStream(MCStream* Value)
{
    FIncomingStream = Value;
}

MCStream* MCInetConnectionEntry::getIncomingStream(void)
{
	return FIncomingStream;
}

void MCInetConnectionEntry::setInfo(MCMessageInfo* Value)
{
    FInfo = Value;
}

MCMessageInfo* MCInetConnectionEntry::getInfo(void)
{
	return FInfo;
}

/*
void MCInetConnectionEntry::setInfoState(MCMessageState Value)
{
	FInfoState = Value;
}

MCMessageState MCInetConnectionEntry::getInfoState(void)
{
	return FInfoState;
}
*/

void MCInetConnectionEntry::setLastActionTime(__int64 Value)
{
	FLastActionTime = Value;
}

__int64 MCInetConnectionEntry::getLastActionTime(void)
{
	return FLastActionTime;
}

void MCInetConnectionEntry::setMsgSize(mcInt32 Value)
{
	FMsgSize = Value;
}

mcInt32 MCInetConnectionEntry::getMsgSize(void)
{
	return FMsgSize;
}

void MCInetConnectionEntry::setOutgoingStream(MCStream* Value)
{
	FOutgoingStream = Value;
}

MCStream* MCInetConnectionEntry::getOutgoingStream(void)
{
	return FOutgoingStream;
}

void MCInetConnectionEntry::setProcessingJob(MCInetTransportJob* Value)
{
	FProcessingThread = Value;
}

MCInetTransportJob* MCInetConnectionEntry::getProcessingJob(void)
{
	return FProcessingThread;
}

void MCInetConnectionEntry::setRemoteAddress(const char* Value)
{
	if(FRemoteAddress)
		MCMemFree(FRemoteAddress);
	if(Value)
		FRemoteAddress = _strdup(Value);
	else
		FRemoteAddress = NULL;
}

char* MCInetConnectionEntry::getRemoteAddress(void)
{
	return FRemoteAddress;
}


void MCInetConnectionEntry::setRemoteSAddress(const char* Value)
{
	if(FRemoteSAddress)
		MCMemFree(FRemoteSAddress);
	if(Value)
		FRemoteSAddress = _strdup(Value);
	else
		FRemoteSAddress = NULL;
}

char* MCInetConnectionEntry::getRemoteSAddress(void)
{
	return FRemoteSAddress;
}

void MCInetConnectionEntry::setSealID(mcInt32 Value)
{
	FSealID = Value;
}

mcInt32 MCInetConnectionEntry::getSealID(void)
{
	return FSealID;
}

mcInt32 MCInetConnectionEntry::getClientID(void)
{
    return FClientID;
}

void MCInetConnectionEntry::setClientID(mcInt32 Value)
{
    FClientID = Value;
}

bool MCInetConnectionEntry::getRejectedTransformer(void)
{
    return FRejectedTransformer;
}

void MCInetConnectionEntry::setRejectedTransformer(bool Value)
{
    FRejectedTransformer = Value;
}

mcInt32 MCInetConnectionEntry::getReqTime(void)
{
    return FReqTime;
}

void MCInetConnectionEntry::setReqTime(mcInt32 ReqTime)
{
    FReqTime = ReqTime;
}

mcInt32 MCInetConnectionEntry::getConnID(void)
{
	return FConnID;
}

void MCInetConnectionEntry::setConnID(mcInt32 value)
{
	FConnID = value;
}

#ifdef USE_NAMESPACE
}
#endif
