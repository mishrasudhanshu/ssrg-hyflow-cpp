//====================================================
//                                                    
//  EldoS MsgConnect                                 
//                                                    
//   Copyright (c) 2001-2010, EldoS                   
//                                                    
//====================================================

#ifndef __MCSOCKETBASE__
#define __MCSOCKETBASE__
 
#include "MC.h"

typedef struct {
	mcInt32 dwSignature; // "EIDP", $50444945
	mcInt32 dwDataSize; // total size of the data
	mcInt32 dwCompressID;
	mcInt32 dwEncryptID;
	mcInt32 dwSealID;
} MCInetHeader ;

typedef enum {stmP2P, stmServer, stmClient} MCInetTransportMode;

typedef enum {bpFlexible, bpStrict} MCBandwidthPolicy;

#endif
