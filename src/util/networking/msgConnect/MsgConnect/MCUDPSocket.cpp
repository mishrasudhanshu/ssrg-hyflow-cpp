//====================================================
//                                                    
//  EldoS MsgConnect                                 
//                                                    
//   Copyright (c) 2001-2010, EldoS                   
//                                                    
//====================================================
//#include "MC.h"
#include "MCUtils.h"
#include "MCUDPSocket.h" 
#ifndef _WIN32_WCE
#include <assert.h>
#endif
 
#ifdef USE_NAMESPACE
namespace MsgConnect
{
#endif

static char * mcstrtok(const char *strToken, const char* strDelim)
{
	if (!strDelim || *strDelim == 0)
		return NULL;

	char *result = (char *) strToken;
	if (strDelim[1] == 0)
	{
		while ((*result != 0) && (*result != *strDelim))
		{
			result ++;
		}
		if (*result == 0)
			return NULL;
		else
			return result;
	}
	else
	{
		while ((*result != 0))
		{
			if (strchr(strDelim, *result) != NULL)
				break;
			else
				result++;
		}
		if (*result == 0)
			return NULL;
		else
			return result;
	}
}

MCUDPTransport::MCUDPTransport(): MCSimpleTransport() 
{
	FDefaultTransportName = "UDP";
	FMessengerPort = 1459;
	FFailOnInactive = false;
	FMessengerAddress = (char*) MCMemAlloc(strlen("0.0.0.0") + 1);
	strcpy(FMessengerAddress, "0.0.0.0");
	FTransportMode = (MCInetTransportMode)0;
	FReceiverThread = NULL;
	FSenderThread = NULL;
	FMulticastList = new MCStringList();
	FReuseServerPort = true;
}
  
MCUDPTransport::~MCUDPTransport()
{
	setActive(false);
	
	if (NULL != FMulticastList)
		delete(FMulticastList);

	if (NULL != FMessengerAddress)
    {
        free(FMessengerAddress);
        FMessengerAddress = NULL;
    }    
}

void MCUDPTransport::setFailOnInactive(bool Value)
{
	if(FFailOnInactive != Value)
		FFailOnInactive = Value;
}

bool MCUDPTransport::getFailOnInactive(void)
{
	return FFailOnInactive;
}

void MCUDPTransport::setMessengerAddress(char* Value)
{
	bool b;
	mcUInt32 i;

	if(!FMessengerAddress && !Value)
		return;
	if(!FMessengerAddress || !Value || (strcmp(FMessengerAddress, Value) != 0))
	{
		if(FMessengerAddress)
			MCMemFree(FMessengerAddress);
		b = getActive();
		setActive(false);
		if(Value && strcmp(Value, "255.255.255.255") != 0)
		{
			i = inet_addr(Value);
			if(i == INADDR_NONE)
                THROW_ERROR(MCError_InvalidAddress);
		}
		FMessengerAddress = Value;
		setActive(b);
	}
}

char* MCUDPTransport::getMessengerAddress(void)
{
	return FMessengerAddress;
}

void MCUDPTransport::setMessengerPort(unsigned Value)
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

mcUInt32 MCUDPTransport::getMessengerPort(void)
{
	return FMessengerPort;
}

mcUInt32 MCUDPTransport::getMessengerPortBound(void)
{
	if (getActive() == false || FReceiverThread == NULL)
		return 0;
	else
		return FReceiverThread->FRecvSocket->getLocalPort();
}

void MCUDPTransport::setReuseServerPort(bool Value)
{
	FReuseServerPort = Value;
}

bool MCUDPTransport::getReuseServerPort(void)
{
	return FReuseServerPort;
}

void MCUDPTransport::setTransportMode(MCInetTransportMode Value)
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

MCInetTransportMode MCUDPTransport::getTransportMode(void)
{
	return(FTransportMode);
}


MCUDPSenderThread* MCUDPTransport::CreateSenderThread(bool x)
{
	return new MCUDPSenderThread(x);
}

MCUDPReceiverThread* MCUDPTransport::CreateReceiverThread(bool x)
{
	return new MCUDPReceiverThread(x);
}

bool MCUDPTransport::DeliverMessage(char* DestAddress, MCMessageInfo* Info)
{
	//FLogger->PutMessage(llTrivial, "DeliverMessage is started.\n", 0, "MCInetTransport::DeliverMessage");
    bool r;

	char *p = NULL, *p1 = NULL;
	char *SocketName = NULL, *QueueName = NULL, *URLName = NULL;

  if(mcInt32(FTransportMode) != stmP2P && mcInt32(FTransportMode) != stmClient)
		return(false);

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
            
		// get ID/socket name
		if(p - p1 > 0)
			SocketName = strCopy(p1, 0, p - p1);
		else
			THROW_ERROR(MCError_BadDestinationName);

		//get URL name
		p1 = p;
		while ((*p != '|') && *p)
			p++;
		if (!*p)
			THROW_ERROR(MCError_BadDestinationName); // there is no queue name

		if (*p == '|')
			URLName = strCopy(p1, 0, p - p1);
    else
			THROW_ERROR(MCError_BadDestinationName);
		p++;

    // get queue name
		p1 = p;
		while(*p)
			p++;
		if(p - p1 > 0)
			QueueName = strCopy(p1, 0, p - p1);
		else
			THROW_ERROR(MCError_BadDestinationName);

    // set MessageInfo properties
		strncpy(Info->DQueue, QueueName, DQueueMaxLength - 1);
		Info->DQueue[DQueueMaxLength - 1] = 0;
		Info->DMessenger = SocketName;
		Info->URL = URLName;
		Info->State = imsDispatching;
//		MCMemFree(QueueName); QueueName = NULL;
		FCriticalSection->Enter();
		TRY_BLOCK {
			InsertInfoIntoQueueWithPriorities(FOutgoingQueue, Info, false);
			FSenderThread->KickThread(true);
		} FINALLY (
			FCriticalSection->Leave();
		)
		r = true;
	}
	else
		r = false;
	MCMemFree(QueueName); QueueName = NULL;
	return r;
}

