//====================================================
//                                                    
//  EldoS MsgConnect                                 
//                                                    
//   Copyright (c) 2001-2010, EldoS                   
//                                                    
//====================================================
 
#ifndef __MCMMF__
#define __MCMMF__


#include "MC.h"
#include "MCBase.h"
#include "MCSyncs.h"
#include "MCThreads.h"
#include "MCStream.h"

#ifdef USE_NAMESPACE
namespace MsgConnect
{
#endif


#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
#   include <unistd.h>
#   include <sys/mman.h>
#   include <sys/types.h>
#   include <sys/stat.h>
#   include <fcntl.h>
#endif


typedef struct {
	mcInt32 dwMMFSize;
	mcInt32 dwErrorCode;
	mcInt32 bIsFirst;
	mcInt32 dwDataSize;   // total size of the data
	mcInt32 dwBlockStart; // the number of byte on which current block starts
	mcInt32 dwBlockSize;  // size of current block
	// here goes the data
} MCMMFHeader;

/*
class EMCMMFError: public EMCError {
protected:
	static char*    GetMessageFromErrorCode(mcInt32 ErrorCode);
public:
    EMCMMFError(mcInt32 acode): EMCError(acode) { }
};
*/
  
class MCMMFSenderThread;
class MCMMFReceiverThread;

class MCMMFTransport:public MCSimpleTransport {
friend class MCMMFSenderThread;
friend class MCMMFReceiverThread;
protected:
	//MCMutex*				FCriticalSection;
	char*                   FMessengerName;
	MCMMFReceiverThread*    FReceiverThread;
	MCMMFSenderThread*      FSenderThread;

	virtual bool			DeliverMessage(char* DestAddress,MCMessageInfo* Info);
	virtual void			DoSetActive(void);
	
    virtual void			KickSender(bool MessageFlag);
    virtual void			CancelMessageInSender(MCMessageInfo* Info);
    virtual bool			WasCancelled(MCMessageInfo* Info);
    virtual void			SetInfoSMessenger(MCMessageInfo* Info);

    virtual MCMMFSenderThread* CreateSenderThread(bool x);
    virtual MCMMFReceiverThread* CreateReceiverThread(bool x);
public:
	MCMMFTransport(void);
	virtual ~MCMMFTransport(void);

	/*
	The name of MMF that is used to transfer the messages 
	from other processes on the same computer. 
	Can't be empty.
	*/
	char*           getMessengerName(void);
	void            setMessengerName(const char* Value);
};
  
class MCMMFSenderThread: public MCThread {
private:
friend class MCMMFTransport;
	__int64             FCancelID;
	MCMessageInfo*		FSendingInfo;
	MCEvent*            FStopEvent;
	MCMMFTransport*     FOwner;
protected:
	virtual void    Execute(void);
	virtual bool	CanExecute(void);
	void            SendMessage(void);
public:
    MCMMFSenderThread(bool CreateSuspended): MCThread(CreateSuspended) { }
	virtual ~MCMMFSenderThread(void);
	void            Initialize(MCMMFTransport* Owner);
};
  
class MCMMFReceiverThread:public MCThread {
friend class MCMMFTransport;
private:
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
	char*               FMn;
	mcInt32                 FMapping;
#else
#endif
	MCEvent*            FStopEvent;
	MCMMFTransport*     FOwner;
	/*
	This event is used to signal possibility to send the data.
	Once a sender grabs the event, nobody except the received is able to write 
	to MMF.
	*/
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
	MCEvent*            FMMFFreeMutex;
	//This event is used to signal that reply has been received by sender
	MCEvent*            FReceiptEvent;

	//This event is used to signal about incoming data available in the MMF
	MCEvent*            FRequestEvent;

    //This event is used to signal about reply available in the MMF 
	MCEvent*            FReplyEvent;
#else
#endif
protected:
	virtual void    Execute(void);
	virtual bool	CanExecute(void);
	void            ReceiveMessage(void);
	void            Initialize(MCMMFTransport* Owner);
public:
	MCMMFReceiverThread(bool CreateSuspended);
	virtual ~MCMMFReceiverThread(void);
};

#ifdef USE_NAMESPACE
}
#endif


#endif
