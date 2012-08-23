#include <cstdlib>
#include <string>
#include <cstdio>

#include "MC.h"
#include "MCBase.h"
#include "MCSock.h"
#include "MCSocket.h"
//#include "../../messages/types/ObjectRequestMsg.h"
//#include "../../../benchMarks/tm/bank/BankAccount.h"
//#include "../../messages/types/ObjectResponseMsg.h"

using namespace MsgConnect;

void __stdcall event1(void* resvd, void* Sender, MCMessage& Message, bool& Handled)
{
	printf("1:main::got event: ");
	if(Message.Data && (Message.DataSize > 0))
	{
		char*s = (char*)MCMemAlloc(Message.DataSize+1);
		memmove(s, Message.Data, Message.DataSize);
		s[Message.DataSize] = 0;
		printf("1:%s\n", s);
		s[0]='B';
//		vt_dstm::HyMessageType t = ((vt_dstm::HyflowMessage)s).msg_t;
//		Message.Data = "Got Request of "; // + t;
	}
	Handled = true;
}

void __stdcall event2(void* resvd, void* Sender, MCMessage& Message, bool& Handled)
{
	printf("2:main::got event: ");
	if(Message.Data && (Message.DataSize > 0))
	{
		char*s = (char*)MCMemAlloc(Message.DataSize + 1);
		memmove(s, Message.Data, Message.DataSize);
		s[Message.DataSize] = 0;
		printf("2:%s\n", s);
	}
	Handled = true;
}

void __stdcall callback1(unsigned int UserData, MCMessage& Message)
{
	char*s = (char*)MCMemAlloc(Message.DataSize + 1);
	memmove(s, Message.Data, Message.DataSize);
	s[Message.DataSize] = 0;
	printf("1C:%s\n", s);
	printf("1C:Got Response.\n");
}

void __stdcall callback2(unsigned int UserData, MCMessage& Message)
{
	char*s = (char*)MCMemAlloc(Message.DataSize + 1);
	memmove(s, Message.Data, Message.DataSize);
	s[Message.DataSize] = 0;
	printf("2C:%s\n", s);
	printf("2C:Message was sent.\n");
}

int main(void)
{
	MCBaseInitialization();

	MCMessage Message;

	// Set Node1
	MCMessenger* mc1 = new MCMessenger();
	MCQueue* mq1 = new MCQueue();
	MCMessageHandlers* mhs1 = mq1->getHandlers();
	MCMessageHandler* mh1 = NULL;//new MCMessageHandler(mhs);
	MCSocketTransport* st1 = new MCSocketTransport();


	mc1->setMaxTimeout(ULONG_MAX);
	st1->setActive(false);
	st1->setAttemptsToConnect(1);
	st1->setFailOnInactive(true);
	st1->setMaxTimeout(900000l);
	st1->setMessengerAddress("127.0.0.1");
	st1->setMessengerPort(14583);
	st1->setTransportMode(stmP2P);
	st1->setMessenger(mc1);
	st1->setActive(true);

	mh1 = mhs1->Add();
	mh1->setMsgCodeLow(1);
	mh1->setMsgCodeHigh(1);
	mh1->setOnMessage(&event1);
	mh1->setEnabled(true);

	mq1->setQueueName("SendNote1");
	mq1->setMessenger(mc1);

	// Set Node2
	MCMessenger* mc2 = new MCMessenger();
	MCQueue* mq2 = new MCQueue();
	MCMessageHandlers* mhs2 = mq2->getHandlers();
	MCMessageHandler* mh2 = NULL;//new MCMessageHandler(mhs);
	MCSocketTransport* st2 = new MCSocketTransport();


	mc2->setMaxTimeout(ULONG_MAX);
	st2->setActive(false);
	st2->setAttemptsToConnect(1);
	st2->setFailOnInactive(true);
	st2->setMaxTimeout(900000l);
	st2->setMessengerAddress("127.0.0.1");
	st2->setMessengerPort(14584);
	st2->setTransportMode(stmP2P);
	st2->setMessenger(mc2);
	st2->setActive(true);

	mh2 = mhs2->Add();
	mh2->setMsgCodeLow(1);
	mh2->setMsgCodeHigh(1);
	mh2->setOnMessage(&event2);
	mh2->setEnabled(true);

	mq2->setQueueName("SendNote2");
	mq2->setMessenger(mc2);

	Message.MsgCode = 1;
	Message.Param1 = 0;
	Message.Param2 = 0;
	Message.DataType = bdtVar;

//	vt_dstm::ObjectRequestMsg req("0-0",true);
//	req.msg_t = vt_dstm::HyMessageType::MSG_OBJECT_RQ;

//	std::ostringstream net_stream;
//	boost::archive::text_oarchive archive(net_stream);
//	archive << req;
//	std::string msg = net_stream.str();

	std::string msg = "Hi";
	Message.Data = (void*)msg.c_str();
	Message.DataSize = msg.size();
	printf("Trying to send message...\n");
	
	char dest1[] = "Socket:127.0.0.1:14583|SendNote1";
	char dest2[] = "Socket:127.0.0.1:14584|SendNote2";

	mc1->SendMessageCallback(dest1, &Message, &callback1, 0, NULL);
//	mc2->SendMessageCallback(dest2, &Message, &callback2, 0, NULL);

//	mc1->SendMessageCallback(dest2, &Message, &callback1, 0, NULL);
//	mc2->SendMessageCallback(dest1, &Message, &callback2, 0, NULL);

	printf("SendMessageCallback is finished.\n");
	for(long ii = 0;ii < 4;ii++)
	{
		printf("1:%d\n", ii);
		Sleep(1000);
		mc1->DispatchMessages();
//		mc2->DispatchMessages();
	}
	printf("Quitting.\n");
	delete mc1;
	delete mc2;
	return 0;
}
