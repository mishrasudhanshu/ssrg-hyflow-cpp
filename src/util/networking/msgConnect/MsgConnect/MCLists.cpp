//====================================================
//                                                    
//  EldoS MsgConnect                                 
//                                                    
//   Copyright (c) 2001-2010, EldoS                   
//                                                    
//====================================================
#include "MC.h"
#include "MCUtils.h"
#include "MCPlatforms.h"
#include "MCLists.h"
#include <stdlib.h>
#include <string.h>
 
#ifndef _WIN32_WCE
#include <assert.h>
#else

#endif

#ifdef USE_NAMESPACE
namespace MsgConnect
{
#endif


// --------------------- tlist -----------------------

MCList::MCList(void)
{
    OnDelete = 0;
    DeleteItem = 0;
    CompareItems = &DefaultCompare;
    Data = 0;
    Size = 0;
    Len = 0;
    DuplicatesAllowed = true;
    Sorted = false;//true;
}

MCList::~MCList(void)
{
    DeleteAll();
    MCMemFree(Data); Data = NULL;
}

void MCList::setDuplicatesOff(bool DisableDuplicates)
{
    DuplicatesAllowed = !DisableDuplicates;
}

void MCList::setSorted(bool Sorted)
{
    this->Sorted = Sorted;
}

mcInt32 MCList::Length(void)
{
    return Len;
}

void MCList::DeleteAll(void)
{
    for(mcInt32 x=Len; x>0; x--)
        Del(x-1);
  
    Len = 0;
}

void MCList::Del(mcInt32 N)
{
    if (N < 0 || N > Len-1) 
        return;
    if(0 != OnDelete) 
        OnDelete(this, Data[N]);

    if(DeleteItem) 
        DeleteItem(this, Data[N]);
  
    if(N < Len-1)
        memmove(&Data[N], &Data[N+1], (Len-N-1)*sizeof(void*));
    Len--;
}

bool MCList::Del(void* V)
{
    mcInt32 index = Find(V);
    if(-1 != index)
    {
        Del(index);
        return true;
    }
    else
        return false;
}

typedef void* PVOID;

void MCList::Upgrade(mcInt32 Upg)
{
    mcInt32 delta = (mcInt32)(Size * 1.33 + 0.5);
    if (delta < 1)
        delta = 32;

    if (Upg < 1) 
        return;
    mcInt32 x = (delta - Size) < Upg ? Upg : delta;
    void** p = (void**)MCMemAlloc(sizeof(void*) * (Size + x));

    memset(p, 0, (Size + x) * sizeof(void*));
    if(0 != Size)
    {
        memcpy(p, Data, Size*sizeof(void*));
        if(Data) 
            MCMemFree(Data);
        Data = 0;
    }
    Size += x;
    Data = p;
}

mcInt32 MCList::Add(void* V)
{
    if (Sorted)
			return(Insert(IndexOf(V, NULL), V));
		else
			return(Insert(Len, V));
}

mcInt32 MCList::DefaultCompare(void* Sender,void* V1,void* V2)
{
    return ((mcPtrInt)V1 - (mcPtrInt)V2);
}

mcInt32 MCList::IndexOf(void* V, bool *Found)
{
	mcInt32 L, H, I, C;
	bool Result;

	L = 0;
	Result = false;
	if (Sorted)
	{
		H = Len - 1;
		while (L <= H)
		{
			I = (L + H) >> 1;
			C = CompareItems(this, Data[I], V);
			if (C < 0)
				L = I + 1;
			else
			{
				H = I - 1;
				if (C == 0)
				{
					Result = true;
					L = I;
				}
			}
		}
	}
	if (NULL != Found)
		*Found = Result;
	return L;
}

mcInt32 MCList::Find(void* V)
{
    mcInt32 x;

    if(!V)
        return -1;

    if(0 != CompareItems)
	{
		if (Sorted)
		{
			bool Found;
			x = IndexOf(V, &Found);
			if (Found)
				return x;
		}
		else
		{
			for(x = 1; x <= Len; x++)
            {
                if (CompareItems(this, Data[x-1], V) == 0)
                    return(x-1);
            }
		}
	}
    
    return -1;
}

mcInt32 MCList::Insert(mcInt32 N, void* V)
{
    if (N < 0 || N > Len || !V)
        return -1;
    if (false == DuplicatesAllowed)
    {
        if (this->Find(V) != -1)
		{
            ;//assert(0);
            return 0;
		}
    }
    if (Len+1 > Size)
        Upgrade(1);
    if (N < Len)
    {
        memmove(&Data[N+1], &Data[N], (Len-N)*sizeof(void*));
    }
    Data[N] = V;
    Len++;
    return N;
}

void* MCList::operator[](mcInt32 N)
{
    if(N < 0 || N >= Len) 
        return NULL;
    return Data[N];
}

void* MCList::At(mcInt32 N)
{
    if(N < 0 || N >= Len) 
        return NULL;
    return Data[N];
}

void MCList::CopyOf(MCList* L)
{
    DeleteAll();
    Upgrade(L->Length() - Size);
    for(mcInt32 x=1; x <= L->Length(); x++)
        Add((*L)[x-1]);
}

// ------------------ tstringlist --------------------

MCStringList::MCStringList(void)
{
    DeleteItem = &StringDelete;
    CompareItems = &StringCompare;
}

void MCStringList::StringDelete(void* Sender, void* V)
{
    if(V) 
        MCMemFree(V);
}

mcInt32 MCStringList::StringCompare(void* Sender, void* V1,void* V2)
{
  return strcmp((char*)V1, (char*)V2);
}

char* MCStringList::Text(void)
{
    char* p;
    char nl[] = "\n";
    mcInt32 nll = strlen(nl);
    mcInt32 t = 0;

    for(mcInt32 x=1; x <= Length(); x++)
        t += strlen((char*)(*this)[x-1]) + nll;

    p = (char*)MCMemAlloc(t+1);
    *p = 0;
    for(mcInt32 x1=1; x1 <= Length(); x1++)
    {
        strcat(p, (char*)(*this)[x1-1]);
        strcat(p,nl);
    }
    return(p);
}

mcInt32 MCStringList::Add(void* V)
{
    return(MCStringList::Add((char*)V));
}

static char* copyString(char* s)
{
    char* r = (char*)MCMemAlloc(strlen(s)+1);
    strcpy(r, s);
    return r;
}

mcInt32 MCStringList::Add(char* V)
{
    if(!V) 
        return(-1);
#ifndef _WIN32_WCE
    return(MCList::Add((void*)copyString(V)));
#else
	return(MCList::Add((void*)copyString(V)));
#endif

}

void* MCStringList::operator[](mcInt32 N)
{
  return MCList::operator[](N);
}

MCWideStringList::MCWideStringList(void):MCList()
{
    DeleteItem = &StringDelete;
    //DupItem = &StringDup;
    CompareItems = &StringCompare;
}

void MCWideStringList::StringDelete(void*, void* V)
{
    if(V) 
        MCMemFree(V);
}

void* MCWideStringList::StringDup(void*, void* V)
{
    return wstrdup((wchar_t*)V);
}

mcInt32 MCWideStringList::StringCompare(void*, void* V1,void* V2)
{
    return strcmp((char*)V1, (char*)V2);
}

wchar_t* MCWideStringList::Text(void)
{
    wchar_t* p;
    wchar_t nl[] = {0xa, 0};
    size_t nll = wstrlen(nl);
    size_t t = 0;

    for(mcInt32 x=1; x <= Length(); x++)
        t += wstrlen((wchar_t*)(*this)[x-1]) + nll;

    p = (wchar_t*)MCMemAlloc(t * sizeof(wchar_t) + 1);
    *p = 0;

    for(mcInt32 x1=1; x1 <= Length(); x1++)
    {
        wstrcat(p, (wchar_t*)(*this)[x1-1]);
        wstrcat(p,nl);
    }
    return p;
}

mcInt32 MCWideStringList::Add(void* V)
{
    return(MCWideStringList::Add((wchar_t*)V));
}

mcInt32 MCWideStringList::Add(wchar_t* V)
{
    if(!V) 
        return(-1);
    return(MCList::Add((void*)wstrdup(V)));
}

void* MCWideStringList::operator[](mcInt32 N)
{
    return MCList::operator[](N);
}

#ifdef USE_NAMESPACE
}
#endif

