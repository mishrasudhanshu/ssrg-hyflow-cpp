//====================================================
//                                                    
//  EldoS MsgConnect                                 
//                                                    
//   Copyright (c) 2001-2010, EldoS                   
//                                                    
//====================================================
#include "MC.h"
#include "MCUtils.h"
#include "MCHTTP.h"
#ifdef _WIN32
//#   include <windows.h>
//#   include <tchar.h>
#endif
#define USE_HTTP11
 
#ifdef USE_NAMESPACE
namespace MsgConnect
{
#endif

#ifndef max
inline mcUInt32 max(mcUInt32 a, mcUInt32 b)
{
  return a > b ? a : b;
}
#endif

// ***************************** MCHttpTransport ****************************

MCHttpTransport::MCHttpTransport()
{
	FDefaultTransportName = (char*)"HTTP";
    FUseProxy = false;
    FProxyAddress = NULL;
    FProxyPort = 3128;
    FReqTime = 1000;
    memset(&FProxyIP, 0, sizeof(FProxyIP));
}

MCHttpTransport::~MCHttpTransport()
{
	MCMemFree(FProxyAddress);
}


MCInetTransportJob* MCHttpTransport::CreateTransportJob(void)
{
	MCHttpTransportJob* trans = new MCHttpTransportJob(); //memory leak
	trans->AdjustBufferSize(FIncomingBufferSize, FOutgoingBufferSize);
	trans->AdjustSpeedLimits(FIncomingSpeedLimit, FOutgoingSpeedLimit);
	return trans;
}

MCInetConnectionEntry* MCHttpTransport::CreateConnectionEntry(void)
{
	MCHttpConnectionEntry* Ent = new MCHttpConnectionEntry();
    Ent->setUseProxy(this->getUseProxy());
    Ent->setProxyIP(this->FProxyIP);
    return Ent;
}

void MCHttpTransport::NotifyJob(MCInetTransportJob* Job, bool MessageFlag)
{
    MCHttpTransportJob* Thr = (MCHttpTransportJob*)Job;
    if (NULL != Thr)
    {
        if(!(Thr->getIsSending()))
	    {
	        KickEntrySocket(Thr->getEntry(), MessageFlag);
        }
    }
}

bool MCHttpTransport::NeedLiveConnectionForDelivery(void)
{
    return false; 
}

#ifndef _WIN32
#	define PROXY_ADDR FProxyIP.sin_addr.s_addr
#else
#	define PROXY_ADDR FProxyIP.sin_addr.S_un.S_addr
#endif

void MCHttpTransport::DoSetActive(void)
{
	if(!FMessenger)
		return;

    MCInetTransport::DoSetActive();
	if(getActive())
	{
        //configure proxy
        if (getUseProxy() == true)
        {
            memset(&FProxyIP, 0, sizeof(FProxyIP));
            PROXY_ADDR = inet_addr(FProxyAddress);
            if (INADDR_NONE == PROXY_ADDR)
            {
                hostent* host = gethostbyname(FProxyAddress);
                if (NULL != host)
                    PROXY_ADDR = inet_addr(host->h_addr_list[0]);
                else
                    RETURN_EXCEPT; //todo - return the MCError_InvalidAddress
            }
            FProxyIP.sin_family = AF_INET;
            FProxyIP.sin_port = htons(FProxyPort);
            /*
            for (mcInt32 i=0; i<FConnections->Length(); i++)
            {
                FConnections->Length();
                MCHttpConnectionEntry* CEntry = (MCHttpConnectionEntry*)((*FConnections)[i]);
                if (CEntry->getSocket()->getDirection() == isdOutgoing)
                    CEntry->setProxyIP(FProxyIP);
            }*/
        }
        else
        {
            memset(&FProxyIP, 0, sizeof(FProxyIP));
        }
	}
}

void MCHttpTransport::setProxyAddress(const char* ProxyAddr)
{
	if (FProxyAddress)
		free(FProxyAddress);
    FProxyAddress = _strdup(ProxyAddr);
}

const char* MCHttpTransport::getProxyAddress(void)
{
    return FProxyAddress;
}

void MCHttpTransport::setProxyPort(unsigned short ProxyPort)
{
    FProxyPort = ProxyPort;
}

unsigned short MCHttpTransport::getProxyPort(void)
{
    return FProxyPort;
}

void MCHttpTransport::setUseProxy(bool UseProxy)
{
    FUseProxy = UseProxy;
}

bool MCHttpTransport::getUseProxy(void)
{
    return FUseProxy;
}

void MCHttpTransport::setRequestTime(mcInt32 ReqTime)
{
    FReqTime = ReqTime;
}

mcInt32 MCHttpTransport::getRequestTime(void)
{
    return FReqTime;
}

// ************************** MCHttpTransportJob *************************

MCHttpTransportJob::MCHttpTransportJob(void)
{
    FAlreadyReceived = -1;
    FHeaderRead = 0;
    FReadingState = 0;
    FWasRead = 0;
    FChunked = false;
    
    FHeaderSize = 0;
    FHeaderSent = 0;
    FSendingStage = ssHeader;
	FServerRequestTime = 1000;

    memset(FIncomingBuffer, 0, FIncomingBufferSize);
}

MCHttpTransportJob::~MCHttpTransportJob(void)
{
}

bool MCHttpTransportJob::IsMessageRead(void)
{
    if (NULL != FEntry)
        return ((MCHttpConnectionEntry*)FEntry)->getMessageRead();
    else
        return false;
}

void MCHttpTransportJob::SetTransporterAddress(MCInetConnectionEntry* Ent, MCSocket* Transp)
{
    MCHttpConnectionEntry* FEntry = (MCHttpConnectionEntry*)Ent;

    if (FEntry->getUseProxy() == true)
	{
        Transp->setRemoteAddress(inet_ntoa(FEntry->getProxyIP().sin_addr));
        Transp->setRemotePort(ntohs(FEntry->getProxyIP().sin_port));
    }
	else
	{
        mcInt32 ClientID = 0; mcInt32 ConnID = 0; mcInt32 Port = 0; char* IP = NULL;
        if (ParseRemoteAddr(FEntry->getRemoteAddress(), &ClientID, &ConnID, &IP, &Port) == true)
        {
            Transp->setRemoteAddress(IP);
            Transp->setRemotePort(Port);
        }
        MCMemFree(IP); IP = NULL;
    }

}

bool MCHttpTransportJob::InitializeConnection()
{
	if (MCInetTransportJob::InitializeConnection())
	{
		FSendingStage = ssHeader;
		FLastRequestTime = GetTickCount();
		FServerRequestTime = 1000;
		return true;
	}
    else
		return false;
}

bool MCHttpTransportJob::PostConnectionStep(void)
{
    FAlreadyReceived = -1;
    FHeaderRead = 0;
    memset(FIncomingBuffer, 0, FIncomingBufferSize);
    return true;
}


/*
ReceiveData reads the data from the socket and adds them to the incoming 
packet (optionally creating this packet). 
ReceiveData returns true if the socket can be read or false if the socket 
was closed.
*/

#ifndef _WIN32
#include <ctype.h>
#ifndef QNX
void _strupr(char* s)
{
    for (mcInt32 i=0; i<strlen(s); i++)
	s[i] = toupper(s[i]);
}
#endif
#endif

bool MCHttpTransportJob::ParseHeaders(const char* Headers, mcUInt32 Len, char* URL, char* Host, mcInt32& HttpCode, bool& POST, mcInt32& ContentLength, bool& Chunked)
{
    const char *HeaderStart = Headers, *HeaderEnd = strstr(Headers, "\r\n");
    char HeaderName[128], HeaderValue[128], TempValue[128];
    URL[0] = 0; Host[0] = 0; HttpCode = 200; ContentLength = 0; POST = false;

    while ((NULL != HeaderEnd) && (HeaderEnd != (Headers + Len)))
    {
        const char* SpaceP = strchr(HeaderStart, ' ');
        if ((SpaceP > HeaderEnd) || (NULL == SpaceP))
            return false;
        if (SpaceP - HeaderStart > 126)
            return false;
        if (HeaderEnd - SpaceP > 126)
            return false;
        memmove(HeaderName, HeaderStart, SpaceP - HeaderStart);
        HeaderName[SpaceP - HeaderStart] = 0;
#if !defined(_WIN32_WCE) && defined(_WIN32)
		strupr(HeaderName);
#else
		_strupr(HeaderName);
#endif
        memmove(HeaderValue, SpaceP + 1, HeaderEnd - SpaceP - 1);
        HeaderValue[HeaderEnd - SpaceP - 1] = 0;
        TRY_BLOCK
        {
            if (strcmp(HeaderName, "POST") == 0)
            {//get the POST params
                char* SpaceP2 = strchr(HeaderValue, ' ');
                if (NULL != SpaceP2)
                    strncpy(URL, HeaderValue, SpaceP2-HeaderValue);
                else
                    URL[0] = 0;
                POST = true;
            } 
            else
            if (strcmp(HeaderName, "CONTENT-LENGTH:") == 0)
            {//get the MsgConnect length
                if (0 != HeaderValue[0])
                    ContentLength = atoi(HeaderValue);
                else
                    ContentLength = 0;
            }
            else
            if (strcmp(HeaderName, "TRANSFER-ENCODING:") == 0)
            {
                if (strlen(HeaderValue) >= 7)
                {
#if !defined(_WIN32_WCE) && defined(_WIN32)
		            strupr(HeaderValue);
#else
		            _strupr(HeaderValue);
#endif
                    Chunked = strncmp(HeaderValue, "CHUNKED", 7) == 0;
                }
            }
            else
            if (strcmp(HeaderName, "HOST:") == 0)
                strcpy(Host, HeaderValue);
            else
            if ((strcmp(HeaderName, "HTTP/1.1") == 0) ||
                (strcmp(HeaderName, "HTTP/1.0") == 0))
            {//get the HTTP code
                strncpy(TempValue, HeaderValue, 3);
                HttpCode = atoi(TempValue);
            }
			else
            if (strcmp(HeaderName, "X-REQUEST-TIME:") == 0)
            {//get the MsgConnect length
                if (0 != HeaderValue[0])
                    FServerRequestTime = atoi(HeaderValue);
                else
                    FServerRequestTime = 1000;
            }

            HeaderStart = HeaderEnd + 2;
            HeaderEnd = strstr(HeaderStart, "\r\n");
        }
        CATCH_EVERY
        {
            return false;
        }
    }
    return true;    
}

static char CRLFCRLF[] = "\r\n\r\n";
static char CRLF[] = "\r\n";

#ifndef USE_HTTP11
bool MCHttpTransportJob::ReceiveData(bool& tooBigPacket)
{
	mcInt32 Received;
	mcInt32 ToRecv;
	tooBigPacket = false;

    const char* TS1 = NULL;
    char Host[128], URL[128];
    mcInt32 ContentLength = 0, HttpLength = 0, HttpCode = 0;
    bool POST = false, Chunked = false;

    bool res = false;
    TRY_BLOCK
    {
		FEntry->getCS()->Enter();
        TRY_BLOCK
        {
			if(!getIsReceiving())
			{
				if (0 == FHeaderRead)
                    memset(FIncomingBuffer, 0, FIncomingBufferSize);
				
                if(FTransporter->Receive(&FIncomingBuffer[FHeaderRead], FIncomingBufferSize, Received) == 0)
                {
                    if (Received > 0)
				    {
                        TS1 = strstr((const char*)&FIncomingBuffer[0], CRLFCRLF);
                        if (NULL != TS1)
                        {//HTTP header is read
                            //return the unnecessary data
                            mcInt32 remains = FHeaderRead + Received - 4 - (TS1 - (char*)&FIncomingBuffer[0]);
                            if (0 != remains)
                                ((MCSocket*)FTransporter)->ReturnData((void*)(TS1+4), remains);
                            FHeaderRead += Received; FHeaderRead -= remains;

                            if (false == ParseHeaders((const char*)&FIncomingBuffer[0], TS1 - (char*)FIncomingBuffer + 2, URL, Host, HttpCode, POST,  ContentLength, Chunked))
                            {
                                res = false;
                                FEntry->setMsgSize(0);
                                memset(FIncomingBuffer, 0, FIncomingBufferSize);
                                RETURN_EXCEPT;
                            }
                            
                            
                            if ((false == POST) && (200 != HttpCode))
                            {//it is failed reply from HTTP server
                                res = false;
                                FAlreadyReceived = -1; FHeaderRead = 0;
                                memset(FIncomingBuffer, 0, FIncomingBufferSize);
                                RETURN_EXCEPT;
                            }

                            if (Chunked)
                            {
                                //now we do not support this
                                res = false;
                                FAlreadyReceived = -1; FHeaderRead = 0;
                                memset(FIncomingBuffer, 0, FIncomingBufferSize);
                                RETURN_EXCEPT;
                            }
                            else
                            {
                                FEntry->setMsgSize(ContentLength);
                                if (ContentLength > FOwner->getMaxMsgSize())
                                {
                                    FEntry->setMsgSize(0);
                                    tooBigPacket = true;
                                    //FTransporter->Close(false);
                                    res = false;
                                }
                            }
                            
                            if (false == tooBigPacket)
                            {
                                delete FEntry->getIncomingStream(); FEntry->setIncomingStream(NULL);
                                FEntry->setIncomingStream(new MCMemStream());
                                setIsReceiving(true);
                                FHeaderRead = 0;
                            }
                        } 
                        else
                            FHeaderRead += Received;
                        res = tooBigPacket ? false : true;
                    }
                    else
                        return false;
                }
                else
                {
                    //OutputDebugString("MCHTTPTransport::ReceiveData:: Socket error occured.\n");
                    FHeaderRead = 0;
                    res = false; 
                    FAlreadyReceived = -1;
                    return res;
                }
            }
            else
            {
                FAlreadyReceived = -1; FHeaderRead = 0;
                ToRecv = FEntry->getMsgSize() - FEntry->getIncomingStream()->Len();
                if (ToRecv > (mcInt32) FIncomingBufferSize)
                    ToRecv = (mcInt32) FIncomingBufferSize;

                if (FTransporter->Receive(FIncomingBuffer, ToRecv, Received) == 0)
                {
                    FEntry->getIncomingStream()->Write(FIncomingBuffer, Received);
                    res = true;
                }
            }
		} 
        CATCH_EVERY
        {}
		FEntry->getCS()->Leave();
    }
    CATCH_EVERY
    {}
	return res;
}

#else


const mcInt32 ReadingHeader = 0;
const mcInt32 ReadingChunkHeader = 1;
const mcInt32 ReadingChunkBody = 2;
const mcInt32 ReadingChunkLimiter = 3;
const mcInt32 ReadingBody = 4;
const mcInt32 ReadingEntityHeaders = 5;

inline bool isHEX(char x)
{
    if (x >= '0' && x <= '9')
        return true;
    if (x >= 'a' && x <= 'f')
        return true;
    if (x >= 'A' && x <= 'F')
        return true;
    return false;
}

inline mcInt32 HEX2Bits(char x)
{
    if (x >= '0' && x <= '9')
        return x - (mcInt32)'0';
    if (x >= 'a' && x <= 'f')
        return x - (mcInt32)'a' + 10;
    if (x >= 'A' && x <= 'F')
        return x - (mcInt32)'A' + 10;
    RETURN_EXCEPT;
    return 0;//for dummy compilers
}

static mcInt32 findChunkedSize(const char* s, mcInt32 maxSize)
{
    if (0 == maxSize)
        return 0;
    mcInt32 i = 0;
    char buf[32];
    while (isHEX(s[i]) && i < maxSize && i < 31)
        i++;
    mcInt32 len = i;
    strncpy(buf, s, len);
    mcInt32 res = 0;
    for (i=0; i<len; i++)
        res = (res << 4) + HEX2Bits(s[i]);
    return res;
}

bool MCHttpTransportJob::ReceiveData()
{
    mcInt32 HttpCode = 200, ContentLength = 0, HttpLength = 0, toRead = 0;
    mcInt32 Received = 0;
    bool POST = false, Chunked = false;
    char* TS1 = NULL;
    char URL[256], Host[256];
    bool res = true;
    bool isData = false;
    mcInt32 remains = 0;
    
    FEntry->getCS()->Enter();
    TRY_BLOCK
    {
        do 
        {
            isData = false;
            switch (FReadingState)
            {
            case ReadingHeader:
                if (FTransporter->Receive(&FIncomingBuffer[FWasRead], 512, Received) == 0)
                {
                    if (Received > 0)
			        {//we've got data

						FblInSessionTransferred += Received;
						FblInMsgTransferred += Received;
						FblInSecTransferred += Received;

                        FEntry->setMsgSize(0); setIsReceiving(true);
                        ((MCHttpConnectionEntry*)FEntry)->setMessageRead(false);
                        //do we received all header?
                        TS1 = strstr((char *) &FIncomingBuffer[0], CRLFCRLF);
                        if (NULL != TS1)
                        {//yes! HTTP header is here
                            
                            //return the unnecessary data to stream
                            remains = FWasRead + Received - 4 - 
                                (TS1 - (char*)&FIncomingBuffer[0]);
                            if (0 != remains)
                                ((MCSocket*)FTransporter)->ReturnData(TS1+4, remains);
                            FWasRead += Received; FWasRead -= remains;

                            //ok, ParseHeaders
                            if (false == ParseHeaders((const char*)&FIncomingBuffer[0], TS1 - (char*)FIncomingBuffer + 2, URL, Host, HttpCode, POST,  ContentLength, Chunked))
                                RETURN_EXCEPT;

                            if ((false == POST) && (200 != HttpCode))
                                RETURN_EXCEPT;
                            
                            FChunked = Chunked;
                            FWasRead = 0;
                            memset(FIncomingBuffer, 0, FIncomingBufferSize);

                            delete FEntry->getIncomingStream(); FEntry->setIncomingStream(NULL);
                            FEntry->setIncomingStream(new MCMemStream());
                            if (false == FChunked)
                            {
								if (ContentLength > FOwner->getMaxMsgSize() + 200)
                                {
                                    FEntry->setMsgSize(0);
                                    RETURN_EXCEPT;
                                }
                                FEntry->setMsgSize(ContentLength);
                                FReadingState = ReadingBody;
                            }
                            else
                            {
                                FReadingState = ReadingChunkHeader;
                                FReadMsgSize = 0;
                            }
                        }
                        else
                        {//HTTP header is not read now
                            FWasRead += Received;
                        }
                        memset(FIncomingBuffer+FWasRead, 0, FIncomingBufferSize-FWasRead);
                    }
                    else
                    {//connection was close by remote side
                        res = false;
                    }
                }
                else
                    res = false;

                break;

            case ReadingChunkHeader:
                //read chunk-size, chunk-ext (if any) and CRLF
                //maybe we already read this shit?
                if (FTransporter->Receive(&FIncomingBuffer[FWasRead], 512, Received) != 0)
                    res = false;
                else
                if (0 == Received)
                        res = false;

                if (res)
                {// There is fresh data :)

                    //is it full chunk header?
                    TS1 = strstr((char *)&FIncomingBuffer[0], CRLF);
                    
                    if (NULL != TS1)
                    {//yes! chunk header is here!
                        
                        //return the unnecessary data to stream
                        remains = FWasRead + Received - 2 - (TS1 - (char*)&FIncomingBuffer[0]);
                        if (0 != remains)
                            ((MCStdSocket*)FTransporter)->ReturnData(TS1+2, remains);

                        Received -= remains;

                        //handle chunk size
                        mcInt32 chunkSizeLen = (char*)TS1 - (char*)FIncomingBuffer;
                        FChunkSize = findChunkedSize((const char*)FIncomingBuffer, chunkSizeLen);
                        FChunkRead = 0;
                        FReadMsgSize += FChunkSize;
						if (FReadMsgSize > FOwner->getMaxMsgSize() + 200)
                        {
                            FEntry->setMsgSize(0);
                            RETURN_EXCEPT;
                        }                               

                        FWasRead = 0;
                        
                        if (0 != FChunkSize)
                            FReadingState = ReadingChunkBody;
                        else
                            FReadingState = ReadingEntityHeaders;
                        memset(FIncomingBuffer, 0, FIncomingBufferSize);
                    }
                    else
					{
                        FWasRead += Received;
					}
                }
                break;

            case ReadingChunkBody:
                if (FTransporter->Receive(FIncomingBuffer, FIncomingBufferSize, Received) != 0)
                    res = false;
                else
                if (0 == Received)
                    res = false;
                
                //read FChunkSize
                if (res)
                {
                    mcInt32 write2Stream = 0;
                    if (FChunkSize != FChunkRead)
                        write2Stream = Received > FChunkSize - FChunkRead ? FChunkSize - FChunkRead : Received;
                    if (0 != write2Stream)
                    {
                        FEntry->getIncomingStream()->Write(FIncomingBuffer, write2Stream);
                        FChunkRead += write2Stream;
                        if (FChunkRead == FChunkSize)
                        {
                            FReadingState = ReadingChunkLimiter;
                            FWasRead = 0;
                            FChunkRead = 0; FChunkSize = 0;
                        }
                    }
                    remains = Received - write2Stream;
                    if (0 != remains)
                        ((MCStdSocket*)FTransporter)->ReturnData(FIncomingBuffer+write2Stream, remains);
                    memset(FIncomingBuffer, 0, FIncomingBufferSize);
                }
                break;
            case ReadingChunkLimiter:
                if (FTransporter->Receive(&FIncomingBuffer[FWasRead], 512, Received) != 0)
                    res = false;
                else
                if (0 == Received)
                    res = false;
                if (Received >= 2)
                {
                    if (FIncomingBuffer[0] == 0xD && FIncomingBuffer[1] == 0xA)
                    {
                        FReadingState = ReadingChunkHeader; FWasRead = 0;
                        ((MCStdSocket*)FTransporter)->ReturnData(FIncomingBuffer+2, Received-2);
                    }
                    else
                        THROW_ERROR(MCError_InvalidHTTP);
                }
                else
                    FWasRead += Received;
                break;
                

            case ReadingEntityHeaders:
                if (FTransporter->Receive(&FIncomingBuffer[FWasRead], 512, Received) != 0)
                    res = false;
                else
                if (0 == Received)
                    res = false;

                if (res)
                {
                    TS1 = strstr((char*)&FIncomingBuffer[0], CRLFCRLF);
                    if (NULL != TS1)
                    {//entity headers are read. Just ignore them now
                        ((MCHttpConnectionEntry*)FEntry)->setMessageRead(true);
                        FEntry->setMsgSize(FReadMsgSize);
                        FReadingState = ReadingHeader;
                        remains = FWasRead + Received - 4 - (TS1 - (char*)&FIncomingBuffer[0]);
                        if (0 != remains)
                            ((MCStdSocket*)FTransporter)->ReturnData(TS1+4, remains);
                    }
                    else
                    {
                        //maybe we read empty line?
                        TS1 = strstr((char*)&FIncomingBuffer[0], CRLF);
                        if ((char*)TS1 == (char*)FIncomingBuffer)
                        {//entity headers are read. Just ignore them now
                            ((MCHttpConnectionEntry*)FEntry)->setMessageRead(true);
                            FEntry->setMsgSize(FReadMsgSize);
                            
                            FReadingState = ReadingHeader;
                            remains = Received - 2 - (TS1 - (char*)&FIncomingBuffer[0]);
                            if (0 != remains)
                                ((MCStdSocket*)FTransporter)->ReturnData(TS1+2, remains);
                            
                        }
                        else
                            FWasRead += Received;
                    }
                }
                break;

            case ReadingBody:
                toRead = FEntry->getMsgSize() - FEntry->getIncomingStream()->Pos();

                if(toRead > (mcInt32) FblToRecv)
					toRead = FblToRecv;

				if (FTransporter->Receive(FIncomingBuffer, toRead, Received) == 0)
                {
                    if (Received > 0)
                    {
						FblInSessionTransferred += Received;
						FblInMsgTransferred += Received;
						FblInSecTransferred += Received;
                
						FEntry->getIncomingStream()->Write(FIncomingBuffer, Received);
                        if (FEntry->getMsgSize() == FEntry->getIncomingStream()->Pos())
                        {//message is read
                            ((MCHttpConnectionEntry*)FEntry)->setMessageRead(true);
                            FReadingState = ReadingHeader;
                            FWasRead = 0;
                        }
                    }
                    else
                        res = false;
                }
                else
                    res = false;
                break;
            }
        }
        while(isData);
    }
    FINALLY
    (
        FEntry->getCS()->Leave();
    )
    return res;
}

#endif

void MCHttpTransportJob::UpdateConnectionContext(void)
{
    if (getEntry()->getInfo())
        ((MCHttpConnectionEntry*)getEntry())->setURL(getEntry()->getInfo()->URL);
}

const char    HTTPPostFormatString1[] = "POST %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: MsgConnect\r\n";
const char    HTTPPostFormatString2[] = "Content-Length: %d\r\nPragma: no-cache\r\nCache-Control: no-cache\r\nContent-Type: application/octet-stream\r\n\r\n";

mcInt32 MCHttpTransportJob::ClientBuildHTTPHeader(char* Buffer, mcInt32 MaxBufferLen, mcInt32 DataLen)
{
    mcInt32 l = 0;
    char URL[256], *IP = NULL;
    mcInt32 ClientID = 0; mcInt32 ConnID = 0; mcInt32 Port = 0;
    ParseRemoteAddr(FEntry->getRemoteAddress(), &ClientID, &ConnID, &IP, &Port);
    MCHttpConnectionEntry* httpEntry = (MCHttpConnectionEntry*)FEntry;
    if (httpEntry->getURL() != NULL)
        sprintf(URL, "http://%s:%d%s", IP, Port, httpEntry->getURL());
    else
        sprintf(URL, "http://%s:%d/", IP, Port);
    memset(Buffer, 0, MaxBufferLen); l = 0;
    sprintf(Buffer, HTTPPostFormatString1, URL, IP); l = strlen(Buffer);
    sprintf(Buffer+l, HTTPPostFormatString2, ((MCHttpConnectionEntry*)FEntry)->getContentLength()); l = strlen(Buffer);
    MCMemFree(IP);
    return l;
}

const char    HTTPReplyFormatString1[] = "HTTP/1.1 200 OK\r\n";
const char    HTTPReplyFormatString2[] = "Content-Type: application/octet-stream\r\n";
const char    HTTPReplyFormatString5[] = "Content-Length: %d\r\nX-Request-Time: %d\r\n\r\n";

mcInt32 MCHttpTransportJob::ServerBuildHTTPHeader(char* Buffer, mcInt32 MaxBufferLen, MCHttpConnectionEntry* HttpEnt)
{
    memset(Buffer, 0, MaxBufferLen); mcInt32 l = 0;
    sprintf(Buffer, HTTPReplyFormatString1); l = strlen(Buffer);
    sprintf(Buffer+l, HTTPReplyFormatString2); l = strlen(Buffer);
    sprintf(Buffer+l, HTTPReplyFormatString5, HttpEnt->getContentLength(), ((MCHttpTransport*)FOwner)->getRequestTime());
	l = strlen(Buffer);
    return l;
}

/*
Writes data to the socket. Returns true if the operation was successful and 
false if the socket returned an error.
*/

bool MCHttpTransportJob::SendData(void)
{
	mcInt32 ToSend, Sent;
	bool r = false;
    
    FEntry->getCS()->Enter();
	TRY_BLOCK
    {
		if(getIsSending())
		{
			
			TRY_BLOCK 
            {
                if (ssHeader == FSendingStage)
                {
                    if (0 == FHeaderSent)
                    {//build header
                        ((MCHttpConnectionEntry*)FEntry)->setContentLength(FEntry->getOutgoingStream()->Len());
                        MCMessageInfo* info = FEntry->getInfo();
						if (info->OldState == imsDispatching ||
                            info->OldState == imsEmptyRequest) //is it request?
                            FHeaderSize = ClientBuildHTTPHeader((char*)&FOutgoingBuffer[0], FOutgoingBufferSize, FEntry->getOutgoingStream()->Len());
                        else //else it is reply...
                            FHeaderSize = ServerBuildHTTPHeader((char*)&FOutgoingBuffer[0], FOutgoingBufferSize, ((MCHttpConnectionEntry*)FEntry));
                    }
                    if (FTransporter->Send(&FOutgoingBuffer[FHeaderSent], FHeaderSize - FHeaderSent, Sent) == 0)
                    {
						FblOutSessionTransferred += Sent;
						FblOutMsgTransferred += Sent;
						FblOutSecTransferred += Sent;

                        FHeaderSent += Sent;
                        if (FHeaderSent == FHeaderSize)
                        {//all header is sent
                            FHeaderSent = 0; FHeaderSize = 0;
                            FSendingStage = ssBody;
                        }
                        r = true;
                    }
                    else
                    {
                        //OutputDebugString("MCHTTPTransport::SendData:: Socket error occured.\n");
                        r = false;
                    }
                }
                else
                {//send message's body
                    if(FEntry->getOutgoingStream()->Len() - FEntry->getOutgoingStream()->Pos() < (mcInt32)FblToSend)
					    ToSend = FEntry->getOutgoingStream()->Len() - FEntry->getOutgoingStream()->Pos();
				    else
					    ToSend = FblToSend;
                    FEntry->getOutgoingStream()->Read(FOutgoingBuffer, ToSend);
					mcInt32 res = FTransporter->Send(FOutgoingBuffer, ToSend, Sent);
					if (res == 0)
				    {
						FblOutSessionTransferred += Sent;
						FblOutMsgTransferred += Sent;
						FblOutSecTransferred += Sent;

					    FEntry->getOutgoingStream()->SetPos(FEntry->getOutgoingStream()->Pos() - ToSend + Sent);
					    if (FEntry->getOutgoingStream()->Pos() == FEntry->getOutgoingStream()->Len())
                            FSendingStage = ssHeader;
                        r = true;
				    }
                    else
#ifdef _WIN32					
					if (res == WSAEWOULDBLOCK)
#else
					if (res == EWOULDBLOCK)
#endif					
						r = true;
					else
                        r = false;
                }
			} FINALLY (
				;
			)
		}
    }
    CATCH_EVERY
    {
    	r = false;
    }
    FEntry->getCS()->Leave();
	return r;
}

void MCHttpTransportJob::ChangeLastSendTime(MCMessageState state)
{
	if (state != imsEmptyRequest && state != imsEmptyReply)
	{
		FActivityTime = GetTickCount();
		FLastMsgSendTime = FActivityTime;
	}
	else
	{
		FActivityTime = max(FLastMsgRecvTime, FLastMsgSendTime);
	}
}

void MCHttpTransportJob::ChangeLastRecvTime(MCMessageState state)
{
	if (state != imsEmptyRequest && state != imsEmptyReply)
	{
		FActivityTime = GetTickCount();
		FLastMsgRecvTime = FActivityTime;
	}
	else
	{
		FActivityTime = max(FLastMsgRecvTime, FLastMsgSendTime);
	}
}

bool MCHttpTransportJob::IsRequestNeeded()
{
	mcUInt32 CurrentTime;
    mcInt32 LR2;
	mcInt32 LastRequest;
    
	if (FTransporter->getDirection() == isdIncoming /*|| getIsSending()*/)
		return false;

	CurrentTime = GetTickCount();
	if (CurrentTime > FLastRequestTime)
		LastRequest = CurrentTime - FLastRequestTime;
	else
		LastRequest = (0xFFFFFFFF - FLastRequestTime) + CurrentTime;

	if (CurrentTime > FLastMsgRecvTime)
		LR2 = CurrentTime - FLastMsgRecvTime;
	else
		LR2 = (0xFFFFFFFF - FLastMsgRecvTime) + CurrentTime;

	if (LR2 < LastRequest)
		LastRequest = LR2;

	if (LastRequest > FServerRequestTime)
	{
		FLastRequestTime = CurrentTime;
		return true;
	}
	else
		return false;
}

// ************************** MCHttpConnectionEntry *************************

MCHttpConnectionEntry::MCHttpConnectionEntry(void)
{
	FUseProxy = false;
    FURL = NULL;
    FMessageRead = false;
	memset(&FProxyIP, 0, sizeof(FProxyIP));
}

MCHttpConnectionEntry::~MCHttpConnectionEntry(void)
{
}

void MCHttpConnectionEntry::Reset(void)
{
    FCS->Enter();
    MCInetConnectionEntry::Reset();
    MCMemFree(FURL); FURL = NULL;
	FUseProxy = false;
	
	memset(&FProxyIP, 0, sizeof(FProxyIP));
    FMessageRead = false;
	FCS->Leave();
}

void MCHttpConnectionEntry::setUseProxy(bool Value)
{
	FUseProxy = Value;
}

bool MCHttpConnectionEntry::getUseProxy(void)
{
	return FUseProxy;
}

void MCHttpConnectionEntry::setProxyIP(sockaddr_in Value)
{
	FProxyIP = Value;
}

sockaddr_in MCHttpConnectionEntry::getProxyIP(void)
{
	return FProxyIP;
}

void MCHttpConnectionEntry::setContentLength(mcInt32 Value)
{
    FContentLength = Value;
}

mcInt32 MCHttpConnectionEntry::getContentLength(void)
{
    return FContentLength;
}

char* MCHttpConnectionEntry::getURL(void)
{
    return FURL;
}

void MCHttpConnectionEntry::setURL(char* URL)
{
    MCMemFree(FURL); FURL = NULL;
    if (URL)
        FURL = strCopy(URL, 0, strlen(URL));
}

bool MCHttpConnectionEntry::getMessageRead(void)
{
    return FMessageRead;
}

void MCHttpConnectionEntry::setMessageRead(bool Value)
{
    FMessageRead = Value;
}



//----------------------------------------------------------------------

#ifdef USE_NAMESPACE
}
#endif
