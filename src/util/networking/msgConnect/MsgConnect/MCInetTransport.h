//====================================================
//                                                    
//  EldoS MsgConnect                                 
//                                                    
//   Copyright (c) 2001-2010, EldoS                   
//                                                    
//====================================================

#ifndef __MCINETTRANSPORT__
#define __MCINETTRANSPORT__
 
#include "MC.h"
#include "MCSock.h"
#include "MCBase.h"
#include "MCSyncs.h"
#include "MCThreads.h"
#include "MCStream.h"
#include "MCSocketBase.h"

#ifdef USE_NAMESPACE
namespace MsgConnect
{
#endif

class MCInetTransport;
class MCInetConnectionEntry;

typedef void (STDCALLCONV *MCTransformerFailProc)(MCInetTransport* Transport, MCInetConnectionEntry* Entry);

typedef void (STDCALLCONV *MCSocketConnectedEvent)(void* UserData, void* Sender, 
	MCSocketDir Direction, const char* RemoteHost, unsigned short Port, bool& AllowConnection);
typedef void (STDCALLCONV *MCSocketDisconnectedEvent)(void* UserData, void* Sender, 
	MCSocketDir Direction, const char* RemoteHost, unsigned short Port);

class MCSocket;
class MCInetTransportJob;
class MCInetListenerJob;
class MCInetConnectionEntry;


class MCInetTransport: public MCBaseTransport, MCThreadFactory, MCJobErrorHandler {
friend class MCInetTransportJob;
friend class MCInetListenerJob;
friend class MCSocketTransportJob;
friend class MCHttpTransportJob;

protected:
//	static volatile long	FConnectionID;
	bool				FNoUDPNotify;
	mcUInt32			FAttemptsInterval;
	mcUInt32			FConnectionTimeout;
	mcUInt32			FAttemptsToConnect;
	mcUInt32			FClientThreadLimit;
	MCList*             FConnections;
	bool				FFailOnInactive;
	mcUInt32            FInactivityTime;
	mcUInt32            FIncomingMemoryThreshold;
	char*               FMessengerAddress;
	mcUInt32            FMessengerPort;
	mcUInt32            FOutgoingMemoryThreshold;
	char*				FTempFilesFolder;
	
    mcUInt32			FThreadPoolSize;
	
    MCInetTransportMode     FTransportMode;
	bool                    FUseTempFilesForIncoming;
	bool                    FUseTempFilesForOutgoing;
    mcInt32                     FReqTime;
	bool					FNoTransformerFallback;

    MCThreadPool            FThreadPool;
    MCTransformerFailProc   FTransformerFailProc;

	MCSocketConnectedEvent	FOnConnected;
	void*					FOnConnectedData;

	MCSocketDisconnectedEvent	FOnDisconnected;
	void*					FOnDisconnectedData;
	
	/*
	This event is used to tell the thread about new messages to be delivered. 
	Also it is used to wake up the waiting thread when it is time to terminate.
	*/
//	MCHandle OutgoingCount;
	/*
	Queue of messages pending for delivery.
	These messages are delivered to the 
	recepient Messenger and after that are 
	moved to SentQueue (or deleted)
	*/
    MCList*                 FOutgoingQueue;

    mcUInt32			FIncomingBufferSize;
	mcUInt32			FOutgoingBufferSize;
	
	mcUInt32			FIncomingSpeedLimit;
    mcUInt32			FOutgoingSpeedLimit;
    
	bool					FReuseServerPort;

protected:
    //  Web tunneling support
    bool                    FUseWebTunneling;
    char                    *FWebTunnelAddress;
    unsigned short          FWebTunnelPort;
    MCWebTunnelAuthentication FWebTunnelAuthentication;
    char                    *FWebTunnelUserId;
    char                    *FWebTunnelPassword;

protected:
	MCSocketFactory*		FDefaultSocketFactory;
	MCSocketFactory*		FSocketFactory;
	MCBandwidthPolicy		FBandwidthPolicy;
	MCInetListenerJob*		FListenerJob;
	
