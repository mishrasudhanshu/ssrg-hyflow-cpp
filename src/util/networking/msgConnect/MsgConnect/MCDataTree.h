//====================================================
//                                                    
//  EldoS MsgConnect                                 
//                                                    
//   Copyright (c) 2001-2010, EldoS                   
//                                                    
//====================================================

#ifndef MCDATATREE_H
#define MCDATATREE_H
 
#include "MC.h"
#include "MCPlatforms.h"
#include "MCStreams.h"
#include "MCLists.h"

#ifdef USE_NAMESPACE
namespace MsgConnect
{
#endif

class MCDataTree;
class MCTreeItem;

enum {evtUnknown=0, evtBoolean, evtInt, evtString, evtMultiString,
      evtBinary, evtDouble, evtWideString, evtMultiWideString};

typedef unsigned char MCValueType;

typedef struct {
public:
  MCValueType ValueType;
  union {
    bool             BoolValue;
    mcInt32             IntValue;
    char*            StrValue;
    MCStringList*     MStrValue;
    double           DoubleValue;
    wchar_t*         WideStrValue;
    MCWideStringList* MWideStrValue;
#ifndef PALM
    struct {
    void* DataValue;
    mcInt32 DataLength;
    } DataRec;
#define DATAVALUE DataRec.DataValue
#define DATALENGTH DataRec.DataLength
#else
    void* DataValue;
#define DATAVALUE DataValue
#define DATALENGTH DataLength
#endif
  };
#ifdef PALM
  mcInt32  DataLength;
#endif
} MCValueData;


class MCTreeItem {
private:
friend class MCDataTree;
    MCList*         FChildren;
    bool            FIsKey;
    MCTreeItem*     FParent;
    MCValueData*    FValueData;
    char*           FValueName;
    static DEFSECT void     OnValueDelete(void* Sender, void* Data);
    static DEFSECT mcInt32     ItemsCompare(void* Sender,void* V1,void* V2);
    DEFSECT bool            DubFrom(MCTreeItem* Peer, bool CopySubKeys, bool CopyValue);
    DEFSECT void            SetParent(MCTreeItem* Value);
protected:
    DEFSECT MCTreeItem(void);
    DEFSECT MCTreeItem*     GetValue(char* Name);
    DEFSECT MCTreeItem*     GetParent(void);
public:
    DEFSECT virtual         ~MCTreeItem(void);
    DEFSECT void            Invalidate(void);
    DEFSECT bool            IsKey(void);
    DEFSECT mcInt32            SubCount(void);
};

typedef void (*NotifyEvent)(void* Sender);

class MCDataTree {
private:
    bool            FBinaryMode;
    char*           FComment;
    MCTreeItem*     FCurEntry;
    char*           FCurrentKey;
    char            FDelimiter;
    char            FDivChar;
    bool            FLazyWrite;
    bool            FModified;
    char*           FPath;
    MCTreeItem*     FRoot;
    bool            FSimple;

