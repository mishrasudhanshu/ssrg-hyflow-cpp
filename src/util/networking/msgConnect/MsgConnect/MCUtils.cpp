//====================================================
//                                                    
//  EldoS MsgConnect                                  
//                                                    
//   Copyright (c) 2001-2010, EldoS                   
//                                                    
//==================================================== 

#include "MC.h"
#include "MCUtils.h"

#ifdef USE_NAMESPACE
namespace MsgConnect
{
#endif

#ifndef __APPLE__
mcUInt32 MCMainThreadID;
#else
pthread_t MCMainThreadID;
#endif

bool IsLinux = false;



void* MCMemAlloc(mcInt32 Size)
{
	if(Size > 0)
    {
        void* result = (void*)malloc(Size);
        if (NULL == result)
            THROW_ERROR(MCError_NotEnoughMemory);
        return result;
    }
	else
		return NULL;
}

void MCMemFree(void* Block)
{
	if (NULL != Block)
        free(Block);
}

void MCFreeNull(void*& Block)
{
	MCMemFree(Block);
	Block = NULL;
}

// ********************************* MCObject *********************************
MCObject::MCObject(void)
//:FLogger(NULL)
{
//	FLogger = MCLogger::Singletone();
}

MCObject::~MCObject()
{
/*
	if (FLogger == MCLogger::Singletone()) 
    {
        FLogger->Release(); 
        FLogger->Release();
    }
*/
}

/*

void MCObject::setLogger(MCLogger* Logger)
{
	if (FLogger == MCLogger::Singletone()) 
    {
		FLogger->Release();
        FLogger->Release();
    }
	FLogger = Logger;
}
*/

// ********************************* EMCError *********************************


EMCError::EMCError(mcInt32 ErrorCode)
:FErrorCode(ErrorCode), FSubCode(0), msg(NULL)
{
}

EMCError::EMCError(mcInt32 ErrorCode, mcInt32 SubCode)
:FErrorCode(ErrorCode), FSubCode(SubCode), msg(NULL)
{
}

#ifdef ENABLE_SEH2CPP
EMCError::EMCError(mcInt32 ErrorCode, mcInt32 SubCode, EXCEPTION_POINTERS* ExpPtr)
:FErrorCode(ErrorCode), FSubCode(SubCode), FExPtr(ExpPtr)
{
}
#endif

mcInt32 EMCError::ErrorCode(void)  const 
{
	return FErrorCode;
}

mcInt32 EMCError::SubCode(void) const 
{
    return FSubCode;
}

TCHAR* EMCError::GetErrorMessage(void) const 
{
	switch(FErrorCode) {
		case MCError_BadTransport:
			return TEXT("No suitable transport found");
    	case MCError_TransportInactive:
      		return TEXT("Transport is inactive");
		case MCError_BadDestinationName:
			return TEXT("Bad destination address format");
		case MCError_WrongSenderThread:
			return TEXT("Use destination messenger's thread context.");
		case MCError_WrongReceiverThread:
			return TEXT("This method should be called within Messenger creator thread");
		case MCError_NoQueueName:
			return TEXT("Messenger Queue name must be specified.");
		case MCError_SecurityInitFailed:
			return TEXT("Security descriptor initialization failed");
		case MCError_InvalidEncryptionKey:
			return TEXT("Invalid key specified for encryption component");
		case MCError_NotEnoughMemory:
			return TEXT("Not enough memory to perform operation");
		case MCError_WrongCredentials:
			return TEXT("Sender presented invalid credentials");
        case MCError_GenTransportFailure:
            return TEXT("Transport failure due to unexpected error");
        case MCError_NoDecryptor:
            return TEXT("Decryptor is not found");
        case MCError_NoDecompressor:
            return TEXT("Decompressor is not found");
        case MCError_NoSignatureHandler:
            return TEXT("Decompressor is not found");
        case MCError_InvalidSignature:
            return TEXT("Invalid signature in data stream");
        case MCError_TransportTimeout:
            return TEXT("Timeout during message tranferring");
        case MCError_ReplyDeferred:
            return TEXT("Reply to message is deferred. It is internal error. Please write us bug report");
        case MCError_SEHRaised:
            return TEXT("SEH exception was raised.");
		case MCError_MMFCreationFailed:
			return TEXT("Creation of memory mapping failed");
		case MCError_MutexCreationFailed:
			return TEXT("Creation of memory mapping access mutex failed");
		case MCError_NoMessengerName:
			return TEXT("No messenger name specified");
		case MCError_NotASocket:
			return TEXT("Operation called on invalid socket");
		case MCError_WrongSocketState:
			return TEXT("Socket is in state that is not acceptable for current operation");
		case MCError_WinsockInitFailed:
			return TEXT("Failed to initialize Winsock");
		case MCError_InvalidAddress:
			return TEXT("Invalid address specified");
		case MCError_ConnectionFailed:
			return TEXT("Connection failed");
		case MCError_ConnectionLost:
			return TEXT("Connection to remote host was lost");
        
        case MCError_InvalidHTTP:
            return TEXT("Cannot parse HTTP header - seems it is invalid.");
		default:
			return TEXT("Unspecified error");
	}
}


#ifdef USE_NAMESPACE
}
#endif

