//====================================================
//                                                    
//  EldoS MsgConnect                                 
//                                                    
//   Copyright (c) 2001-2010, EldoS                   
//                                                    
//====================================================

#ifndef __MCBASE__
#define __MCBASE__
 
#include "MC.h"
#include "MCUtils.h"
#include "MCLists.h"
#include "MCSyncs.h"
#include "MCLogger.h"
#include "MCStream.h"


#ifdef USE_NAMESPACE
namespace MsgConnect
{
#endif

//#define SEH2CPP
    
//#undef SendMessageTimeout
#ifdef _WIN32_WCE	
#	undef MsgWaitForMultipleObjects
#endif

//#undef SendMessage
//#undef SendMessageTimeout
//#undef SendMessageCallback

void MCBaseInitialization(void); // !!! CALL THIS BEFORE ANY MC OPERATION !!!
void InitializeThreads(void);

class MCBaseTransport;

typedef enum {bdtConst, bdtVar} MCBinDataType;

// The sequence of fields must remain the same. New fields can be added before
// DataType field

/*
#ifdef _WIN32
bool    SupportsTSNames(void);
#endif
void*   MCMemAlloc(mcInt32 Size);
void    MCMemFree(void* Block);
void    MCFreeNull(void*& Block);
*/

#ifdef _WIN32
extern MCList* DirectTransports;
#endif

typedef enum {
	itcNormal,
	itcTransactionPart,
	itcTransactionStart,
	itcTransactionCommit,
	itcTransactionRollback
} MCTransactCmd;

struct MCMessage {
	__int64			MsgID;
	MCTransactCmd	TransCmd;
	short			TransID;
	signed char		Priority;
	mcInt32 		MsgCode;
	mcInt32 		Result;
	mcInt32 		Param1;
	mcInt32 		Param2;
	MCBinDataType	DataType;
	mcInt32			DataSize;
	void*			Data;
    
    MCMessage(void) {
		Priority = 0;
		TransCmd = itcNormal;
        Data = NULL;
        DataSize = 0;
        Result = 0;
        DataType = bdtVar;
    };

    /*
	~MCMessage() {
        if (Data) 
            MCMemFree(Data);
        Data = NULL;
    };
	*/

    void copyTo(MCMessage* msg) 
	{
		memmove(&msg->MsgID, &MsgID, sizeof(MCMessage));
    };
};

typedef struct {
	byte UserName[16];
	byte Password[16];
} MCMessageCredentials;

typedef enum { 
	imsQueued,      // placed to the queue for delivery								0
	imsDispatching, // in delivery queue											1
	imsWaiting,     // waiting for processing										2
	imsProcessing,  // currently in ProcessMessage									3
	imsComplete,    // complete and waiting for delivery to destination				4
	imsFailed,      // declined by transport										5
    imsEmptyRequest,//																6
    imsEmptyReply,  // empty messages - used for resuming aborted connections		7
	imsExpired,		// the message expired while in delivery queue					8
    imsNone         // just none state is not set									9
} MCMessageState;

class MCQueue;

#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
#else
#	define	STDCALLCONV
#endif


const mcInt32 DQueueMaxLength=32;

//define handlers
typedef void (STDCALLCONV *MCNotifyProc)(mcUInt32 UserData, MCMessage& Message);
typedef void (STDCALLCONV *MCErrorProc)(mcUInt32 UserData, MCMessage& Message, mcInt32 ErrorCode);
typedef void (STDCALLCONV *MCHandleMessageEvent)(void* UserData, void* Sender,
	MCMessage& Message, bool& Handled);
typedef void (STDCALLCONV *MCValidateCredentialsEvent)(void* UserData, void* Sender, 
	MCMessage* Message, MCMessageCredentials* Credentials, bool& Valid);
//typedef void (STDCALLCONV *MCHandleMessageTimeout)(void* resvd, void* Sender, MCMessage* Message);


// The sequence of fields must remain the same. New fields can be added before
// Message field
#include "MCStream.h"

class MCMessageInfo
{
public:
	// transferrable fields
	double              Time;
	__int64             OrigID;
	mcInt32                IsSendMsg;
	char                DQueue[DQueueMaxLength];
	MCMessageCredentials 
                        Credentials;
	mcInt32                MCError;
	MCMessageState      State;
	MCMessage           Message;
	