	virtual void                CancelMessage(MCList* Queue, MCMessageInfo* Info);
	virtual MCSocket*           CreateClientSocket(void);
	virtual MCSocket*           CreateServerSocket(void);
	virtual MCInetListenerJob* 
                                CreateListenerJob(void);
	virtual bool                DeliverMessage(char* DestAddress, MCMessageInfo* Info);
	virtual void                DeliveryFailed(MCMessageInfo* Info);
	virtual void                DoSetActive(void);
	MCInetConnectionEntry*      FindEntry(char* SocketName, MCMessageInfo* Info);
	void                        KickEntrySocket(MCInetConnectionEntry* Entry, bool MessageFlag);
	virtual void                MessageProcessed(MCMessageInfo* Info);
	void                        MessageReceived(MCMessageInfo* Info);
	void                        ReplyReceived(MCMessageInfo* Info);
    static bool                 CompareAddress(const char* Addr1, const char* Addr2, bool Strict);
	
    virtual MCInetConnectionEntry*      
                                CreateConnectionEntry(void) PURE_VIRTUAL;
    virtual MCInetTransportJob*      
                                CreateTransportJob(void) PURE_VIRTUAL;
    
	virtual void				GetMessageSource(MCMessageInfo* Info, char* Buffer, mcInt32* BufferSize);
    virtual void                NotifyJob(MCInetTransportJob* Job, bool MessageFlag) PURE_VIRTUAL;
    //virtual void				PutReconnectionPacket(void);
	//MCMessageInfo*             IsAnyMsgForDelivery(void);
    bool                        IsImmediateDelivery(const char* RemAddr);
    void                        PutMessageToOutgoing(const char* SocketName, MCMessageInfo* Info);
    virtual bool                NeedLiveConnectionForDelivery(void) PURE_VIRTUAL;
    virtual mcUInt32	        getDeliveryInterval(void) PURE_VIRTUAL;
    virtual void				CleanupMessages();
	virtual void				CleanOutgoingQueue();
	virtual MCMessageInfo*		GetMessageByID(__int64 MsgID);
//	long						GenConnID(void);

    //MCThreadFactory implementation
    virtual MCWorkerThread*     CreateThread(void);
#ifdef USE_CPPEXCEPTIONS
    virtual void HandleError(const EMCError& Error);
#else
    virtual void HandleError(mcUInt32 ErrorCode);
#endif
public:
	MCInetTransport();
    virtual ~MCInetTransport();
	bool                        InitiateDelivery(MCInetTransportJob* Job, MCInetConnectionEntry* Entry);

	virtual int					GetIncomingConnectionCount();
	virtual int					GetOutgoingConnectionCount();


	virtual mcInt32				GetOutgoingMessagesCount();

	mcUInt32              getMessengerPortBound(void);

	/*
	ThreadPoolSize defines the number of threads that are kept pre-initialized.
	0 - thread pool is not used
	*/
	mcUInt32					getThreadPoolSize(void);
	void                        setThreadPoolSize(mcUInt32 Value);

