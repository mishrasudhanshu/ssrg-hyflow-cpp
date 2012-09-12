//====================================================
//                                                    
//  EldoS MsgConnect                                 
//                                                    
//   Copyright (c) 2001-2010, EldoS                   
//                                                    
//====================================================
#include "MC.h"
#include "MCUtils.h"
#include "MCSock.h"
#if !defined(QNX) && !defined(__APPLE__)
//#	define socklen_t 	size_t
#endif
 
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
#	include <stdio.h>
#	include <stdlib.h>
#	include <sys/ioctl.h>
#	include <errno.h>
#define WSAEWOULDBLOCK EWOULDBLOCK
#endif

#include "MC.h"

#ifdef USE_NAMESPACE
namespace MsgConnect
{
#endif

const mcInt32 SocketTypes[2] = {SOCK_STREAM, SOCK_DGRAM};
      
bool WinsockInitialized = false;
mcInt32 SocketsCreated = 0;

void WinsockErrorCheck(mcInt32 ErrorCode)
{
	if(ErrorCode != 0)
        THROW_ERROR(ErrorCode);
}

class MCBase64Codec
{

public:
    //  Encode buffer to Base64
    static char* Encode(const char *pInput, size_t Len);

};

//  Encode buffer to Base64
char* MCBase64Codec::Encode(const char *pInput, size_t Len)
{
    const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	char *pEncoded = (char *)calloc((mcInt32) floor(Len + (Len * 0.7) + 1), sizeof(pInput[0]));  //  Memory consuming version

    char ch, pc = '\0';
    int j = 0, x = 0;
    for (size_t i = 0; i < Len; i++)
    {
        ch = pInput[i];
        switch (j)
        {
        case 0:
            pEncoded[x++] = alphabet[(ch & 0xfc) >> 2];
            break;
        case 1:
            pEncoded[x++] = alphabet[((pc & 0x03) << 4) ^ ((ch & 0xf0) >> 4)];
            break;
        case 2:
            pEncoded[x++] = alphabet[((pc & 0x0f) << 2) ^ ((ch & 0xc0) >> 6)];
            pEncoded[x++] = alphabet[ch & 0x3f];
            j = -1;
            break;
        }
        j++;
        pc = ch;
    }
    switch (j)
    {
    case 1:
        pEncoded[x++] = alphabet[((pc & 0x03) << 4)];
        pEncoded[x++] = '=';
        pEncoded[x++] = '=';
        break;
    case 2:
        pEncoded[x++] = alphabet[((pc & 0x0f) << 2)];
        pEncoded[x++] = '=';
        break;
    }
    pEncoded[x] = '\0';

    char *pResult = strdup(pEncoded);
    free(pEncoded);

    return pResult;
}

// ********************************* MCSocket *********************************

MCSocket::MCSocket(void)
{
	InitializeWinSock();
    SocketsCreated++;
	FSocket = INVALID_SOCKET;
	FState = issNotASocket;
	FLocalAddress = NULL;
	FRemoteAddress = NULL;
	FCloseRequest = false;
	FDirection = isdOutgoing;
	FRemotePort = 0;
	FLocalPort = 0;
	FIsClient = false;

    //  Web tunneling support
    FUseWebTunneling = false;
    FWebTunnelAddress = NULL;
    FWebTunnelPort = 4080;
    FWebTunnelAuthentication = wtaNoAuthentication;
    FWebTunnelUserId = NULL;
    FWebTunnelPassword = NULL;

    FIncomingBuf.Data = MCMemAlloc(SOCKETBUF_INITIAL_SIZE);
    if (NULL == FIncomingBuf.Data)
        THROW_ERROR(MCError_NotEnoughMemory);
        
    FIncomingBuf.Len = 0;
    FIncomingBuf.Size = SOCKETBUF_INITIAL_SIZE;
}

MCSocket::~MCSocket(void)
{
    if (NULL != FRemoteAddress)
    {
        MCMemFree(FRemoteAddress);
        FRemoteAddress = NULL;
    }

    if (NULL != FLocalAddress)
    {
        MCMemFree(FLocalAddress);
        FLocalAddress = NULL;
    }
	if (NULL != FIncomingBuf.Data)
	{
		MCMemFree(FIncomingBuf.Data);
	}

    //  Web tunneling support
    if (NULL != FWebTunnelAddress)
    {
        free(FWebTunnelAddress);
        FWebTunnelAddress = NULL;
    }
    if (NULL != FWebTunnelUserId)
    {
        free(FWebTunnelUserId);
        FWebTunnelUserId = NULL;
    }
    if (NULL != FWebTunnelPassword)
    {
        free(FWebTunnelPassword);
        FWebTunnelPassword = NULL;
    }

	SocketsCreated--;
	FinalizeWinSock();
}

/*
This method is called by the transport thread after socket is connected. 
All communications between connection parties, that are not part of normal 
data exchange (SOCKS authentication, SSL connection initiation),
should be implemented inside of this method.
*/
mcInt32 MCSocket::AfterConnection(MCSocket* CtlSocket, mcInt32 Timeout)
{
	// intentionally left blank
	return 0;
}

mcInt32  MCSocket::AsyncConnect(MCSocket* CtlSocket, mcInt32 Timeout)
{
    FDirection = isdOutgoing;
    return 0;
}

bool MCSocket::Connect(void)
{
	FDirection = isdOutgoing;
	return true;
}

mcInt32  MCSocket::ExtConnect(MCSocket* CtlSocket, mcInt32 Timeout)
{
    FDirection = isdOutgoing;
    return 0;
}

void MCSocket::FinalizeWinSock(void)
{
#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
#endif
}

bool MCSocket::HasBufferedOutgoingData()
{
	return false;
}

bool MCSocket::HasBufferedIncomingData()
{
	return FIncomingBuf.Len > 0;
}

TSocket MCSocket::getSocket(void)
{
	return FSocket;
}

void MCSocket::InitializeWinSock(void)
{
#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
        THROW_ERROR(MCError_WinsockInitFailed);
	WinsockInitialized = true;
#endif
}

mcInt32 MCSocket::Listen(mcInt32 /*BackLog*/)
{
	FDirection = isdIncoming;
	return 0;
}

bool MCSocket::PopFromCtlSocket(MCSocket* CtlSocket)
{
	mcInt32 i;
	char p;
	char* RemoteHost;
	mcUInt32 RemotePort;

	CtlSocket->ReceiveFrom(&p, 1, i, RemoteHost, RemotePort);
	return p != 0;
}

bool MCSocket::PostprocessOutgoingData()
{
	return true;
}

bool MCSocket::PreprocessIncomingData()
{
	return true;
}
	
void MCSocket::setLocalAddress(const char* Value)
{
	if(FState != issNotASocket && FState != issInitialized && FState != issConnected)
        THROW_ERROR(MCError_WrongSocketState);
	if(FLocalAddress)
		MCMemFree(FLocalAddress);
	if(Value)
		FLocalAddress = strdup(Value);
	else
		FLocalAddress = NULL;
}

void MCSocket::setLocalPort(mcUInt32 Value)
{
	if(FState != issNotASocket && FState != issInitialized)
        THROW_ERROR(MCError_WrongSocketState);
	FLocalPort = Value;
}

void MCSocket::setRemoteAddress(const char* Value)
{
	if(FState != issNotASocket && FState != issInitialized)
        THROW_ERROR(MCError_WrongSocketState);
	if(FRemoteAddress)
		MCMemFree(FRemoteAddress);
	if(Value)
		FRemoteAddress = strdup(Value);
	else
		FRemoteAddress = NULL;
}

mcInt32 MCSocket::StartAsyncConnect()
{
	FDirection = isdOutgoing;
	return 0;
}

mcInt32 MCSocket::SyncConnect(void)
{
	FDirection = isdOutgoing;
	return 0;
}

void MCSocket::setRemotePort(mcUInt32 Value)
{
	if(FState != issNotASocket && FState != issInitialized)
        THROW_ERROR(MCError_WrongSocketState);
	FRemotePort = Value;
}

MCSocketDir MCSocket::getDirection(void)
{
	return FDirection;
}

MCSocketState MCSocket::getState(void)
{
	return FState;
}

char* MCSocket::getLocalAddress(void)
{
	return FLocalAddress;
}

mcUInt32 MCSocket::getLocalPort(void)
{
	return FLocalPort;
}

char* MCSocket::getRemoteAddress(void)
{
	return FRemoteAddress;
}

mcUInt32 MCSocket::getRemotePort(void)
{
	return FRemotePort;
}


//  Web tunneling support
void MCSocket::setUseWebTunneling(bool Value)
{
    FUseWebTunneling = Value;
}

bool MCSocket::getUseWebTunneling()
{
    return FUseWebTunneling;
}

void MCSocket::setWebTunnelAddress(const char *Value)
{
    if (FWebTunnelAddress)
        free(FWebTunnelAddress);
    FWebTunnelAddress = strdup(Value);
}

const char* MCSocket::getWebTunnelAddress()
{
    return FWebTunnelAddress;
}

void MCSocket::setWebTunnelPort(unsigned short Value)
{
    FWebTunnelPort = Value;
}

unsigned short MCSocket::getWebTunnelPort()
{
    return FWebTunnelPort;
}

MCWebTunnelAuthentication MCSocket::getWebTunnelAuthentication()
{
    return FWebTunnelAuthentication;
}

void MCSocket::setWebTunnelAuthentication(MCWebTunnelAuthentication Value)
{
    FWebTunnelAuthentication = Value;
}

void MCSocket::setWebTunnelUserId(const char *Value)
{
    if (FWebTunnelUserId)
        free(FWebTunnelUserId);
    FWebTunnelUserId = strdup(Value);
}

const char* MCSocket::getWebTunnelUserId()
{
    return FWebTunnelUserId;
}

void MCSocket::setWebTunnelPassword(const char *Value)
{
    if (FWebTunnelPassword)
        free(FWebTunnelPassword);
    FWebTunnelPassword = strdup(Value);
}

const char* MCSocket::getWebTunnelPassword()
{
    return FWebTunnelPassword;
}

#ifdef MC_NO_PURE_VIRTUAL
MCSocket* MCSocket::CreateAcceptingSocket()
{
	MC_pure_virtual();
	return NULL;
}

MCSocket* MCSocket::Accept(void)
{
	MC_pure_virtual();
	return NULL;
}

mcInt32 MCSocket::Bind(void)
{
	MC_pure_virtual();
	return 0;
}

void MCSocket::Close(bool Forced)
{
	MC_pure_virtual();
}

void MCSocket::FinishAsyncConnect(void)
{
	MC_pure_virtual();
}

mcInt32 MCSocket::Init(MCSocketType SocketType, bool AllocateSocket)
{
	MC_pure_virtual();
	return 0;
}

mcInt32 MCSocket::IntReceive(void* Data, mcInt32 DataLen, mcInt32& Received)
{
	MC_pure_virtual();
	return 0;
}

mcInt32 MCSocket::IntSend(void* Data, mcInt32 DataLen, mcInt32& Sent)
{
	MC_pure_virtual();
	return 0;
}

mcInt32 MCSocket::Receive(void* Data, mcInt32 DataLen, mcInt32& Received)
{
	MC_pure_virtual();
	return 0;
}

mcInt32 MCSocket::ReceiveFrom(void* Data, mcInt32 DataLen, mcInt32& Received, 
						   char*& RemoteAddress, mcUInt32& RemotePort)
{
	MC_pure_virtual();
	return 0;
}

void MCSocket::ReturnData(void* Data, mcInt32 DataLen)
{
	MC_pure_virtual();
}

mcInt32 MCSocket::ReusePort()
{
	MC_pure_virtual();
	return 0;
}

mcInt32 MCSocket::Send(void* Data, mcInt32 DataLen, mcInt32& Sent)
{
	MC_pure_virtual();
	return 0;
}

mcInt32 MCSocket::SendTo(void* Data, mcInt32 DataLen, mcInt32& Sent, 
					  char* RemoteAddress, mcUInt32 RemotePort)
{
	MC_pure_virtual();
	return 0;
}
#endif

// ******************************** MCStdSocket *******************************

MCStdSocket::MCStdSocket()
:MCSocket()
{
}

MCStdSocket::~MCStdSocket()
{
    Close(false);
}

MCSocket* MCStdSocket::Accept(void)
{
	TSocket NewSocket;
	struct sockaddr_in FAddr;
	int FAddrLen;
	char* addr;
	MCSocket* r;

	if(FState != issListening)
        THROW_ERROR(MCError_WrongSocketState);
	FAddrLen = sizeof(FAddr);
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
	NewSocket = accept(FSocket, (sockaddr*)&FAddr, (socklen_t*)&FAddrLen);
#else
#endif
	if(NewSocket != (mcInt32)INVALID_SOCKET)
	{
		r = CreateAcceptingSocket();
		r->Init(istStream, false);
		r->FSocket = NewSocket;
		r->FDirection = isdIncoming;
		//OutputDebugString("Incoming socket accepted\n");
		r->setRemotePort(ntohs(FAddr.sin_port));
		addr = inet_ntoa(FAddr.sin_addr);
		if(addr)
			r->setRemoteAddress(addr);
		else
			r->setRemoteAddress(NULL);
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
		if(getsockname(NewSocket, (sockaddr*)&FAddr, (socklen_t*)&FAddrLen) != SOCKET_ERROR)
#else
#endif
		{
			r->FLocalPort = ntohs(FAddr.sin_port);
			addr = inet_ntoa(FAddr.sin_addr);
			if(addr)
				r->setLocalAddress(addr);
			else
				r->setLocalAddress(NULL);
		}
		else
		{
			r->setLocalAddress(getLocalAddress());
			r->setLocalPort(getLocalPort());
		}
		r->FState = issConnected;
   	}
	else
		return NULL;
	return r ;
}

mcInt32 MCStdSocket::AsyncConnect(MCSocket* CtlSocket, mcInt32 Timeout)
{
	MCSocket::AsyncConnect(CtlSocket, Timeout);
	fd_set FDSendSet, FDRecvSet, FDErrSet;
    mcInt32 elapsed = 0;
	mcUInt32 highSocketHandle;
    double startTime;    

	int select_res;
	mcInt32 result = StartAsyncConnect();

	while (FState == issConnecting && ((0 == Timeout) || (elapsed <= Timeout)))
    {
        FD_ZERO(&FDSendSet);
		FD_ZERO(&FDRecvSet);
		FD_ZERO(&FDErrSet);
        FD_SET(FSocket, &FDSendSet);
		FD_SET(FSocket, &FDErrSet);
		if (CtlSocket)
		{
			highSocketHandle = FSocket > CtlSocket->getSocket() ? FSocket : CtlSocket->getSocket();
			FD_SET(CtlSocket->getSocket(), &FDRecvSet);
			FD_SET(CtlSocket->getSocket(), &FDErrSet);
		}
		else
			highSocketHandle = FSocket;

        timeval TimeVal;
		TimeVal.tv_sec = 0;
		TimeVal.tv_usec = (Timeout - elapsed) * 1000;
#ifdef __APPLE__
                TimeVal.tv_sec = TimeVal.tv_usec / 1000000;
                TimeVal.tv_usec = TimeVal.tv_usec % 1000000;
#endif
		
		timeval* PTV = &TimeVal;
        startTime = Now();
	
		
		if (CtlSocket == NULL)
			select_res = select(highSocketHandle+1, NULL, &FDSendSet, &FDErrSet, PTV);
		else
			select_res = select(highSocketHandle+1, &FDRecvSet, &FDSendSet, &FDErrSet, PTV);
        
        if ( select_res > 0)
        {
#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
			if (FDErrSet.fd_count > 0)
				break;
#endif

			if (CtlSocket != NULL && FD_ISSET(CtlSocket->getSocket(), &FDRecvSet))
			{
				if (!PopFromCtlSocket(CtlSocket))
					break;
			}
			if (FD_ISSET(FSocket, &FDSendSet))
			{
				FinishAsyncConnect();
				break;
			}
        }
        else
        if (select_res == SOCKET_ERROR)
        {
#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
            select_res = WSAGetLastError();
#else
            select_res = errno;
#endif
            RETURN_EXCEPT;
        }

        elapsed += (mcInt32)((Now() - startTime) * 86400 * 1000 + 0.5);
    }
	if (getState() != issConnected)
	{
		Close(true);
#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
    	result = WSAETIMEDOUT;
#else
		result = ETIMEDOUT;
#endif
	}
	else
		result = true;
	return result;
}

mcInt32 MCStdSocket::Bind(void)
{
	struct sockaddr_in FAddr;
	mcUInt32 addr;
	struct hostent* HostEnt = NULL;
	mcInt32 r = 0;
	int i = 0;
	struct in_addr TAddr;
    

	if((FSocket == (mcInt32)INVALID_SOCKET) && (Init(istStream) != 0))
        THROW_ERROR(MCError_NotASocket);
	if(FState != issInitialized)
        THROW_ERROR(MCError_WrongSocketState);
	  
	if (NULL != getLocalAddress())
        addr = inet_addr(getLocalAddress());
    else
        addr = INADDR_NONE;

	if(addr == INADDR_NONE)
	{
		HostEnt = gethostbyname(getLocalAddress());
		if(HostEnt)
			memmove(&addr, &HostEnt->h_addr[0], sizeof(addr));
	}
	if(addr == INADDR_NONE)
        THROW_ERROR(MCError_InvalidAddress);
	memset(&FAddr, 0, sizeof(FAddr));
	FAddr.sin_family = AF_INET;
	FAddr.sin_addr.s_addr = addr;
	FAddr.sin_port = htons(getLocalPort());
	r = bind(FSocket, (sockaddr*)&FAddr, sizeof(FAddr));
	if(r == 0)
	{
		if((addr == 0) || (getLocalPort() == 0))
		{
			memset(&FAddr, 0, sizeof(FAddr));
			FAddr.sin_family = AF_INET;
			i = sizeof(FAddr);
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
			if(getsockname(FSocket, (sockaddr*)&FAddr, (socklen_t*)&i) == 0)
#else
			if(getsockname(FSocket, (sockaddr*)&FAddr, &i) == 0)
#endif
			{
				TAddr.s_addr = FAddr.sin_addr.s_addr;
				setLocalPort(ntohs(FAddr.sin_port));
				setLocalAddress(inet_ntoa(TAddr));
			}
		}
		FState = issBound;
	}
	else
        r = LAST_ERROR;
	return r;
}

void MCStdSocket::Close(bool Forced)
{
	if(FSocket != (mcInt32)INVALID_SOCKET)
	{
		/*
		if (FDirection == isdOutgoing)
			OutputDebugString("Outgoing socket closed\n");
		else
			OutputDebugString("Incoming socket closed\n");
		*/
		if(!Forced)
			shutdown(FSocket, SD_BOTH);
		closesocket(FSocket);
		FSocket = INVALID_SOCKET;
		FState = issNotASocket;
	}
	else
	if(FState == issInitializing)
		FCloseRequest = true;
    if (NULL != FIncomingBuf.Data)
        free(FIncomingBuf.Data);
    FIncomingBuf.Data = NULL;
    FIncomingBuf.Len = 0;
    FIncomingBuf.Size = 0;
}

bool MCStdSocket::Connect()
{
        if (FUseWebTunneling)
            return HTTPConnect(NULL, 0x7FFFFFFF) == 0;
	    else
		    return SyncConnect() == 0;
}

mcInt32 MCStdSocket::ExtConnect(MCSocket* CtlSocket, mcInt32 Timeout) 
{
	MCSocket::ExtConnect(CtlSocket, Timeout);
        if (FUseWebTunneling)
            return HTTPConnect(CtlSocket, Timeout);
	else
	if (Timeout > 0)
		return AsyncConnect(CtlSocket, Timeout);
	else
		return SyncConnect();
}

void MCStdSocket::FinishAsyncConnect()
{
    int i = 0;
    struct in_addr TAddr;

    FState = issConnected;
    {
//		OutputDebugString("Outgoing socket connected\n");
		if(!getLocalAddress() || (getLocalPort() == 0))
		{
			memset(&FAddr, 0, sizeof(FAddr));
			FAddr.sin_family = AF_INET;
			i = sizeof(FAddr);
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
			if(getsockname(FSocket, (sockaddr*)&FAddr, (socklen_t*)&i) == 0)
#else
#endif
			{
				TAddr.s_addr = FAddr.sin_addr.s_addr;
				FLocalPort = ntohs(FAddr.sin_port);
				setLocalAddress(inet_ntoa(TAddr));
			}
		}
    }
}

bool MCStdSocket::SocksSendReceive(bool &Terminated, MCSocket* CtlSocket, mcUInt32 Timeout, 
									void* sendBuf, mcInt32 sendBufSize, mcInt32& wasSent,
									void* readBuf, mcInt32 readBufSize, mcInt32& wasRead, bool needDoubleCRLF)
{

	const unsigned char CRLFCRLF[] = {0x0d, 0x0a, 0x0d, 0x0a};

    mcInt32 highestSocketHandle;
	if (CtlSocket)
		highestSocketHandle = CtlSocket->getSocket() > FSocket ? CtlSocket->getSocket() : FSocket;
	else
		highestSocketHandle = FSocket;

	mcUInt32 TimeoutV = 0;
    timeval TimeVal, *pTV = NULL;
	fd_set FDRecvSet, FDSendSet;
	int selectRes;
		
	wasSent = 0;
	wasRead = 0;

	mcUInt32 started = GetTickCount();
	mcUInt32 elapsed = 0;

	while (true)
	{
		elapsed = GetTickCount() - started;
		if (elapsed >= Timeout)
			return false;

		FD_ZERO(&FDRecvSet); FD_ZERO(&FDSendSet);

		if (Timeout > 0)
			TimeoutV = Timeout - elapsed;
		else
			TimeoutV = 0;

		if (TimeoutV > 0)
		{
			TimeVal.tv_sec = TimeoutV / 1000;
			TimeVal.tv_usec = (TimeoutV % 1000) * 1000;
			pTV = &TimeVal;
		}
		else
			pTV = NULL;
	    
		if (CtlSocket)
			FD_SET(CtlSocket->getSocket(), &FDRecvSet);
		FD_SET(FSocket, &FDSendSet);

		if (CtlSocket)
			selectRes = select(highestSocketHandle+1, &FDRecvSet, &FDSendSet, NULL, pTV);
		else
			selectRes = select(highestSocketHandle+1, NULL, &FDSendSet, NULL, pTV);

		if (0 != selectRes && SOCKET_ERROR != selectRes)
		{
			if (CtlSocket && FD_ISSET(CtlSocket->getSocket(), &FDRecvSet))
			{
				if (!PopFromCtlSocket(CtlSocket))
				{
					Terminated = true;
					return false;
				}
			}
			if (FD_ISSET(FSocket, &FDSendSet))
			{
				if (SOCKET_ERROR == IntSend(sendBuf, sendBufSize, wasSent))
					return false;
				else
					break;
			}
		}
		else
		if (SOCKET_ERROR == selectRes)
		{
			return false;
		}
	}

	int i = 0;
	mcInt32 wasReadPart = 0;
	unsigned char *ptr = (unsigned char *)readBuf;
	memset(ptr, 1, readBufSize);
    
	while (true)
	{
		elapsed = GetTickCount() - started;
		if (elapsed >= Timeout)
			return false;

		FD_ZERO(&FDRecvSet);

		if (Timeout > 0)
			TimeoutV = Timeout - elapsed;
		else
			TimeoutV = 0;

		if (TimeoutV > 0)
		{
			TimeVal.tv_sec = TimeoutV / 1000;
			TimeVal.tv_usec = (TimeoutV % 1000) * 1000;
			pTV = &TimeVal;
		}
		else
			pTV = NULL;

		if (CtlSocket)
			FD_SET(CtlSocket->getSocket(), &FDRecvSet);
		FD_SET(FSocket, &FDRecvSet);

		selectRes = select(highestSocketHandle, &FDRecvSet, NULL, NULL, pTV);
	    
		if (0 != selectRes && SOCKET_ERROR != selectRes)
		{
			if (CtlSocket && FD_ISSET(CtlSocket->getSocket(), &FDRecvSet))
			{
				if (!PopFromCtlSocket(CtlSocket))
				{
					Terminated = true;
					return false;
				}
			}
			if (FD_ISSET(FSocket, &FDRecvSet))
			{
				if (SOCKET_ERROR == IntReceive(ptr, readBufSize, wasReadPart))
					return false;
				else
				{
					wasRead += wasReadPart;
					ptr += wasReadPart;
					readBufSize -= wasReadPart;
						
					if (needDoubleCRLF)
					{
						for (i = 0; i <= wasRead - 4; i++)
						{
							if (memcmp(CRLFCRLF, ptr + i, 4) == 0)
								return true;
						}
					}
					else
						return true;
          
				}
			}
		}
		else
		if (SOCKET_ERROR == selectRes)
		{
			return false;
		}
	}
}


//  Web tunneling support
mcInt32 MCStdSocket::HTTPConnect(MCSocket* CtlSocket, mcInt32 Timeout)
{
    bool res = false;
    mcInt32 len = 1025;
    char* buf = (char*)MCMemAlloc(len);

    char* rAddr = strdup(FRemoteAddress);
    mcUInt32 rPort = FRemotePort;
    setRemoteAddress(FWebTunnelAddress);
    setRemotePort(FWebTunnelPort);
    AsyncConnect(CtlSocket, Timeout);
    
    TRY_BLOCK
    {
        if (FState == issConnected)
        {
            if (wtaRequiresAuthentication == FWebTunnelAuthentication && 
                FWebTunnelUserId != NULL && FWebTunnelPassword != NULL)
            {
                char BasicCredentials[512];
                sprintf(BasicCredentials, "%s:%s", FWebTunnelUserId, FWebTunnelPassword);
                char *pBasicCredentialsEnc = 
                    MCBase64Codec::Encode(BasicCredentials, strlen(BasicCredentials));

                sprintf(buf, "CONNECT %s:%d HTTP/1.0\r\nHost: %s:%d\r\nUser-agent: MsgConnect\r\nProxy-Authorization: Basic %s\r\n\r\n", 
                    rAddr, rPort, rAddr, rPort, pBasicCredentialsEnc);

                free(pBasicCredentialsEnc);
            }
            else
            {
                sprintf(buf, "CONNECT %s:%d HTTP/1.0\r\nHost: %s:%d\r\nUser-agent: MsgConnect\r\n\r\n", 
                    rAddr, rPort, rAddr, rPort);
            }

            mcInt32 psize = strlen(buf);
            mcInt32 cnt = 0, selectRes = 0, rcvcnt = 0;
            bool Term = false;
            if (!SocksSendReceive(Term, CtlSocket, Timeout, buf, psize, cnt, buf, 1024, rcvcnt, true))
                THROW_ERROR(0);
            if (Term)
				THROW_ERROR(-1);
			if (rcvcnt < 12)
				THROW_ERROR(MCError_WebTunnelFailure);

            int httpMajor = 0, httpMinor = 0, httpCode = 0;
			char headerBuf[13];
			memmove(headerBuf, buf, 12);
            headerBuf[12] = 0;

            sscanf(headerBuf, "HTTP/%d.%d %d", &httpMajor, &httpMinor, &httpCode);

            if (1 == httpMajor && (!httpMinor || 1 == httpMinor))
            {
                switch (httpCode)
                {
                case 200:
                    res = true;
                    break;
                case 407:
                    THROW_ERROR(MCError_WebTunnelAuthRequired);
                    break;
                default:
                    THROW_ERROR(MCError_WebTunnelFailure);
                    break;
                }
            }
            else
                THROW_ERROR(MCError_WebTunnelFailure);
    
			if (res)
			{
				buf[rcvcnt] = 0;
				char* tail = strstr(buf, "\r\n\r\n");
				if (tail)
				{
					cnt = tail - buf;
					ReturnData(&(tail[4]), rcvcnt - (cnt + 4));
				}				
			}
        }
        else
            THROW_ERROR(MCError_WebTunnelConnectionFailed);
    }
    FINALLY
    (
        if (len > 0)
            MCMemFree(buf);
        if (!res && FState == issConnected)
            Close(true);
        if (NULL != rAddr)
            free(rAddr);
    )
    return res;
}

mcInt32 MCStdSocket::StartAsyncConnect(void)
{
    mcUInt32 addr;
    struct hostent* HostEnt;
	mcInt32 r = MCSocket::StartAsyncConnect();

    if((FSocket == (mcInt32)INVALID_SOCKET) && (Init(istStream) != 0))
        THROW_ERROR(MCError_NotASocket);
    
    if (FState != issInitialized) 
        THROW_ERROR(MCError_WrongSocketState);

    if (issInitialized == FState)
    {
        FAddr.sin_addr.s_addr = INADDR_NONE;
        addr = inet_addr(getRemoteAddress());
	    
		if(addr == INADDR_NONE)
	    {
		    HostEnt = gethostbyname(getRemoteAddress());
		    if(HostEnt)
			    memmove(&addr, &HostEnt->h_addr[0], sizeof(addr));
	    }
  
	    if(addr == INADDR_NONE)
            THROW_ERROR(MCError_InvalidAddress);

	    memset(&FAddr, 0, sizeof(FAddr));
	    FAddr.sin_family = AF_INET;
	    FAddr.sin_addr.s_addr = addr;
	    FAddr.sin_port = htons(getRemotePort());
        u_long arg = 1;
#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
	    ioctlsocket(FSocket, FIONBIO, &arg); //set non-blocking IO
#else
	    ioctl(FSocket, FIONBIO, &arg); //set non-blocking IO
#endif
		r = connect(FSocket, (sockaddr*)&FAddr, sizeof(FAddr));
	
	    if (0 != r)
        {
            int errorCode = LAST_ERROR;
#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
            if ((errorCode != WSAEWOULDBLOCK) && /*(errorCode != WSAEINVAL) && */(errorCode != WSAEISCONN) && (errorCode != WSAEINPROGRESS))
#else
	        if (errorCode != EINPROGRESS)
#endif
			{
                //DWORD errorCode = LAST_ERROR;
                FState = issInitialized;
                return errorCode;
            }
            else
#if !defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
            if (errorCode == WSAEISCONN)
			{
                FState = issConnected;
				//OutputDebugString("Outgoing socket connected\n");
			}
            else
#endif
				FState = issConnecting;
		
        }
        else
            FState = issConnected;

		if (FState == issConnected)
			FinishAsyncConnect();
    }
    else
        THROW_ERROR(MCError_WrongSocketState);

    return r;
}

mcInt32 MCStdSocket::SyncConnect(void)
{
	struct sockaddr_in FAddr;
	mcUInt32 addr;
	struct hostent* HostEnt;
	mcInt32 r;
	int i;
	struct in_addr TAddr;

	MCSocket::Connect();
	if((FSocket == (mcInt32)INVALID_SOCKET) && (Init(istStream) != 0))
        THROW_ERROR(MCError_NotASocket);
	  
	if(FState != issInitialized)
        THROW_ERROR(MCError_WrongSocketState);
	  
	addr = inet_addr(getRemoteAddress());
  
	if(addr == INADDR_NONE)
	{
		HostEnt = gethostbyname(getRemoteAddress());
		if(HostEnt)
			memmove(&addr, &HostEnt->h_addr[0], sizeof(addr));
	}
  
	if(addr == INADDR_NONE)
        THROW_ERROR(MCError_InvalidAddress);
	  
	memset(&FAddr, 0, sizeof(FAddr));
	FAddr.sin_family = AF_INET;
	FAddr.sin_addr.s_addr = addr;
	FAddr.sin_port = htons(getRemotePort());
	r = connect(FSocket, (sockaddr*)&FAddr, sizeof(FAddr));
  
	if(r == 0)
	{
		//OutputDebugString("Outgoing socket connected\n");
		if(!getLocalAddress() || (getLocalPort() == 0))
		{
			memset(&FAddr, 0, sizeof(FAddr));
			FAddr.sin_family = AF_INET;
			i = sizeof(FAddr);
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
			if(getsockname(FSocket, (sockaddr*)&FAddr, (socklen_t*)&i) == 0)
#else
#endif
			{
				TAddr.s_addr = FAddr.sin_addr.s_addr;
				FLocalPort = ntohs(FAddr.sin_port);
				setLocalAddress(inet_ntoa(TAddr));
			}
		}
		FState = issConnected;
	}
	else
        r = LAST_ERROR;
	return r;
}

/*
//*nix code
mcInt32 MCStdSocket::AsyncConnect(void)
{
    mcUInt32 addr;
    struct hostent* HostEnt;
    mcInt32 r;
    int i;
    struct in_addr TAddr;
    if((FSocket == (mcInt32)INVALID_SOCKET) && (Init(istStream) != 0))
	    THROW_ERROR(MCError_NotASocket);
    if (issInitialized != FState)
        THROW_ERROR(MCError_WrongSocketState);

    MCSocket::AsyncConnect();
    addr = inet_addr(getRemoteAddress());
    if (INADDR_NONE == addr)
    {
        HostEnt = gethostbyname(getRemoteAddress());
        if (NULL != HostEnt)
    	    memmove(&addr, &HostEnt->h_addr[0], sizeof(addr));
    }
	    
    if(addr == INADDR_NONE)
        THROW_ERROR(MCError_InvalidAddress);

    memset(&FAddr, 0, sizeof(FAddr));
    FAddr.sin_family = AF_INET;
    FAddr.sin_addr.s_addr = addr;
    FAddr.sin_port = htons(getRemotePort());
    u_long arg = 1;
    ioctl(FSocket, FIONBIO, &arg);
    r = connect(FSocket, (sockaddr*)&FAddr, sizeof(FAddr));
	
    if (0 != r)
    {
        int errorCode = LAST_ERROR;
        if (errorCode != EINPROGRESS)
        {
            FState = issInitialized;
            return errorCode;
        }
		else
			FState = issConnecting;
    }
    else
        FState = issConnected;
    
    
    return 0;
}
*/

mcInt32 MCStdSocket::ReusePort()
{
	mcInt32 r;
	int reuseAddrVal = 1; //int reuseAddrLen = sizeof(reuseAddrVal);
    r = setsockopt(FSocket, SOL_SOCKET, SO_REUSEADDR, (const char *) &reuseAddrVal, sizeof(reuseAddrVal));
    
	if(r == INVALID_SOCKET)
        r = LAST_ERROR;
	else
		r = 0;
	return r;
}

mcInt32 MCStdSocket::Init(MCSocketType SocketType, bool AllocateSocket)
{
	mcInt32 r ;
	
	if(FState != issNotASocket)
	    THROW_ERROR(MCError_WrongSocketState);
	FState = issInitializing;

	if (AllocateSocket)
	{
		FSocket = socket(PF_INET, SocketTypes[SocketType], IPPROTO_IP);
		
		if(FSocket == (mcInt32)INVALID_SOCKET)
			r = LAST_ERROR;
		else
			r = 0;
		
		if(FCloseRequest || r != 0)
		{
			FCloseRequest = false;
			FState = issNotASocket;
			if(r == 0)
				closesocket(FSocket);
			FSocket = INVALID_SOCKET;
			return r;
		}
	}
	else
		r = 0;

	if(r == 0)
		FState = issInitialized;
    
 	return r;
}

mcInt32 MCStdSocket::Listen(mcInt32 BackLog)
{
	mcInt32 r;
	
    //set REUSE_ADDRESS option
    MCSocket::Listen(BackLog);
	if(FSocket == (mcInt32)INVALID_SOCKET)
        THROW_ERROR(MCError_NotASocket);
	if(FState != issBound)
	{
		if(FState != issInitialized)
            THROW_ERROR(MCError_WrongSocketState);
        r = Bind();
		if(r != 0)
			return r;
	}
	if(BackLog > SOMAXCONN)
		BackLog = SOMAXCONN;
	r = listen(FSocket, BackLog);
	if(r == 0)
		FState = issListening;
	else
        r = LAST_ERROR;
	return r;
}

mcInt32 MCStdSocket::Receive(void* Data, mcInt32 DataLen, mcInt32& Received)
{
	return IntReceive(Data, DataLen, Received);
}

mcInt32 MCStdSocket::IntReceive(void* Data, mcInt32 DataLen, mcInt32& Received)
{
    mcInt32 r = 0;
	if(FState != issConnected)
        THROW_ERROR(MCError_WrongSocketState);
	
	Received = 0;

	if (NULL == Data || 0 == DataLen)
		return 0;

	if (0 != FIncomingBuf.Len)
    {//there is some data in cache
        r = 0;
        Received = DataLen < FIncomingBuf.Len ? DataLen : FIncomingBuf.Len;
        memmove(Data, FIncomingBuf.Data, Received);
        if (Received != FIncomingBuf.Len)
        {
            memmove(FIncomingBuf.Data, (char*)FIncomingBuf.Data + Received, FIncomingBuf.Len - Received);
            FIncomingBuf.Len -= Received;
        }
        else
        {
            FIncomingBuf.Len = 0;
        }
     
    }
    else
    {
        r = recv(FSocket, (char*)Data, DataLen, 0);
	    if(r == SOCKET_ERROR)
	    {
		    Received = 0;
            r = LAST_ERROR;
			/*char Buffer [150];
			if (FDirection == isdOutgoing)
				sprintf(Buffer, "Receive on outgoing socket failed with error %d\n", r);
			else
				sprintf(Buffer, "Receive on incoming socket failed with error %d\n", r);
			OutputDebugString(Buffer);
			*/
	    }
	    else
	    {
		    Received = r;
		    r = 0;
	    }
    }
	return r;
}

mcInt32 MCStdSocket::ReceiveFrom(void* Data, mcInt32 DataLen, mcInt32& Received,
	char*& RemoteAddress, mcUInt32& RemotePort)
{
	struct sockaddr_in FAddr;
	mcInt32 r;
	int i;

	if(FState == issConnected)
	{
		r = Receive(Data, DataLen, Received);
		RemoteAddress = getRemoteAddress();
		RemotePort = getRemotePort();
	}
	else
	if(FState != issBound)
        THROW_ERROR(MCError_WrongSocketState);
	else
	{
		memset(&FAddr, 0, sizeof(FAddr));
		FAddr.sin_family = AF_INET;
		i = sizeof(FAddr);
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
		r = recvfrom(FSocket, (char*)Data, DataLen, 0, (sockaddr*)&FAddr, (socklen_t*)&i);
#else
#endif
		if(r == SOCKET_ERROR)
		{
			Received = 0;
            r = LAST_ERROR;
		}
		else
		{
			RemotePort = ntohs(FAddr.sin_port);
			RemoteAddress = inet_ntoa(FAddr.sin_addr);
			Received = r;
			r = 0;
		}
	}
	return r;
}

mcInt32 MCStdSocket::Send(void* Data, mcInt32 DataLen, mcInt32& Sent)
{
	return IntSend(Data, DataLen, Sent);
}

mcInt32 MCStdSocket::IntSend(void* Data, mcInt32 DataLen, mcInt32& Sent)
{
	mcInt32 r;

	if(FState != issConnected)
        THROW_ERROR(MCError_WrongSocketState);
	r = send(FSocket, (char*)Data, DataLen, 0);
	if(r == SOCKET_ERROR)
	{
		Sent = 0;
        r = LAST_ERROR;
		/*
		char Buffer [150];
		if (FDirection == isdOutgoing)
			sprintf(Buffer, "Send on outgoing socket failed with error %d\n", r);
		else
			sprintf(Buffer, "Send on incoming socket failed with error %d\n", r);
		OutputDebugString(Buffer);
		*/
	}
	else
	{
		Sent = r;
		r = 0;
	}
	return r;
}

mcInt32 MCStdSocket::SendTo(void* Data, mcInt32 DataLen, mcInt32& Sent,
	char* RemoteAddress, mcUInt32 RemotePort)
{
	struct sockaddr_in FAddr;
	mcUInt32 addr;
	struct hostent* HostEnt;
	mcInt32 r;
	int i;

	if(FState == issConnected)
		r = Send(Data, DataLen, Sent);
	else
  //if not (State in [issInitialized, issBound]) then
  //  raise EMCSockError.CreateCode(MCError_WrongSocketState)
  //else
	{
		// translate destination address
		if(!RemoteAddress || (strcmp(RemoteAddress, "255.255.255.255") != 0))
		{
			addr = inet_addr(RemoteAddress);
			if(addr == INADDR_NONE)
			{
				HostEnt = gethostbyname(RemoteAddress);
				if(HostEnt)
					memmove(&addr, &HostEnt->h_addr[0], sizeof(addr));
			}
			if(addr == INADDR_NONE)
                THROW_ERROR(MCError_InvalidAddress);
		}
		else
			addr = INADDR_NONE;
  
		memset(&FAddr, 0, sizeof(FAddr));
		FAddr.sin_family = AF_INET;
		FAddr.sin_addr.s_addr = addr;
		FAddr.sin_port = htons(RemotePort);
		// do send
		r = sendto(FSocket, (char*)Data, DataLen, 0, (sockaddr*)&FAddr, sizeof(FAddr));
  
		if(r == SOCKET_ERROR)
		{
			Sent = 0;
            r = LAST_ERROR;
		}
		else
		{
			if(FState == issInitialized)
			{
				memset(&FAddr, 0, sizeof(FAddr));
				FAddr.sin_family = AF_INET;
				i = sizeof(FAddr);
#if defined(__GNUC__) && !(defined(__MINGW32__) || defined(__MINGW64__))
				if(getsockname(FSocket, (sockaddr*)&FAddr, (socklen_t*)&i) == 0)
#else
#endif
				{
					setLocalPort(ntohs(FAddr.sin_port));
					setLocalAddress(inet_ntoa(FAddr.sin_addr));
					FState = issBound;
				}
			}
			Sent = r;
			r = 0;
		}
	}
	return r;
}

void MCStdSocket::ReturnData(void* Data, mcInt32 DataLen)
{
    if (NULL == Data || 0 == DataLen)
		return;

	if (NULL != FIncomingBuf.Data)
    {
        //look for remaining space
        mcInt32 remaining = FIncomingBuf.Size - FIncomingBuf.Len;
        if (remaining < DataLen)
        {//expand buffer
            void* newBuf = realloc(FIncomingBuf.Data, FIncomingBuf.Size + DataLen - remaining);
            if (NULL == newBuf)
                THROW_ERROR(MCError_NotEnoughMemory);
            FIncomingBuf.Data = newBuf;
            FIncomingBuf.Size += DataLen - remaining;
        }
        
        //move the remaining data to the end of buffer
        memmove((char*)FIncomingBuf.Data+DataLen, FIncomingBuf.Data, FIncomingBuf.Len);
        memmove(FIncomingBuf.Data, Data, DataLen);
        FIncomingBuf.Len += DataLen;    
    }
    else
        THROW_ERROR(MCError_WrongSocketState);
}

mcInt32 MCStdSocket::AddToMulticastSrv(const char* GroupAddress, const char* BindAddress)
{
	if (FState != issBound)
	    THROW_ERROR(MCError_WrongSocketState);

	mcInt32 r;
	struct ip_mreq imreq;

	//memset(&imreq, 0, sizeof(struct ip_mreq));
	
	imreq.imr_multiaddr.s_addr = inet_addr(GroupAddress);
	imreq.imr_interface.s_addr = inet_addr(BindAddress);

	r = setsockopt(FSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, 
		(/*const */char *) &imreq, sizeof(/*struct */ip_mreq));

	if (r) 
		return LAST_ERROR;
	else
		return 0;
}

mcInt32 MCStdSocket::AddToMulticastCli(const char* BindAddress, unsigned char TTL, bool Loop)
{
	mcInt32 r;
	struct in_addr iaddr;
	
	memset(&iaddr, 0, sizeof(struct in_addr));
	
	iaddr.s_addr = inet_addr(BindAddress);

	r = setsockopt(FSocket, IPPROTO_IP, IP_MULTICAST_IF, (const char *) &iaddr, sizeof(struct in_addr));
	if (r) 
		return LAST_ERROR;

	r = setsockopt(FSocket, IPPROTO_IP, IP_MULTICAST_TTL, (const char *) &TTL, sizeof(unsigned char));
	if (r) 
		return LAST_ERROR;

	setsockopt(FSocket, IPPROTO_IP, IP_MULTICAST_LOOP, (const char *) &Loop, sizeof(unsigned char));
	if (r) 
		return LAST_ERROR;

	return 0;
}


// **************************** MCSocketFactory ****************************

MCSocketFactory::MCSocketFactory()
{
	FOnSocketCreation = NULL;
	FOnSocketCreationData = NULL;
}

MCSocket* MCSocketFactory::CreateClientSocket()
{
	MCSocket* result = DoCreateClientSocket();
	if (FOnSocketCreation != NULL)
		(FOnSocketCreation)(FOnSocketCreationData, this, result, issClient);
	return result;
}

MCSocket* MCSocketFactory::CreateServerSocket()
{
	MCSocket* result = DoCreateServerSocket();
	if (FOnSocketCreation != NULL)
		(FOnSocketCreation)(FOnSocketCreationData, this, result, issServer);
	return result;
}

MCSocketCreationEvent MCSocketFactory::getOnSocketCreation(void**UserData)
{
	if (UserData)
		*UserData = FOnSocketCreationData;
	return FOnSocketCreation;
}

void MCSocketFactory::setOnSocketCreation(MCSocketCreationEvent v, void* UserData)
{
	FOnSocketCreationData = UserData;
	FOnSocketCreation = v;
}

#ifdef MC_NO_PURE_VIRTUAL
MCSocket* MCSocketFactory::DoCreateClientSocket()
{
	MC_pure_virtual();
	return NULL;
}

MCSocket* MCSocketFactory::DoCreateServerSocket()
{
	MC_pure_virtual();
	return NULL;
}
#endif

// *************************** MCStdSocketFactory **************************
MCSocket* MCStdSocketFactory::DoCreateClientSocket()
{
	return new MCStdSocket();
}

MCSocket* MCStdSocketFactory::DoCreateServerSocket()
{
	return new MCStdSocket();
}

// ****************************** MCSocketJob ******************************

MCSocketJob::MCSocketJob(void)
:FSocket(NULL), FKickSocket(NULL), FNotifyStop(false), FNotifyMsg(false), FNoUDPNotify(false)
{
}

MCSocketJob::~MCSocketJob(void)
{
    delete FSocket;
    delete FKickSocket;
}

MCSocket* MCSocketJob::getSocket(void)
{
	return FSocket;
}

MCSocket* MCSocketJob::CreateSocket(void)
{
	return new MCStdSocket();
}

/*
SocketClass parameter specifies the class of socket property value.
*/
void MCSocketJob::Initialize(void)
{
	FSocket = CreateSocket();
	FSocket->setLocalPort(0);
	FSocket->setLocalAddress("127.0.0.1");//"0.0.0.0"; // changed by EM on 09.09.2007
	WinsockErrorCheck(FSocket->Init(istDatagram));
	WinsockErrorCheck(FSocket->Bind());
	FKickSocket = new MCStdSocket();
	WinsockErrorCheck(FKickSocket->Init(istDatagram));
}

void MCSocketJob::setNoUDPNotify(bool Value)
{
	FNoUDPNotify = Value;
}

bool MCSocketJob::getNoUDPNotify(void)
{
	return FNoUDPNotify;
}

#ifdef MC_NO_PURE_VIRTUAL
void MCSocketJob::Run(void)
{
	MC_pure_virtual();
}
#endif

#ifdef USE_NAMESPACE
}
#endif