	// local fields
	MCEvent*            ReplyFlag;
	MCBaseTransport*    Transport;
	mcPtrInt            UserData;
	MCNotifyProc        NotifyProc;
	MCNotifyProc        TimeoutProc;
	MCErrorProc         ErrorProc;
	//char				SourcePrefix[4];
	mcInt32				ConnID;
	mcInt32				ClientID;
	char*               SMessenger;
	char*               DMessenger;
    char*               URL;
	mcUInt32			StartTime;
	mcUInt32			Timeout;
    void*               Track;
    MCMessageState      OldState;
    bool                Sent;
    

    MCMessageInfo(MCMessage* Msg);
    ~MCMessageInfo();
    void WriteToStream(MCStream* Stream);
    void LoadFromStream(MCStream* Stream);
    void setSMessenger(char* SMsg);
    void setDMessenger(char* DMsg);
    void setURL(char* URLStr);
	void CopyMessage(MCMessageInfo* Dest);
};

class MCBaseTransformer: public MCObject {
public:
	virtual mcInt32 GetID(void) PURE_VIRTUAL;
};

class MCBaseCompression:public MCBaseTransformer {
friend class MCBaseTransport;
protected:
	virtual void Compress(char* Remote, void* InData, mcInt32 InDataSize,
		void*& OutData, mcInt32& OutDataSize,bool& Success) PURE_VIRTUAL;
	virtual void Decompress(char* Remote, void* InData, mcInt32 InDataSize,
		void*& OutData, mcInt32& OutDataSize, mcInt32 TransformID,
		bool& Success) PURE_VIRTUAL;
};
  
class MCCBaseEncryption:public MCBaseTransformer {
friend class MCBaseTransport;
protected:
	virtual void Encrypt(char* Remote, void* InData, mcInt32 InDataSize,
		void*& OutData, mcInt32& OutDataSize,bool& Success) PURE_VIRTUAL;
	virtual void Decrypt(char* Remote, void* InData, mcInt32 InDataSize,
		void*& OutData, mcInt32& OutDataSize, mcInt32 TransformID,
		bool& Success) PURE_VIRTUAL;
};
  
class MCBaseSealing:public MCBaseTransformer {
friend class MCBaseTransport;
protected:
	virtual void Seal(char* Remote, void* InData, mcInt32 InDataSize,
		void*& OutData, mcInt32& OutDataSize,bool& Success) PURE_VIRTUAL;
	virtual void UnSeal(char* Remote, void* InData, mcInt32 InDataSize,
		void*& OutData, mcInt32& OutDataSize, mcInt32 TransformID,
		bool& Success) PURE_VIRTUAL;
};

//#define INFINITE 0xFFFFFFFF

class MCMessenger: public MCObject
{
private:
friend class MCBaseTransport;
friend class MCSimpleTransport;
friend class MCInetTransport;
friend class MCInetTransportJob;
friend class MCDirectTransport;
friend class MCSocketTransport;
friend class MCSocketTransportJob;
friend class MCMMFTransport;
friend class MCMMFSenderThread;
friend class MCMMFReceiverThread;
friend class MCHttpTransport;
friend class MCHttpTransportJob;
friend class MCMessageInfo;
friend class MCUDPTransport;
friend class MCUDPSenderThread;
friend class MCUDPReceiverThread;
friend class MCOBEXTransport;
friend class MCOBEXThread;
friend class MCQueue;

#ifndef __APPLE__
	unsigned long       FThreadID;
#else
	pthread_t           FThreadID;
#endif
	/*
	The list of transports registered for the given messenger
	*/
	MCList*             FTransportList;
protected:

	MCCriticalSection* FCompleteCriticalSection;
	/*
	MainEvent is set to signaled state when a message appears in Incoming 
	queue. 
	*/
	MCEvent*            FCompleteEvent;
	/*
	Queue of messages pending for receiving.
	These messages are read by PeekMessage/GetMessage.
	*/
	MCList*             FCompleteQueue;
	mcUInt32       FMaxTimeout;
	mcUInt32       FCleanupInterval;
	mcUInt32       FLastCleanup;
	MCValidateCredentialsEvent 
                        FOnValidateCredentials;
	void*				FOnValidateCredentialsData;
	MCList*             FQueues;
	
	MCCriticalSection*  FForwardCriticalSection;
	MCList*             FForwardQueue;