	/*
	Use ConnectionTimeout property to specify how much time should be spent
    trying to establish connection.
    Time is expressed in ms. 0 means do not wait.
	*/
	mcUInt32					getConnectionTimeout(void);
	void                        setConnectionTimeout(mcUInt32 Value);
	/*
	Use AttemptsInterval property to specify how much time should be spent 
	between attempts to connect to remote side.
	Time is expressed in ms. 0 means do not wait.
	*/
	mcUInt32               getAttemptsInterval(void);
	void                        setAttemptsInterval(mcUInt32 Value);
	/*
	With AttemptsToConnect you specify how much time the thread will try to 
	connect to the remote side before giving up and marking the messages with 
	failed status.
	0 means try until all messages are cancelled.
	*/
	mcUInt32               getAttemptsToConnect(void);
	void                        setAttemptsToConnect(mcUInt32 Value);
	/*
	Use ClientThreadLimit to limit the number of client threads that can be 
	started at the same time. 
	0 - start as many threads as needed. 
	*/
	void                        setClientThreadLimit(mcUInt32 Value);
	mcUInt32               getClientThreadLimit(void);
	/*
	When FailOnInactive is true and Active property is set to false, any 
	pending messages are canceled and marked as failed.
	When FailOnInactive is false, messages are kept in hope for delivery in 
	future.
	*/
	void                        setFailOnInactive(bool Value);
	bool                        getFailOnInactive(void);
	/*
	Specifies connection inactivity time in milliseconds - time after which 
	idle connection is disconnected.
	Set 0 to wait until socket is closed by the other side.
	*/
	void                        setInactivityTime(mcUInt32 Value);
	mcUInt32               getInactivityTime(void);
	/*
	Temporary files for incoming data are used when data size exceeds the value 
	of this property. 
	If the amount of data to be received is less than threshold, memory stream 
	is used.
	*/
	void                        setIncomingMemoryThreshold(mcUInt32 Value);
	mcUInt32               getIncomingMemoryThreshold(void);
	/*
	This address is used to bind the listening socket to.
	Use 0 to bind to any address. Use specific IP to bind only to the interface,
	to which the address belongs.
	*/
	void                        setMessengerAddress(char* Value);
	char*                       getMessengerAddress(void);
	/*
	The number of the port that the receiver socket will listen.
	Used to receive messages from other computers.
	*/
	void                        setMessengerPort(mcUInt32 Value);
	mcUInt32                    getMessengerPort(void);
	/*
	Temporary files for outgoing data are used when data size exceeds the value 
	of this property. 
	If the amount of data to be sent is less than threshold, memory stream is 
	used.
	*/
	void                        setOutgoingMemoryThreshold(mcUInt32 Value);
	mcUInt32               getOutgoingMemoryThreshold(void);
	/*
	Defines the folder where temporary files will be stored. If the folder is 
	not specified, system default folder is used.
	*/
	void                        setTempFilesFolder(char* Value);
	char*                       getTempFilesFolder(void);
	/*
	TransportMode specifies how the connections are established.
	In P2P mode connection can be initiated or accepted. This is optimal for 
	LANs and when both sides are not behind the proxy/firewall.
	In server mode connections are only accepted, but not initiated. This mode 
	is optimal for servers that are supposed to accept connections from clients 
	which are behind the proxy/firewall.
	In client mode connections are only initiated, but not accepted. This mode 
	is optimal for clients that might work behind the proxy/firewall.
	*/
	void                        setTransportMode(MCInetTransportMode Value);
	MCInetTransportMode         getTransportMode(void);
	/*
	Defines whether temporary files are used for incoming data. 
	When UseTempFiles is true, temporary files are placed in the folder defined 
	by TempFilesFolder.
	When UseTempFiles is false, temporary files are not used and the data is 
	stored in memory.
	*/
	void                        setUseTempFilesForIncoming(bool Value);
	bool                        getUseTempFilesForIncoming(void);
	/*
	Defines whether temporary files are used for outgoing data. 
	When UseTempFiles is true, temporary files are placed in the folder defined 
	by TempFilesFolder.
	When UseTempFiles is false, temporary files are not used and the data is 
	stored in memory.
	*/
	void                        setUseTempFilesForOutgoing(bool Value);
	bool                        getUseTempFilesForOutgoing(void);


	/*
	Defines whether unprotected data is sent if the parties couldn't agree 
	on some transformer
	*/
	void						setNoTransformerFallback(bool Value);
	bool						getNoTransformerFallback();

	/*
	Defines size of incoming buffer
	*/
	void						setIncomingBufferSize(mcUInt32 Value);
	mcUInt32				getIncomingBufferSize();

	/*
	Defines size of outgoing buffer
	*/
	void						setOutgoingBufferSize(mcUInt32 Value);
	mcUInt32				getOutgoingBufferSize();

	/*
	Defines bandwidth limitation for inbound traffic
	*/
	void						setIncomingSpeedLimit(mcUInt32 Value);
	mcUInt32				getIncomingSpeedLimit();