void MCUDPTransport::DoSetActive(void)
{
	bool initFailed = false;

	if(!FMessenger)
		return;

	if(getActive())
	{
		FSenderThread = NULL;
    
		if((getTransportMode() == stmP2P) || (getTransportMode() == stmServer))
		{
			FReceiverThread = new MCUDPReceiverThread(true);
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
			FReceiverThread->setFreeOnTerminate(false);
		}

		if((getTransportMode() == stmP2P) || (getTransportMode() == stmClient))
		{
			FSenderThread = new MCUDPSenderThread(true);
			TRY_BLOCK
			{
				FSenderThread->Initialize(this);
			}
			CATCH_EVERY
			{
				delete FSenderThread; FSenderThread = NULL;
				delete FReceiverThread; FReceiverThread = NULL;
				initFailed = true;
			}
			if (true == initFailed)
#ifndef USE_CPPEXCEPTIONS
				RaiseException(1, 0, 0, NULL);
#else
				throw EMCError(0);
#endif	
			FSenderThread->setFreeOnTerminate(false);
		}
		if((getTransportMode() == stmP2P) || (getTransportMode() == stmServer))
			FReceiverThread->Resume();
		if((getTransportMode() == stmP2P) || (getTransportMode() == stmClient))
			FSenderThread->Resume();
	}
  else
	{
		if (NULL != FSenderThread)
		{
			FSenderThread->Terminate();
			FSenderThread->KickThread(false);
//			FSenderThread->FStopEvent->setEvent();
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
			FSenderThread->WaitFor();
#else
			WaitForSingleObject(FSenderThread->getHandle(), INFINITE);
#endif
		}
		if (NULL != FReceiverThread)
		{
			FReceiverThread->Terminate();
			FReceiverThread->KickThread(false);
//			FReceiverThread->FStopEvent->setEvent();
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
			FReceiverThread->WaitFor();
#else
			WaitForSingleObject(FReceiverThread->getHandle(), INFINITE);
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

void MCUDPTransport::KickSender(bool MessageFlag)
{
	if (FSenderThread != NULL)
		FSenderThread->KickThread(MessageFlag);

}

void MCUDPTransport::CancelMessageInSender(MCMessageInfo* Info)
{
	if(FSenderThread != NULL)
	{
		FSenderThread->FCancelID = Info->Message.MsgID;
		while (FSenderThread->FSendingInfo == Info)
			Sleep(100);
	}
}

bool MCUDPTransport::WasCancelled(MCMessageInfo* Info)
{
	return (FSenderThread && (FSenderThread->FCancelID == Info->Message.MsgID));
}

void MCUDPTransport::SetInfoSMessenger(MCMessageInfo* Info)
{
	if(FSenderThread)
	{
    char pt[128];
		sprintf(pt, "%s:%d", FSenderThread->FSendSocket->getLocalAddress(), getMessengerPort());
		Info->setSMessenger(pt);
	}
	else
		Info->SMessenger = NULL;
}

mcInt32 MCUDPTransport::FindMulticastAddress(const char* Value)
{
	char *val = NULL;
	mcInt32 result = -1;
	for (mcInt32 i = 0; i < FMulticastList->Length(); i++)
	{
		val = (char *) ((*FMulticastList)[i]);
		if (strncmp(Value, val, strlen(Value)) == 0)
		{
			result = i;
			break;
		}
	}
	return result;
}

void MCUDPTransport::JoinMulticastGroup(const char* GroupAddress, const char* BindAddress, unsigned char TTL, bool Loop)
{
	if (!GroupAddress)
		return;

	mcInt32 ma = FindMulticastAddress(GroupAddress);
	if (ma != -1)
		FMulticastList->Del(ma);
	bool oa = getActive();
	if (oa)
		setActive(false);
	
	char* ba = "255.255.255.255";
	if (BindAddress && BindAddress[0] != 0)
		ba = (char *) BindAddress;

	char *buf = (char *) MCMemAlloc(strlen(GroupAddress) + strlen(ba) + 8);
	sprintf(buf, "%s|%s|%i|%i", GroupAddress, ba, TTL, (unsigned char) Loop);
	FMulticastList->Add(buf);
	MCMemFree(buf);

	if (oa)
		setActive(true);
}

void MCUDPTransport::LeaveMulticastGroup(const char* GroupAddress)
{
	mcInt32 ma = FindMulticastAddress(GroupAddress);
	if (ma != -1)
	{
		bool oa = getActive();
		if (oa)
			setActive(false);
		FMulticastList->Del(ma);
		if (oa)
			setActive(true);		
	}
}
	

// ================================

MCUDPSenderThread::~MCUDPSenderThread(void)
{
	delete FSendSocket;
}

bool MCUDPSenderThread::CanExecute(void)
{
	return true;
}

void MCUDPSenderThread::Execute(void)
{
	fd_set FDSendSet, FDRecvSet;
	bool errorFlag = false, b;
	mcInt32 i;
	TSocket highSocketHandle = 0;

	while ((false == errorFlag) &&
		(false == getTerminated()))
	{
		TRY_BLOCK
		{
			FD_ZERO(&FDSendSet);
			FD_ZERO(&FDRecvSet);
			FD_SET(FSocket->getSocket(), &FDRecvSet);
                                   
			Owner->FCriticalSection->Enter();
			TRY_BLOCK
			{
				b = false;
				for(i = 0; i < Owner->FOutgoingQueue->Length(); i++)
				{
					MCMessageInfo* r = ((MCMessageInfo*)(*Owner->FOutgoingQueue)[i]);
					if(r->State == imsDispatching || r->State == imsComplete || r->State == imsFailed)
					{
						b = true;
						break;
					}
				}
				if(b)
				{
					FD_SET(FSendSocket->getSocket(), &FDSendSet);
					highSocketHandle = FSocket->getSocket() > FSendSocket->getSocket() ? FSocket->getSocket() : FSendSocket->getSocket();
				}
				else
					highSocketHandle = FSocket->getSocket();
			} FINALLY (
				Owner->FCriticalSection->Leave();
			)
			mcInt32 select_res = select(highSocketHandle + 1, &FDRecvSet, &FDSendSet, NULL, NULL);

			if ((0 != select_res) && (-1 != select_res))
			{
				if(FD_ISSET(FSocket->getSocket(), &FDRecvSet))
				{
					if(!getTerminated())
					{
						if (!MCSocket::PopFromCtlSocket(FSocket))
						{
							errorFlag = true;
							continue;
						}
					}
				}

				if(FD_ISSET(FSendSocket->getSocket(), &FDSendSet))
				{
					if(!SendMessage())
						errorFlag = true;
				}
			}
			else
			{
				if((select_res == 0 && Owner->getFailOnInactive()) ||
					(select_res == -1))
					errorFlag = true;
			}
    }
		CATCH_EVERY
		{
			errorFlag = true;
		}
	}
}

MCUDPSenderThread::MCUDPSenderThread(bool CreateSuspended)
:MCUDPThread(CreateSuspended), FSendSocket(NULL)
{
}


void MCUDPSenderThread::Initialize(MCUDPTransport* AOwner)
{
	MCUDPThread::Initialize(AOwner);
	FSendSocket = new MCStdSocket();
	FSendSocket->setLocalPort(0);
	FSendSocket->setLocalAddress(Owner->getMessengerAddress());
	WinsockErrorCheck(FSendSocket->Init(istDatagram));
	mcInt32 i = 1;
	WinsockErrorCheck(setsockopt(FSendSocket->getSocket(), SOL_SOCKET, SO_BROADCAST, (char*)&i, sizeof(i)));
	i = 1;
#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
	WinsockErrorCheck(ioctlsocket(FSendSocket->getSocket(), FIONBIO, (u_long*)&i));
#else
	WinsockErrorCheck(ioctl(FSendSocket->getSocket(), FIONBIO, (u_long*)&i));
#endif

	for (i = 0; i < Owner->FMulticastList->Length(); i++)
	{
		char* ml = (char *) ((*Owner->FMulticastList)[i]);
		char* mle = ml;
		
		mle = mcstrtok(ml, "|");
		char GroupAddress[16];
		strncpy(GroupAddress, ml, mle - ml); GroupAddress[mle - ml] = 0;
		ml = mle + 1;

		mle = mcstrtok(ml, "|");
		char BindAddress[16];
		strncpy(BindAddress, ml, mle - ml); BindAddress[mle - ml] = 0;
		ml = mle + 1;

		mle = mcstrtok(ml, "|");
		char TTLStr[4];
		strncpy(TTLStr, ml, mle - ml); TTLStr[mle - ml] = 0;
		ml = mle + 1;

		unsigned char TTL = (unsigned char) atoi(TTLStr);
		bool Loop = atoi(ml) != 0;

		WinsockErrorCheck(FSendSocket->AddToMulticastCli(BindAddress, TTL, Loop));
	}
}

void MCUDPSenderThread::MessageFailed(mcInt32 reason, MCMessageInfo* Info, MCMessageState InfoState)
{
	// remove the message from the queue
	Owner->FCriticalSection->Enter();
	Owner->FOutgoingQueue->Del(Info);
	Owner->FCriticalSection->Leave();

	if((InfoState == imsDispatching) && (reason != 0)
	    )
	{

		Owner->FMessenger->FCompleteCriticalSection->Enter();
		TRY_BLOCK
		{
			Info->State = imsFailed;
			Info->MCError = reason;
			Owner->FMessenger->FCompleteQueue->Add(Info);
			Owner->FMessenger->FCompleteEvent->setEvent();
			if(Info->ReplyFlag)
				Info->ReplyFlag->setEvent();
		} FINALLY (
			Owner->FMessenger->FCompleteCriticalSection->Leave();
		)
	}
	else
	if (FCancelID != Info->Message.MsgID)
	{
		// destroy message info
		delete Info; Info = NULL;
	}
}

bool MCUDPSenderThread::SendMessage(void)
{
	char* DataBuf = NULL;
	mcInt32 DataLen = 0;
	char* RemoteHost = NULL;
	mcInt32 RemotePort = 0;

	mcInt32 i = 0, i1 = 0;
	MCMessageInfo* Info = NULL;
	MCMessageState InfoState = imsNone;
	bool result = true;

	if(PrepareMessage(&Info, &DataBuf, &DataLen, &InfoState))
	{
		FSendingInfo = Info;
		TRY_BLOCK
		{
			i = CONST_MAX_MSG_SIZE;
			if(DataLen <= i)
			{
				if(ParseRemoteAddr(Info->DMessenger, &RemoteHost, &RemotePort))
				{
					i = FSendSocket->SendTo(DataBuf, DataLen, i1, RemoteHost, RemotePort);
					if(i != 0)
						MessageFailed(MCError_WrongSocketResult, Info, InfoState);
					else
					{
						if(InfoState == imsDispatching && Info->IsSendMsg
						)
				// if the message is to be replied, we do nothing
							;
						else
						{
							// otherwise we destroy the message
							MessageFailed(0, Info, InfoState);
						}
					}
				}
				else
					MessageFailed(MCError_InvalidAddress, Info, InfoState);
			}
			else
				MessageFailed(MCError_PacketTooLarge, Info, InfoState);
		}
		FINALLY
		(
			FSendingInfo = NULL;
		)
    // FreeAndNil(Info);
	}
	if( DataBuf != NULL )
		MCMemFree(DataBuf);

	if (NULL != RemoteHost)
		free(RemoteHost);
	return result;
}

bool MCUDPSenderThread::PrepareMessage(MCMessageInfo** AInfo, char** DataBuf,
	mcInt32* DataLen, MCMessageState* InfoState)
{
	mcInt32 i, j;
	MCMemStream* MemStream;
	MCInetHeader Header;
//	bool im;
	bool wrt = false;
	bool res = false;
	mcInt32 extra;
//	unsigned char bt = 0;
	//bool result = false;
	MCMessageInfo* Info = NULL;

	*DataLen = 0;
	*DataBuf = NULL;
	*AInfo = NULL;

	Owner->FCriticalSection->Enter();
	TRY_BLOCK
	{
		for(i = 0;i <= Owner->FOutgoingQueue->Length() - 1;i++)
			if(((MCMessageInfo*)((*Owner->FOutgoingQueue)[i]))->State == imsDispatching ||
				((MCMessageInfo*)((*Owner->FOutgoingQueue)[i]))->State == imsComplete ||
				((MCMessageInfo*)((*Owner->FOutgoingQueue)[i]))->State == imsFailed)
			{
				Info = (MCMessageInfo*)((*Owner->FOutgoingQueue)[i]);
				//Owner->FOutgoingQueue->Del(i);
				break;
			}

		if(Info)
		{
			res = true;
			//Owner->FCriticalSection->Leave();
		//	return(false);
		
			{
				//MCMemFree(Info->SMessenger);
				char pt[128];
				sprintf(pt, "%s:%d", FSendSocket->getLocalAddress(), Owner->FMessengerPort);
				Info->setSMessenger(pt);
			}

		// write data for conversion
			MemStream = new MCMemStream();
			TRY_BLOCK
			{
//				im = false;

				{
					Info->WriteToStream(MemStream);//->Write(Info, (mcInt32)((char*)&Info->Message.Data - (char*)Info));
					if(Info->Message.DataSize > 0)
						MemStream->Write(Info->Message.Data, Info->Message.DataSize);
				}
				*InfoState = Info->State;
				Info->State = imsWaiting;

				{
					if(Owner->getCompressor())
						Header.dwCompressID = Owner->getCompressor()->GetID();
					else
						Header.dwCompressID = 0;

					if(Owner->getEncryptor())
						Header.dwEncryptID = Owner->getEncryptor()->GetID();
					else
						Header.dwEncryptID = 0;

					if(Owner->getSealer())
						Header.dwSealID = Owner->getSealer()->GetID();
					else
						Header.dwSealID = 0;
				}

				{
					*DataLen = MemStream->Len();
					*DataBuf = (char*)MCMemAlloc(*DataLen);
					MemStream->SetPos(0);
					MemStream->Read(*DataBuf, *DataLen);
					MemStream->SetPointer(NULL, 0);

					// prepare data block
					char* s = (char*)MCMemAlloc(FSendSocket->getRemoteAddress()?strlen(FSendSocket->getRemoteAddress()):0 + 20);
					sprintf(s, "%s:%d", FSendSocket->getRemoteAddress()?FSendSocket->getRemoteAddress():"",
						Owner->getMessengerPort());
					Owner->PrepareDataBlock(s, *DataBuf, *DataLen, (void*&)*DataBuf, *DataLen, Header.dwEncryptID, Header.dwCompressID, Header.dwSealID);
					MCMemFree(s); s = NULL;
				}

				Header.dwSignature = 0x50444945ul;
				Header.dwDataSize = *DataLen + sizeof(Header) + strlen(Info->SMessenger) + sizeof(mcInt32);

				extra = 2 + 4 + 4 + 4;


				Header.dwDataSize += extra; // add router info and separator
		  // write header
				MemStream->Write(&Header, sizeof(Header));
		  // write source address
				i = strlen(Info->SMessenger);
				j = i;
				// this will be used for routing and transactions
				i += extra;
				MemStream->Write(&i, sizeof(i));
				if(j > 0)
					MemStream->Write(Info->SMessenger, j);

				if (!wrt)
				{
					char buf[14]; 
					memset(buf, 0, 14);
					MemStream->Write(buf, sizeof(buf));
				}

				// write actual data
				MemStream->Write(*DataBuf, *DataLen);
				MemStream->SetPos(0);

				MCMemFree(*DataBuf);
				*DataBuf = NULL;

				*DataLen = MemStream->Len();
				*DataBuf = (char*)MCMemAlloc(*DataLen);
				MemStream->SetPos(0);
				MemStream->Read(*DataBuf, *DataLen);
			} FINALLY (
				delete MemStream; MemStream = NULL;
			)
		}
	} FINALLY (
		Owner->FCriticalSection->Leave();
	)

	*AInfo = Info;
	return(res);
}

MCUDPReceiverThread::~MCUDPReceiverThread(void)
{
	delete FRecvSocket; FRecvSocket = NULL;
	if(FRecvBuffer)
	{
		MCMemFree(FRecvBuffer);
		FRecvBuffer = NULL;
	}
}

bool MCUDPReceiverThread::CanExecute(void)
{
	return true;
}

void MCUDPReceiverThread::Execute(void)
{
	fd_set FDRecvSet;
	bool errorFlag = false;

	while ((false == errorFlag) &&
		(false == getTerminated()))
	{
		TRY_BLOCK
		{
			FD_ZERO(&FDRecvSet);
			FD_SET(FSocket->getSocket(), &FDRecvSet);
			FD_SET(FRecvSocket->getSocket(), &FDRecvSet);
                                   
			TSocket highSocketHandle = 0;
			highSocketHandle = FSocket->getSocket() > FRecvSocket->getSocket() ? FSocket->getSocket() : FRecvSocket->getSocket();
			mcInt32 select_res = select((mcInt32)highSocketHandle + 1, &FDRecvSet, NULL, NULL, NULL);

			if (0 == select_res)
				continue;
			if (-1 != select_res)
			{
				if(FD_ISSET(FSocket->getSocket(), &FDRecvSet))
				{
					if(!getTerminated())
					{
						if (!MCSocket::PopFromCtlSocket(FSocket))
						{
							errorFlag = true;
							continue;
						}
					}
				}

				if(FD_ISSET(FRecvSocket->getSocket(), &FDRecvSet))
				{
					if(!ReceiveMessage())
						errorFlag = true;
				}
			}
			else
				errorFlag = true;
    	}
		CATCH_EVERY
		{
			errorFlag = true;
		}
	}
}

void MCUDPReceiverThread::Initialize(MCUDPTransport* AOwner)
{
	MCUDPThread::Initialize(AOwner);
	FRecvSocket = new MCStdSocket();
	FRecvSocket->setLocalPort(Owner->getMessengerPort());
	FRecvSocket->setLocalAddress(Owner->getMessengerAddress());
	WinsockErrorCheck(FRecvSocket->Init(istDatagram));
	if (Owner->getReuseServerPort())
		WinsockErrorCheck(FRecvSocket->ReusePort());	
	WinsockErrorCheck(FRecvSocket->Bind());
	mcInt32 i = 0;
	i = CONST_MAX_MSG_SIZE;
	FRecvBuffer = (mcInt32*)MCMemAlloc(i);
	FRecvBufSize = i;

	for (i = 0; i < Owner->FMulticastList->Length(); i++)
	{
		char* ml = (char *) ((*Owner->FMulticastList)[i]);
		char* mle = ml;
		
		mle = mcstrtok(ml, "|");
		char GroupAddress[16];
		strncpy(GroupAddress, ml, mle - ml); GroupAddress[mle - ml] = 0;
		ml = mle + 1;

		mle = mcstrtok(ml, "|");
		char BindAddress[16];
		strncpy(BindAddress, ml, mle - ml); BindAddress[mle - ml] = 0;
		//ml = mle + 1;

		WinsockErrorCheck(FRecvSocket->AddToMulticastSrv(GroupAddress, BindAddress));
	}

}

mcInt32 getmcInt32(void* x)
{
	mcInt32 temp = 0;
	memmove(&temp, x, sizeof(temp));
	return temp;
}

bool MCUDPReceiverThread::ReceiveMessage(void)
{
	MCInetHeader Header;
	MCMessageInfo *Info = NULL, *CurInfo = NULL;
	MCMemStream* AStream = NULL;
	MCMemStream* Stream = NULL;
	MCMessage Message;
	mcInt32 i = 0;
	char *s = NULL, *s1 = NULL, *SMsg = NULL;
	mcInt32 Received = 0;
	char* RemoteHost = NULL;
	mcUInt32 RemotePort = 0;
	mcInt32 recvres = 0;
	mcInt32 MsgSize = 0;
	void* DataBuf = NULL;
	mcInt32 DataLen = 0;
	char* RemoteAddr = NULL;
	bool parsed = false;
	bool noprocess = false;

  //this is used always, but is related to MC_COMMERCIAL
	char* RouteTo = NULL;
	char* RecvPath = NULL;
//	char TransactCmd = 0;
	char* DQueue = NULL;
  
	Received = FRecvBufSize;
	recvres = FRecvSocket->ReceiveFrom(FRecvBuffer, FRecvBufSize, Received, RemoteHost, RemotePort);
	bool Result = true;
	if(recvres == 0)
	{
		MsgSize = FRecvBuffer[1];
		if(MsgSize == Received && Received < Owner->getMaxMsgSize())
		{
			Stream = new MCMemStream();
			TRY_BLOCK
			{
				Stream->Write(FRecvBuffer, Received);
				Stream->SetPos(0);

				Stream->Read(&Header, sizeof(Header));

				// read the source messenger name
				Stream->Read(&i, sizeof(i));
				if(i > 0)
				{
					s = (char*)MCMemAlloc(i + 1);
					Stream->Read(s, i);
					s[i] = 0;

					//this is used always, but is related to MC_COMMERCIAL
					if ((mcInt32)strlen(s) < i)
					{
						while (true)
						{
							char* tmp_p = (char*) MCMemAlloc(strlen(s) + 1);
							strcpy(tmp_p, s);

							char* ps = s + strlen(s) + 1;
							char* pe = ps + i - strlen(s);
							if (pe - ps < 14)
								break;
							// read TransactCmd
//							TransactCmd = *ps;
							ps++;

							// read RecvPath
							i = getmcInt32(ps);//*((int*)(ps));
							ps += 4;							

							if (pe - ps < i + 4)
								break;

							if (i)
							{
								RecvPath = strCopy(ps, 0, i);
								ps += i;
							}

							// read RouteTo
							i = getmcInt32(ps);//*((int*)(ps));
							ps += 4;							

							if (pe - ps < i + 4)
								break;

							if (i)
							{
								RouteTo = strCopy(ps, 0, i);
								ps += i;
							}

							// read DQueue
							i = getmcInt32(ps);//*((int*)(ps));
							ps += 4;
							if (pe - ps < i)
								break;

							if (i)
								DQueue = strCopy(ps, 0, i);

							MCMemFree(s);
							s = tmp_p;

							parsed = true;
							break;
						}
						if (!parsed)
						{
							MCMemFree(s);
							s = NULL;
						}
					}
				}
				else
					s = NULL;
        // SMsg := RemoteHost + ':' + IntToStr(RemotePort);

				if(ParseRemoteAddr(s, &s1, (mcInt32*)&RemotePort))
				{
					if(strcmp(s1, "0.0.0.0") == 0)
					{
					  MCMemFree(s1);
					  s1 = _strdup(RemoteHost);
					}
					char* tmp = (char*)MCMemAlloc(strlen(s1) + 11);
					sprintf(tmp, "%s:%d", s1, RemotePort);
					RemoteAddr = tmp;
					MCMemFree(s); s = NULL;
					MCMemFree(s1); s1 = NULL;
				}
				else
				{
					RemoteAddr = s;
					s = NULL;
				}

				SMsg = _strdup(RemoteAddr);
        
				// if we are an intermediate node, we must add sender path to RecvPath
				if (RouteTo != NULL)
				{
					char* tmpPath;
					mcInt32 npl;
					if (RecvPath != NULL)
					{
						npl = strlen(Owner->RealTransportName()) + 1 + strlen(s) + 1 + strlen(RecvPath) + 1;
						tmpPath = (char*) MCMemAlloc(npl);
						tmpPath[0] = 0;
						strcpy(tmpPath, Owner->RealTransportName()); strcat(tmpPath, ":");
						strcat(tmpPath, RemoteAddr); strcat(tmpPath, "|"); strcat(tmpPath, RecvPath);
						MCMemFree(RecvPath);
						RecvPath = tmpPath;
					}
					else
					{
						npl = strlen(Owner->RealTransportName()) + 1 + strlen(s) + 1;
						tmpPath = (char*) MCMemAlloc(npl);
						tmpPath[0] = 0;
						strcpy(tmpPath, Owner->RealTransportName()); strcat(tmpPath, ":");
						strcat(tmpPath, RemoteAddr);
						RecvPath = tmpPath;
					}
				}

				// MC_COMMERCIAL. Fixed to not check for intermediate nodes
				if(!RouteTo)
					//check for suitable transformers
					if(!IsTransformerOK(&Header))
					{
						//Owner->FCriticalSection->Leave();
						noprocess = true;
						//return(Result);
					}

				if (!noprocess)
				{
				// discard the message if routing is not allowed
					if(RouteTo
					)
					{
						//Owner->FCriticalSection->Leave();
						noprocess = true;
						//return(Result);
					}
				}

				if (!noprocess)
				{
					if(Stream->Pos() < Stream->Len())
					{
						// allocate buffer for data
						DataLen = Stream->Len() - Stream->Pos();
						DataBuf = (char*)MCMemAlloc(DataLen); // former memory leak
						if(!DataBuf)
							THROW_ERROR(MCError_NotEnoughMemory);
							  
						TRY_BLOCK
						{
							// read the data
							Stream->Read(DataBuf, DataLen);
  
							// decode the message
							if(Owner->UnprepareDataBlock(RemoteAddr, DataBuf, DataLen, DataBuf, DataLen, Header.dwEncryptID, Header.dwCompressID, Header.dwSealID))
							{
								// decoding was successful
								AStream = new MCMemStream();
								TRY_BLOCK 
								{
									AStream->SetPointer((char*)DataBuf, DataLen);
									Message.DataSize = 0;
									Info = new MCMessageInfo(&Message);
									Info->setSMessenger(SMsg); MCMemFree(SMsg); SMsg = NULL;
									Info->setDMessenger(RemoteAddr); MCMemFree(RemoteAddr); RemoteAddr = NULL;
									Info->Timeout = Owner->FMsgTimeout;

									Info->LoadFromStream(AStream);
									{
										if(Info->State == imsDispatching) // we have received an original message
										{
											MCMessenger::SetMessageID(Info);
											if(AStream->Len() - AStream->Pos() < Info->Message.DataSize)
											{
												// discard the message as the data portion is corrupt
												delete Info; Info = NULL;
												//Owner->FCriticalSection->Leave();
												noprocess = true;
												//return(Result);
											}
											else
											if(Info->Message.DataSize > 0)
											{
												Info->Message.Data = MCMemAlloc(Info->Message.DataSize);
												AStream->Read(Info->Message.Data, Info->Message.DataSize);
											}
											else
												Info->Message.Data = NULL;
											Owner->MessageReceived(Info);
										}
										else // we have received a reply
										{
											// remove the message from the list of pending messages

											Owner->FCriticalSection->Enter();
											TRY_BLOCK
											{
                        //Entry.ClientID := ExtractClientID(S);
                        //find original request
												CurInfo = NULL;
												for(i = 0;i < Owner->FOutgoingQueue->Length();i++)
													if(((MCMessageInfo*)(*Owner->FOutgoingQueue)[i])->OrigID == Info->OrigID)
													{
														CurInfo = (MCMessageInfo*)(*Owner->FOutgoingQueue)[i];
														Owner->FOutgoingQueue->Del(i);
														break;
													}
											}
											FINALLY (
												Owner->FCriticalSection->Leave();
											)
											//original request is found?
											if(!CurInfo) // the message was cancelled - well, destroy the reply
											{
												delete Info;
												Info = NULL;
											}
											else
											{
												// we have to copy result data and signal about reply
												if (CurInfo->Message.DataType != bdtConst)
												{
													if(CurInfo->Message.Data)
													{
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
												delete Info; Info = NULL;
												Owner->ReplyReceived(CurInfo);
											}
										}
									}
								} FINALLY (
									if(AStream)
										AStream->SetPointer(NULL, 0);
									delete AStream;
								)
							}
						} FINALLY (
							MCMemFree(DataBuf); DataBuf = NULL;
						)
					}
				}
			} FINALLY (
				delete Stream;
				if (RecvPath)
					MCMemFree(RecvPath);
				if (DQueue)
					MCMemFree(DQueue);
				if (RouteTo)
					MCMemFree(RouteTo);
				if (SMsg)
					MCMemFree(SMsg);
				if (RemoteAddr)
					MCMemFree(RemoteAddr);
			)
		}
	}
	else
#ifdef _WIN32	
		Result = (recvres == WSAEMSGSIZE);
#else
		Result = true;
#endif	
	return(Result);
}

bool MCUDPReceiverThread::IsTransformerOK(MCInetHeader* Header)
{
  bool r = true;
	Owner->FLinkCriticalSection->Enter();

	if (0 != Header->dwCompressID)
	{
		if (NULL == Owner->getCompressor() ||
			Owner->getCompressor()->GetID() != Header->dwCompressID)
		{
			Header->dwCompressID = 0;
			r = false;
		}
	}

	if (0 != Header->dwEncryptID)
	{
		if (NULL == Owner->getEncryptor() ||
			Owner->getEncryptor()->GetID() != Header->dwEncryptID)
		{
			Header->dwEncryptID = 0;
			r = false;
		}
	}

	if (0 != Header->dwSealID)
	{
		if (NULL == Owner->getSealer() ||
			Owner->getSealer()->GetID() != Header->dwSealID)
		{
			Header->dwSealID = 0;
			r = false;
		}
	}

	Owner->FLinkCriticalSection->Leave();
	return r;
}

MCUDPThread::MCUDPThread(bool CreateSuspended)
:MCThread(CreateSuspended), FSocket(NULL), FKickSocket(NULL), Owner(NULL)
{
}

MCUDPThread::~MCUDPThread()
{
	delete FSocket; FSocket = NULL;
	delete FKickSocket; FKickSocket = NULL;
}

void MCUDPThread::Initialize(MCUDPTransport* AOwner)
{
	Owner = AOwner;

	FSocket = new MCStdSocket();
	FSocket->setLocalPort(0);
	FSocket->setLocalAddress("127.0.0.1");//"0.0.0.0"; // changed by EM on 04.01.2008
	WinsockErrorCheck(FSocket->Init(istDatagram));
	WinsockErrorCheck(FSocket->Bind());
	FKickSocket = new MCStdSocket();
	WinsockErrorCheck(FKickSocket->Init(istDatagram));
}

void MCUDPThread::KickThread(bool MessageFlag)
{
	char c = (char) MessageFlag;
	mcInt32 i;
	char localHost[20] = "127.0.0.1";
	FKickSocket->SendTo(&c, 1, i, localHost,
		FSocket->getLocalPort());
}

bool MCUDPThread::ParseRemoteAddr(const char* S, char** IP, mcInt32* Port)
{
  char *c = strchr((char*) S, ':');
	if(!S || !(c))
		return(false);
	mcInt32 i = (mcInt32)(c - S) + 1;
	if (NULL != IP)
	{
		*IP = (char*)MCMemAlloc(i);
		strncpy(*IP, S, i);
		(*IP)[i-1] = 0;
	}
	if (NULL != Port)
		*Port = atoi(++c);
  return(true);
}

#ifdef USE_NAMESPACE
}
#endif
