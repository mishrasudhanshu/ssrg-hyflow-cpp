//====================================================
//                                                    
//  EldoS MsgConnect                                 
//                                                    
//   Copyright (c) 2001-2010, EldoS                   
//                                                    
//====================================================
 
#ifndef __MCSOCKET__
#define __MCSOCKET__


#include "MC.h"
#include "MCSocketBase.h"
#include "MCInetTransport.h"

#ifdef USE_NAMESPACE
namespace MsgConnect
{
#endif

 
class MCSocketTransport: public MCInetTransport
{
protected:
    virtual MCInetConnectionEntry*      CreateConnectionEntry(void);
    virtual MCInetTransportJob*         CreateTransportJob(void);
    virtual void                        NotifyJob(MCInetTransportJob* Job, bool MessageFlag);
    virtual bool                        NeedLiveConnectionForDelivery(void) 
    { 
        return true; 
    };
    
    virtual mcUInt32						getDeliveryInterval(void)
    {
        return getAttemptsInterval();
    };


public:
    MCSocketTransport(void);
    virtual ~MCSocketTransport();
};

  
class MCSocketTransportJob: public MCInetTransportJob
{
protected:
	bool                    FInHandshake;

    bool                    HSPacketSent(void);
	bool                    HSReplyReceived(bool& Success);
	bool                    HSRequestReceived(void);
	bool                    PerformHandshake(void);
	bool                    PrepareMessageForHandshake(void);
	bool                    PrepareReplyForHandshake(MCInetHeader* RqHeader);
	virtual bool			InitializeConnection();

    virtual void            UpdateConnectionContext(void) {};
    virtual void            SetTransporterAddress(MCInetConnectionEntry* Ent, MCSocket* Transp);
    virtual bool            PostConnectionStep(void);
    virtual bool            NeedCloseCompletedExchg(void);
    virtual bool            ReceiveData(void);
    virtual bool            SendData(void);
    virtual void            MakeFetchPacket(void);
    virtual MCMessageInfo*  MakeEmptyReply(MCMessageInfo* Info);

public:
    MCSocketTransportJob();
    virtual ~MCSocketTransportJob();

    bool                    getInHandshake(void);
	void                    setInHandshake(bool Value);

};

#ifdef USE_NAMESPACE
}
#endif

#endif