	MCCriticalSection*  FIncomingCriticalSection;
	/*
	MainEvent is set to signaled state when a message appears in Incoming 
	queue. 
	*/
	MCEvent*            FIncomingEvent;
	/*
	Queue of messages pending for receiving.
	These messages are read by PeekMessage/GetMessage.
	*/
	MCList*             FIncomingQueue;
	
    void                    AddQueue(MCQueue* AQueue);
	void                    AddTransport(MCBaseTransport* ATransport);
	void                    DispatchCompleteMessages(void);
	void                    DoSendMessage(const char* Destination, MCMessageInfo* Info);
	MCQueue*                FindQueue(char* QueueName);
	mcInt32                    HasPendingMessages(MCList* Queue);
	void                    RemoveQueue(MCQueue* AQueue);
	void                    RemoveTransport(MCBaseTransport* ATransport);
	bool                    ValidateCredentials(MCMessageInfo* Info);
	
	void                    DoMessageProcessed(MCMessageInfo* Info);
	void					CleanupForwardQueue();

	static mcInt32             FindMessageInQueue(__int64 MessageID, MCList* Queue);
/*	static MCMessageInfo*   CreateMessageInfo(MCMessage* Message);
	static void             DestroyMessageInfo(MCMessageInfo* Info);
    static MCMessageInfo*   CopyMessageInfo(MCMessageInfo* Info);
    static void             SetSMessenger(MCMessageInfo* Info, char* SMsg);
    static void             SetDMessenger(MCMessageInfo* Info, char* DMsg); */

public:
	MCMessenger();
	virtual ~MCMessenger();

    static void             SetMessageID(MCMessageInfo* MessageInfo);
	static void             SetOrigID(MCMessageInfo* MessageInfo);

	static void				CreateMessageSimple(mcInt32 MsgCode, mcInt32 Param1, mcInt32 Param2, MCMessage& Message);
	static void				CreateMessageFromBinary(mcInt32 MsgCode, mcInt32 Param1, mcInt32 Param2, void* Binary, mcInt32 BinarySize, bool IsVarData, MCMessage& Message);
	static void				CreateMessageFromText(mcInt32 MsgCode, mcInt32 Param1, mcInt32 Param2, const char* Text, bool IsVarData, MCMessage& Message);

	void                    MCDispatchMessages(void);
    void                    DispatchMessages(void) { MCDispatchMessages(); };
	/*
	Gets new message from the message queue. Doesn't return until any message 
	is available.
	*/
	void                    MCGetMessage(MCMessage* Message);
    void                    GetMessage(MCMessage* Message) { MCGetMessage(Message); };
	
	void                    MessageProcessed(MCMessage* Message);
	
	bool                    ForwardMessage(const char* Destination, MCMessage* Message,
		                        mcUInt32 Timeout, MCMessageCredentials* Credentials);
	
	mcUInt32                MsgWaitForMultipleObjects(mcUInt32 nCount, void* pHandles, bool fWaitAll, mcUInt32 dwMilliseconds);
	/*
	Gets new message from the message queue. Return immediately even when no 
	message is available.
	Returns true if the message has been picked and false otherwise.
	*/
	bool                    MCPeekMessage(MCMessage* Message);
    bool                    PeekMessage(MCMessage* Message) 
                                { return MCPeekMessage(Message); };
	
    void                    MCPostMessage(const char* Destination, MCMessage* Message,
		                        MCMessageCredentials* Credentials);
	void                    PostMessage(const char* Destination, MCMessage* Message,
								MCMessageCredentials* Credentials) 
								{ MCPostMessage(Destination, Message, Credentials); };

	/*
	Gets new message from the message queue. Doesn't return until any message 
	is available.
	*/
	mcInt32                    MCSendMessage(const char* Destination, MCMessage* Message,
		                        MCMessageCredentials* Credentials);
	mcInt32                    SendMessage(const char* Destination, MCMessage* Message,
		                        MCMessageCredentials* Credentials) 
								{ return MCSendMessage(Destination, Message, Credentials); };

	void                    MCSendMessageCallback(const char* Destination, MCMessage* Message, 
		                        MCNotifyProc CompletionProc, mcPtrInt UserData, MCMessageCredentials* Credentials);
	void                    SendMessageCallback(const char* Destination, MCMessage* Message, 
		                        MCNotifyProc CompletionProc, mcPtrInt UserData, MCMessageCredentials* Credentials)
								{ MCSendMessageCallback(Destination, Message, CompletionProc, UserData, Credentials); };

