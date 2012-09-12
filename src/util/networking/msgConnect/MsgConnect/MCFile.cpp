//====================================================
//                                                    
//  EldoS MsgConnect                                 
//                                                    
//   Copyright (c) 2001-2010, EldoS                   
//                                                    
//====================================================
#include "MC.h"
#include "MCFile.h"
 
#ifdef USE_NAMESPACE
namespace MsgConnect
{
#endif


TFile::TFile(char* Fn, int Am)
:TStream() 
{
    char m[4]="  b";
    FName = _strdup(Fn);
    if(!Am) 
        Am = stRead;
    if(Am & stRead) 
        m[0] = 'r';
    if((Am & stWrite) || (Am & stCreate)) 
        m[0] = 'w';
    if(Am & stCreate) 
        m[1] = '+'; 
    else 
    { 
        m[1] = 'b'; 
        m[2] = 0; 
    }
    IntFile = fopen(Fn, m);
    FSize = RSize = 0;
}

TFile::~TFile(void) 
{ 
    if(!Deleted) 
        Shutdown(); 
}

long TFile::Read(void* Buf, mcInt32 Cnt) 
{
    return((mcInt32)fread(Buf, 1, Cnt, IntFile));
}

long TFile::Write(const void* Buf, mcInt32 Cnt) 
{
    RSize = false;
    return((mcInt32)fwrite(Buf, 1, Cnt, IntFile));
}

long TFile::Seek(mcInt32 Cnt) 
{
    fseek(IntFile, Cnt, SEEK_SET);
    return this->Pos();
}

long TFile::Pos(void) 
{
    return(ftell(IntFile));
}

long TFile::Size(void) 
{
    if(!RSize) 
    {
        mcInt32 cp = this->Pos();
        fseek(IntFile, 0, SEEK_END);
        FSize = this->Pos();
        this->Seek(cp);
        RSize = true;
    }
    return(FSize);
}

bool TFile::Eof(void) 
{ 
    return(feof(IntFile) != 0); 
}

LPSTR TFile::PathName(void) 
{ 
    return(_strdup(FName)); 
}

void TFile::DeleteFile(void) 
{ 
    char* fn = _strdup(FName);
    TStream::DeleteFile();
#ifndef _WIN32_WCE
    remove(fn);
#else
// !!! troubles to delete file
#endif
    free(fn);
}

void TFile::Shutdown(void) 
{ 
    fclose(IntFile); 
    free(FName); 
}

TStream* OpenFile(char* File, int m)
{
    if(!File) 
        return NULL;
    TFile* f = new TFile(File, m);
    if(!f->IntFile) 
    { 
        delete f; 
        f = NULL; 
    }
    return f;
}

void InitFileProc(void)
{
    OpenStream = /*&*/OpenFile;
}

#ifdef USE_NAMESPACE
}
#endif