    DEFSECT void                IntLoadKey(MCStringList* SL, mcInt32 CurLine, char* LoadInto);
    DEFSECT void                IntLoadBinKey(TStream* F, char* LoadInto);
    DEFSECT void                IntSaveKey(TStream* F, char* KeyName, MCTreeItem* KeyEntry);
    DEFSECT void                IntSaveBinKey(TStream* F, char* KeyName, MCTreeItem* KeyEntry);
protected:
    DEFSECT virtual MCTreeItem* GetValueEntry(char* Key, char* ValueName);
    DEFSECT virtual void        ParseLine(char* S, char*& Name, char*& Value, bool& HasName);
    DEFSECT virtual void        TriggerBeforeSaveEvent(void);
    DEFSECT virtual void        TriggerAfterLoadEvent(void);
    DEFSECT virtual void        TriggerBeforeLoadEvent(void);
    DEFSECT virtual void        TriggerAfterSaveEvent(void);
public:
    unsigned int DefaultCodePage;
    DEFSECT MCDataTree(void);
    DEFSECT virtual             ~MCDataTree(void);
    DEFSECT bool                Clear(void);
    DEFSECT virtual bool        ClearKey(char* Key);
    DEFSECT MCTreeItem*         CreateValue(char* Key, char* ValueName);
    DEFSECT bool                Delete(char* Key, char* ValueName);
    DEFSECT bool                EnumSubKeys(char* Key, MCStringList* Strings);
    DEFSECT bool                EnumValues(char* Key, MCStringList* Strings);
    DEFSECT char*               FullKey(char* Key);
    DEFSECT MCValueType         GetValueType(char* Key, char* ValueName);
    DEFSECT bool                KeyExists(char* Key);
    DEFSECT void                LoadFromStream(TStream* Stream);
    DEFSECT void                SaveToStream(TStream* Stream);
    DEFSECT bool                Load(void);
    DEFSECT virtual void        Loaded(void);
    DEFSECT bool                MoveEntry(char* Key, char* ValueName, char* NewKey);
    DEFSECT bool                CopyToDataTree(char* Key, char* ValueName, MCDataTree* NewTree,
        char *NewKey, bool CopySubKeys, bool CopyValue);
    DEFSECT bool                CopyEntry(char* Key, char* ValueName, char *NewKey, bool CopySubKeys, bool CopyValue);
    DEFSECT bool                OpenKey(char* Key, bool CanCreate);
    DEFSECT char*               OwnKey(char* Key);
    DEFSECT char*               ParentKey(char* Key);
    DEFSECT bool                ReadBinary(char* Key, char* ValueName, void* Buffer, mcInt32& Value);
    DEFSECT bool                ReadBool(char* Key, char* ValueName, bool DefValue, bool& Value);
    DEFSECT bool                ReadDouble(char* Key, char* ValueName, double DefValue, double& Value);
    DEFSECT bool                ReadInteger(char* Key, char* ValueName, mcInt32 DefValue, mcInt32& Value);
    DEFSECT bool                ReadMultiString(char* Key, char* ValueName, MCStringList* Strings);
    DEFSECT bool                ReadMultiWideString(char* Key, char* ValueName, MCWideStringList* Strings);
    DEFSECT bool                ReadString(char* Key, char* ValueName, char* DefValue, char*& Value);
    DEFSECT bool                ReadWideString(char* Key, char* ValueName, wchar_t* DefValue, wchar_t*& Value);
    DEFSECT virtual bool        RenameKey(char* Key, char* NewName);
    DEFSECT virtual bool        RenameValue(char* Key, char* ValueName, char* NewName);
    DEFSECT bool                Save(void);
    DEFSECT virtual void        SetValueType(char* Key, char* ValueName, MCValueType NewType);
    DEFSECT mcInt32                SubKeysCount(char* Key);
    DEFSECT bool                ValueExists(char* Key, char* ValueName);
    DEFSECT mcInt32                ValuesCount(char* Key);
    DEFSECT bool                WriteBinary(char* Key, char* ValueName, void* Buffer, mcInt32 BufferLen);
    DEFSECT bool                WriteBool(char* Key, char* ValueName, bool Value);
    DEFSECT bool                WriteDouble(char* Key, char* ValueName, double Value);
    DEFSECT bool                WriteInteger(char* Key, char* ValueName, mcInt32 Value);
    DEFSECT bool                WriteMultiString(char* Key, char* ValueName, MCStringList* Strings);
    DEFSECT bool                WriteMultiWideString(char* Key, char* ValueName, MCWideStringList* Strings);
    DEFSECT bool                WriteString(char* Key, char* ValueName, char* Value);
    DEFSECT bool                WriteWideString(char* Key, char* ValueName, wchar_t* Value);
    DEFSECT void                SetCurrentKey(char* NewValue);
    DEFSECT char*               GetCurrentKey(void);
    DEFSECT void                SetComment(const char* NewValue);
    DEFSECT char*               GetComment(void);
    DEFSECT bool                Modified(void);
    DEFSECT bool                GetBinaryMode(void);
    DEFSECT void                SetBinaryMode(bool);
    DEFSECT void                SetDelimiter(char NewValue);
    DEFSECT char                GetDelimiter(void);
    DEFSECT void                SetDivChar(char NewValue);
    DEFSECT char                GetDivChar(void);
    DEFSECT void                SetLazyWrite(bool NewValue);
    DEFSECT bool                GetLazyWrite(void);
    DEFSECT void                SetPath(LPSTR NewValue);
    DEFSECT char*               GetPath(void);
    DEFSECT void                SetSimple(bool NewValue);
    DEFSECT bool                GetSimple(void);
    DEFSECT char*               WarningMessage;
    DEFSECT NotifyEvent         OnBeforeLoad;
    DEFSECT NotifyEvent         OnAfterSave;
    DEFSECT NotifyEvent         OnBeforeSave;
    DEFSECT NotifyEvent         OnAfterLoad;
    // -----
    DEFSECT void                LoadKeyFromStream(TStream* F, char* KeyName);
    DEFSECT void                SaveKeyToStream(TStream* F, char* KeyName);
};

#ifdef USE_NAMESPACE
}
#endif

#endif