	/*
	Defines bandwidth limitation for outbound traffic
	*/
	void						setOutgoingSpeedLimit(mcUInt32 Value);
	mcUInt32				getOutgoingSpeedLimit();

    void                        setTransformerFailingHandler(MCTransformerFailProc Handler);
    MCTransformerFailProc       getTransformerFailingHandler(void);
    //  Web tunneling support
    void                        setUseWebTunneling(bool Value);
    bool                        getUseWebTunneling();
    void                        setWebTunnelAddress(const char *Value);
    const char*                 getWebTunnelAddress();
    void                        setWebTunnelPort(unsigned short Value);
    unsigned short              getWebTunnelPort();
    MCWebTunnelAuthentication   getWebTunnelAuthentication();
    void                        setWebTunnelAuthentication(MCWebTunnelAuthentication Value);
    void                        setWebTunnelUserId(const char *Value);
    const char*                 getWebTunnelUserId();
    void                        setWebTunnelPassword(const char *Value);
    const char*                 getWebTunnelPassword();

	void                        setReuseServerPort(bool Value);
    bool                        getReuseServerPort(void);
    
	void						setSocketFactory(MCSocketFactory* Value);
	MCSocketFactory*			getSocketFactory();

	void						setBandwidthPolicy(MCBandwidthPolicy Value);
	MCBandwidthPolicy			getBandwidthPolicy();

#ifndef _WIN32_WCE
	void                        setNoUDPNotify(bool Value);
	bool                        getNoUDPNotify(void);
#endif

    MCSocketConnectedEvent		getOnConnected(void* *UserData = NULL);
	void						setOnConnected(MCSocketConnectedEvent Value, void* UserData = NULL);

    MCSocketDisconnectedEvent	getOnDisconnected(void* *UserData = NULL);
	void						setOnDisconnected(MCSocketDisconnectedEvent Value, void* UserData = NULL);

	void                        DebugBreakConnections(void);
};


class MCInetTransportJob: public MCSocketJob
{
protected:
	MCInetConnectionEntry*  FEntry;
	bool                    FHandshakeWaitingReply;
	byte*                   FIncomingBuffer;
	byte*                   FOutgoingBuffer;
    mcUInt32		    FIncomingBufferSize;
	mcUInt32		    FOutgoingBufferSize;

    // Bandwidth limitation begin
    mcUInt32		    FIncomingSpeedLimit;
    mcUInt32		    FOutgoingSpeedLimit;

    mcUInt32		    FblInSessionStartTime;
    mcUInt32		    FblOutSessionStartTime;

    mcUInt32		    FblInMsgStartTime;
    mcUInt32		    FblOutMsgStartTime;

    __int64				    FblInSessionTransferred;
    __int64				    FblOutSessionTransferred;

    __int64				    FblInMsgTransferred;
    __int64				    FblOutMsgTransferred;

	__int64				    FblInSecTransferred;
    __int64				    FblOutSecTransferred;

	mcUInt32			FblInNextTime;
    mcUInt32			FblOutNextTime;

    mcUInt32		    FblToRecv;
	mcUInt32		    FblToSend;

    mcUInt32		    FActivityTime;
    mcUInt32		    FLastMsgRecvTime;
	mcUInt32		    FLastMsgSendTime;
	MCMessageState		    FLastMsgRecvState;
	MCMessageState		    FLastMsgSendState;

	// bandwidth limitation end

	MCSocket*               FTransporter;
	bool                    FInitialized;
	bool                    FIsReceiving;
	bool                    FIsSending;
	MCInetTransport*        FOwner;
    
	MCCriticalSection*		FCriticalSection;
    MCList                  FOutgoingQueue;
	mcInt32                     FServerRequestTime;

	bool					FWasConnected;

	void                    DeleteEntry(void);

	void                    WaitForSignal(mcUInt32 ms);
    bool                    SetMessageForSending(void);
    void                    MakeAsyncConnect(void);
    bool                    ClientConnect(void);
	bool					FailCurrentMessage(mcInt32 ErrorCode);
	void					ClientExecuteFinalizeBlock(mcInt32 &errorCode);
	virtual void            ClientExecute(void); 
	void					ServerExecuteFinalizeBlock(void);
    virtual void            ServerExecute(void);
    