	bool                    MCSendMessageTimeout(const char* Destination, MCMessage* Message,
		                        mcUInt32 Timeout, bool Block, MCMessageCredentials* Credentials,
		                        mcInt32& MsgResult);
	bool                    SendMessageTimeout(const char* Destination, MCMessage* Message,
		                        mcUInt32 Timeout, bool Block, MCMessageCredentials* Credentials,
		                        mcInt32& MsgResult)
								{ return MCSendMessageTimeout(Destination, Message, Timeout, Block, Credentials, MsgResult); };

	void                    SendMessageTimeoutCallback(const char* Destination, MCMessage* Message,
		                        mcUInt32 Timeout, MCNotifyProc CompletionProc, 
								MCNotifyProc TimeoutProc, MCErrorProc ErrorProc, mcPtrInt UserData, 
								MCMessageCredentials* Credentials);

	/*
	Waits for any message to appear in the message queue. 
	Returns only when any message appears.
	*/
	void                    MCWaitMessage(void);
    void                    WaitMessage(void) { MCWaitMessage(); };
    bool                    MCWaitMessageEx(mcUInt32 Timeout);
    bool                    WaitMessageEx(mcUInt32 Timeout) { return MCWaitMessageEx(Timeout); };
	void					CleanupQueues();
	bool					CancelMessage(__int64 MsgID);
	bool					CancelMessageEx(__int64 MsgID, bool CancelComplete);
	bool					GetMessageSent(__int64 MsgID);
    //const char*             GetMessageDestQueue(__int64 MsgID);
    void                    GetMessageSource(const MCMessage* Message, char* Buffer, mcInt32* BufferSize);

	void					GetQueueStatus(const char* QueueName, mcInt32 MsgCodeLow, mcInt32 MsgCodeHigh, 
								mcInt32 &MsgCount, mcInt32 &MsgTotalSize, mcInt32 &MsgMaxSize);
	
	void*					getDirectTransportList();
	void					setDirectTransportList(void* value);


#ifndef __APPLE__
	mcUInt32           getThreadID(void);
	void                    setThreadID(mcUInt32 Thr); //GetCurrentThread() or getpid() result should be used
#else
	pthread_t               getThreadID(void);
	void                    setThreadID(pthread_t Thr); //GetCurrentThread() or getpid() result should be used
#endif

    mcUInt32           getMaxTimeout(void);
	void                    setMaxTimeout(mcUInt32 Value);
    mcUInt32           getCleanupInterval(void);
	void                    setCleanupInterval(mcUInt32 Value);
    MCValidateCredentialsEvent 
                            getOnValidateCredentials(void* *UserData = NULL);
	void                    setOnValidateCredentials(MCValidateCredentialsEvent Value, void* UserData = NULL);

};

class MCMessageHandlers;
  
class MCMessageHandler
{
private:
friend class MCMessageHandlers;
	bool                FEnabled;
	mcInt32                FMsgCodeHigh;
	mcInt32                FMsgCodeLow;
	MCList*             FCollection;
	MCHandleMessageEvent 
                        FOnMessage;
	void*				FOnMessageData;
public:
	MCMessageHandler(MCMessageHandlers* Collection);
	
    void                    Assign(MCMessageHandler& Source);
	bool                    getEnabled(void);
	void                    setEnabled(bool v);
	mcInt32                    getMsgCodeHigh(void);
	void                    setMsgCodeHigh(mcInt32 v);
	mcInt32                    getMsgCodeLow(void);
	void                    setMsgCodeLow(mcInt32 v);
	/*
	The handler should process the message and set Handled to true if the 
	message is processed.
	There is no need to call MCMessenger.MessageProcessed manually.
	*/
	MCHandleMessageEvent    getOnMessage(void**UserData = NULL);
	void                    setOnMessage(MCHandleMessageEvent v, void* UserData = NULL);
};

class MCQueue;
  
class MCMessageHandlers: public MCList
{
friend class MCQueue;
protected:
	MCQueue*            FOwner;
    

