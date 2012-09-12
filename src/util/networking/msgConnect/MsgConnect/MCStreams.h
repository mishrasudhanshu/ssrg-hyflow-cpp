//====================================================
//                                                    
//  EldoS MsgConnect                                 
//                                                    
//   Copyright (c) 2001-2010, EldoS                   
//                                                    
//====================================================

#ifndef MCSTREAMS_H
#define MCSTREAMS_H
 
#include "MC.h"
#include "MCPlatforms.h"

#ifdef USE_NAMESPACE
namespace MsgConnect
{
#endif


enum { stRead=1,stWrite=2,stCreate=4 };

class TStream {
protected:
    int         Mode;
    int         Deleted; // set to true by delete_file()
    DEFSECT virtual void Shutdown(void);  // actual destruction code called by destructor
public:
    DEFSECT TStream(void);
    DEFSECT virtual long    Read(void* Buf, long Cnt);        // reading from
    DEFSECT virtual long    Write(const void* Buf, long Cnt); // writing to
    DEFSECT virtual long    Seek(long Cnt);  // setting stream position
    DEFSECT virtual long    Pos(void);       // querying stream position
    DEFSECT virtual long    Size(void);      // querying stream size
    DEFSECT virtual bool    Eof(void);       // is end of stream
    DEFSECT virtual char*   PathName(void); // returns open stream name
    DEFSECT virtual void    DeleteFile(void); // closes file with shutdown() call and then
                                  // deletes it
    DEFSECT virtual ~TStream(void);
};

extern TStream* (*OpenStream)(char* File, int AMode); // call with file==NULL
                                                      // opens temporary file

DEFSECT char*   ReadTextFromStream(TStream* F);
DEFSECT void    WriteTextToStream(TStream* F, char* S, bool NewLine = true);

DEFSECT char*   ReadStringFromStream(TStream* F, long i);
DEFSECT void    WriteStringToStream(TStream* F, char* S, bool write_len);

DEFSECT wchar_t* ReadWideStringFromStream(TStream* F, long i);
DEFSECT void     WriteWideStringToStream(TStream* F, wchar_t* S, bool write_len);

#ifdef USE_NAMESPACE
}
#endif


#endif
