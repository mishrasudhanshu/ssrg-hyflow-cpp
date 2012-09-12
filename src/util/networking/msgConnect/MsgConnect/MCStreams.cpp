//====================================================
//                                                    
//  EldoS MsgConnect                                 
//                                                    
//   Copyright (c) 2001-2010, EldoS                   
//                                                    
//====================================================
#include "MC.h"
#include "MCStreams.h"
 
#ifdef USE_NAMESPACE
namespace MsgConnect
{
#endif


TStream::TStream(void) 
{ 
    Deleted = false; 
}

long TStream::Read(void*, long) 
{ 
    return 0; 
}

long TStream::Write(const void*, long) 
{ 
    return 0; 
}

long TStream::Seek(long) 
{ 
    return 0; 
}

long TStream::Pos(void) 
{ 
    return 0;
}

long TStream::Size(void) 
{ 
    return 0; 
}

bool TStream::Eof(void) 
{ 
    return true; 
}

char* TStream::PathName(void) 
{ 
    return NULL; 
}

void TStream::Shutdown(void)
{ 
}

void TStream::DeleteFile(void) 
{ 
    Shutdown(); 
    Deleted = true; 
}

TStream::~TStream(void) 
{ 
    if(!Deleted) 
        Shutdown(); 
}

TStream* DefOpenStream(char*, int)
{
    return NULL;
}

TStream* (*OpenStream)(char* File, int AMode) = &DefOpenStream;

void WriteTextToStream(TStream* F, char* S, bool NewLine)
{
    if(!F) 
        return;
    const char* nl = "\r\n";
    if(S) 
        F->Write(S, (mcInt32)strlen(S));
    if(NewLine) 
        F->Write(nl, (mcInt32)strlen(nl));
}

char* ReadStringFromStream(TStream* F, long i)
{
    if(!F) 
        return NULL;
    long x = 0;
    char* r = NULL;

    if(!F) 
        return NULL;
    if(i >= 0)
        x = i;
    else
    {
        if(F->Read(&x, sizeof(x)) < (mcInt32)sizeof(x)) 
            return NULL;
#ifdef M68K
        x = TurnLong(x);
#endif
    }
    if(x < 0)
        x = 0;
    if(x > 0) 
    {
        r = (char*)malloc(x + sizeof(char));
        if(F->Read(r, x) < x) 
            return NULL;
        r[x] = 0;
    }
    return r;
}

void WriteStringToStream(TStream* F,char* S, bool write_len)
{
    if(NULL == F) 
        return;
    long x = (mcInt32)(S ? strlen(S) : 0);
    if(x < 0)
        x = 0;
    if(write_len)
    {
        long x1 =
#ifdef M68K
        TurnLong
#endif
        (x);
        if(F->Write(&x1, sizeof(x1)) < (mcInt32)sizeof(x1))
            return;
    }
    if(x > 0 && S)
        F->Write(S, x);
}

wchar_t* ReadWideStringFromStream(TStream* F, long i)
{
    if(!F) 
        return NULL;
    long x = i;
    wchar_t* r = NULL;

    if(!F) 
        return NULL;
    if(i < 0)
    {
        if(F->Read(&x, sizeof(x)) < (mcInt32)sizeof(x))
            return NULL;
#ifdef M68K
        x = TurnLong(x);
#endif
    }
    if(x < 0)
        x = 0;
    if(x > 0) 
    {
        r = (wchar_t*)malloc(x + sizeof(wchar_t));
        if(F->Read(r, x) < (signed long)x)
            return NULL;
        r[x / sizeof(wchar_t)] = 0;
    }
    return r;
}

void WriteWideStringToStream(TStream* F, wchar_t* S, bool write_len)
{
    if(NULL == F) 
        return;
    long x = S ? (wstrlen(S) * sizeof(wchar_t)) : 0;
    if(write_len)
    {
        long x1 =
#ifdef M68K
        TurnLong
#endif
        (x);
        if(F->Write(&x1, sizeof(x1)) < (mcInt32)sizeof(x1)) 
            return;
    }
    if(x > 0 && S)
        F->Write(S, x);
}

void* lrealloc(void* V, long N, long
#ifdef PALM_CW
Os
#endif
)
{
#ifndef PALM_CW
    return(realloc(V, N));
#else
    void* t = NULL;
    long cl = (Os > N) ? N : Os;
    if(N < 1) 
    { 
        free(V); 
        return NULL; 
    }
    t = malloc(N);
    if(NULL == V) 
        return t;
    if(Os > 0) 
        memcpy(t, V, cl);
    free(V);
    return t;
#endif
}

char* ReadTextFromStream(TStream* F)
{
#ifdef PALM
#   define BS 128
#else
#   define BS 16384
#endif
    static char nl[] = "\r\n";
    static size_t nll = strlen(nl);
    long x = 0, z = -2, lr = BS, ox = 0, olr;
    char* r = NULL, *p;

    if(NULL == F) 
        return NULL;
    while(!F->Eof()) 
    {
        x += lr;
        r = (char*)lrealloc(r, x, ox);
        ox = x;
        p = r + x - lr;
        olr = lr;
        lr = F->Read(p, BS);
        if(lr < 1) 
        { 
            free(r); 
            r = NULL; 
            break; 
        }
        for(signed long l=((x>BS)?-1:0); l<(signed long)(lr+1-nll); l++)
        {
            if(0 == _strncmp(&p[l], nl, nll)) 
            {
                z = l;
                break;
            }
        }
        if(z > -2) 
        {
            x = x - olr + z;
            r = (char*)lrealloc(r, x+1, ox);
            if(x >= 0) 
                r[x] = 0;
            F->Seek(F->Pos() - lr + z + 1);
            break;
        }
    }
    if(r && !r[0]) 
    { 
        free(r); 
        return(NULL); 
    }
    return r;
}

#ifdef USE_NAMESPACE
}
#endif