	static void             MCMessageHandlerDelete(void* sender,void* v);
	MCQueue*                GetOwner(void);
    bool                    HandleMessage(MCMessageInfo* Info);
public:
	MCMessageHandlers(MCQueue* Queue);
	/*
	Adds a message handler to the list and returns the reference to this 
	handler.
	*/
	MCMessageHandler*       Add(void);
	/*
	Use getItems/setItems to access particular handler in the list.
	*/
	MCMessageHandler*       getItems(mcInt32 Index);
	void                    setItems(mcInt32 Index, const MCMessageHandler* Value);
};
  
class MCQueue: public MCObject
{
friend class MCMessenger;
protected:
	MCMessageHandlers*  FHandlers;
	MCMessenger*        FMessenger;
	MCHandleMessageEvent 
                        FOnUnhandledMessage;
	void*				FOnUnhandledMessageData;
	char*               FQueueName;
    //MCList*             FMessages;	
    //MCCriticalSection*  FGuard;

    void                    ProcessMessage(MCMessageInfo* Info);
    
    bool                    HandleMessage(MCMessageInfo* Info);
    void                    ProcessMessages(void);
    void                    AddIncoming(MCMessageInfo* Info);

public:
	MCQueue();
	virtual ~MCQueue();

	MCMessageHandlers*      getHandlers(void);
	void                    setHandlers(MCMessageHandlers* Value);
	MCMessenger*            getMessenger(void);
	void                    setMessenger(MCMessenger* Value);
	MCHandleMessageEvent    getOnUnhandledMessage(void* *UserData = NULL);
	void                    setOnUnhandledMessage(MCHandleMessageEvent Value, void* UserData = NULL);
	/*
	This is the name of the queue as it will be known to the sender.
	*/
	char*                   getQueueName(void);
	void                    setQueueName(char* Value);
};


  
class MCBaseTransport: public MCObject
{
friend class MCMessenger;
friend class MCQueue;
friend class MCInetTransportJob;

protected:
	char*               FName;
#ifndef __APPLE__
	mcUInt32       FThreadID;
#else
	pthread_t           FThreadID;
#endif

	char*               FDefaultTransportName;
	bool                FActive;
	MCBaseCompression*  FCompressor;
	MCCBaseEncryption*  FEncryptor;
	/*
		LinkCriticalSection is used to secure operations with 
		compressors/encryptors 
		and to prevent access to those objects from several threads.
	*/
	MCCriticalSection*	FLinkCriticalSection;
	MCCriticalSection*	FCriticalSection;
	mcInt32                FMaxMsgSize;
	mcUInt32			FMaxTimeout;
	mcUInt32			FMsgTimeout;
	MCMessenger*        FMessenger;
	MCBaseSealing*      FSealer;
	bool				FIgnoreIncomingPriorities;
	bool				FDiscardUnsentMessages;

	static void				InsertInfoIntoQueueWithPriorities(MCList* Queue, MCMessageInfo* Info, bool IgnoreMode);
	virtual bool            AddressValid(char* Address, bool ChangeAddress = true);
	virtual void            CancelMessage(MCList* Queue, MCMessageInfo* Info) PURE_VIRTUAL;
	virtual bool            DeliverMessage(char* DestAddress, MCMessageInfo* Info) PURE_VIRTUAL;
	virtual void            DoSetActive(void);
	virtual void            Loaded(void);
	virtual void            MessageProcessed(MCMessageInfo* Info) PURE_VIRTUAL;
	virtual void			CleanupMessages(void) PURE_VIRTUAL;
	virtual void			CleanOutgoingQueue(void) PURE_VIRTUAL;
	virtual bool			CancelMessageByID(__int64 MsgID, mcUInt32 ErrorCode = MCError_Cancelled, bool Discard = false);
	virtual MCMessageInfo*	GetMessageByID(__int64 MsgID);
	/*
		PrepareDataBlock takes some buffer and prepares it for delivery. After the 
		call to this method the Data block is considered to be invalid (
		PrepareDataBlock most likely will dispose of it).
	*/
	void                    PrepareDataBlock(char* Remote,void* Data, mcInt32 DataSize, void*& OutputData,
		                        mcInt32& OutputSize, mcInt32& EncryptID, mcInt32& CompressID, mcInt32& SealID);
	/*
		PrepareDataBlock takes some buffer and prepares it for delivery. After the 
		call to this method the Data block is considered to be invalid (
		PrepareDataBlock most likely will dispose of it).
	*/
	bool                    UnprepareDataBlock(char* Remote,void* Data, mcInt32 DataSize, void*& OutputData,
		                        mcInt32& OutputSize, mcInt32 EncryptID, mcInt32 CompressID, mcInt32 SealID);
    virtual void            GetMessageSource(MCMessageInfo* Info, char* Buffer, mcInt32* BufferSize);

