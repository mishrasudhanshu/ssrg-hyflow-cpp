//====================================================
//                                                    
//  EldoS MsgConnect                                 
//                                                    
//   Copyright (c) 2001-2010, EldoS                   
//                                                    
//====================================================

#ifndef MCFILE_H
#define MCFILE_H
 
#include "MC.h"
#include "MCStreams.h"

#ifdef USE_NAMESPACE
namespace MsgConnect
{
#endif
class TFile:public TStream {
#ifndef PALM
	long	FSize;
	bool	RSize;
#endif
protected:
#ifndef PALM
	FILE*	IntFile;
	char*	FName;
#else
	long	FSize;
	long	FPos;
	char*	File;
#endif
	DEFSECT virtual void	Shutdown(void);
	DEFSECT friend TStream*	OpenFile(char* File, int m);
public:
  TFile(char*, int);
  DEFSECT virtual ~TFile(void);

  DEFSECT virtual long		Read(void* Buf, mcInt32 Cnt);
  DEFSECT virtual long		Write(const void* Buf, mcInt32 Cnt);
  DEFSECT virtual long		Seek(mcInt32 Cnt);
  DEFSECT virtual long		Pos(void);
  DEFSECT virtual long		Size(void);
  DEFSECT virtual bool		Eof(void);
  DEFSECT virtual char*		PathName(void);
  DEFSECT virtual void		DeleteFile(void);
};

DEFSECT void InitFileProc(void);

#ifdef USE_NAMESPACE
}
#endif

#endif
