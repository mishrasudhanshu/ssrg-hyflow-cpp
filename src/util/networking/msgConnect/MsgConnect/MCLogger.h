//====================================================
//                                                    
//  EldoS MsgConnect                                 
//                                                    
//   Copyright (c) 2001-2010, EldoS                   
//                                                    
//====================================================

#ifndef __MCLOGGER__
#define __MCLOGGER__

#include "MC.h"
#include "MCSyncs.h"
 
#ifdef USE_NAMESPACE
namespace MsgConnect
{
#endif


typedef enum {
	llMandatory = 0, 
	llCritical, 
	llEmergency, 
	llSerious, 
	llUrgent, 
	llImportant, 
	llPriority, 
	llInformation, 
	llLowLevel, 
	llTrivial
} MCLogLevel;

#ifndef __GNUC__
#else
typedef void (*MCLogMessageHandler)(void* Sender, unsigned int ThreadID, const char* Msg, unsigned int Code, const char* Location);
#endif

class MCLogger
{
friend class MCObject;
protected:
	MCLogMessageHandler FOnLogMessage;
	MCLogLevel			FLevel;
	unsigned int		FRefCount;
	MCCriticalSection	FGuard;
	static MCLogger*	FSingletone;

	static MCLogger*	Singletone(void);
public:
	MCLogger(void);
	virtual ~MCLogger();
	void				PutMessage(MCLogLevel LogLevel, const char* Msg, unsigned int Code, const char* Location);
	void				setOnLogMessage(MCLogMessageHandler Handler);
	MCLogMessageHandler getOnLogMessage(void);
	void				setLogLevel(MCLogLevel Level) { FLevel = Level; };
	MCLogLevel			getLogLevel(void) { return FLevel; };
	void				Release(void);
};

#ifdef USE_NAMESPACE
}
#endif


#endif
