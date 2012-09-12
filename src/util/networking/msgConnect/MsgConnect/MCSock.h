//====================================================
//                                                    
//  EldoS MsgConnect                                 
//                                                    
//   Copyright (c) 2001-2010, EldoS                   
//                                                    
//====================================================

#ifndef __MCSOCK__
#define __MCSOCK__
 
#include "MC.h"
#include "MCBase.h"
#include "MCSyncs.h"
#include "MCThreads.h"

#if defined(WIN32) && !defined(_WIN32_WCE)
#if (_MSC_VER < 1300)// && !defined(__BORLANDC__)
#	include <ws2tcpip.h>
#endif
#endif

#ifdef USE_NAMESPACE
namespace MsgConnect
{
#endif


typedef enum {issNotASocket = 0, issInitializing, issInitialized, issBound, issConnected, issListening, issConnecting}
	MCSocketState;
typedef enum {isdOutgoing, isdIncoming} MCSocketDir;
typedef enum {istStream, istDatagram} MCSocketType;
typedef enum {issClient, issServer} MCSocketSide;


enum MCWebTunnelAuthentication { wtaNoAuthentication, wtaRequiresAuthentication };

/*
class EMCSockError: public EMCError {
protected:
	static char*    GetMessageFromErrorCode(mcInt32 AErrorCode);
public:
	static void     RaiseLastWinsockError(mcInt32 AErrorCode);
    EMCSockError(mcInt32 l): EMCError(l) { }
};
*/

#ifndef LINUX
#ifdef _WINDOWS
typedef SOCKET TSocket;
#else
typedef mcInt32 TSocket; // according to WinSock.pas
#endif
#else
typedef mcInt32 TSocket;
#endif
  
class MCSocket {
friend class MCStdSocket;
friend class MCOBEXSocket;
friend class MCInetTransportJob;
protected:
	bool            FCloseRequest;
	MCSocketDir     FDirection;
	char*           FLocalAddress;
	mcUInt32    	FLocalPort;
	char*           FRemoteAddress;
	mcUInt32    	FRemotePort;
	TSocket         FSocket;
	MCSocketState   FState;
	bool            FIsClient;
    
    struct {
        void* Data;
        mcInt32 Len;
        mcInt32 Size;
    } FIncomingBuf;

protected:
    //  Web tunneling support
    bool                    FUseWebTunneling;
    char                    *FWebTunnelAddress;
    unsigned short          FWebTunnelPort;
    MCWebTunnelAuthentication FWebTunnelAuthentication;
    char                    *FWebTunnelUserId;
    char                    *FWebTunnelPassword;

protected:
	virtual MCSocket*		CreateAcceptingSocket() PURE_VIRTUAL;
public:
	MCSocket(void);
	virtual ~MCSocket(void);

	virtual MCSocket*       Accept(void) PURE_VIRTUAL;
	/*
	This method is called by the transport thread after socket is connected. 
	All communications between connection parties, that are not part of normal 
	data exchange (SOCKS authentication, SSL connection initiation),
	should be implemented inside of this method.
	*/
	virtual mcInt32            AfterConnection(MCSocket* CtlSocket, mcInt32 Timeout);
	virtual mcInt32			AsyncConnect(MCSocket* CtlSocket, mcInt32 Timeout);
    virtual mcInt32            Bind(void) PURE_VIRTUAL;
	virtual void            Close(bool Forced) PURE_VIRTUAL;
	virtual bool            Connect(void);
    
	virtual mcInt32			ExtConnect(MCSocket* CtlSocket, mcInt32 Timeout);
    
	static void             FinalizeWinSock(void);
	virtual void            FinishAsyncConnect(void) PURE_VIRTUAL;

	virtual bool			HasBufferedOutgoingData();
	virtual bool			HasBufferedIncomingData();	

	virtual mcInt32            Init(MCSocketType SocketType, bool AllocateSocket = true) PURE_VIRTUAL;
	static void             InitializeWinSock(void);
	
	virtual mcInt32            IntReceive(void* Data, mcInt32 DataLen, mcInt32& Received) PURE_VIRTUAL;
	virtual mcInt32            IntSend(void* Data, mcInt32 DataLen, mcInt32& Sent) PURE_VIRTUAL;

	virtual mcInt32            Listen(mcInt32 BackLog);
	static bool				PopFromCtlSocket(MCSocket* CtlSocket);
	virtual bool			PostprocessOutgoingData();
	virtual bool			PreprocessIncomingData();
	virtual mcInt32            Receive(void* Data, mcInt32 DataLen, mcInt32& Received) PURE_VIRTUAL;
	virtual mcInt32            ReceiveFrom(void* Data, mcInt32 DataLen, mcInt32& Received, char*& RemoteAddress,
		                        mcUInt32& RemotePort) PURE_VIRTUAL;
	virtual void			ReturnData(void* Data, mcInt32 DataLen) PURE_VIRTUAL;
	virtual mcInt32			ReusePort() PURE_VIRTUAL;
	virtual mcInt32            Send(void* Data, mcInt32 DataLen, mcInt32& Sent) PURE_VIRTUAL;
	virtual mcInt32            SendTo(void* Data, mcInt32 DataLen, mcInt32& Sent, char* RemoteAddress,
		                        mcUInt32 RemotePort) PURE_VIRTUAL;
	virtual mcInt32			StartAsyncConnect();
	virtual mcInt32			SyncConnect(void);
    
