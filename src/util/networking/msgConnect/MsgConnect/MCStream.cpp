//====================================================
//                                                    
//  EldoS MsgConnect                                 
//                                                    
//   Copyright (c) 2001-2010, EldoS                   
//                                                    
//====================================================
#include "MC.h"
#include "MCUtils.h"
#include "MCStream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MCThreads.h"
 
#ifdef USE_NAMESPACE
namespace MsgConnect
{
#endif


MCStream::MCStream(void)
{
	FLen = 0;
	FPos = 0;
}

long MCStream::Len(void)
{
	return FLen;
}

long MCStream::Pos(void)
{
	return FPos;
}

long MCStream::SetPos(long l)
{
	return FPos = l;
}

#ifdef MC_NO_PURE_VIRTUAL
long MCStream::Read(void* d, long l)
{
	MC_pure_virtual();
	return 0;
}

long MCStream::Write(void* d, long l)
{
	MC_pure_virtual();
	return 0;
}
#endif

MCMemStream::MCMemStream(void):MCStream()
{
	FData = NULL;
	FSize = 0;
	FExt = false;
}

long MCMemStream::Grow(long l)
{
	long delta = (mcInt32)(FSize * 1.33 + 0.5) - FSize;
    if (delta < 32)
        delta = 32;
	
    char* p = NULL;
    if(FExt)
		return 0;
	
    if(l < 1)
		return 0;
    delta = delta < l ? l : delta;

	p = new char[FSize + delta];
	memset(p, 0, (FSize + delta) * sizeof(char));
	if(FSize)
	{
		memcpy(p, FData, FSize * sizeof(char));
		delete[] FData; FData = NULL;
	}
	FSize += delta;
	FData = p;
	return delta;
}

MCMemStream::~MCMemStream(void)
{
	Free();
}

void MCStream::Free(void)
{
	FLen = 0;
	FPos = 0;
}

void MCMemStream::Free(void)
{
	if(!FExt && FData)
		delete[] FData;
	FData = NULL;
    MCStream::Free();
	FSize = 0;
	FExt = false;
}

long MCMemStream::SetPos(long l)
{
	if(l < 0)
		return FPos;
	if(FExt && (l >= FLen))
	{
		MCStream::SetPos(FLen);
		return FPos;
	}
	MCStream::SetPos(l);
	if(l > FSize)
		Grow(l - FSize);
	if(l > FLen)
		FLen = l;
	return FPos;
}

long MCMemStream::Read(void* d, long l)
{
	if(l < 1)
		return 0;
	long x = ((FPos + l) > FLen) ? (FLen - FPos) : l;
	memcpy(d, &FData[FPos], x);
	FPos += x;
	return x;
}

long MCMemStream::Write(void* d, long l)
{
	if(l < 1)
		return(0);
	long x = l;
	if((FPos + l) > FSize)
		if(Grow(FPos + l - FSize) == 0)
			x = FSize - FPos;
		else
			FLen = FPos + l;
	memcpy(&FData[FPos], d, x);
	FPos += x;
	if(FPos > FLen)
		FLen = FPos;
	return x;
}

void MCMemStream::SetPointer(char* data, long l)
{
    Free();
	if(l < 0)
		return;
	if(data)
	{
		FExt = true;
		FData = data;
		FSize = FLen = l;
	}
	else
	{
		FData = NULL;
		FSize = 0;
		FExt = false;
	}
}

#ifdef _WIN32

#ifdef UNICODE
#   include <tchar.h>
#endif

MCTmpFileStream::MCTmpFileStream(char*s, __int64 i)
:FName(NULL)
{
#ifdef _UNICODE
    int reqSize = 0;
    MultiByteToWideChar(CP_ACP, 0, s, -1, NULL, reqSize);
    TCHAR* sW = (TCHAR*)MCMemAlloc(reqSize+1);
    MultiByteToWideChar(CP_ACP, 0, s, -1, sW, reqSize);
    TCHAR buf[MAX_PATH];
    GetTempFileName(sW, _T("mc"), 0, buf);
    FName = (TCHAR*)MCMemAlloc((_tcslen(buf)+1) * sizeof(TCHAR));
    _tcscpy(FName, buf);
    MCMemFree(sW); sW = NULL;
#else
    TCHAR buf[MAX_PATH];
    GetTempFileName(s, "mc", 0, buf);
    FName = (TCHAR*)MCMemAlloc(strlen(buf)+1);
    strcpy(FName, buf);
#endif

    FFile = CreateFile(FName, GENERIC_WRITE | GENERIC_READ, 0, NULL, CREATE_ALWAYS, 
        FILE_ATTRIBUTE_NOT_CONTENT_INDEXED | FILE_ATTRIBUTE_TEMPORARY | 
        FILE_FLAG_SEQUENTIAL_SCAN, 0);

    if (INVALID_HANDLE_VALUE == FFile)
        THROW_ERROR(GetLastError());
}

MCTmpFileStream::~MCTmpFileStream()
{
    Free();
}

long MCTmpFileStream::Len(void)
{
	unsigned int oldPos = SetFilePointer(FFile, 0, NULL, FILE_CURRENT);
    unsigned int len = SetFilePointer(FFile, 0, NULL, FILE_END);
    SetFilePointer(FFile, oldPos, NULL, FILE_BEGIN);
    return len;
}

long MCTmpFileStream::Pos(void)
{
	return SetFilePointer(FFile, 0, NULL, FILE_CURRENT);
}

long MCTmpFileStream::SetPos(long l)
{
	return SetFilePointer(FFile, l, NULL, FILE_BEGIN);
}

long MCTmpFileStream::Read(void* d, long l)
{
    DWORD wasRead = 0;
    ReadFile(FFile, d, l, &wasRead, NULL);
    return wasRead;
}

long MCTmpFileStream::Write(void* d, long l)
{
    DWORD wasWritten = 0;
    WriteFile(FFile, d, l, &wasWritten, NULL);
    return wasWritten;
}

void MCTmpFileStream::Free(void)
{
	if (INVALID_HANDLE_VALUE != FFile)
    {
        CloseHandle(FFile); 
        FFile = INVALID_HANDLE_VALUE;
    }
    if (NULL != FName)
    {
        DeleteFile(FName);
        MCMemFree(FName); FName = NULL;
    }
}

#else


MCTmpFileStream::MCTmpFileStream(char*s, __int64 i): MCStream()
{
	char buf[1024];
	//build temp file name
#ifdef __APPLE__
#else
	sprintf(buf, "mc%ld%d%d.tmp", i, IntGetCurrentThreadID(), GetCurrentThread());
#endif
	FName = (char*)malloc(strlen(s)+strlen(buf)+1);
	strcpy(FName, s);
	strcat(FName, buf);
	FFile = fopen(FName, "wb");
	FLen = 0;
	FRsize = false;
}

long MCTmpFileStream::Len(void)
{
	if(!FRsize)
	{
		long cp = this->Pos();
		fseek(FFile, 0, SEEK_END);
		FLen = this->Pos();
		this->SetPos(cp);
		FRsize = true;
	}
	return MCStream::Len();
}

long MCTmpFileStream::Pos(void)
{
	return ftell(FFile);
}

long MCTmpFileStream::SetPos(long l)
{
	FRsize = false;
	return fseek(FFile, l, SEEK_SET);
}

long MCTmpFileStream::Read(void* d, long l)
{
    return fread(d, 1, l, FFile);
}

long MCTmpFileStream::Write(void* d, long l)
{
  FRsize = false;
  return fwrite(d, 1, l, FFile);
}

void MCTmpFileStream::Free(void)
{
	fclose(FFile);
#ifdef _WIN32_WCE
    long l = strlen(FName)+1;
    wchar_t* w = new wchar_t[l];
    mbstowcs(w, FName, l);
	DeleteFile(w);
#else
	remove(FName);
#endif
    free(FName);
}
#endif

#ifdef USE_NAMESPACE
}
#endif