    mcInt32                     IsMsgPendingForClient(MCMessageInfo* deferredInfo);
    bool                    HandleIncomingData(void);
    bool                    HandleOutgoingData(void);
    bool                    PerformRecvSend(bool& errorFlag, bool& closeConnection, bool& timeout);
    virtual void            Run(void);
	MCMessageInfo*          GetMessageForDelivery(void);
	/*
	This method is called from Execute to transform a message into a data 
	block. Method returns true if the message was prepared and false if there 
	was no message.
	*/
	bool                    PrepareMessageForSending(void);
	/*
	ReceiveData reads the data from the socket and adds them to the incoming 
	packet (optionally creating this packet). 
	ReceiveData returns true if the socket can be read or false if the socket 
	was closed.
	*/
	virtual bool            ReceiveData(void) PURE_VIRTUAL;
	virtual void            UpdateConnectionContext(void) PURE_VIRTUAL;
    virtual bool            SendData(void) PURE_VIRTUAL;
    virtual bool            IsMessageRead(void);
    //virtual bool            MessageReceivedCompletely(void);
    void                    UpdateRemoteSAddress(MCMessageInfo* Info);

    //new receiving logic
    virtual bool            MessageReceivedCompletely2(void);
    virtual bool            IsMessageFetched(void);
    bool                    ReadHeader(MCInetHeader* Header, char** s, char* TransactCmd, 
                                       char** RouteTo, char** RecvPath, char** DQueue);
    bool                    ParseRoutingStrings(char* s, char* *outS, char* TransactCmd, 
                                                char** RouteTo, char** RecvPath, char** DQueue);
    bool                    UpdateEntry(char* *s, char **OriginalSMsg);
    void                    PutErrorPacket(char* DMessenger, char* SMessenger, char* RecvPath, 
                                            __int64 MsgID, mcInt32 Error);
    MCMessageInfo*          LoadAndUnpackMessage(MCInetHeader* Header, void** DataBuf, 
                                                        mcInt32* DataLen, MCMemStream** AStream, 
                                                        char* s, char* SMsg, char TransactCmd,
                                                        char* RecvPath);
    bool                    PrepareRouting(char* s, char* OriginalSMsg, char** RecvPath);

    bool                    HandleEmptyRequest(MCMessageInfo* Info, MCMemStream* AStream);
    bool                    HandleEmptyReply(MCMessageInfo* Info, MCMemStream* AStream);
    bool                    HandleDispatching(MCMessageInfo* Info, MCMemStream* AStream, char* RecvPath);
    bool                    HandleReply(MCMessageInfo* Info, MCMemStream* AStream);
    bool                    HandleUnknown(MCMessageInfo* Info, MCMemStream* AStream);
    //end of NRL

    virtual bool            MessageSentCompletely(void);

    virtual void            SetTransporterAddress(MCInetConnectionEntry* Ent, MCSocket* Transp) PURE_VIRTUAL;
    virtual bool            PostConnectionStep(void) PURE_VIRTUAL;
    
    //char*                   UpdateClientID(const char* S, mcInt32 ID);
    //int						ExtractClientID(const char* S);
	bool					IsTransformerOK(MCInetHeader* Header);
    MCMessageInfo*          MakeEmptyPacket(MCMessageState state);
    void                    FinalizeJob(void);
    bool                    AreRequestsInQueue(void);
	virtual bool			IsRequestNeeded() {return false; };
	virtual void			ChangeLastSendTime(MCMessageState state);
	virtual void			ChangeLastRecvTime(MCMessageState state);

	virtual bool			InitializeConnection();
	virtual void			FinalizeConnection();

public:
	MCInetTransportJob(void);
	virtual ~MCInetTransportJob();

	virtual void            Initialize2(MCSocket* Socket, MCInetConnectionEntry* Entry); 
	void                    AdjustBufferSize(mcUInt32 Incoming, mcUInt32 Outgoing);
	void                    AdjustSpeedLimits(mcUInt32 Incoming, mcUInt32 Outgoing);
	static bool	            ParseRemoteAddr(const char* S, mcInt32 *ClientID, mcInt32 *ConnID, char** IP, mcInt32* Port);