	/*
	Direction specifies whether the socket initiated connection (outgoing 
	socket) or the socket is listening or accepted connection (incoming).
	*/
	virtual TSocket         getSocket(void);
	virtual MCSocketDir     getDirection(void);
	virtual MCSocketState   getState(void);
	void                    setLocalAddress(const char* Value);
	void                    setLocalPort(mcUInt32 Value);
	void                    setRemoteAddress(const char* Value);
	void                    setRemotePort(mcUInt32 Value);
	char*                   getLocalAddress(void);
	mcUInt32            getLocalPort(void);
	char*                   getRemoteAddress(void);
	mcUInt32            getRemotePort(void);
    //  Web tunneling support
    void                    setUseWebTunneling(bool Value);
    bool                    getUseWebTunneling();
    void                    setWebTunnelAddress(const char *Value);
    const char*             getWebTunnelAddress();
    void                    setWebTunnelPort(unsigned short Value);
    unsigned short          getWebTunnelPort();
    MCWebTunnelAuthentication   getWebTunnelAuthentication();
    void                    setWebTunnelAuthentication(MCWebTunnelAuthentication Value);
    void                    setWebTunnelUserId(const char *Value);
    const char*             getWebTunnelUserId();
    void                    setWebTunnelPassword(const char *Value);
    const char*             getWebTunnelPassword();
};
  
class MCStdSocket: public MCSocket {
protected:
    struct sockaddr_in FAddr;
	virtual MCSocket*		CreateAcceptingSocket() { return new MCStdSocket; };
	virtual mcInt32			HTTPConnect(MCSocket* CtlSocket, mcInt32 Timeout);
	bool					SocksSendReceive(bool &Terminated, MCSocket* CtlSocket, mcUInt32 Timeout, 
								void* sendBuf, mcInt32 sendBufSize, mcInt32& wasSent,
								void* readBuf, mcInt32 readBufSize, mcInt32& wasRead, bool needDoubleCRLF = false);

public:
	MCStdSocket(void);
	virtual ~MCStdSocket();

	virtual MCSocket*       Accept(void);
    virtual mcInt32			AddToMulticastSrv(const char* GroupAddress, const char* BindAddress);
    virtual mcInt32			AddToMulticastCli(const char* BindAddress, unsigned char TTL, bool Loop);
	virtual mcInt32			AsyncConnect(MCSocket* CtlSocket, mcInt32 Timeout);
    virtual mcInt32            Bind(void);
	virtual void            Close(bool Forced);
	virtual bool            Connect(void);
	virtual mcInt32			ExtConnect(MCSocket* CtlSocket, mcInt32 Timeout);

    virtual void            FinishAsyncConnect(void);
	sockaddr_in*            GetInAddr(void) { return &FAddr; }
	virtual mcInt32            Init(MCSocketType SocketType, bool AllocateSocket = true);
	virtual mcInt32            IntReceive(void* Data, mcInt32 DataLen, mcInt32& Received);
	virtual mcInt32            IntSend(void* Data, mcInt32 DataLen, mcInt32& Sent);
	
    virtual mcInt32            Listen(mcInt32 BackLog);
	virtual mcInt32            Receive(void* Data, mcInt32 DataLen, mcInt32& Received);
	virtual mcInt32            ReceiveFrom(void* Data, mcInt32 DataLen, mcInt32& Received, char*& RemoteAddress,
		                        mcUInt32& RemotePort);
	virtual void			ReturnData(void* Data, mcInt32 DataLen);
	virtual mcInt32			ReusePort();
    virtual mcInt32            Send(void* Data, mcInt32 DataLen, mcInt32& Sent);
	virtual mcInt32            SendTo(void* Data, mcInt32 DataLen, mcInt32& Sent, char* RemoteAddress,
		                        mcUInt32 RemotePort);
	
	virtual mcInt32			StartAsyncConnect();
	virtual mcInt32			SyncConnect(void);
};

typedef void (STDCALLCONV *MCSocketCreationEvent)(void* UserData, void* Sender,
	MCSocket *Socket, MCSocketSide SocketSide);	

class MCSocketFactory
{
protected:
	MCSocketCreationEvent	FOnSocketCreation;
	void*					FOnSocketCreationData;

	virtual MCSocket*		DoCreateClientSocket() PURE_VIRTUAL;
	virtual MCSocket*		DoCreateServerSocket() PURE_VIRTUAL;
public:
							MCSocketFactory();

	MCSocket*				CreateClientSocket();
	MCSocket*				CreateServerSocket();

	MCSocketCreationEvent	getOnSocketCreation(void**UserData = NULL);
	void                    setOnSocketCreation(MCSocketCreationEvent v, void* UserData = NULL);

};
  
class MCStdSocketFactory : public MCSocketFactory
{
	virtual MCSocket*		DoCreateClientSocket();
	virtual MCSocket*		DoCreateServerSocket();
};

class MCSocketJob:public MCThreadJob {
protected:
friend class MCInetTransport;
friend class MCSocketTransport;
friend class MCHTTPTransport;
friend class MCSocketThread;

	MCStdSocket*        FKickSocket;
    MCSocket*           FSocket;
	bool				FNotifyStop;
    bool				FNotifyMsg;
    bool				FNoUDPNotify;

public:
	MCSocketJob(void);
	virtual ~MCSocketJob();
	
    virtual MCSocket*       CreateSocket(void);
	virtual void            Initialize(void);
    MCStdSocket*            getKickSocket(void) { return FKickSocket; };
	MCSocket*               getSocket(void);
	
    //MCThreadJob implementation
    virtual void            Run(void) PURE_VIRTUAL;

	void					setNoUDPNotify(bool Value);
	bool					getNoUDPNotify(void);
};


void WinsockErrorCheck(mcInt32 ErrorCode);

#ifdef USE_NAMESPACE
}
#endif


#endif
