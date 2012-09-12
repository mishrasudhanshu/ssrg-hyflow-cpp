//====================================================
//                                                    
//  EldoS MsgConnect                                 
//                                                    
//   Copyright (c) 2001-2010, EldoS                   
//                                                    
//====================================================

#ifndef __MCHTTP__
#define __MCHTTP__
 

#include "MC.h"
#include "MCBase.h"
#include "MCSock.h"
#include "MCSyncs.h"
#include "MCThreads.h"
#include "MCSocket.h"
#include "MCStream.h"
#include "MCSocketBase.h"

#ifdef USE_NAMESPACE
namespace MsgConnect
{
#endif

class MCHttpConnectionEntry;
class MCHttpTransport: public MCInetTransport
{
friend class MCHttpTransportJob;
protected:
    char*                   FProxyAddress;
    struct sockaddr_in      FProxyIP;
    unsigned short          FProxyPort;
    bool                    FUseProxy;
	mcInt32                 FRequestTime;
	

    virtual MCInetTransportJob*
                                CreateTransportJob(void);
    virtual void                NotifyJob(MCInetTransportJob* Job, bool MessageFlag);
	virtual void                DoSetActive(void);
    virtual MCInetConnectionEntry*
                                CreateConnectionEntry(void);
    virtual bool                NeedLiveConnectionForDelivery(void);
    virtual mcUInt32	        getDeliveryInterval(void) 
    {
        return FReqTime;
    };

public:
	MCHttpTransport();
    virtual ~MCHttpTransport();
	
    void                        setProxyAddress(const char* ProxyAddr);
    const char*                 getProxyAddress(void);
    void                        setProxyPort(unsigned short ProxyPort);
    unsigned short              getProxyPort(void);
    void                        setUseProxy(bool UseProxy);
    bool                        getUseProxy(void);
    void                        setRequestTime(mcInt32 ReqTime);
    mcInt32                     getRequestTime(void);
};
  
// TransportJob performs data transfer for the sockets.

enum HttpSendingStage 
{
    ssHeader = 0,
    ssBody   = 1
};

class MCHttpTransportJob: public MCInetTransportJob {
protected:
    mcInt32                 FAlreadyReceived;
    mcInt32                 FHeaderRead;
    mcInt32                 FReadingState;
    mcInt32                 FWasRead;
    bool					FChunked;
    mcInt32                 FChunkSize;
    mcInt32                 FChunkRead;
    mcInt32                 FReadMsgSize;
    mcInt32                 FHeaderSize;
    mcInt32                 FHeaderSent;
    HttpSendingStage		FSendingStage;
	mcUInt32				FLastRequestTime;

    virtual bool            IsMessageRead(void);
    virtual void            UpdateConnectionContext(void);
    virtual void            SetTransporterAddress(MCInetConnectionEntry* Ent, MCSocket* Transp);
	bool					ParseHeaders(const char* Headers, mcUInt32 Len, char* URL, char* Host, mcInt32& HttpCode, bool& POST, mcInt32& ContentLength, bool& Chunked);
	virtual bool            PostConnectionStep(void);
	virtual bool			InitializeConnection();
	
	bool                    ReceiveData();

    /*
	Writes data to the socket. Returns true if the operation was successful and 
	false if the socket returned an error.
	*/
	bool					SendData(void);
    mcInt32                 ServerBuildHTTPHeader(char* Buffer, mcInt32 MaxBufferLen, MCHttpConnectionEntry* HttpEnt);
    mcInt32                 ClientBuildHTTPHeader(char* Buffer, mcInt32 MaxBufferLen, mcInt32 DataLen);
	virtual bool			IsRequestNeeded();
	virtual void			ChangeLastSendTime(MCMessageState state);
	virtual void			ChangeLastRecvTime(MCMessageState state);


public:
	MCHttpTransportJob(void);
	virtual ~MCHttpTransportJob(void);
};
  

class MCHttpConnectionEntry: public MCInetConnectionEntry {
protected:
	bool					FUseProxy;
	struct sockaddr_in		FProxyIP;
    mcInt32                 FContentLength;
    char*                   FURL;
    bool                    FMessageRead;
    
public:
	MCHttpConnectionEntry(void);
	virtual ~MCHttpConnectionEntry(void);

	virtual void    Reset(void);
	void			setUseProxy(bool Value);
	bool			getUseProxy(void);
	void			setProxyIP(sockaddr_in Value);
	sockaddr_in		getProxyIP(void);
    void            setContentLength(mcInt32 Value);
    mcInt32			getContentLength(void);
    char*           getURL(void);
    void            setURL(char* URL);
    bool            getMessageRead(void);
    void            setMessageRead(bool Value);
};

#ifdef USE_NAMESPACE
}
#endif

#endif
