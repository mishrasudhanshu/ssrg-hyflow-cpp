//====================================================
//                                                    
//  EldoS MsgConnect                                 
//                                                    
//   Copyright (c) 2001-2010, EldoS                   
//                                                    
//====================================================

#ifndef MCUDP_H
#define MCUDP_H
 
#include "MC.h"
#include "MCBase.h"
#include "MCSyncs.h"
#include "MCThreads.h"
#include "MCStream.h"
#include "MCSock.h"
#include "MCSocketBase.h"

#ifdef USE_NAMESPACE
namespace MsgConnect
{
#endif

const int CONST_MAX_MSG_SIZE = 512;

class MCUDPTransport;
class MCUDPSenderThread;
class MCUDPReceiverThread;

class MCUDPThread : public MCThread {
friend class MCUDPTransport;
friend class MCUDPSenderThread;
friend class MCUDPReceiverThread;
protected:
	
    MCSocket* FSocket;
	MCSocket* FKickSocket;
	MCUDPTransport* Owner;
	void KickThread(bool MessageFlag);
	static bool	ParseRemoteAddr(const char* S, char** IP, mcInt32* Port);

public:
	MCUDPThread(bool CreateSuspended);
	virtual ~MCUDPThread();
	virtual void Initialize(MCUDPTransport* AOwner);
};

class MCUDPSenderThread : public MCUDPThread {
friend class MCUDPTransport;
protected:
    __int64 FCancelID;
	MCMessageInfo*	FSendingInfo;

	MCStdSocket*	FSendSocket;
	
    
	virtual void Execute(void);
	virtual bool CanExecute(void);
	bool         SendMessage(void);
	void         MessageFailed(mcInt32 reason, MCMessageInfo* Info, MCMessageState InfoState);
	bool         PrepareMessage(MCMessageInfo** Info, char** DataBuf,
		mcInt32* DataLen, MCMessageState* InfoState);
public:
	MCUDPSenderThread(bool CreateSuspended);
	virtual ~MCUDPSenderThread(void);
	virtual void Initialize(MCUDPTransport* AOwner);
};

class MCUDPReceiverThread : public MCUDPThread {
friend class MCUDPTransport;
protected:

	MCStdSocket* FRecvSocket;
	mcInt32* FRecvBuffer;
	mcInt32 FRecvBufSize;
	MCUDPReceiverThread(bool CreateSuspended): MCUDPThread(CreateSuspended), FRecvSocket(NULL), FRecvBuffer(NULL) {};

	virtual void Execute(void);
	virtual bool CanExecute(void);
	bool         ReceiveMessage(void);
	bool         IsTransformerOK(MCInetHeader* Header);
public:
	virtual ~MCUDPReceiverThread(void);
	virtual void Initialize(MCUDPTransport* AOwner);
};

class MCUDPTransport : public MCSimpleTransport {
friend class MCUDPSenderThread;
friend class MCUDPReceiverThread;
protected:
	bool                    FFailOnInactive;
	char*                   FMessengerAddress;
	mcUInt32				FMessengerPort;
	MCInetTransportMode     FTransportMode;
	MCUDPReceiverThread*    FReceiverThread;
	MCUDPSenderThread*      FSenderThread;
	MCStringList*			FMulticastList;
	bool					FReuseServerPort;
	virtual bool            DeliverMessage(char* DestAddress, MCMessageInfo* Info);
	virtual void            DoSetActive(void);

    virtual void			KickSender(bool MessageFlag);
    virtual void			CancelMessageInSender(MCMessageInfo* Info);
    virtual bool			WasCancelled(MCMessageInfo* Info);
    virtual void			SetInfoSMessenger(MCMessageInfo* Info);
	mcInt32					FindMulticastAddress(const char* Value);
	
    virtual MCUDPSenderThread* CreateSenderThread(bool x);
    virtual MCUDPReceiverThread* CreateReceiverThread(bool x);
public:
	MCUDPTransport(void);
	virtual ~MCUDPTransport();
	void					JoinMulticastGroup(const char* GroupAddress, const char* BindAddress, unsigned char TTL, bool Loop);
    void					LeaveMulticastGroup(const char* GroupAddress);

	mcUInt32                getMessengerPortBound(void);

	void                    setFailOnInactive(bool Value);
	bool                    getFailOnInactive(void);
	void                    setMessengerAddress(char* Value);
	char*                   getMessengerAddress(void);
	void                    setMessengerPort(unsigned Value);
	mcUInt32                getMessengerPort(void);
	void                    setTransportMode(MCInetTransportMode Value);
	MCInetTransportMode     getTransportMode(void);
	void                    setRoutingAllowed(bool Value);
	bool                    getRoutingAllowed(void);
	void                    setReuseServerPort(bool Value);
    bool                    getReuseServerPort(void);
    
};

class HackMCMessenger : public MCMessenger {};

#ifdef USE_NAMESPACE
}
#endif


#endif
