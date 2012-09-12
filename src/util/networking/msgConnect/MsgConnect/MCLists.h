//====================================================
//                                                    
//  EldoS MsgConnect                                 
//                                                    
//   Copyright (c) 2001-2010, EldoS                   
//                                                    
//====================================================

#ifndef __MCLISTS__
#define __MCLISTS__

#include "MC.h"
#include "MCPlatforms.h"
 
#ifdef USE_NAMESPACE
namespace MsgConnect
{
#endif


class MCList {
protected:
    void**          Data;
    mcInt32            Len;
    mcInt32            Size;
    bool            DuplicatesAllowed;
    bool            Sorted;
    
    void                Upgrade (mcInt32 Upg=20);
    static mcInt32         DefaultCompare(void* Sender, void* V1,void* V2);
		mcInt32                IndexOf(void* V, bool *Found);
public:
    MCList(void);
    virtual ~MCList(void);
    mcInt32                Length(void);
    virtual mcInt32        Add(void* V);
    mcInt32                Insert(mcInt32 N, void* V);
    mcInt32                Find(void* V);
    void                Del(mcInt32 N);
    bool                Del(void* V);
    void                DeleteAll(void);
    virtual void*       operator[](mcInt32 N);
    void                CopyOf(MCList* L);
    void                setDuplicatesOff(bool DisableDuplicates);
    void                setSorted(bool Sorted);
    void*               At(mcInt32 N);
    void (*OnDelete)(void* Sender, void* V);
    void (*DeleteItem)(void* Sender, void* V);
    mcInt32 (*CompareItems)(void* Sender, void* V1, void* V2);
};

class MCStringList: public MCList 
{
    static mcInt32 StringCompare(void* Sender,void* V1,void* V2);
    static void StringDelete(void* Sender,void* V);
public:
    MCStringList(void);
    char*               Text(void);
    mcInt32                Add(void* V);
    virtual mcInt32        Add(char* V);
    virtual void*       operator[](mcInt32 N);
};


class MCWideStringList: public MCList 
{
    static mcInt32     StringCompare(void* Sender, void* V1, void* V2);
    static void     StringDelete(void* Sender, void* V);
    static void*    StringDup(void* Sender,void* V);
public:
    MCWideStringList(void);
    wchar_t*            Text(void);
    mcInt32                Add(void* V);
    virtual mcInt32        Add(wchar_t* V);
    virtual void*       operator[](mcInt32 N);
};

#ifdef USE_NAMESPACE
}
#endif


#endif
