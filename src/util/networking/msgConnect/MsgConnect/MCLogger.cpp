//====================================================
//                                                    
//  EldoS MsgConnect                                 
//                                                    
//   Copyright (c) 2001-2010, EldoS                   
//                                                    
//====================================================
#include "MC.h"
#include "MCLogger.h"
#ifdef _WIN32
#	include <windows.h>
#else
//#	include <sys/types.h>
//#	include <unistd.h>
//#   	include <iostream.h>
#endif
 
#ifdef USE_NAMESPACE
namespace MsgConnect
{
#endif


MCLogger* MCLogger::FSingletone = NULL;

MCLogger::MCLogger(void)
:FOnLogMessage(NULL), FLevel(llTrivial), FRefCount(0)
{
}

MCLogger::~MCLogger()
{
}

MCLogger* MCLogger::Singletone(void)
{
	if (NULL == MCLogger::FSingletone)
		FSingletone = new MCLogger();
	FSingletone->FRefCount++;
	return FSingletone;
}

void MCLogger::Release(void)
{
	if (this == FSingletone)
	{
		FRefCount--;
		if (0 == FRefCount)
		{
			delete this;
			MCLogger::FSingletone = NULL;
		}
	}
}

void MCLogger::setOnLogMessage(MCLogMessageHandler Handler)
{
	FOnLogMessage = Handler;
}

MCLogMessageHandler MCLogger::getOnLogMessage(void)
{
	return FOnLogMessage;
}

void MCLogger::PutMessage(MCLogLevel LogLevel, const char* Msg, unsigned int Code, const char* Location)
{
	FGuard.Enter();
	if (FLevel >= LogLevel) 
	{
		if (NULL != FOnLogMessage)
		{
        	    TRY_BLOCK
        	    {
				int ThreadID = 0;
#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
#ifndef _WIN32_WCE
#else
#endif
#else
				ThreadID = getpid();
#endif

				FOnLogMessage(this, ThreadID, Msg, Code, Location);
		    }
        	    CATCH_EVERY
        	    {
        	    }
		}
		else
		{
#if defined(_WIN32) && !defined(_WIN32_WCE) && !defined(UNICODE) && !defined(_UNICODE)
        	    OutputDebugString(Msg);
#endif
#ifndef _WIN32
//        	    std::cerr<<Msg;
#endif
		}
	}
	FGuard.Leave();
}

#ifdef USE_NAMESPACE
}
#endif

