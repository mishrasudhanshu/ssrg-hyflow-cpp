//====================================================
//                                                    
//  EldoS MsgConnect                                 
//                                                    
//   Copyright (c) 2001-2010, EldoS                   
//                                                    
//====================================================

#ifndef __MCSTREAM__
#define __MCSTREAM__
 
#include "MC.h"
#include <stdio.h>

#ifdef USE_NAMESPACE
namespace MsgConnect
{
#endif


class MCStream 
{
protected:
	long FLen;
	long FPos;
public:
	MCStream(void);
	virtual ~MCStream(void) {}
	virtual long Len(void);
	virtual long Pos(void);
	virtual long SetPos(long l);
	virtual long Read(void* d, long l) PURE_VIRTUAL;
	virtual long Write(void* d, long l) PURE_VIRTUAL;
	virtual void Free(void);
};

class MCMemStream:public MCStream {
protected:
	char*   FData;
	long    FSize;
	bool    FExt;
	virtual long    Grow(long l);
public:
	MCMemStream(void);
	virtual ~MCMemStream(void);
	virtual void    Free(void);
	virtual long    SetPos(long l);
	virtual long    Read(void* d, long l);
	virtual long    Write(void* d, long l);
	virtual void    SetPointer(char* data, long l);
};

#ifdef _WIN32
class MCTmpFileStream: public MCStream 
{
protected:
    HANDLE      FFile;
    TCHAR*      FName;
public:
	MCTmpFileStream(char* s, __int64 i);
    virtual ~MCTmpFileStream();
    virtual long    Len(void);
	virtual long    Pos(void);
	virtual long    SetPos(long l);
	virtual long    Read(void* d, long l);
	virtual long    Write(void* d, long l);
	virtual void    Free(void);
};

#else

class MCTmpFileStream: public MCStream {
protected:
	FILE*   FFile;
	bool    FRsize;
	char*   FName;
public:
	MCTmpFileStream(char* s, __int64 i);
	virtual long    Len(void);
	virtual long    Pos(void);
	virtual long    SetPos(long l);
	virtual long    Read(void* d, long l);
	virtual long    Write(void* d, long l);
	virtual void    Free(void);
};
#endif

#ifdef USE_NAMESPACE
}
#endif


#endif