	MCInetConnectionEntry*  getEntry(void);
	void                    setEntry(MCInetConnectionEntry* Value);
	bool                    getIsReceiving(void);
	void                    setIsReceiving(bool Value);
	bool                    getIsSending(void);
	void                    setIsSending(bool Value);
	MCInetTransport*        getOwner(void);
	void                    setOwner(MCInetTransport* Value);
	MCSocket*               getTransporter(void);
	void                    setTransporter(MCSocket* Value);
    MCList*                 getOutgoingQueue(void);
	MCCriticalSection*		getCriticalSection(void);
};


class MCInetListenerJob: public MCSocketJob {
private:
protected:
	MCInetTransport*    FOwner;
	MCSocket*           FListener;
	bool                FInitialized;

    virtual bool        AcceptConnection(void);
public:
	MCInetListenerJob(void);
	virtual ~MCInetListenerJob(void);
	virtual void        Run(void);
	virtual void        Initialize(MCSocket *ASocket);
	MCSocket*           getListener();
	MCInetTransport*    getOwner(void);
	void                setOwner(MCInetTransport* Value);
};
  
class MCInetConnectionEntry {
friend class MCInetTransportJob;
protected:
	mcInt32                FAttempt;
	__int64             FCancelID;
	mcInt32                FCompressID;
	MCCriticalSection* 
                        FCS;
	mcInt32                FEncryptID;
	MCStream*           FIncomingStream;
	MCMessageInfo*      FInfo;
//	MCMessageState      FInfoState;
	__int64             FLastActionTime;
	mcInt32                FMsgSize;
	MCStream*           FOutgoingStream;
	MCInetTransportJob* FProcessingThread;
	char*               FRemoteAddress;
	mcInt32                FSealID;
    mcInt32                 FClientID;
    bool                FRejectedTransformer;
    char*               FRemoteSAddress;
    mcInt32                 FReqTime;
	long				FConnID;
public:
	MCInetConnectionEntry(void);
	virtual ~MCInetConnectionEntry(void);
	void                Reset(void);
	mcInt32                getAttempt(void);
	void                setAttempt(mcInt32 Value);
	__int64             getCancelID(void);
	void                setCancelID(__int64 Value);
	mcInt32                getCompressID(void);
	void                setCompressID(mcInt32 Value);
	MCCriticalSection*   getCS(void);
	mcInt32                getEncryptID(void);
	void                setEncryptID(mcInt32 Value);
	MCStream*           getIncomingStream(void);
	void                setIncomingStream(MCStream* Value);
	MCMessageInfo*      getInfo(void);
	void                setInfo(MCMessageInfo* Value);
//	MCMessageState      getInfoState(void);
//	void                setInfoState(MCMessageState Value);
	__int64             getLastActionTime(void);
	void                setLastActionTime(__int64 Value);
	mcInt32                getMsgSize(void);
	void                setMsgSize(mcInt32 Value);
	MCStream*           getOutgoingStream(void);
	void                setOutgoingStream(MCStream* Value);
	MCInetTransportJob* getProcessingJob(void);
	void                setProcessingJob(MCInetTransportJob* Value);
	char*               getRemoteAddress(void);
	void                setRemoteAddress(const char* Value);
	mcInt32                getSealID(void);
	void                setSealID(mcInt32 Value);
    mcInt32                 getClientID(void);
    void                setClientID(mcInt32 Value);
    bool                getRejectedTransformer(void);
    void                setRejectedTransformer(bool Value);
    char*               getRemoteSAddress(void);
    void                setRemoteSAddress(const char* Addr);
    mcInt32                 getReqTime(void);
    void                setReqTime(mcInt32 value);
	mcInt32				getConnID(void);
	void				setConnID(mcInt32 value);
};

void ShowDebugInfo(const char* format,...);

#ifdef USE_NAMESPACE
}
#endif


#endif
