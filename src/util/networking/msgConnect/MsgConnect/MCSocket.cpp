//====================================================
//                                                    
//  EldoS MsgConnect                                 
//                                                    
//   Copyright (c) 2001-2010, EldoS                   
//                                                    
//====================================================
#include "MC.h"
#include "MCUtils.h"
#include "MCSocket.h"
 
#ifdef USE_NAMESPACE
namespace MsgConnect
{
#endif


// ***************************** MCInetTransport ****************************

MCSocketTransport::MCSocketTransport()
{
    FDefaultTransportName = "SOCKET";
}


MCSocketTransport::~MCSocketTransport()
{
    ;
}

void MCSocketTransport::NotifyJob(MCInetTransportJob* Job, bool MessageFlag)
{
    MCSocketTransportJob* Thr = (MCSocketTransportJob*)Job;
    if (NULL != Thr)
    {
        if(!(Thr->getIsSending() || Thr->getInHandshake()))
	    {
	        KickEntrySocket(Job->getEntry(), MessageFlag);
        }
    }
}

void MCSocketTransportJob::MakeFetchPacket(void)
{
}

bool MCSocketTransportJob::InitializeConnection()
{
	if (MCInetTransportJob::InitializeConnection())
	{
		FHandshakeWaitingReply = false;
		return true;
	}
    else
		return false;
}

MCMessageInfo* MCSocketTransportJob::MakeEmptyReply(MCMessageInfo* Info)
{
    return NULL;
}

MCInetTransportJob* MCSocketTransport::CreateTransportJob(void)
{
    MCInetTransportJob* trans = new MCSocketTransportJob();
	trans->AdjustBufferSize(FIncomingBufferSize, FOutgoingBufferSize);
	trans->AdjustSpeedLimits(FIncomingSpeedLimit, FOutgoingSpeedLimit);
	return trans;
}

MCInetConnectionEntry* MCSocketTransport::CreateConnectionEntry(void)
{
    return new MCInetConnectionEntry();
}

MCSocketTransportJob::MCSocketTransportJob(void)
{
    FInHandshake = false;
}

MCSocketTransportJob::~MCSocketTransportJob()
{
}

bool MCSocketTransportJob::PostConnectionStep(void)
{
	//if (FTransporter->getDirection() == isdOutgoing)
	//	ShowDebugInfo("Outgoing %d handshake calling\n", GetCurrentThreadId());
	//else
	//	ShowDebugInfo("Incoming %d handshake calling\n", GetCurrentThreadId());

	bool res = PerformHandshake();

	//if (FTransporter->getDirection() == isdOutgoing)
	//	ShowDebugInfo("Outgoing %d handshake done\n", GetCurrentThreadId());
	//else
	//	ShowDebugInfo("Incoming %d handshake done\n", GetCurrentThreadId());

	FInHandshake = false;
    FHandshakeWaitingReply = false;
    return res;
}

void MCSocketTransportJob::SetTransporterAddress(MCInetConnectionEntry* Ent, MCSocket* Transp)
{
    mcInt32 ClientID = 0, ConnID = 0; 
	mcInt32 Port = 0; char* IP = NULL;
    if (ParseRemoteAddr(FEntry->getRemoteAddress(), &ClientID, &ConnID, &IP, &Port) == true)
    {
        Transp->setRemoteAddress(IP);
        Transp->setRemotePort(Port);
    }
    MCMemFree(IP); IP = NULL;
}

bool MCSocketTransportJob::NeedCloseCompletedExchg(void)
{
    return false;
}



bool MCSocketTransportJob::HSPacketSent(void)
{
	bool r = false;
	try {
		FEntry->getCS()->Enter();
		try {
			r = FEntry->getOutgoingStream() &&
                (FEntry->getOutgoingStream()->Len() == FEntry->getOutgoingStream()->Pos());
			if(r)
			{
				setIsSending(false);
				delete FEntry->getOutgoingStream();
				FEntry->setOutgoingStream(NULL);
			}
		} FINALLY (
			FEntry->getCS()->Leave();
		)
#ifdef USE_CPPEXCEPTIONS
	} catch(...) {
#else
	} __except(1) {
#endif
	}
	return r;
}

bool MCSocketTransportJob::HSReplyReceived(bool& Success)
{
	MCInetHeader Header;
	mcInt32 i;
	bool r = false;

	try {
		FEntry->getCS()->Enter();
		try {
			r = FEntry->getIncomingStream() &&
				(FEntry->getMsgSize() > 0) &&
				(FEntry->getIncomingStream()->Len() == FEntry->getMsgSize());
			if(r)
			{
				FEntry->getIncomingStream()->SetPos(0);
  
				// read the header
				FEntry->getIncomingStream()->Read(&Header, sizeof(Header));
  
				// read 0 (source messenger name, as it is supposed to be)
				FEntry->getIncomingStream()->Read(&i, sizeof(i));
//                char SMsg[128];
//                FEntry->getIncomingStream()->Read(SMsg, i);


				Success = ((Header.dwEncryptID != -1) &&
					(Header.dwCompressID != -1) &&
					(Header.dwSealID != -1));

				if (Success)
				{

					Success = Success && (((Header.dwEncryptID == 0) && ((!FOwner->FNoTransformerFallback) || (FOwner->FEncryptor == NULL)) ||
						((FOwner->FEncryptor != NULL) && (FOwner->FEncryptor->GetID() == Header.dwEncryptID))));

					Success = Success && (((Header.dwCompressID == 0) && ((!FOwner->FNoTransformerFallback) || (FOwner->FCompressor == NULL)) ||
						((FOwner->FCompressor != NULL) && (FOwner->FCompressor->GetID() == Header.dwCompressID))));

					Success = Success && (((Header.dwSealID == 0) && ((!FOwner->FNoTransformerFallback) || (FOwner->FSealer == NULL)) ||
						((FOwner->FSealer  != NULL) && (FOwner->FSealer->GetID() == Header.dwSealID))));

				}


				if (Success)
				{
					FEntry->setEncryptID(Header.dwEncryptID);
					FEntry->setCompressID(Header.dwCompressID);
					FEntry->setSealID(Header.dwSealID);
				}
  
				FEntry->setMsgSize(0);
				setIsReceiving(false);
				delete FEntry->getIncomingStream(); FEntry->setIncomingStream(NULL);
				
			}
		} FINALLY (
			FEntry->getCS()->Leave();
		)
#ifdef USE_CPPEXCEPTIONS
	} catch(...) {
#else
	} __except(1) {
#endif
	}
	return r;
}

bool MCSocketTransportJob::HSRequestReceived(void)
{
	MCInetHeader Header;
	mcInt32 i;
	char* SMessenger;
	bool r = false;

	TRY_BLOCK
    {
		FEntry->getCS()->Enter();
		TRY_BLOCK
        {
			r = FEntry->getIncomingStream() &&
				(FEntry->getMsgSize() > 0) &&
				(FEntry->getIncomingStream()->Len() == FEntry->getMsgSize());
			if(r)
			{
				FEntry->getIncomingStream()->SetPos(0);
  
				// read the header
				FEntry->getIncomingStream()->Read(&Header, sizeof(Header));
  
				// read source messenger name
				FEntry->getIncomingStream()->Read(&i, sizeof(i));
			
				if(i > 0)
				{
					SMessenger = (char*)MCMemAlloc(i + 1);
					FEntry->getIncomingStream()->Read(SMessenger, i);
					SMessenger[i] = 0;
					
                    //FEntry->setRemoteAddress(SMessenger);
                    MCASSERT(SMessenger);
					MCMemFree(SMessenger);
				}

				FEntry->setMsgSize(0);
  
				PrepareReplyForHandshake(&Header);
				setIsReceiving(false);
				delete FEntry->getIncomingStream(); FEntry->setIncomingStream(NULL);				
			}
		} FINALLY (
			FEntry->getCS()->Leave();
		)
    }
    CATCH_EVERY
    {

    }
	return r;
}

bool MCSocketTransportJob::PerformHandshake(void)
{
	timeval TimeVal;
	timeval* PTV;
	mcInt32 select_res;
	fd_set FDSendSet, FDRecvSet;
	mcInt32 i;
	bool r = false;

	while(true)
	{
		if(!getIsSending())
		{
			// if this is client
			if(FTransporter->getDirection() == isdOutgoing)
			{
				// if we are not waiting for reply, then we have just started.
				if(!FHandshakeWaitingReply)
				{
					// create a packet for sending.
					if(!PrepareMessageForHandshake())
					{
						//if(FTransporter->getDirection() == isdIncoming)
						//	OutputDebugString("Incoming handshake failed (Prepare failed)\n");
						//else
						//	OutputDebugString("Outgoing handshake failed (Prepare failed)\n");
						return(r);
					}
					else
					{
						setIsSending(true);
						continue;
					}
				}
			}
		}
  
		if (FTransporter->HasBufferedIncomingData())
		{
			if (ReceiveData())
			{
				if(FTransporter->getDirection() == isdIncoming)
				{
					HSRequestReceived();
					continue;
				}
				else
				if(FTransporter->getDirection() == isdOutgoing)
				{
					if (HSReplyReceived(r))
					{
						break;
					}
				}
				continue;
			}
			else
				break;
		}
		else
		{
  
			// prepare parameters for select()
  
			// initialize sets for select
			FD_ZERO(&FDSendSet);
			FD_ZERO(&FDRecvSet);
  
			FD_SET(FSocket->getSocket(), &FDRecvSet);
  
			// decide whether we expect to get something
			// We read from socket if (a) we are in receiving state, or
			// (b) we are server that is waiting for client's request, or
			// (c) we are client which is getting reply
			TSocket highSocketHandle;
			if(getIsReceiving() ||
				((FTransporter->getDirection() == isdIncoming) && (!getIsSending())) ||
				((FTransporter->getDirection() == isdOutgoing) && FHandshakeWaitingReply))
			{
				FD_SET(FTransporter->getSocket(), &FDRecvSet);
				highSocketHandle = FSocket->getSocket() > FTransporter->getSocket() ? FSocket->getSocket() : FTransporter->getSocket();
			}
			else
			{
				highSocketHandle = FSocket->getSocket();
			}
  
			i = FOwner->getInactivityTime();
			if(i > 0)
			{
				TimeVal.tv_sec = i / 1000;
				TimeVal.tv_usec = (i % 1000) * 1000;
				PTV = &TimeVal;
			}
			else
				PTV = NULL;
  
			if(getIsSending())
			{
				FD_SET(FTransporter->getSocket(), &FDSendSet);
//#ifdef __GNUC__
//				select_res = select(FD_SETSIZE, &FDRecvSet, &FDSendSet, NULL, PTV);
//#else
				select_res = select((mcInt32)highSocketHandle + 1, &FDRecvSet, &FDSendSet, NULL, PTV);
//#endif
			}
			else
//#ifdef __GNUC__
//				select_res = select(FD_SETSIZE, &FDRecvSet, NULL, NULL, PTV);
//#else
				select_res = select((mcInt32)highSocketHandle + 1, &FDRecvSet, NULL, NULL, PTV);
//#endif
  
			switch(select_res) {
				case -1:
				case 0: // return thread to the pool or wherever it goes ...
					return r;
				default:
					// CtlSocket signaled that we have a message to send
					if(FD_ISSET(FSocket->getSocket(), &FDRecvSet))
					{
						if(!getTerminated())
						{
							if (!MCSocket::PopFromCtlSocket(FSocket))
								return r;
						}
					}
  
					// we have something to receive from the remote side
					if(FD_ISSET((mcUInt32)FTransporter->getSocket(), &FDRecvSet))
					{
						if (FTransporter->PreprocessIncomingData())
						{
							if(ReceiveData())
							{
								if(FTransporter->getDirection() == isdIncoming)
								{
									HSRequestReceived();
									continue;
								}
								else
								if(FTransporter->getDirection() == isdOutgoing)
								{
									if (HSReplyReceived(r))
									{
										/*
										if (r)
											OutputDebugString("Outgoing handshake OK\n");
										else
											OutputDebugString("Outgoing handshake failed\n");
										*/
										return r; // break;
									}
								}
								continue;
							}
							else
							{
								// give up due to connection error
								/*if(FTransporter->getDirection() == isdIncoming)
									OutputDebugString("Incoming handshake failed\n");
								else
									OutputDebugString("Outgoing handshake failed\n");
								*/
								return(r); // break;
							}
						}
					}
					// we can send
					if(FD_ISSET(FTransporter->getSocket(), &FDSendSet))
					{
						if (FTransporter->HasBufferedOutgoingData() || (getIsSending() && SendData()))
						{	
							if (FTransporter->PostprocessOutgoingData())
							{
								if (getIsSending())
								{
									if(FTransporter->getDirection() == isdIncoming)
									{
										if(HSPacketSent() && !FTransporter->HasBufferedOutgoingData())
										{
											return true; // break;
										}
									}
									else
									if(FTransporter->getDirection() == isdOutgoing)
									{
										if(HSPacketSent())
										{
											FHandshakeWaitingReply = true;
											continue;
										}
									}
								}
							}
						}
						else
						if (getIsSending())
						{
								return false;
						}
					}
			}
		}
		if(getTerminated())
			break;
	}
	return(r);
}


/*
This method is called from PerformHandshake to create a request data block 
for handshake. Method returns true if the block was prepared and false if 
there was some error.
*/

bool MCSocketTransportJob::PrepareMessageForHandshake(void)
{
	mcInt32 i;
	MCMemStream* MemStream;
	MCInetHeader Header;
	//char* SMessenger;
	bool r = false;

	TRY_BLOCK
    {
		FOwner->FLinkCriticalSection->Enter();
		try {
			if(FOwner->FCompressor)
				Header.dwCompressID = FOwner->FCompressor->GetID();
			else
				Header.dwCompressID = 0;
			if(FOwner->FEncryptor)
				Header.dwEncryptID = FOwner->FEncryptor->GetID();
			else
				Header.dwEncryptID = 0;
			if(FOwner->FSealer)
				Header.dwSealID = FOwner->FSealer->GetID();
			else
				Header.dwSealID = 0;
		} FINALLY (
			FOwner->FLinkCriticalSection->Leave();
		)
		MemStream = new MCMemStream();
  
		FEntry->getCS()->Enter();
		try {
			// prepare source address
            i = 0;
			// prepare data block
			Header.dwSignature = 0x50444945ul;
			Header.dwDataSize = sizeof(Header) + sizeof(i) + i;
			
			FEntry->setOutgoingStream(MemStream);
			// write outgoing data
			// write header
			MemStream->Write(&Header, sizeof(Header));
  
			// write source address
			MemStream->Write(&i, sizeof(i));
			
			MemStream->SetPos(0);
  
			setIsSending(true);
		} FINALLY (
			FEntry->getCS()->Leave();
		)
		r = true;
    }
    CATCH_EVERY
    {
    }
	return r;
}


/*
This method is called from HSRequestReceived create a reply data block for 
handshake. Method returns true if the block was prepared and false if there 
was some error.
*/

bool MCSocketTransportJob::PrepareReplyForHandshake(MCInetHeader* RqHeader)
{
	mcInt32 i;
	MCMemStream* MemStream;
	MCInetHeader Header;
	bool r = false;

	TRY_BLOCK 
    {
		FOwner->FLinkCriticalSection->Enter();
		TRY_BLOCK 
        {
			if(FOwner->FCompressor && (FOwner->FCompressor->GetID() == RqHeader->dwCompressID))
				Header.dwCompressID = RqHeader->dwCompressID;
			else
			if ((FOwner->getNoTransformerFallback()) && 
			   ((FOwner->FCompressor != NULL) && (FOwner->FCompressor->GetID() != RqHeader->dwCompressID)))
				Header.dwCompressID = -1;
			else      
				Header.dwCompressID = 0;

			if (FOwner->FEncryptor && (FOwner->FEncryptor->GetID() == RqHeader->dwEncryptID))
				Header.dwEncryptID = RqHeader->dwEncryptID;
			else
			if ((FOwner->getNoTransformerFallback()) && 
			   ((FOwner->FEncryptor != NULL) && (FOwner->FEncryptor->GetID() != RqHeader->dwEncryptID))) 
				Header.dwEncryptID = -1;
			else      
				Header.dwEncryptID = 0;
			
			if(FOwner->FSealer && (FOwner->FSealer->GetID() == RqHeader->dwSealID))
				Header.dwSealID = RqHeader->dwSealID;
			else
			if ((FOwner->getNoTransformerFallback()) && 
			   ((FOwner->FSealer != NULL) && (FOwner->FSealer->GetID() != RqHeader->dwSealID)))
				Header.dwSealID = -1;
			else      
				Header.dwSealID = 0;

		} FINALLY (
			FOwner->FLinkCriticalSection->Leave();
		)
		MemStream = new MCMemStream();
  
		FEntry->setCompressID(Header.dwCompressID);
		FEntry->setEncryptID(Header.dwEncryptID);
		FEntry->setSealID(Header.dwSealID);
  
		//FEntry->getCS()->Enter();
        //TRY_BLOCK {

            // prepare data block
			Header.dwSignature = 0x50444945ul;
			Header.dwDataSize = sizeof(Header) + sizeof(mcInt32);// + strlen(FEntry->getRemoteAddress());
			FEntry->setOutgoingStream(MemStream);
  
			// write outgoing data
			// write header
			FEntry->getOutgoingStream()->Write(&Header, sizeof(Header));
  
			// write source address
			i = 0;//strlen(FEntry->getRemoteAddress());
			FEntry->getOutgoingStream()->Write(&i, sizeof(i));
            if (i > 0)
                FEntry->getOutgoingStream()->Write(FEntry->getRemoteAddress(), i);
			
            FEntry->getOutgoingStream()->SetPos(0);
  
			setIsSending(true);
		//} FINALLY ( {}
		//	FEntry->getCS()->Leave();
		//)
		r = true;
	}
    CATCH_EVERY 
    {
	}
	return r;
}

bool MCSocketTransportJob::ReceiveData(void)
{
	mcInt32 Received;
	mcInt32 MsgSize;
	mcInt32 ToRecv;
	bool r = false;
    bool tooBigPacket = false;

	TRY_BLOCK 
    {
		FEntry->getCS()->Enter();
		TRY_BLOCK
        {
			if(!getIsReceiving())
			{
				// initiate new message
				if(FTransporter->Receive(&FIncomingBuffer[0 - FEntry->getMsgSize()], sizeof(mcInt32) * 2, Received) == 0)
				{
					if(Received > 0)
					{
						FblInSessionTransferred += Received;
						FblInMsgTransferred += Received;
						FblInSecTransferred += Received;

						if((Received + (-FEntry->getMsgSize())) < (mcInt32)(sizeof(mcInt32) * 2))
						{
							FEntry->setMsgSize(FEntry->getMsgSize() - Received);
							// here we will get negative value. Negative value is used to signal
							// that we have not yet allocated an incoming stream and
							// -Entry.MsgSize is the amount of data already in incoming buffer
							r = true;
	//						return(r);
							RETURN
						}
                    
						mcUInt32 readSignature = *((mcUInt32*)FIncomingBuffer);
						if (readSignature != 0x50444945ul)
						{
							FEntry->setMsgSize(0);
							r = false;
							RETURN
						}
  
						// calculate total size of already received data
						Received = -FEntry->getMsgSize() + Received;
  
						// set message size
						MsgSize = *((mcUInt32*)&FIncomingBuffer[sizeof(mcInt32)]);
						
						/*
						char bubuf[50];
						sprintf(bubuf, "MsgSize: %d \n", MsgSize);
						ShowDebugInfo(bubuf);
						*/
						
						FEntry->setMsgSize(MsgSize);
  
						// check owner's properties, then allocate stream
						FEntry->getCS()->Leave();
						FOwner->FCriticalSection->Enter();
						TRY_BLOCK 
						{
							FEntry->getCS()->Enter();
							TRY_BLOCK 
							{
								if(FOwner->getMaxMsgSize() != 0 && FEntry->getMsgSize() > FOwner->getMaxMsgSize())
								{
									FEntry->setMsgSize(0);
									tooBigPacket = true;
									//FTransporter->Close(false);
									r = false;
								}
								else
								{
									// allocate stream for incoming data
									if (FEntry->getIncomingStream() != NULL)
									{
										delete FEntry->getIncomingStream(); FEntry->setIncomingStream(NULL);
									}
									if(!getInHandshake() && FOwner->getUseTempFilesForIncoming() && (MsgSize > (mcInt32)FOwner->getIncomingMemoryThreshold()))
										FEntry->setIncomingStream(new MCTmpFileStream(FOwner->getTempFilesFolder(), MsgSize));
									else
										FEntry->setIncomingStream(new MCMemStream());
									MCASSERT(FEntry->getIncomingStream() != NULL);
								}
							} FINALLY (
								FEntry->getCS()->Leave();
							)
						} FINALLY (
							FOwner->FCriticalSection->Leave();
						)

						FEntry->getCS()->Enter();
						if (!tooBigPacket)
						{
							FEntry->getIncomingStream()->Write(FIncomingBuffer, Received);
							setIsReceiving(true);
							r = true;
						}
					}
				}
				else
				{
					//char Buffer [150];
					//sprintf(Buffer, "ReceiveData failed with error %d\n", err);
					//OutputDebugString(Buffer);
				}
			}				
			else // already receiving
			{
				MCASSERT(FTransporter != NULL);
				MCASSERT(FEntry->getIncomingStream() != NULL);

				ToRecv = FEntry->getMsgSize() - FEntry->getIncomingStream()->Len();
				
				if(ToRecv > (mcInt32) FblToRecv)
					ToRecv = FblToRecv;

				if(FTransporter->Receive(FIncomingBuffer, ToRecv, Received) == 0)
				{
					FblInSessionTransferred += Received;
					FblInMsgTransferred += Received;
					FblInSecTransferred += Received;

					FEntry->getIncomingStream()->Write(FIncomingBuffer, Received);
					r = true;
				}
			}
		} FINALLY (
			FEntry->getCS()->Leave();
		)
    }
    CATCH_EVERY
    {
    }
	return r;
}

bool MCSocketTransportJob::SendData(void)
{
	mcInt32 ToSend, Sent;
	bool r = false;

	FEntry->getCS()->Enter();
    TRY_BLOCK
    {   
		if(getIsSending())
		{
			TRY_BLOCK
            {
				if(FEntry->getOutgoingStream()->Len() - FEntry->getOutgoingStream()->Pos() < (mcInt32)FblToSend)
					ToSend = FEntry->getOutgoingStream()->Len() - FEntry->getOutgoingStream()->Pos();
				else
					ToSend = FblToSend;
				FEntry->getOutgoingStream()->Read(FOutgoingBuffer, ToSend);
				if(FTransporter->Send(FOutgoingBuffer, ToSend, Sent) == 0)
				{
					FblOutSessionTransferred += Sent;
					FblOutMsgTransferred += Sent;
					FblOutSecTransferred += Sent;

					FEntry->getOutgoingStream()->SetPos(FEntry->getOutgoingStream()->Pos() - ToSend + Sent);
					r = true;
				}
			} FINALLY (
                ;				
			)
		}
    }
    CATCH_EVERY
    {
    }
    FEntry->getCS()->Leave();
	
    return r;
}

bool MCSocketTransportJob::getInHandshake(void)
{
	return FInHandshake;
}

void MCSocketTransportJob::setInHandshake(bool Value)
{
	FInHandshake = Value;
}


#ifdef USE_NAMESPACE
}
#endif

