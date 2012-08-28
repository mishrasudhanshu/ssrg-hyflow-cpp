#include <cstdlib>
#include <string>
#include <cstdio>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include "MC.h"
#include "MCBase.h"
#include "MCSock.h"
#include "MCSocket.h"
#include "../../messages/types/ObjectAccessMsg.h"
#include "../../../benchMarks/tm/bank/BankAccount.h"
#include "../../messages/HyflowMessage.h"
#include <boost/serialization/export.hpp>

#include "MSCtest.h"

using namespace MsgConnect;


static void __stdcall event1(void* resvd, void* Sender, MCMessage& Message, bool& Handled)
{
	if(Message.Data && (Message.DataSize > 0))
	{
		std::cerr<<"Message should not come here\n";
	}
	Handled = true;
}

static void __stdcall event2(void* resvd, void* Sender, MCMessage& Message, bool& Handled)
{
	printf("Got Event on queue1: \n");
	if(Message.Data && (Message.DataSize > 0))
	{
		std::string data((char*)Message.Data, Message.DataSize);
		std::istringstream dataStreamIn(data);
		boost::archive::text_iarchive ia(dataStreamIn);
		vt_dstm::HyflowMessage hyMsg;
		ia >> hyMsg;
		vt_dstm::ObjectAccessMsg *oaM =  (vt_dstm::ObjectAccessMsg *)hyMsg.getMsg();

		if (oaM->getId().compare("3-1") == 0 ) {
			std::cout<< "MSC Network object request serialization Test passed"<<std::endl;
		}else {
			std::cerr<< "MSC Network object request serialization Test FAILED!!!"<<std::endl;
		}

		vt_dstm::BankAccount *ba = new vt_dstm::BankAccount(1000,"3-1");
		oaM->setObject(ba);

		// Serialize the Message
		std::ostringstream dataStreamOut;
		boost::archive::text_oarchive oa(dataStreamOut);
		oa << hyMsg;
		std::string msg = dataStreamOut.str();

		char*s = (char*)MCMemAlloc(msg.size());
		memmove(s, msg.c_str(), msg.size());
		Message.Data = (void*)s;
		Message.DataSize = msg.size();
		Handled = true;
	}
}

static void __stdcall callback1(unsigned int UserData, MCMessage& Message)
{
	printf("Got callback on queue1: \n");
	if(Message.Data && (Message.DataSize > 0))
	{
		std::string data((char*)Message.Data, Message.DataSize);
		std::istringstream _dataStreamIn(data);
		boost::archive::text_iarchive ia(_dataStreamIn);
		vt_dstm::HyflowMessage hyMsg;
		ia >> hyMsg;
		vt_dstm::ObjectAccessMsg *oaM =  (vt_dstm::ObjectAccessMsg *)hyMsg.getMsg();

		vt_dstm::BankAccount *bankAccount = (vt_dstm::BankAccount *) oaM->getObject();
		if (bankAccount->checkBalance() == 1000 ) {
			std::cout<< "MSC Network object response serialization Test passed"<<std::endl;
		}else {
			std::cerr<< "MSC Network object response serialization Test FAILED!!!"<<std::endl;
		}
	}
}

namespace vt_dstm {

void MSCtest::test(){
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
	MCQueue* mq2 = new MCQueue();
	MCMessageHandlers* mhs2 = mq2->getHandlers();
	MCMessageHandler* mh2 = NULL;//new MCMessageHandler(mhs);
	MCSocketTransport* st2 = new MCSocketTransport();

	st2->setActive(false);
	st2->setAttemptsToConnect(1);
	st2->setFailOnInactive(true);
	st2->setMaxTimeout(900000l);
	st2->setMessengerAddress("127.0.0.1");
	st2->setMessengerPort(14584);
	st2->setTransportMode(stmP2P);
	st2->setMessenger(mc1);
	st2->setActive(true);

	mh2 = mhs2->Add();
	mh2->setMsgCodeLow(1);
	mh2->setMsgCodeHigh(1);
	mh2->setOnMessage(&event2);
	mh2->setEnabled(true);

	mq2->setQueueName("SendNote2");
	mq2->setMessenger(mc1);

	Message.MsgCode = 1;
	Message.Param1 = 0;
	Message.Param2 = 0;
	Message.DataType = bdtVar;

	vt_dstm::ObjectAccessMsg bq("3-1",true);
	vt_dstm::HyflowMessage objReq;
	objReq.msg_t = vt_dstm::MSG_ACCESS_OBJECT;
	objReq.setMsg(&bq);

	std::ostringstream net_stream;
	boost::archive::text_oarchive archive(net_stream);
	archive << objReq;
	std::string msg = net_stream.str();

	Message.DataSize = msg.size();
	Message.Data = (void*)msg.c_str();
	
	char dest2[] = "Socket:127.0.0.1:14584|SendNote2";

	mc1->SendMessageCallback(dest2, &Message, &callback1, 0, NULL);

	std::cout<<"Callback message is sent"<<std::endl;

	for(long ii = 0;ii < 4;ii++)
	{
		Sleep(1000);
		mc1->DispatchMessages();
	}
	printf("Quitting.\n");
	delete mc1;
}

}	/*namespace vt_dstm */
