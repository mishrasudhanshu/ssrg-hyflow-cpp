#include "MC.h"
#include "MCBase.h"
#include "MCSock.h"
#include "MCSocket.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

using namespace MsgConnect;

void __stdcall event1(void* resvd, void* Sender, MCMessage& Message, bool& Handled)
{
	printf("1:main::got event: ");
	if(Message.Data && (Message.DataSize > 0))
	{
		char*s = (char*)MCMemAlloc(Message.DataSize + 1);
		memmove(s, Message.Data, Message.DataSize);
		s[Message.DataSize] = 0;
		printf("1:%s\n", s);
		s[0]='B';
		Message.Data = s;
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
	printf("1C:Message was sent.\n");
}

void __stdcall callback2(unsigned int UserData, MCMessage& Message)
{
	printf("2:Message was sent.\n");
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

	Message.MsgCode = 1;
	Message.Param1 = 0;
	Message.Param2 = 0;
	Message.DataType = bdtVar;
	char msg[] = "Test message to someone";
	Message.Data = (void*)msg;//"Test message";
	Message.DataSize = (size_t)strlen((const char*)msg);//"Test message");//(const char*)Message.Data);
	printf("Trying to send message...\n");
	
	char dest[] = "Socket:127.0.0.1:14583|SendNote1";
	mc1->SendMessageCallback(dest, &Message, &callback1, 0, NULL);

	printf("SendMessageCallback is finished.\n");
	for(long ii = 0;ii < 4;ii++)
	{
		printf("1:%d\n", ii);
		Sleep(1000);
		mc1->DispatchMessages();
	}
	printf("Quitting.\n");
	delete mc1;

	return 0;
}
