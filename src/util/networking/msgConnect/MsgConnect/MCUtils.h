//====================================================
//                                                    
//  EldoS MsgConnect                                 
//                                                    
//   Copyright (c) 2001-2010, EldoS                   
//                                                    
//====================================================

#ifndef __MCUTILS__
#define __MCUTILS__
 
#include "MC.h"

#ifdef USE_NAMESPACE
namespace MsgConnect
{
#endif

extern bool IsLinux;
extern bool IsWin95;
extern bool IsWinNT;
extern bool IsWin2000;
extern bool IsWinNTUp;
extern bool IsWin2000Up;
extern bool IsWinXP;
extern bool IsWinXPUp;
extern bool IsWin95OSR2;
extern bool IsWin98;
extern bool IsWinME;
extern bool IsWin98Up;


#ifdef _WIN32
bool    SupportsTSNames(void);
#endif
void*   MCMemAlloc(mcInt32 Size);
void    MCMemFree(void* Block);
void    MCFreeNull(void* &Block);


enum {
    MCError_WrongReceiverThread	= 1,
	MCError_WrongSenderThread	= 2,

    MCError_NoQueueName			= 3,
	MCError_BadTransport		= 4,
	MCError_BadDestinationName	= 5,
	MCError_SecurityInitFailed	= 6,
	MCError_GenTransportFailure	= 8,
	MCError_NoDecryptor			= 9,
	MCError_NoDecompressor		= 10,
	MCError_NoSignatureHandler	= 11,
	MCError_InvalidSignature	= 12,
	MCError_TransportTimeout	= 13,
	MCError_InvalidEncryptionKey= 14,
	MCError_NotEnoughMemory		= 15,
	MCError_WrongCredentials	= 16,
    //MCError_CheckSynchronizeError = 18,
    MCError_ThreadCreateError	= 19,
    MCError_ThreadError			= 20,
    MCError_ReplyDeferred       = 25,
	MCError_NoTransformer		= 26,
	MCError_RoutingNotAllowed	= 27,
	MCError_TransportInactive	= 28,
    MCError_SEHRaised           = 29,
	MCError_SynchroObjectFailed = 30,
	MCError_Cancelled			= 50,
	MCError_Shutdown			= 51,
    MCError_PacketTooLarge      = 52,
	
    MCError_NoMessengerName		= 101,
    MCError_MMFCreationFailed	= 102,
    MCError_MutexCreationFailed	= 103,
    
    MCError_NotASocket          = 201,
    MCError_WrongSocketState    = 202,
    MCError_WinsockInitFailed   = 203,
    MCError_InvalidAddress      = 204,
    MCError_ConnectionFailed    = 205,
    MCError_ConnectionLost      = 206,
    MCError_WrongSocketResult   = 207,
    MCError_WrongSocketType     = 208,

    MCError_InvalidHTTP         = 301,
    
    MCError_SocksNoAcceptableAuthMethods    = 501,
    MCError_SocksAuthFailure                = 502,
    MCError_SocksGenFailure                 = 503,
    MCError_SocksNotallowed                 = 504,
    MCError_SocksNetUnreachable             = 505,
    MCError_SocksHostUnreachable            = 506,
    MCError_SocksConnectionRefused          = 507,
    MCError_SocksTTLExpired                 = 508,
    MCError_SocksCmdNotSupported            = 509,
    MCError_SocksAddrTypeNotSupported       = 510,

    MCError_WebTunnelConnectionFailed       = 550,
    MCError_WebTunnelAuthRequired           = 551,
    MCError_WebTunnelFailure                = 552,

};

class MCObject
{
protected:
//	MCLogger* FLogger;
public:
	MCObject(void);
	virtual ~MCObject();
//	void		setLogger(MCLogger* Logger);
};


#if defined(_WIN32) && defined(USE_CPPEXCEPTIONS) && defined(SEH2CPP) && !defined(__BORLANDC__)
#   define ENABLE_SEH2CPP
#endif

class EMCError
{
#ifdef ENABLE_SEH2CPP
friend void SEHTranslator(unsigned int u, PEXCEPTION_POINTERS pExp);
#endif
friend class MCWorkerThread;
private:
	mcInt32        FErrorCode;
    mcInt32        FSubCode;
	TCHAR*      msg;
#ifdef ENABLE_SEH2CPP
    EXCEPTION_POINTERS* FExPtr;
    EMCError(mcInt32 ErrorCode, mcInt32 SubCode, EXCEPTION_POINTERS* ExpPtr);
#endif
public:
	EMCError(mcInt32 ErrorCode);
    EMCError(mcInt32 ErrorCode, mcInt32 SubCode);

	mcInt32 ErrorCode(void) const ;
    mcInt32 SubCode(void) const ; 
    TCHAR* GetErrorMessage() const ;
};

#ifdef USE_NAMESPACE
}
#endif

#endif
