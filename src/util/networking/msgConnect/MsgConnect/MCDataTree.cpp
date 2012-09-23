//====================================================
//                                                    
//  EldoS MsgConnect                                 
//                                                    
//   Copyright (c) 2001-2010, EldoS                   
//                                                    
//====================================================

#include "MC.h"
#ifdef _WIN32_WCE
//#include <excpt.h>
#endif
#include "MCDataTree.h"
#include "MCPlatforms.h"
 
#ifdef USE_NAMESPACE
namespace MsgConnect
{
#endif


const char* cUTF8magic = "\xef\xbb\xbf";

// ********************************* utf8 **********************************

char* EncodeUTF8string(wchar_t* w)
{
	mcUInt32 c, count;
	unsigned char* r;
	if(!w)
		return(NULL);
	mcInt32 wl = wstrlen(w);
	r = (unsigned char*)malloc(wl * 3 + 4);
	strcpy((char*)r, cUTF8magic);
	count = 3;
	for(mcInt32 i = 0;i < wl;i++)
	{
		c = w[i];
		if(c <= 0x7f)
		{
			r[count] = (char)c;
			count++;
		}
		else if(c > 0x7ff)
		{
			r[count] = (unsigned char)(0xe0 | (c >> 12)); // ? <<
			r[count+1] = (unsigned char)(0x80 | ((c >> 6) & 0x3f));
			r[count+2] = (unsigned char)(0x80 | (c & 0x3f));
			count += 3;
		}
		else //  0x7F < w[i] <= 0x7FF
		{
			r[count] = (unsigned char)(0xc0 | (c >> 6));
			r[count+1] = (unsigned char)(0x80 | (c & 0x3f));
			count += 2;
		}
	}
  r[count] = 0;
  return((char*)r);
}

wchar_t* DecodeUTF8string(char* s)
{
	mcUInt32  count, wc;
	unsigned char c;
	wchar_t* r;

	if(strlen(s) < 4)
    return(NULL);
	if(_strncmp(s, cUTF8magic, strlen(cUTF8magic)) != 0)
		return(NULL);
	size_t ss = strlen(s);
	r = (wchar_t*)malloc(ss * 3 + 2);
	count = 0;
	unsigned int i = 3;
	while(i < ss)
	{
		wc = (unsigned char)s[i];
		i++;
		if(wc & 0x80)
		{
			wc = wc & 0x3f;
			if(i > ss)
				break;
			if(wc & 0x20)
			{
				c = (unsigned char)s[i];
				i++;
				if((c & 0xc0) != 0x80)
				{
				  free(r);
					return(NULL);
				}
				if(i > ss)
					break;
				wc = (wc << 6) | (c & 0x3f);
			}
			c = (unsigned char)s[i];
			i++;
			if((c & 0xcc0) != 0x80)
			{
			  free(r);
				return(NULL);
			}
			r[count] = (wchar_t)((wc << 6) | (c & 0x3f));
		}
		else
			r[count] = (wchar_t)wc;
		count++;
	}
	r[count] = 0;
	return(r);
}

// *******************************************************************

void Str2Data(char* Src, void* &B, mcInt32 &L)
{
    if(NULL == Src) 
        return;
    size_t x = (strlen(Src) - 4) >> 1;
    char t[3];
    unsigned char* p = (unsigned char*)malloc(x);
    mcInt32 r;
    L = (mcInt32)x;
    B = p;
    Src += 4;
    t[2] = 0;
    while(*Src) 
    {
        t[0] = Src[0]; 
        t[1] = Src[1];
        r = strtol(t, NULL, 16);
        *p = (unsigned char)r;
        Src += 2; 
        p++;
    }

}

char* Data2Str(void* B, mcInt32 L)
{
    if(NULL == B || L < 1) 
        return NULL;
  
    char* r = (char*)malloc((L << 1) + 5);
    char* p;
    unsigned char *s=(unsigned char*)B;
  
    memset(r, 0, (L << 1) + 5);
    strncpy(r, "454C", 4);
    p = r + 4;
    for(mcInt32 zl=0; zl<L; zl++)
    {
#ifdef _SNPRINTF_ENABLED_
        _snprintf(p, 3, "%02X", *s++);
#else
        MakeHexByteAt(p, *s++);
#endif
        p+=2;
    }
    return r;
}

/*
void LoadListFromStream(MCStringList* L, TStream* F)
{
    if(NULL == L || NULL == F) 
        return;
    while(!F->Eof())
    {
        char* s = ReadTextFromStream(F);
        L->Add(s);
        if(s) 
            free(s);
    }
}
*/
void LoadListFromStream(MCStringList* L, TStream* F)
{
  if(NULL == L || NULL == F) 
    return;
	mcInt32 size = F->Size() - F->Pos() + 1;
	char* s = (char*)malloc(size);
	char* x = s;
	char* y;
  if(NULL == s) 
    return;
  F->Read(s, size - 1);
  s[size - 1] = 0;
  L->DeleteAll();
  while(*x) {
    y = x;
    while(*x && *x != '\n' && *x != '\r')
      x++;
    char c = *x;
    *x = 0;
    L->Add(y);
    *x = c;
    if(*x == '\r')
      x++;
    if(*x == '\n')
      x++;
  }
  free(s);
}

// ****************************************************************************

MCTreeItem::MCTreeItem(void)
{
    FIsKey = false;
    FParent = NULL;
    FValueData = new MCValueData();
    memset(FValueData, 0, sizeof(*FValueData));
    FValueName = NULL;
    FChildren = new MCList();
	FChildren->CompareItems = ItemsCompare;
	FChildren->setSorted(true);
    FChildren->DeleteItem = &MCTreeItem::OnValueDelete;
}

MCTreeItem* MCTreeItem::GetParent(void)
{
    return FParent;
}

bool MCTreeItem::IsKey(void)
{
    return FIsKey;
}

MCTreeItem::~MCTreeItem(void)
{
    this->Invalidate();
    delete FValueData;
    if (NULL != FValueName) 
        free(FValueName);
    delete FChildren;
}

mcInt32 MCTreeItem::SubCount(void)
{
    return FChildren->Length();
}

MCTreeItem* MCTreeItem::GetValue(char* Name)
{
    mcInt32 Index;
		MCTreeItem Item;

		Item.FValueName = Name;
		Index = FChildren->Find(&Item);
		Item.FValueName = NULL;
		if (Index >= 0)
			return (MCTreeItem*)(FChildren->At(Index));
		else
			return NULL;
		/*
		char* n = UpperTrim(Name);
    for(mcInt32 i=1; i <= FChildren->Length(); i++)
    {
        char* p = UpperTrim(((MCTreeItem*)((*FChildren)[i-1]))->FValueName);
        if(strcmp(p,n)==0)
        {
            free(p);
            free(n);
            return((MCTreeItem*)((*FChildren)[i-1]));
        }
        free(p);
    }
    free(n);
    return NULL;
		*/
}

void MCTreeItem::Invalidate(void)
{
    if(evtString == FValueData->ValueType)
    {
        if(NULL != FValueData->StrValue) 
            free(FValueData->StrValue);
    }
    else 
    if(evtWideString == FValueData->ValueType)
    {
        if(NULL != FValueData->WideStrValue) 
            free(FValueData->WideStrValue);
    }
    else 
    if(evtMultiString == FValueData->ValueType)
        delete FValueData->MStrValue;
    else 
    if(evtMultiWideString == FValueData->ValueType)
        delete FValueData->MWideStrValue;
    else 
    if(evtBinary == FValueData->ValueType)
        free(FValueData->DATAVALUE);
    FValueData->ValueType = evtUnknown;
}

void MCTreeItem::OnValueDelete(void*, void* Data)
{
    MCTreeItem* Entry = (MCTreeItem*)Data;
    Entry->Invalidate();
    delete Entry;
}

mcInt32 MCTreeItem::ItemsCompare(void* Sender, void* V1, void* V2)
{
	return UpperTrimCmp(((MCTreeItem*)V1)->FValueName, 
		((MCTreeItem*)V2)->FValueName);
/*
	char C1, C2;
	char* N1 = ((MCTreeItem*)V1)->FValueName;
	char* N2 = ((MCTreeItem*)V2)->FValueName;

	while(*N1 && isspace(*N1)) 
		N1++;
	while(*N2 && isspace(*N2)) 
		N2++;
	while (true)
	{
		C1 = *N1++;
		if (isspace(C1))
			C1 = 0;
		else if ((C1 >= 'a') || (C1 <= 'z'))
			C1 &= (~0x20);

		C2 = *N2++;
		if (isspace(C2))
			C2 = 0;
		else if ((C2 >= 'a') || (C2 <= 'z'))
			C2 &= (~0x20);

		if (C1 > C2)
			return  +1;
		else if (C1 < C2)
			return  -1;
		else if (C1 == 0)
			return 0;
	}
	return 0; 
*/
}

void MCTreeItem::SetParent(MCTreeItem* Value)
{
    if(NULL != FParent) 
        FParent->FChildren->Del(this);
    FParent = Value;
    if(NULL != Value) 
        FParent->FChildren->Add(this);
}

bool MCTreeItem::DubFrom(MCTreeItem* Peer, bool CopySubKeys, bool CopyValue)
{
  MCTreeItem* x;
  MCTreeItem* z;
  bool r = true;

  if(!Peer)
    return(false);
  FIsKey = Peer->FIsKey;
  memset(FValueData, 0, sizeof(*FValueData));
  if(CopyValue)
  {
    FValueData->ValueType = Peer->FValueData->ValueType;
    switch(FValueData->ValueType) {
      case evtBoolean:
        FValueData->BoolValue = Peer->FValueData->BoolValue;
        break;
      case evtInt:
        FValueData->IntValue = Peer->FValueData->IntValue;
        break;
      case evtDouble:
        FValueData->DoubleValue = Peer->FValueData->DoubleValue;
        break;
      case evtString:
        if(Peer->FValueData->StrValue)
          FValueData->StrValue = _strdup(Peer->FValueData->StrValue);
        break;
      case evtMultiString:
        FValueData->MStrValue = new MCStringList();
        FValueData->MStrValue->CopyOf(Peer->FValueData->MStrValue);
        break;
      case evtMultiWideString:
        FValueData->MWideStrValue = new MCWideStringList();
        FValueData->MWideStrValue->CopyOf(Peer->FValueData->MWideStrValue);
        break;
      case evtBinary:
#ifndef M68K
        if(Peer->FValueData->DATAVALUE)
        {
          FValueData->DATALENGTH = Peer->FValueData->DATALENGTH;
          FValueData->DATAVALUE = malloc(FValueData->DATALENGTH);
          memmove(FValueData->DATAVALUE, Peer->FValueData->DATAVALUE, FValueData->DATALENGTH);
#else
        if(Peer->FValueData->DataValue)
        {
          FValueData->DataLength = Peer->FValueData->DataLength;
          FValueData->DataValue = malloc(FValueData->DataLength);
          memmove(FValueData->DataValue, Peer->FValueData->DataValue, FValueData->DataLength);
#endif
        }
        break;
      case evtWideString:
        if(Peer->FValueData->WideStrValue)
          FValueData->WideStrValue = wstrdup(Peer->FValueData->WideStrValue);
        break;
    }
  }
  for(mcInt32 i = 1;i <= Peer->FChildren->Length();i++)
  {
    z = (MCTreeItem*)((*Peer->FChildren)[i - 1]);
    if(!CopySubKeys && z->FIsKey)
      continue;
    x = new MCTreeItem();
    x->FValueName = _strdup(z->FValueName);
    x->SetParent(this);
    r = r && x->DubFrom(z, CopySubKeys, true);
    if(!r)
      break;
  }
  return(r);
}

// ****************************************************************************

MCDataTree::MCDataTree(void)
{
    WarningMessage = _strdup("Automatically generated file. DO NOT MODIFY!!!");
    FRoot = new MCTreeItem();
    FDelimiter = '\\';
    FCurrentKey = Char2Str(FDelimiter, NULL);
    FCurEntry = FRoot;
    FLazyWrite = true;
    FComment = _strdup(";");
    FDivChar = '=';
    FSimple = false;
    FModified = false;
    FBinaryMode = false;
    FPath = NULL;
    OnBeforeLoad = NULL;
    OnAfterSave = NULL;
    OnBeforeSave = NULL;
    OnAfterLoad = NULL;
}

MCDataTree::~MCDataTree(void)
{
#ifdef USE_CPPEXCEPTIONS
    try {
#else
    __try {
#endif
        if(FLazyWrite && FModified) 
            Save();
    }
#ifdef USE_CPPEXCEPTIONS
    catch(...) {}
#else
    __except(-1 /*EXCEPTION_CONTINUE_EXECUTION from <excpt.h> */) {}
#endif
    free(WarningMessage);
    free(FComment);
    free(FCurrentKey);
    free(FPath);
    FCurEntry = NULL;
    FRoot->FChildren->DeleteAll();
    delete FRoot;
}

bool MCDataTree::GetBinaryMode(void)
{
    return FBinaryMode;
}

void MCDataTree::SetBinaryMode(bool Val)
{
    FBinaryMode = Val;
}

bool MCDataTree::Modified(void)
{
    return FModified;
}

char* MCDataTree::GetCurrentKey(void)
{
    return FCurrentKey;
}

void MCDataTree::SetComment(const char* newValue)
{
    if(FComment) 
        free(FComment);
    FComment = _strdup(newValue);
}

char* MCDataTree::GetComment(void)
{
    return FComment;
}

char MCDataTree::GetDelimiter(void)
{
    return FDelimiter;
}

void MCDataTree::SetDivChar(char NewValue)
{
    FDivChar = NewValue;
}

char MCDataTree::GetDivChar(void)
{
    return FDivChar;
}

bool MCDataTree::GetLazyWrite(void)
{
    return FLazyWrite;
}

char* MCDataTree::GetPath(void)
{
    return FPath;
}
 
bool MCDataTree::GetSimple(void)
{
    return FSimple;
}

bool MCDataTree::Clear(void)
{
    FRoot->FChildren->DeleteAll();
    FCurrentKey = Char2Str(FDelimiter, FCurrentKey);
    FCurEntry = FRoot;
    FModified = true;
    return true;
}

bool MCDataTree::ClearKey(char* Key)
{
    MCTreeItem* E = GetValueEntry(Key, NULL);
    if((NULL != E) && E->FIsKey)
    {
        E->FChildren->DeleteAll();
        if(0 == FLazyWrite) 
            Save();
        FModified=true;
        return(true);
    }
    return false;
}

MCTreeItem* MCDataTree::CreateValue(char* Key, char* ValueName)
{
    char* S = _strdup(FCurrentKey);
    MCTreeItem* E;
    if(OpenKey(Key,true))
    {
        E = GetValueEntry(Key, ValueName);
        if(NULL == E)
        {
            E = new MCTreeItem();
            E->FValueName = _strdup(ValueName);
            E->SetParent(FCurEntry);
        }
        else 
            E->Invalidate();
        FModified=true;
        E->FValueData->ValueType=evtUnknown;
        free(S);
        return(E);
    }
    OpenKey(S, false);
    free(S);
    return NULL;
}

bool MCDataTree::Delete(char* Key, char* ValueName)
{
    MCTreeItem* E = GetValueEntry(Key, ValueName);
    if(E && (E != FRoot))
    {
        if(FCurEntry==E)
        {
            FCurEntry=E->GetParent();
            for(mcInt32 l = (mcInt32)strlen(FCurrentKey)-1; l>=0; l--)
                if(FCurrentKey[l] == FDelimiter) 
                { 
                    FCurrentKey[l] = 0; 
                    break; 
                }
        }
        E->FParent->FChildren->Del(E);
        FModified = true;
        return true;
    }
    else
        return false;
}

bool MCDataTree::EnumSubKeys(char* Key, MCStringList* Strings)
{
    char* S = _strdup(FCurrentKey);
  
    if(OpenKey(Key, true))
    {
        for(mcInt32 i=1; i<=FCurEntry->FChildren->Length(); i++)
            if(((MCTreeItem*)((*(FCurEntry->FChildren))[i-1]))->FIsKey)
                Strings->Add(((MCTreeItem*)((*(FCurEntry->FChildren))[i-1]))->FValueName);
        free(S);
        return true;
    }
    OpenKey(S, false);
    free(S);
    return false;
}

bool MCDataTree::EnumValues(char* Key, MCStringList* Strings)
{
    char* S = _strdup(FCurrentKey);
    if(OpenKey(Key, true))
    {
        for(mcInt32 i=1; i<=FCurEntry->FChildren->Length(); i++)
            if(!(((MCTreeItem*)((*(FCurEntry->FChildren))[i-1]))->FIsKey))
                Strings->Add(((MCTreeItem*)((*(FCurEntry->FChildren))[i-1]))->FValueName);
        free(S);
        return true;
    }
    OpenKey(S, false);
    free(S);
    return false;
}

char* MCDataTree::FullKey(char* Key)
{
    char* z;
    size_t t;
    if((NULL != Key) || (Key[0] != FDelimiter))
    {
        if((FCurrentKey[0] == FDelimiter) && (0 == FCurrentKey[1]))
        {
            t = strlen(Key)+2;
            z = (char*)malloc(t);
#ifdef _SNPRINTF_ENABLED_
            _snprintf(z,t,"%c%s",FDelimiter,Key);
#else
            *z = FDelimiter;
            strcpy(&z[1],Key);
#endif
        }
        else
        {
            t = strlen(FCurrentKey) + strlen(Key) + 2;
            z = (char*)malloc(t);
#ifdef _SNPRINTF_ENABLED_
            _snprintf(z,t,"%s%c%s",FCurrentKey,FDelimiter,Key);
#else
            strcpy(z,FCurrentKey);
            mcInt32 zl = strlen(z);
            z[zl] = FDelimiter;
            z[zl+1] = 0;
            strcat(z, Key);
#endif
        }
    }
    t = strlen(z);
    if(z[t-1] == FDelimiter) 
        z[t-1] = 0;
    return z;
}

MCTreeItem* MCDataTree::GetValueEntry(char* Key, char* ValueName)
{
	MCTreeItem* r = NULL;
	char* S = NULL;

  if ((Key != NULL) && (Key[0] != 0))
	{
		S = _strdup(FCurrentKey);
		if(!OpenKey(Key, false))
		{
			free(S);
			return NULL;
		}
	}
	if(ValueName)
		r = FCurEntry->GetValue(ValueName);
	else if (FCurEntry != FRoot)
		r = FCurEntry;
	if (NULL != S)
	{
		OpenKey(S,false);
		free(S);
	}
	return r;
}

MCValueType MCDataTree::GetValueType(char* Key,char* ValueName)
{
    MCTreeItem* E = GetValueEntry(Key, ValueName);
    if(E) 
        return(E->FValueData->ValueType);
    else 
        return(evtUnknown);
}

bool MCDataTree::KeyExists(char* Key)
{
    char* S = _strdup(FCurrentKey);
    bool r = OpenKey(Key, false);
    OpenKey(S, false);
    free(S);
    return r;
}

void MCDataTree::IntLoadBinKey(TStream* F, char* LoadInto)
{
    size_t i/*,lz*/;
    MCTreeItem *E, *KE = FRoot;
    char* S = NULL;
    char* KEN = NULL;
    bool b, SkipValues = false;
    MCValueType VT;
    char* xLoadInto = LoadInto;

    if(!xLoadInto || !*xLoadInto)
      xLoadInto = NULL;
    else
    {
      mcInt32 zl = (mcInt32)strlen(LoadInto);
      xLoadInto = (char*)malloc(zl + 2);
#ifdef _SNPRINTF_ENABLED_
      _snprintf(xLoadInto, zl + 2, "%s%c", LoadInto, FDelimiter);
#else
      strcpy(xLoadInto, FCurrentKey);
      xLoadInto[zl] = FDelimiter;
      xLoadInto[zl + 1] = 0;
#endif
      S = _strdup(FCurrentKey);
      OpenKey(xLoadInto, false);
      KE = FCurEntry;
      OpenKey(S, false);
      free(S);
      S = NULL;
    }

    while(F->Pos() < F->Size())
    {
        F->Read(&b, sizeof(b));
        if(S) 
		{
            free(S);
			S = NULL;
		}
        S = ReadStringFromStream(F, -1);
        if(b)
        {
            if(S && (S[0] != FDelimiter)) 
            {
                if(xLoadInto)
                {
                  mcInt32 t = (mcInt32)strlen(xLoadInto) + (mcInt32)strlen(S) + 1;
                  char* z = (char*)malloc(t);
#ifdef _SNPRINTF_ENABLED_
                  _snprintf(z, t, "%s%s", xLoadInto, S);
#else
                  strcpy(z, xLoadInto);
                  strcat(z, S);
#endif
                  free(S);
                  S = z;
                }
                else
                    S = CharNstr(FDelimiter, S);
            }
            if(!OpenKey(S, true))
            {
                SkipValues = true;
                continue;
            }
            if(KEN) 
                free(KEN);
            KEN = S; 
            S = NULL;
            KE = FCurEntry;
            char* z = Char2Str(FDelimiter, NULL);
            OpenKey(z, false);
            free(z);
        }
        else
        {
            E = GetValueEntry(KEN, S);
            F->Read(&VT, sizeof(VT));
            F->Read(&i, sizeof(i));
#ifdef M68K
            i = TurnLong(i);
#endif
            if(SkipValues)
            {
                F->Seek((mcInt32)(F->Pos()+i));
                continue;
            }
            if(E)
            {
                if((evtMultiString == VT)^(evtMultiString == E->FValueData->ValueType))
                {
                    if(evtMultiString == VT)
                    {
                        E->FValueData->ValueType = evtMultiString;
                        E->FValueData->MStrValue = new MCStringList();
                    } 
                    else 
                        E->Invalidate();
                }
                if((evtMultiWideString == VT)^(evtMultiWideString == E->FValueData->ValueType))
                {
                    if(evtMultiWideString == VT)
                    {
                        E->FValueData->ValueType = evtMultiWideString;
                        E->FValueData->MWideStrValue = new MCWideStringList();
                    } 
                    else 
                        E->Invalidate();
                }
            }
            else
            {
                E = new MCTreeItem();
                E->FValueName = _strdup(S);
                E->SetParent(KE);
            }
            E->FValueData->ValueType = VT;
            switch(VT) {
            case evtBoolean:
                F->Read(&E->FValueData->BoolValue, sizeof(E->FValueData->BoolValue));
                break;
            case evtDouble:
                {
                    double d;
                    F->Read(&d, sizeof(d));
#ifdef M68K
                    E->FValueData->DoubleValue = TurnDouble(d);
#else
                    E->FValueData->DoubleValue = d;
#endif
                }
                break;
            case evtInt:
                {
                    mcInt32 d;
                    F->Read(&d, sizeof(d));
#ifdef M68K
                    E->FValueData->IntValue = TurnLong(d);
#else
                    E->FValueData->IntValue = d;
#endif
                }
                break;
            case evtString:
                E->FValueData->StrValue = ReadStringFromStream(F, i);
                break;
            case evtMultiString:
                if(!E->FValueData->MStrValue)
                    E->FValueData->MStrValue = new MCStringList();
                if(i > 0)
                {
                    free(S);
                    while(i > 0)
                    {
                        S = ReadStringFromStream(F, -1);
                        E->FValueData->MStrValue->Add(S);
                        i -= sizeof(mcInt32) + strlen(S);
                        free(S);
                    }
                    S = NULL;
                }
                break;
            case evtMultiWideString:
                if(!E->FValueData->MWideStrValue)
                    E->FValueData->MWideStrValue = new MCWideStringList();
                if(i > 0)
                {
                    while(i > 0)
                    {
                        wchar_t* t = ReadWideStringFromStream(F, -1);
                        E->FValueData->MWideStrValue->Add(t);
                        i -= sizeof(mcInt32) + wstrlen(t) * sizeof(wchar_t);
                        free(t);
                    }
                }
                break;
            case evtBinary:
#ifndef M68K
                E->FValueData->DATALENGTH = (mcInt32)i;
#else
                E->FValueData->DataLength = i;
#endif
                E->FValueData->DATAVALUE = malloc(E->FValueData->DATALENGTH);
                F->Read(E->FValueData->DATAVALUE, E->FValueData->DATALENGTH);
                break;
            case evtWideString:
                E->FValueData->WideStrValue = ReadWideStringFromStream(F, i);
                break;
            } // switch
        } // else
    }
    if(KEN) 
        free(KEN);
    if(S) 
        free(S);
}

bool CmpFloatStr(double D, char* S)
{
    char a[50];
#ifdef _SNPRINTF_ENABLED_
    _snprintf(a, 50, "%f", D);
#else
    MakeDoubleAt(a, D, 15, 23);
#endif
    return(0 == strcmp(S, a));
}

bool CmpIntStr(mcInt32 L, char* S)
{
    char a[50];
#ifdef _SNPRINTF_ENABLED_
    _snprintf(a, 50, "%ld", (long int)L);
#else
    MakeLongAt(a,L,49);
#endif
    return(0 == strcmp(S, a));
}

char* AddFloatStr(char* S, double D)
{
    char a[50];
#ifdef _SNPRINTF_ENABLED_
    _snprintf(a, 50, "%f", D);
#else
    MakeDoubleAt(a, D, 15, 23);
#endif
    return(AddStr(S, a));
}

char* AddIntStr(char* s, mcInt32 l)
{
    char a[50];
#ifdef _SNPRINTF_ENABLED_
    _snprintf(a, 50, "%ld", (long int)l);
#else
    MakeLongAt(a, l, 49);
#endif
    return(AddStr(s, a));
}


void MCDataTree::IntLoadKey(MCStringList* SL, mcInt32 CurLine, char* LoadInto)
{
    char* S = NULL;
    char* KEN = Char2Str(FDelimiter, NULL);
    char* ValueName = NULL;
    char* Value = NULL;
    MCTreeItem *E, *KE = FRoot;
    mcInt32 i, j = 0, l;
    bool b, eos = false, SkipValues = false;
    double d;
    char* xLoadInto = LoadInto;
    if(!xLoadInto || !*xLoadInto)
      xLoadInto = NULL;
    else
    {
      xLoadInto = _strdup(LoadInto);
      mcInt32 zl = (mcInt32)strlen(xLoadInto);
      if(xLoadInto[zl - 1] == FDelimiter)
        xLoadInto[zl - 1] = 0;
    }

    while(!eos)
    {
        j++;
        if(Value) 
        { 
            free(Value); 
            Value = NULL; 
        }

        if(ValueName) 
        { 
            free(ValueName); 
            ValueName = NULL; 
        }

        if(S) 
		{
            free(S);
			S = NULL;
		}
        if(SL->Length() <= CurLine) 
            break;
        else 
            S = _strdup((char*)(*SL)[CurLine++]);

        if((NULL == S) || (!*S) || (FComment && _strncmp(S, FComment, strlen(FComment)) == 0)) 
            continue;

        if('[' == S[0])
        {
            SkipValues = false;
            if(']' == S[strlen(S)-1])
            {
                S[0] = FDelimiter;
                S[strlen(S)-1] = 0;
                if(xLoadInto)
                {
                  mcInt32 t = (mcInt32)strlen(xLoadInto) + (mcInt32)strlen(S) + 1;
                  char* z = (char*)malloc(t);
#ifdef _SNPRINTF_ENABLED_
                  _snprintf(z, t, "%s%s", xLoadInto, S);
#else
                  strcpy(z, xLoadInto);
                  strcat(z, S);
#endif
                  free(S);
                  S = z;
                }
            }
            else 
                S[0] = FDelimiter;
            if(strcmp(S, FCurrentKey) !=0 && !OpenKey(S,true))
            {
                SkipValues = true;
                continue;
            }
            if(KEN) 
                free(KEN);
            KEN = S; 
            S = NULL;
            KE = FCurEntry;
            char* z = Char2Str(FDelimiter, NULL);
            OpenKey(z, false);
            free(z);
        }
        else
        {
            if(!SkipValues)
            {
                ParseLine(S, ValueName, Value, b);
                if(!b)
                {
                    if(Value) 
                        free(Value);
                    Value = ValueName;
                    ValueName = (char*)malloc(50);
#ifdef _SNPRINTF_ENABLED_
                    _snprintf(ValueName, 50, "#%ld", (long int)j);
#else
                    ValueName[0] = '#';
                    MakeLongAt(&ValueName[1], j, 49);
#endif
                }
                i = -1;
                if(']' == ValueName[strlen(ValueName)-1])
                {
                    l = -1;
                    for(mcInt32 x=0; ValueName[x]; x++)
                    {
                        if('[' == ValueName[x]) 
                        { 
                            l = x; 
                            break; 
                        }
                    }
                    mcInt32 l1 = -1;
                    for(mcInt32 x1=l+1; ValueName[x1]; x1++)
                    {
                        if(']' == ValueName[x1]) 
                        { 
                            l1 = x1; 
                            break; 
                        }
                    }
                    
                    if(-1 == l || -1 == l1) 
                    {
                        i = -1;
                        free(ValueName);
                        ValueName = NULL;
                    }
                    else
                    {
                        char *z;
                        ValueName[l1] = 0;
                        i = strtol(&ValueName[l+1], &z, 0);
                        if(*z) 
                            i = -1;
                        ValueName[l] = 0;
                    }
                }
                
                E = GetValueEntry(KEN,ValueName);
                if(E)
                {
                    if(-1 == i) 
                        E->Invalidate();
                    if(i >= 0)
                    {
                        if(evtMultiString != E->FValueData->ValueType && evtMultiWideString != E->FValueData->ValueType)
                        {
                            E->Invalidate();
                            E->FValueData->ValueType = evtMultiString;
                            E->FValueData->MStrValue = new MCStringList();
                        }
                    }
                }
                else
                if(NULL == ValueName) 
                    continue;
                else
                {
                    E = new MCTreeItem();
                    E->FValueName = ValueName; 
                    ValueName = NULL;
                    E->SetParent(KE);
                }

                if(NULL != ValueName) 
                { 
                    free(ValueName); 
                    ValueName = NULL; 
                }
                // MultiString
                if(-1 != i)
                {
                    bool w = (0 == _strncmp(cUTF8magic, Value, strlen(cUTF8magic)));
                    bool f = 0 == i;
                    if(f) {
                        if(!w && evtMultiString != E->FValueData->ValueType)
                        {
                            E->Invalidate();
                            E->FValueData->ValueType = evtMultiString;
                            E->FValueData->MStrValue = new MCStringList();
                        }
                        else
                        if(w && evtMultiWideString != E->FValueData->ValueType)
                        {
                            E->Invalidate();
                            E->FValueData->ValueType = evtMultiWideString;
                            E->FValueData->MWideStrValue = new MCWideStringList();
                        }
                    }
                    if(evtMultiString == E->FValueData->ValueType)
                    {
                        E->FValueData->MStrValue->Add(Value);
                    }
                    else
                    {
                        if(w)
                            E->FValueData->MWideStrValue->Add(DecodeUTF8string(Value));
                        else
                        {
                            wchar_t* t = str2wstr(Value, DefaultCodePage);
                            E->FValueData->MWideStrValue->Add(t);
                            free(t);
                        }
                    }
                    continue;
                }
                char False[20] = "FALSE";
                char True[20] = "TRUE";
                // boolean
                if(0 == UpperTrimCmp(Value,False) || 0 == UpperTrimCmp(Value,True))
                {
                    E->FValueData->ValueType = evtBoolean;
                    if(0 == UpperTrimCmp(Value,True))
                        E->FValueData->BoolValue = true;
                    else
                        E->FValueData->BoolValue = false;
                    continue;
                }

                // integer
                char* zz;
                i = strtol(Value, &zz, 0);
                if(!*zz && CmpIntStr(i, Value))
                {
                    E->FValueData->ValueType = evtInt;
                    E->FValueData->IntValue = i;
                    continue;
                }
                
                // float
                d = strtod(Value, &zz);
                if(!*zz && CmpFloatStr(d, Value))
                {
                    E->FValueData->ValueType = evtDouble;
                    E->FValueData->DoubleValue = d;
                    continue;
                }
  
                // binary
                if(0 == _strncmp("454C", Value, 4))
                {
                    E->FValueData->ValueType = evtBinary;
                    Str2Data(Value, E->FValueData->DATAVALUE, E->FValueData->DATALENGTH);
                    continue;
                }

                // wide string
                if(0 == _strncmp(cUTF8magic, Value, strlen(cUTF8magic)))
                {
                  E->FValueData->ValueType = evtWideString;
                  E->FValueData->WideStrValue = DecodeUTF8string(Value);
                  continue;
                }

                // string
                E->FValueData->ValueType = evtString;
                if(Value)
                  E->FValueData->StrValue = _strdup(Value);
            }
        }
    }
    if(Value) 
        free(Value);
    if(ValueName) 
        free(ValueName);
    if(KEN) 
        free(KEN);
    if(S)
        free(S);
    if(xLoadInto)
      free(xLoadInto);
}


void MCDataTree::LoadFromStream(TStream* Stream)
{
    MCStringList* SL;
    mcInt32 i;

    TriggerBeforeLoadEvent();

#ifdef USE_CPPEXCEPTIONS
    try {
#else
    __try {
#endif
        FRoot->FChildren->DeleteAll();
        FCurrentKey = Char2Str(FDelimiter, FCurrentKey);
        FCurEntry = FRoot;
        if(FBinaryMode)
            IntLoadBinKey(Stream, NULL);
        else
        {
            SL = new MCStringList();
#ifdef USE_CPPEXCEPTIONS
            try 
            {
#else
            __try 
            {
#endif
                LoadListFromStream(SL, Stream);
                i = 0;
                IntLoadKey(SL, i, NULL);
            }
#ifdef USE_CPPEXCEPTIONS
            catch(...) 
            {
                delete SL;
                throw;
            };
            delete SL;
#else
            __finally 
            {
                delete SL;
            }
#endif
        }
    }
#ifdef USE_CPPEXCEPTIONS
    catch(...) 
    {
        TriggerAfterLoadEvent();
        throw;
    };
    TriggerAfterLoadEvent();
#else
    __finally 
    {
        TriggerAfterLoadEvent();
    }
#endif
}


bool MCDataTree::Load(void)
{
    TStream* F = NULL;
    MCStringList* SL = NULL;
    bool r = false;

    TriggerBeforeLoadEvent();
#ifndef WINCE
    try 
    {
#else
    __try 
    {
#endif
        FRoot->FChildren->DeleteAll();
        FCurrentKey = Char2Str(FDelimiter, FCurrentKey);
        FCurEntry = FRoot;
        F = OpenStream(FPath, stRead);
        if(F) 
        {
            if(FBinaryMode)
            {
                IntLoadBinKey(F, NULL);
            }
            else
            {
                SL = new MCStringList();
#ifdef USE_CPPEXCEPTIONS
                try 
                {
#else
                __try 
                {
#endif
                    LoadListFromStream(SL,F);
                    IntLoadKey(SL, 0, NULL);
                }
#ifdef USE_CPPEXCEPTIONS
                catch(...) 
                {
                    delete SL;
                    throw;
                }
                delete SL;
#else
                __finally 
                {
                    delete SL;
                }
#endif
            }
        
        }
    r = true;
    }
#ifndef WINCE
    catch(...) 
    {
        if(F) 
            delete F;
        TriggerAfterLoadEvent();
        throw;
    }
    if(F) 
        delete F;
    TriggerAfterLoadEvent();
#else
    __finally 
    {
        if(F) delete F;
        TriggerAfterLoadEvent();
    }
#endif
    return r;
}


void MCDataTree::Loaded(void)
{
    if(FPath)
#ifdef USE_CPPEXCEPTIONS
    try
#else
    __try
#endif
    {
      Load();
    }
#ifdef USE_CPPEXCEPTIONS
    catch(...) {}
#else
    __except(-1 /*EXCEPTION_CONTINUE_EXECUTION from <excpt.h> */) {}
#endif
}

bool MCDataTree::MoveEntry(char* Key, char* ValueName, char* NewKey)
{
    MCTreeItem *E, *E1;
    char* S = _strdup(FCurrentKey);

    if(OpenKey(Key, false))
    {
        E = FCurEntry->GetValue(ValueName);
        if(E)
        {
            E1 = this->GetValueEntry(NewKey, NULL);
            E->SetParent(E1); // this will remove the entry from the old parent's children list
                                // and add it to the new parent's children list
            free(S);
            return(true);
        }
    }
    OpenKey(S, false);
    free(S);
    return false;
}

bool MCDataTree::CopyToDataTree(char* Key, char* ValueName, MCDataTree* NewTree,
  char *NewKey, bool CopySubKeys, bool CopyValue)
{
  MCTreeItem* E;
  MCTreeItem* E1;
  bool r = true;

  if((Key && strlen(Key) == 1 && Key[0] == FDelimiter) && (!ValueName || !(*ValueName)))
    E = FRoot;
  else
    E = GetValueEntry(Key, ValueName);
  if(E)
  {
    E1 = NewTree->CreateValue(NewKey, ValueName);
    if(E1)
    {
      r = r && E1->DubFrom(E, CopySubKeys, CopyValue);
    }
  }
  return(r);
}

bool MCDataTree::CopyEntry(char* Key, char* ValueName, char *NewKey, bool CopySubKeys,
  bool CopyValue)
{
  return(CopyToDataTree(Key, ValueName, this, NewKey, CopySubKeys, CopyValue));
}

char* HF1(char* d, char* s, char c)
{
  size_t l = strlen(s) + strlen(d) + 2;
  char* x = (char*)malloc(l); x[0] = 0;
  if(d) strcat(x, d);
  if(s) strcat(x, s);
  x[l-2] = c; 
  x[l-1] = 0;
  if(d) 
      free(d);
  return x;
}
#undef NVA
#ifdef PALM_CW
#  define NVA
#endif
#ifdef WINCE
#  define NVA
#endif
#ifdef WINFULL
#  define NVA
#endif

bool MCDataTree::OpenKey(char* aKey, bool CanCreate)
{
    MCTreeItem *CE;
    mcInt32 i;
    char *S;
    bool r;
    size_t zl = aKey ? strlen(aKey) : 0;
#ifndef NVA
    char Key[zl+1];
#else
    char* Key;
#endif

    if(aKey) 
    {
#ifdef NVA
        Key = (char*)malloc(zl+1);
#endif
        strcpy(Key,aKey);
        //S = UpperTrim(Key);
        //S1 = UpperTrim(FCurrentKey);
        r = UpperTrimCmp(Key,FCurrentKey) == 0; 
        //free(S); 
        //free(S1);
    } 
    else 
    { 
        r = true;
#ifndef NVA
        strcpy(Key,FCurrentKey); 
#else
        Key = _strdup(FCurrentKey);
#endif
    }
    if(!aKey || r)
#ifndef NVA
        return true;
#else
    { 
        if(Key) 
            free(Key); 
        return true; 
    }
#endif
  // In simple mode
    if(FSimple)
    {
        if(!Key) 
            return(true);
        else
        {
            FCurEntry = FRoot;
            FCurrentKey = Char2Str(FDelimiter, FCurrentKey);
            if(Key[0] == FDelimiter) 
            {
                size_t z = strlen(Key) - 1;
                memmove(Key, &Key[1], z);
                Key[z] = 0;
            }
            CE = FRoot->GetValue(Key);
            if(CE)
            {
                FCurEntry = CE;
                FCurrentKey = CharNstr(FDelimiter, Key, FCurrentKey);
#ifdef NVA
                free(Key);
#endif
                return true;
            }
            else
            if(CanCreate)
            {
                CE = new MCTreeItem();
                CE->FIsKey = true;
                CE->FValueName = _strdup(Key);
                CE->SetParent(FRoot);
                FModified = true;
                FCurEntry = CE;
                FCurrentKey = CharNstr(FDelimiter, Key, FCurrentKey);
#ifdef NVA
                free(Key);
#endif
                return true;
            }
        }
#ifdef NVA
        free(Key);
#endif
        return false;
    }
    // in standard mode
    if(Key)
    {
        if(Key[0] == FDelimiter) // starting from root
        {
            FCurEntry = FRoot;
            FCurrentKey = Char2Str(FDelimiter, FCurrentKey);
            CE = FRoot;
            size_t z = strlen(Key) - 1;
            memmove(Key, &Key[1], z);
            Key[z] = 0;
        }
        else
        { // starting from current
            CE = FCurEntry;
            if(('\\' != FCurrentKey[0]) && (0 != FCurrentKey[1]))
                FCurrentKey = StrNchar(FCurrentKey, FDelimiter);
        }
        if(!strlen(Key))
#ifdef NVA
        { 
            free(Key);
#endif
            return true;
#ifdef NVA
        }
#endif
        if(Key[strlen(Key)-1] == FDelimiter)
            Key[strlen(Key)-1] = 0;
        while(true) 
        {
            i = -1;
            for(mcInt32 z=0; Key[z]; z++)
            if(Key[z] == FDelimiter) 
            { 
                i = z; 
                break; 
            }
            if(i > -1)
            {
                S = (char*)malloc(i+1);
                strncpy(S, Key, i);
                S[i] = 0;
                if(!*S) 
                { 
                    free(S);
#ifdef NVA
                    free(Key);
#endif
                    return(false); 
                }
                size_t z = strlen(Key) - i - 1;
                memmove(Key, &Key[i+1], z);
                Key[z] = 0;
            }
            else
            {
								i = strlen(Key);
								S = (char*)malloc(i+1);
								strncpy(S, Key, i);
                S[i] = 0;
                //S = _strdup(Key);
                Key[0] = 0;
            }
            CE = CE->GetValue(S);
            if(!CE)
            {
                if(!CanCreate) 
                { 
                    free(S);
#ifdef NVA
                    free(Key);
#endif
                    return(false); 
                }
                FCurrentKey = HF1(FCurrentKey, S, FDelimiter);
                CE = new MCTreeItem();
                CE->FIsKey = true;
                CE->FValueName = S;
                CE->SetParent(FCurEntry);
                FModified = true;
            }
            else
            {
                if(!CE->FIsKey)
                {
                    if(CanCreate)
                    {
                        CE->FIsKey = true;
                        FModified = true;
                        FCurrentKey = HF1(FCurrentKey, S, FDelimiter);
                    }
                    else
                    {  
#ifdef NVA
                        free(Key);
#endif
                        free(S);
                        return false;
                    }
                }
                else
                    FCurrentKey = HF1(FCurrentKey,S,FDelimiter);
                free(S);
            }
            FCurEntry = CE;
            if(!*Key) 
                break;
        }
        if((strlen(FCurrentKey) > 1) && (FCurrentKey[strlen(FCurrentKey)-2] == FDelimiter))
            FCurrentKey[strlen(FCurrentKey)-2] = 0;
    }
#ifdef NVA
    if(Key) 
        free(Key);
#endif
  return true;
}

char* MCDataTree::OwnKey(char* Key)
{
    char* t = FullKey(Key), *t1;
    mcInt32 l = (mcInt32)strlen(t), l1 = -1;
    for(mcInt32 z=l-1; z>=0; z--)
        if(t[z] == FDelimiter) 
        { 
            l1 = z; 
            break; 
        }
  
    if(l1 < 0) 
        return t;
    else
    {
        t1 = (char*)malloc(l - l1);
        strcpy(t1, &t[l1+1]);
        free(t);
        return(t1);
    }
}

char* MCDataTree::ParentKey(char* Key)
{
    char* t = FullKey(Key), *t1;
    mcInt32 l = (mcInt32)strlen(t), l1 = -1;
    for(mcInt32 z=(mcInt32)l-1; z>=0; z--)
        if(t[z] == FDelimiter) 
        { 
            l1 = z; 
            break; 
        }
    if(l1 < 0) 
    { 
        free(t); 
        return(Char2Str(FDelimiter, NULL)); 
    }
    else
    {
        t1 = (char*)malloc(l1 + 1);
        strcpy(t1, t);
        free(t);
        return t1;
    }
}

void MCDataTree::ParseLine(char* S, char*& Name, char*& Value, bool& HasName)
{
    bool b = false, iv = false;
    size_t z = strlen(S) + 1, vx = 0, nx = 0;
    char* n = (char*)malloc(z);
    char* v = (char*)malloc(z);

    memset(n, 0, z);
    memset(v, 0, z);
    for(mcUInt32 i=0; i<z-1; i++)
    {
        if(b)
        {
            if('"' == S[i]) 
                b = false;
            if(iv) 
                v[vx++] = S[i];
            else 
                n[nx++] = S[i];
        }
        else
        {
            if('"' == S[i]) 
                b = true;
            if(FComment && (0 == _strncmp(FComment, &S[i], strlen(FComment))))
                break;
            if((S[i] == FDivChar) && !iv) 
                iv = true;
            else
                if(iv) 
                    v[vx++] = S[i];
                else 
                    n[nx++] = S[i];
        }
    }
    HasName = iv;
#ifndef PALM_CW
    Name = (char*)realloc(n, strlen(n) + 1);
    Value = (char*)realloc(v, strlen(v) + 1);
#else
    Name = n;
    Value = v;
#endif
}

bool MCDataTree::ReadBinary(char* Key, char* ValueName, void* Buffer, mcInt32& BufferLen)
{
    MCTreeItem* E;

    E = GetValueEntry(Key, ValueName);
    if(E && evtBinary == E->FValueData->ValueType)
    {
        if(BufferLen < E->FValueData->DATALENGTH)
            BufferLen = E->FValueData->DATALENGTH;
        else
        {
            memmove(Buffer, E->FValueData->DATAVALUE, E->FValueData->DATALENGTH);
            BufferLen = E->FValueData->DATALENGTH;
            return true;
        }
    }
    return false;
}

bool MCDataTree::ReadBool(char* Key, char* ValueName, bool DefValue, bool& Value)
{
    MCTreeItem* E;

    Value = DefValue;
    E = GetValueEntry(Key, ValueName);
    if(E)
    {
        if(evtBoolean == E->FValueData->ValueType)
        {
            Value = E->FValueData->BoolValue;
            return true;
        }
        else
        if(evtInt == E->FValueData->ValueType)
        {
            Value = E->FValueData->IntValue != 0;
            return true;
        }
    }
    return false;
}

bool MCDataTree::ReadInteger(char* Key, char* ValueName, mcInt32 DefValue, mcInt32& Value)
{
    MCTreeItem* E;
    E = GetValueEntry(Key,ValueName);
    if(E)
    {
        if(evtInt == E->FValueData->ValueType)
        {
            Value = E->FValueData->IntValue;
            return true;
        }
        else
        if(evtString == E->FValueData->ValueType)
        {
            char* z;
            Value = strtol(E->FValueData->StrValue, &z, 0);
            if(*z) 
              Value = DefValue;
        }
        else
        if(evtWideString == E->FValueData->ValueType)
        {
            wchar_t* z;
            Value = wstrtol(E->FValueData->WideStrValue, &z, 0);
            if(*z) 
              Value = DefValue;
        }
        else
            Value = DefValue;
    }
    else
        Value = DefValue;
    return false;
}

bool MCDataTree::ReadMultiString(char* Key, char* ValueName, MCStringList* Strings)
{
    MCTreeItem* E;

    E = GetValueEntry(Key, ValueName);
    if(E && (evtMultiString == E->FValueData->ValueType))
    {
        Strings->CopyOf(E->FValueData->MStrValue);
        return true;
    }
    return false;
}

bool MCDataTree::ReadMultiWideString(char* Key, char* ValueName, MCWideStringList* Strings)
{
    MCTreeItem* E;

    E = GetValueEntry(Key, ValueName);
    if(E && (evtMultiWideString == E->FValueData->ValueType))
    {
        Strings->CopyOf(E->FValueData->MWideStrValue);
        return true;
    }
    return false;
}

bool MCDataTree::ReadWideString(char* Key, char* ValueName, wchar_t* DefValue, wchar_t*& Value)
{
    MCTreeItem* E;
    char* tValue = NULL;
    bool r = false;

    E = GetValueEntry(Key, ValueName);
    if(E)
    {
        if(evtMultiString == E->FValueData->ValueType)
        {
            tValue = E->FValueData->MStrValue->Text();
            r = true;
        }
        else 
        if(evtMultiWideString == E->FValueData->ValueType)
        {
            Value = E->FValueData->MWideStrValue->Text();
            r = true;
        }
        else 
        if (evtString == E->FValueData->ValueType)
        {
            if(E->FValueData->StrValue)
                tValue = _strdup(E->FValueData->StrValue);
            else
                tValue = NULL;
            r = true;
        }
        else
        if (evtWideString == E->FValueData->ValueType)
        {
            if(E->FValueData->WideStrValue)
                Value = wstrdup(E->FValueData->WideStrValue);
            else
                Value = NULL;
            r = true;
        }
        else 
        if(evtInt == E->FValueData->ValueType)
        {
            tValue = (char*)malloc(50);
#ifdef _SNPRINTF_ENABLED_
            _snprintf(tValue, 50, "%ld", (long int)E->FValueData->IntValue);
#else
            MakeLongAt(tValue, E->FValueData->IntValue, 49);
#endif
            r = true;
        }
        else 
        if(evtBoolean == E->FValueData->ValueType)
        {
            if(E->FValueData->BoolValue)
                tValue = _strdup("true");
            else
                tValue = _strdup("false");
            r = true;
        }
        else 
        if(evtBinary == E->FValueData->ValueType)
        {
            tValue = Data2Str(E->FValueData->DATAVALUE, E->FValueData->DATALENGTH);
            r = true;
        }
    }
    if(!r) {
		Value = wstrdup(DefValue);
    }
    else {
      if(tValue) {
        Value = str2wstr(tValue, DefaultCodePage);
        free(tValue);
      }
    }
    return r;
}

bool MCDataTree::ReadString(char* Key, char* ValueName, char* DefValue, char*& Value)
{
    MCTreeItem* E;

    E = GetValueEntry(Key, ValueName);
    if(E)
    {
        if(evtMultiString == E->FValueData->ValueType)
        {
            Value = E->FValueData->MStrValue->Text();
            return true;
        }
        else 
        if(evtMultiWideString == E->FValueData->ValueType)
        {
            wchar_t* w = E->FValueData->MWideStrValue->Text();
            Value = wstr2str(w, DefaultCodePage);
            if(NULL != w)
                free(w);
            return true;
        }
        else 
        if (evtString == E->FValueData->ValueType)
        {
            if(E->FValueData->StrValue)
                Value = _strdup(E->FValueData->StrValue);
            else
                Value = NULL;
            return true;
        }
        else 
        if (evtWideString == E->FValueData->ValueType)
        {
            if(E->FValueData->WideStrValue)
                Value = wstr2str(E->FValueData->WideStrValue, DefaultCodePage);
            else
                Value = NULL;
            return true;
        }
        else 
        if(evtInt == E->FValueData->ValueType)
        {
            Value = (char*)malloc(50);
#ifdef _SNPRINTF_ENABLED_
            _snprintf(Value, 50, "%ld", (long int)E->FValueData->IntValue);
#else
            MakeLongAt(Value, E->FValueData->IntValue, 49);
#endif
            return true;
        }
        else 
        if(evtBoolean == E->FValueData->ValueType)
        {
            if(E->FValueData->BoolValue)
                Value = _strdup("true");
            else
                Value = _strdup("false");
            return true;
        }
        else 
        if(evtBinary == E->FValueData->ValueType)
        {
            Value = Data2Str(E->FValueData->DATAVALUE, E->FValueData->DATALENGTH);
            return true;
        }
    }
    Value = _strdup(DefValue);
    return false;
}

bool MCDataTree::RenameKey(char* Key, char* NewName)
{
    MCTreeItem* E = GetValueEntry(Key,NULL), *P;
    if(E && (E != FRoot))
    {
        if(!strchr(NewName, FDelimiter)) 
            return false;
    
        char* s = (char*)malloc(strlen(Key) + strlen(NewName) + 1);
        char* z = strrchr(s, FDelimiter) + 1;
        strcpy(z, NewName);
        bool b = KeyExists(s);
        free(s);
        if(b) 
            return false;
				P = E->FParent;
				E->SetParent(NULL);
        free(E->FValueName);
        E->FValueName = _strdup(NewName);
				E->SetParent(P);
        FModified = true;
        return true;
    }
    return false;
}

bool MCDataTree::RenameValue(char* Key,char* ValueName,char* NewName)
{
    MCTreeItem* E = GetValueEntry(Key, ValueName), *P;
    if(E && (E != FRoot) && !GetValueEntry(Key, NewName)&& !strchr(NewName, FDelimiter))
    {
        P = E->FParent;
				E->SetParent(NULL);
				free(E->FValueName);
        E->FValueName = _strdup(NewName);
				E->SetParent(P);
        FModified=true;
        return true;
    }
    else 
        return false;
}

char* BoolValues[] = {"false", "true"};

void MCDataTree::IntSaveBinKey(TStream* F, char* KeyName, MCTreeItem* KeyEntry)
{
    bool b = true;
    mcInt32 i, k;
    size_t j;
    MCTreeItem* E;

    F->Write(&b, sizeof(b));
    WriteStringToStream(F, KeyName, true);
    for(i=1; i <= KeyEntry->FChildren->Length(); i++)
    {
        E = (MCTreeItem*)(*KeyEntry->FChildren)[i-1];
        if(evtUnknown == E->FValueData->ValueType) 
            continue;
        b = false;
        F->Write(&b, sizeof(b));
        WriteStringToStream(F, E->FValueName, true);
        F->Write(&E->FValueData->ValueType, sizeof(E->FValueData->ValueType));
        switch(E->FValueData->ValueType) 
        {
        case evtBoolean:
            j = sizeof(bool);
            break;
        case evtDouble:
            j = sizeof(double);
            break;
        case evtInt:
            j = sizeof(mcInt32);
            break;
        case evtString:
            j = E->FValueData->StrValue?strlen(E->FValueData->StrValue):0;
            break;
        case evtWideString:
            j = E->FValueData->WideStrValue?(wstrlen(E->FValueData->WideStrValue)*sizeof(wchar_t)):0;
            break;
        case evtMultiString:
            j = 0;
            for(k = 1; k <= E->FValueData->MStrValue->Length(); k++)
            {
                char* s = (char*)(*E->FValueData->MStrValue)[k-1];
                j += sizeof(mcInt32) + (s?strlen(s):0);
            }
            break;
        case evtMultiWideString:
            j = 0;
            for(k = 1; k <= E->FValueData->MWideStrValue->Length(); k++)
            {
                wchar_t* s = (wchar_t*)(*E->FValueData->MWideStrValue)[k-1];
                j += sizeof(mcInt32) + (s?wstrlen(s)*sizeof(wchar_t):0);
            }
            break;
        case evtBinary:
            j = E->FValueData->DATALENGTH;
            break;
        }

#ifdef M68K
        j = TurnLong(j);
#endif
        F->Write(&j, sizeof(j));
        switch(E->FValueData->ValueType) 
        {
        case evtBoolean:
            F->Write(&E->FValueData->BoolValue, sizeof(E->FValueData->BoolValue));
            break;
        case evtDouble:
        {
            double d =
#ifdef M68K
                TurnDouble(
#endif
                E->FValueData->DoubleValue
#ifdef M68K
                )
#endif
            ;
            F->Write(&d, sizeof(d));
            break;
        }
        case evtInt:
        {
#ifdef M68K
            mcInt32 d = TurnLong(E->FValueData->IntValue);
#else
            mcInt32 d = E->FValueData->IntValue;
#endif
            F->Write(&d,sizeof(d));
            break;
        }

        case evtString:
            WriteStringToStream(F, E->FValueData->StrValue, false);
            break;
        case evtWideString:
            WriteWideStringToStream(F, E->FValueData->WideStrValue, false);
            break;
        case evtMultiString:
            for(k=1; k<=E->FValueData->MStrValue->Length(); k++)
                WriteStringToStream(F, (char*)(*E->FValueData->MStrValue)[k-1], true);
            break;
        case evtMultiWideString:
            for(k=1; k<=E->FValueData->MWideStrValue->Length(); k++)
                WriteWideStringToStream(F, (wchar_t*)(*E->FValueData->MWideStrValue)[k-1], true);
            break;
        case evtBinary:
            F->Write(E->FValueData->DATAVALUE, E->FValueData->DATALENGTH);
            break;
        }
    } // for
    for(i=1; i<=KeyEntry->FChildren->Length(); i++)
    {
        E = (MCTreeItem*)(*KeyEntry->FChildren)[i-1];
        if(E->FIsKey) {
            if(KeyName)
            {
                size_t zl=strlen(KeyName)+strlen(E->FValueName)+2;
                char* z=(char*)malloc(zl);
#ifdef _SNPRINTF_ENABLED_
                _snprintf(z, zl, "%s%c%s", KeyName, FDelimiter, E->FValueName);
#else
                strcpy(z, KeyName);
                z[strlen(z)+1] = 0;
                z[strlen(z)] = FDelimiter;
                strcat(z, E->FValueName);
#endif
                IntSaveBinKey(F,z,E);
                free(z);
            }
            else {
                IntSaveBinKey(F, E->FValueName, E);
            }
        }
    } // for
}

void MCDataTree::IntSaveKey(TStream* F, char* KeyName, MCTreeItem* KeyEntry)
{
    mcInt32 i, j;
    MCTreeItem *E;
    char* S;

    if(KeyName)
    {
        size_t zl = strlen(KeyName) + 7;
        S = (char*)malloc(zl);
#ifdef _SNPRINTF_ENABLED_
        _snprintf(S, zl, "\n[%s]\n", KeyName);
#else
        strcpy(S, "\r\n[");
        strcat(S, KeyName);
        strcat(S, "]\r\n");
#endif
        WriteTextToStream(F, S, false);
        free(S);
    }
    for(i=1; i<=KeyEntry->FChildren->Length(); i++)
    {
        E = (MCTreeItem*)(*KeyEntry->FChildren)[i-1];
        if(E->FValueName && (E->FValueName[0]!='#'))
            S = StrNchar(E->FValueName, FDivChar, NULL);
        else
            S = NULL;
        switch(E->FValueData->ValueType) 
        {
        case evtUnknown:
            break;
        case evtDouble:
            S = AddFloatStr(S, E->FValueData->DoubleValue);
            WriteTextToStream(F, S);
            break;
        case evtInt:
            S = AddIntStr(S, E->FValueData->IntValue);
            WriteTextToStream(F, S);
            break;
        case evtBoolean:
            if(FSimple)
                S = AddIntStr(S, E->FValueData->BoolValue);
            else
                S = AddStr(S, BoolValues[E->FValueData->BoolValue?1:0]);
            WriteTextToStream(F, S);
            break;
        case evtString:
            S = AddStr(S, E->FValueData->StrValue);
            WriteTextToStream(F, S);
            break;
        case evtWideString:
            {
              char* ts = EncodeUTF8string(E->FValueData->WideStrValue);
              S = AddStr(S, ts);
              if(ts)
                free(ts);
              WriteTextToStream(F, S);
            }
            break;
        case evtMultiString:
            if(S) 
                free(S);
            for(j=1; j<=E->FValueData->MStrValue->Length(); j++)
            {
                size_t zl = strlen(E->FValueName) +
                    strlen((char*)(*E->FValueData->MStrValue)[j-1]) + 20;
                S = (char*)malloc(zl);
#ifdef _SNPRINTF_ENABLED_
                _snprintf(S, zl, "%s[%d]%c%s", E->FValueName, j-1, FDivChar,
                        (char*)((*E->FValueData->MStrValue)[j-1]));
#else
                strcpy(S, E->FValueName);
                strcat(S, "[");
                MakeLongAt(S, j-1, 17);
                strcat(S, "]");
                S[strlen(S)+1] = 0;
                S[strlen(S)] = FDivChar;
                strcat(S, (const char*)(*E->FValueData->MStrValue)[j-1]);
#endif
                WriteTextToStream(F, S);
                if(S) 
                { 
                    free(S); 
                    S = NULL; 
                }
            }
            break;
        case evtMultiWideString:
            if(S) 
                free(S);
            for(j=1; j<=E->FValueData->MWideStrValue->Length(); j++)
            {
                char* ts = EncodeUTF8string((wchar_t*)(*E->FValueData->MWideStrValue)[j-1]);
                size_t zl = strlen(E->FValueName) +
                    strlen(ts) + 20;
                S = (char*)malloc(zl);
#ifdef _SNPRINTF_ENABLED_
                _snprintf(S, zl, "%s[%d]%c%s", E->FValueName, j-1, FDivChar, ts);
#else
                strcpy(S, E->FValueName);
                strcat(S, "[");
                MakeLongAt(S, j-1, 17);
                strcat(S, "]");
                S[strlen(S)+1] = 0;
                S[strlen(S)] = FDivChar;
                strcat(S, ts);
#endif
                WriteTextToStream(F, S);
                if(S) 
                { 
                    free(S); 
                    S = NULL; 
                }
            }
            break;
        case evtBinary:
            char* d = Data2Str(E->FValueData->DATAVALUE, E->FValueData->DATALENGTH);
            S = AddStr(S, d);
            free(d);
            WriteTextToStream(F, S);
            break;
        } // case
        if(S) 
            free(S);
    } // for
    for(i=1; i<=KeyEntry->FChildren->Length(); i++)
    {
        E = (MCTreeItem*)(*KeyEntry->FChildren)[i-1];
        if(E->FIsKey) {
            if(KeyName)
            {
                size_t zl = strlen(KeyName) + strlen(E->FValueName) + 2;
                char* z = (char*)malloc(zl);
#ifdef _SNPRINTF_ENABLED_
                _snprintf(z, zl, "%s%c%s", KeyName, FDelimiter, E->FValueName);
#else
                strcpy(z, KeyName);
                z[strlen(z)+1] = 0;
                z[strlen(z)] = FDelimiter;
                strcat(z, E->FValueName);
#endif
                IntSaveKey(F,z,E);
                free(z);
            }
            else {
                IntSaveKey(F, E->FValueName, E);
            }
        }
    } // for
}

void MCDataTree::SaveToStream(TStream* Stream)
{
    TriggerBeforeSaveEvent();
#ifdef USE_CPPEXCEPTIONS
    try 
#else
    __try
#endif
    {
        if(FBinaryMode)
            IntSaveBinKey(Stream, NULL, FRoot);
        else
        {
            if(WarningMessage)
            {
                char* s = _strdup(FComment);
                s = AddStr(s, WarningMessage);
                WriteTextToStream(Stream, s);
                free(s);
            }
            IntSaveKey(Stream, NULL, FRoot);
        }
		FModified = false;
    }
#ifdef USE_CPPEXCEPTIONS
    catch(...) 
    {
        TriggerAfterSaveEvent();
        throw;
    }
    TriggerAfterSaveEvent();
#else
    __finally 
    {
        TriggerAfterSaveEvent();
    }
#endif
}

bool MCDataTree::Save(void)
{
    TStream *F = NULL;
    bool r = false;

	if (!FPath)
		return false;

    TriggerBeforeSaveEvent();
#ifdef USE_CPPEXCEPTIONS
    try 
#else
    __try
#endif
    {
        F = OpenStream(FPath, stWrite|stCreate);
        //if(!FPath) 
        //    FPath = _strdup(F->PathName()); // return(false);
#ifdef USE_CPPEXCEPTIONS
        try 
#else
        __try 
#endif
        {
            if(this->FBinaryMode)
                IntSaveBinKey(F,NULL,FRoot);
            else
            {
                if(WarningMessage)
                {
                    char* s = _strdup(FComment);
                    s = AddStr(s, WarningMessage);
                    WriteTextToStream(F, s);
                    free(s);
                }
                IntSaveKey(F,NULL,FRoot);
            }
            FModified = false;
            r = true;
        }
#ifdef USE_CPPEXCEPTIONS
        catch(...) 
        {
            delete F;
            throw;
        }
        delete F;
#else
        __finally 
        {
            delete F;
        }
#endif
    }
#ifdef USE_CPPEXCEPTIONS
    catch(...) 
    {
        TriggerAfterSaveEvent();
        throw;
    }
    TriggerAfterSaveEvent();
#else
    __finally 
    {
        TriggerAfterSaveEvent();
    }
#endif
    return r;
}

void MCDataTree::SetCurrentKey(char* NewValue)
{
    if (0 != strcmp(FCurrentKey, NewValue)) 
        OpenKey(NewValue, true);
}

void MCDataTree::SetDelimiter(char NewValue)
{
    if(FDelimiter != NewValue)
    {
        FDelimiter = NewValue;
        if(FCurEntry == FRoot)
            FCurrentKey = Char2Str(FDelimiter, FCurrentKey);
        if(!FLazyWrite) 
            Save();
    }
}

void MCDataTree::SetLazyWrite(bool NewValue)
{
    if(FLazyWrite != NewValue) 
        FLazyWrite = NewValue;
}

void MCDataTree::SetPath(char* NewValue)
{
    if((FPath && NewValue && 0 != strcmp(FPath, NewValue)) || NewValue)
    {
        if(FPath) 
            free(FPath);
        FPath = _strdup(NewValue);
    }
}

void MCDataTree::SetSimple(bool NewValue)
{
    char* RN;
    char TD;
    TStream* f;

    if(FSimple != NewValue)
    {
        RN = FPath;
        FPath = NULL;
        Save();
        FSimple = NewValue;
        if(FSimple)
        {
            TD = FDelimiter;
            FDelimiter = 0;
            Load();
            FDelimiter = TD;
        }
        else 
            Load();
        f = OpenStream(FPath, stWrite);
        f->DeleteFile();
        delete f;
        free(FPath);
        FPath = RN;
    }
}

void MCDataTree::SetValueType(char* Key, char* ValueName, MCValueType NewType)
{
    MCTreeItem *E = CreateValue(Key, ValueName);
    if(E)
    {
        E->Invalidate();
        E->FValueData->ValueType = NewType;
        if(evtMultiString == NewType) 
            E->FValueData->MStrValue = new MCStringList();
        if(evtMultiWideString == NewType) 
            E->FValueData->MWideStrValue = new MCWideStringList();
    }
    FModified = true;
}

mcInt32 MCDataTree::SubKeysCount(char* Key)
{
    mcInt32 r = -1;
    MCStringList *sl = new MCStringList();
    if(EnumSubKeys(Key, sl)) 
        r = sl->Length();
    delete sl;
    return r;
}

bool MCDataTree::ValueExists(char* Key, char* ValueName)
{
    char* S = _strdup(FCurrentKey);
    MCTreeItem *E;
    bool r = false;

    if(OpenKey(Key, false))
    {
        E = FCurEntry->GetValue(ValueName);
        if(E && !E->FIsKey) 
            r = true;
    }
    OpenKey(S, false);
    free(S);
    return r;
}

mcInt32 MCDataTree::ValuesCount(char* Key)
{
    mcInt32 r = -1;
    MCStringList *sl = new MCStringList();
    if(EnumValues(Key, sl)) 
        r = sl->Length();
    delete sl;
    return r;
}

bool MCDataTree::WriteBinary(char* Key, char* ValueName, void* Buffer, mcInt32 BufferLen)
{
    char* S;
    MCTreeItem *E;

    if(FSimple) 
        return false;
    S = _strdup(FCurrentKey);
    if(!OpenKey(Key, true))
    {
        OpenKey(S, false);
        free(S);
        return false;
    }
    E = CreateValue(NULL, ValueName);
    E->FValueData->ValueType = evtBinary;
    E->FValueData->DATAVALUE = malloc(BufferLen);
    memmove(E->FValueData->DATAVALUE, Buffer, BufferLen);
    E->FValueData->DATALENGTH = BufferLen;
    FModified = true;
    if(!FLazyWrite) 
        Save();
    OpenKey(S, false);
    free(S);
    return true;
}

bool MCDataTree::WriteBool(char* Key,char* ValueName,bool Value)
{
    char* S;
    MCTreeItem *E;

    if(FSimple) 
        return false;
    S = _strdup(FCurrentKey);
    if(!OpenKey(Key, true))
    {
        OpenKey(S, false);
        free(S);
        return false;
    }
    E = CreateValue(NULL, ValueName);
    E->FValueData->ValueType = evtBoolean;
    E->FValueData->BoolValue = Value;
    FModified = true;
    if(!FLazyWrite) 
        Save();
    OpenKey(S, false);
    free(S);
    return true;
}

bool MCDataTree::WriteInteger(char* Key, char* ValueName, mcInt32 Value)
{
    char* S;
    MCTreeItem *E;

    S = _strdup(FCurrentKey);
    if(!OpenKey(Key, true))
    {
        OpenKey(S, false);
        free(S);
        return false;
    }
    E = CreateValue(NULL, ValueName);
    E->FValueData->ValueType = evtInt;
    E->FValueData->IntValue = Value;
    FModified = true;
    if(!FLazyWrite) 
        Save();
    OpenKey(S, false);
    free(S);
    return true;
}

bool MCDataTree::WriteMultiString(char* Key, char* ValueName, MCStringList* Strings)
{
    char* S;
    MCTreeItem *E;

    if(FSimple) 
        return false;
    S = _strdup(FCurrentKey);
    if(!OpenKey(Key, true))
    {
        OpenKey(S, false);
        free(S);
        return false;
    }
    E = CreateValue(NULL, ValueName);
    E->FValueData->ValueType = evtMultiString;
    E->FValueData->MStrValue = new MCStringList();
    E->FValueData->MStrValue->CopyOf(Strings);
    FModified = true;
    if(!FLazyWrite) 
        Save();
    OpenKey(S, false);
    free(S);
    return true;
}

bool MCDataTree::WriteMultiWideString(char* Key, char* ValueName, MCWideStringList* Strings)
{
    char* S;
    MCTreeItem *E;

    if(FSimple) 
        return false;
    S = _strdup(FCurrentKey);
    if(!OpenKey(Key, true))
    {
        OpenKey(S, false);
        free(S);
        return false;
    }
    E = CreateValue(NULL, ValueName);
    E->FValueData->ValueType = evtMultiWideString;
    E->FValueData->MWideStrValue = new MCWideStringList();
    E->FValueData->MWideStrValue->CopyOf(Strings);
    FModified = true;
    if(!FLazyWrite) 
        Save();
    OpenKey(S, false);
    free(S);
    return true;
}

bool MCDataTree::WriteString(char* Key, char* ValueName, char* Value)
{
    char* S;
    MCTreeItem *E;

    S = _strdup(FCurrentKey);
    if(!OpenKey(Key, true))
    {
        OpenKey(S, false);
        free(S);
        return false;
    }
    E = CreateValue(NULL, ValueName);
    E->FValueData->ValueType = evtString;
    E->FValueData->StrValue = Value? _strdup(Value) : NULL;
    FModified = true;
    if(!FLazyWrite) 
        Save();
    OpenKey(S, false);
    free(S);
    return true;
}

bool MCDataTree::WriteWideString(char* Key, char* ValueName, wchar_t* Value)
{
    char* S;
    MCTreeItem *E;

    S = _strdup(FCurrentKey);
    if(!OpenKey(Key, true))
    {
        OpenKey(S, false);
        free(S);
        return false;
    }
    E = CreateValue(NULL, ValueName);
    E->FValueData->ValueType = evtWideString;
    E->FValueData->WideStrValue = Value?wstrdup(Value):NULL;
    FModified = true;
    if(!FLazyWrite) 
        Save();
    OpenKey(S, false);
    free(S);
    return true;
}

void MCDataTree::TriggerBeforeSaveEvent(void)
// Triggers the OnBeforeSave event. This is a virtual method (descendants 
// of this component can override it).
{
  if(OnBeforeSave) 
      OnBeforeSave(this);
}

void MCDataTree::TriggerAfterLoadEvent(void)
{
  if(OnAfterLoad) 
      OnAfterLoad(this);
}

void MCDataTree::TriggerBeforeLoadEvent(void)
{
  if(OnBeforeLoad) 
      OnBeforeLoad(this);
}

void MCDataTree::TriggerAfterSaveEvent(void)
{
  if(OnAfterSave) 
      OnAfterSave(this);
}

bool MCDataTree::ReadDouble(char* Key, char* ValueName, double DefValue, double& Value)
{
    MCTreeItem *E = GetValueEntry(Key, ValueName);

    if(E)
    {
        if(evtDouble == E->FValueData->ValueType)
        {
            Value = E->FValueData->DoubleValue;
            return true;
        }
        else
        if(evtString == E->FValueData->ValueType)
        {
            char* zz;
            if(E->FValueData->StrValue)
            {
                Value = strtod(E->FValueData->StrValue, &zz);
                if(*zz) 
                    Value = DefValue;
            }
            else
                Value = (double)NULL;
        }
        else
        if(evtWideString == E->FValueData->ValueType)
        {
            if(E->FValueData->WideStrValue)
            {
                wchar_t* zz;
                Value = wstrtod(E->FValueData->WideStrValue, &zz);
                if(*zz) 
                    Value = DefValue;
            }
            else
                Value = (double)NULL;
        }
        else
        if(evtInt == E->FValueData->ValueType) 
            Value = E->FValueData->IntValue;
        else 
            Value = DefValue;
    }
    else 
        Value = DefValue;
    return false;
}

bool MCDataTree::WriteDouble(char* Key, char* ValueName, double Value)
{
    char* S;
    MCTreeItem *E;

    S = _strdup(FCurrentKey);
    if(!OpenKey(Key, true))
    {
        OpenKey(S, false);
        free(S);
        return false;
    }
    E = CreateValue(NULL, ValueName);
    E->FValueData->ValueType = evtDouble;
    E->FValueData->DoubleValue = Value;
    FModified = true;
    if(!FLazyWrite) 
        Save();
    OpenKey(S, false);
    free(S);
    return true;
}

void MCDataTree::LoadKeyFromStream(TStream* F, char* KeyName)
{
  MCStringList* SL;
  if(FBinaryMode)
    IntLoadBinKey(F, KeyName);
  else
  {
    SL = new MCStringList();
#ifdef USE_CPPEXCEPTIONS
    try 
    {
#else
    __try 
    {
#endif
      LoadListFromStream(SL,F);
      IntLoadKey(SL, 0, KeyName);
    }
#ifdef USE_CPPEXCEPTIONS
    catch(...) 
    {
      delete SL;
      throw;
    }
    delete SL;
#else
    __finally 
    {
      delete SL;
    }
#endif
  }
}

void MCDataTree::SaveKeyToStream(TStream* F, char* KeyName)
{
  char* S = _strdup(FCurrentKey);
  if(OpenKey(KeyName, false))
  {
    if(FBinaryMode)
      IntSaveBinKey(F, FCurEntry->FValueName, FCurEntry);
    else
      IntSaveKey(F, FCurEntry->FValueName, FCurEntry);
  }
  OpenKey(S, false);
}

#ifdef USE_NAMESPACE
}
#endif