	char*					RealTransportName();
	
public:
	MCBaseTransport(void);
	virtual ~MCBaseTransport(void);

	void Shutdown(bool DiscardUnsentMessages);

	virtual mcInt32			GetOutgoingMessagesCount() PURE_VIRTUAL;
	/*
		Specifies whether the transport is active and can be used to deliver 
		messages.
	*/
	bool                    getActive(void);
	void                    setActive(bool Value);
	MCBaseCompression*      getCompressor(void);
	void                    setCompressor(MCBaseCompression* Value);
	MCCBaseEncryption*      getEncryptor(void);
	void                    setEncryptor(MCCBaseEncryption* Value);
	MCBaseSealing*          getSealer(void);
	void                    setSealer(MCBaseSealing* Value);
	/*
		USe this property to specify how much data can be sent in a single
		message.
		If the message is larger, than the specified limit, it is declined.
	*/
	mcInt32                    getMaxMsgSize(void);
	void                    setMaxMsgSize(mcInt32 v);
	char*					getName(void);
	void					setName(char* Value);

	/*
		MaxTimeout specifies a timeout for SendMessage method (so that
		Sendessage is not blocked forever if the destination is unreachable).
	*/
	mcUInt32           getMaxTimeout(void);
	void                    setMaxTimeout(mcUInt32 v);
	MCMessenger*            getMessenger(void);
	void                    setMessenger(MCMessenger* Value);
	bool                    getIgnoreIncomingPriorities(void);
	void                    setIgnoreIncomingPriorities(bool Value);
	
};
  
class MCDirectTransport:public MCBaseTransport 
{
protected:
	
	MCList*                 FOutgoingQueue;
	
	char*					FTransportID;
	
	virtual void			DoSetActive();
	virtual void            CancelMessage(MCList* Queue, MCMessageInfo* Info);
	virtual bool            DeliverMessage(char* DestAddress, MCMessageInfo* Info);
	virtual void            MessageProcessed(MCMessageInfo* Info);
	virtual void            DoMessageProcessed(MCMessageInfo* Info);
	virtual void            ReplyReceived(MCMessageInfo* Info);
	virtual void            MessageReceived(MCMessageInfo* Info);
	virtual void			CleanupMessages();
	virtual void			CleanOutgoingQueue(void);
	virtual MCMessageInfo*  GetMessageByID(__int64 MsgID);
public:
							MCDirectTransport();
	virtual					~MCDirectTransport();
	virtual mcInt32			GetOutgoingMessagesCount();

	static MCDirectTransport* FindTransportByID(char* ID);
	char*					getTransportID() { return FTransportID; };
};

class MCSimpleTransport: public MCBaseTransport 
{
protected:
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
	MCSemaphore*            FOutgoingCount;
#else
#endif
	MCList*                 FOutgoingQueue;

    virtual void			CleanupMessages();
	virtual void			CleanOutgoingQueue(void);
	virtual MCMessageInfo*	GetMessageByID(__int64 MsgID);
	void					MessageReceived(MCMessageInfo* Info);
	void					ReplyReceived(MCMessageInfo* Info);
	virtual void			DeliveryFailed(MCMessageInfo* Info);
	virtual void			MessageProcessed(MCMessageInfo* Info);
	virtual void			CancelMessage(MCList* Queue, MCMessageInfo* Info);
	
    virtual void			KickSender(bool MessageFlag);
    virtual void			CancelMessageInSender(MCMessageInfo* Info);
    virtual bool			WasCancelled(MCMessageInfo* Info);
    virtual void			SetInfoSMessenger(MCMessageInfo* Info);

public:
							MCSimpleTransport();
	virtual					~MCSimpleTransport();

	virtual mcInt32			GetOutgoingMessagesCount();
};

// internal service functions

void    equPstr(char* p, char* s, mcInt32 max);
char*   Pstr2c(char* p);
char*   strCopy(const char* s, mcInt32 f, mcInt32 l = -1);
mcInt32    strPos(char c, const char* s);

extern double	Now(void);
extern mcInt32 	CreateUniqueID(void);
extern void     DefineOS(void);
#ifdef USE_NAMESPACE
}
#endif

#endif
