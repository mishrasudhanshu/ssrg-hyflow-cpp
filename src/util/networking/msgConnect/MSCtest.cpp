#include <cstdlib>
#include <string>
#include <cstdio>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/thread.hpp>
#include <boost/serialization/export.hpp>

#include "MC.h"
#include "MCBase.h"
#include "MCSock.h"
#include "MCSocket.h"
#include "../../messages/types/ObjectAccessMsg.h"
#include "../../../benchMarks/tm/bank/BankAccount.h"
#include "../../messages/HyflowMessage.h"
#include "../../messages/HyflowMessageFuture.h"
#include "../../logging/Logger.h"
#include "../NetworkManager.h"

#include "MSCtest.h"
#include "MSCNetwork.h"

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
	vt_dstm::Logger::debug("Got Event on queue2: \n");
	if(Message.Data && (Message.DataSize > 0))
	{
		std::string data((char*)Message.Data, Message.DataSize);
		std::istringstream iStream(data);
		boost::archive::text_iarchive ia(iStream);
		vt_dstm::HyflowMessage hyMsg;
		ia >> hyMsg;
		vt_dstm::ObjectAccessMsg *oaM =  (vt_dstm::ObjectAccessMsg *)hyMsg.getMsg();

		if (oaM->getId().compare("3-1") == 0 ) {
			std::cout<< "MSC Network object request serialization Test passed"<<std::endl;
		}else {
			std::cerr<< "MSC Network object request serialization Test FAILED!!!"<<std::endl;
		}

		vt_dstm::BankAccount ba(1000,"3-1");
		vt_dstm::ObjectAccessMsg oaMS(oaM->getId(),true);
		oaMS.setObject(&ba);
		vt_dstm::HyflowMessage hmsg;
		hmsg.setMsg(&oaMS);

		// Serialize the Message
		std::ostringstream ostream;
		boost::archive::text_oarchive oa(ostream);
		oa << hmsg;
		std::string msg = ostream.str();

		char *buffer = new char[msg.size()];
		std::memcpy(buffer, msg.c_str(), msg.size());
		Message.Data = (void*)buffer;
		Message.DataSize = msg.size();
	}
	Handled = true;
}

static void __stdcall callback1(unsigned int UserData, MCMessage& Message)
{
	vt_dstm::Logger::debug("Got callback on queue1: \n");
	if(Message.Data && (Message.DataSize > 0))
	{
		std::string data((char*)Message.Data, Message.DataSize);
		std::istringstream _dataStreamIn(data);
		boost::archive::text_iarchive ia(_dataStreamIn);
		vt_dstm::HyflowMessage hyMsg;
		ia >> hyMsg;
		vt_dstm::ObjectAccessMsg *oaM =  (vt_dstm::ObjectAccessMsg *)hyMsg.getMsg();

		vt_dstm::BankAccount *bankAccount = (vt_dstm::BankAccount *) oaM->getObject();
		if (bankAccount->getId().compare("3-1") == 0 ) {
			std::cout<< "MSC Network object response serialization Test passed"<<std::endl;
		}else {
			std::cerr<< "MSC Network object response serialization Test FAILED!!!"<<std::endl;
		}
	}
}



namespace vt_dstm {

volatile bool MSCtest::hyShutdown = false;

static void dispatcher(MCMessenger* mc){
	boost::posix_time::seconds workTime(0.001);
	vt_dstm::Logger::debug("Dispatcher Started\n");

	while (!MSCtest::hyShutdown) {
		mc->DispatchMessages();
		boost::this_thread::sleep(workTime);
	}
}

void MSCtest::testbase(){
	MCBaseInitialization();

	MCMessage Message;
	MCMessenger* mc = new MCMessenger();
	mc->setMaxTimeout(ULONG_MAX);

	// Set Node1
	MCQueue* mq1 = new MCQueue();
	MCMessageHandlers* mhs1 = mq1->getHandlers();
	MCMessageHandler* mh1 = NULL;//new MCMessageHandler(mhs);
	MCSocketTransport* st1 = new MCSocketTransport();

	st1->setActive(false);
	st1->setAttemptsToConnect(1);
	st1->setFailOnInactive(true);
	st1->setMaxTimeout(900000l);
	st1->setMessengerAddress("127.0.0.1");
	st1->setMessengerPort(14583);
	st1->setTransportMode(stmP2P);
	st1->setMessenger(mc);
	st1->setActive(true);

	mh1 = mhs1->Add();
	mh1->setMsgCodeLow(1);
	mh1->setMsgCodeHigh(1);
	mh1->setOnMessage(&event1);
	mh1->setEnabled(true);

	mq1->setQueueName("SendNote1");
	mq1->setMessenger(mc);

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
	st2->setMessenger(mc);
	st2->setActive(true);

	mh2 = mhs2->Add();
	mh2->setMsgCodeLow(1);
	mh2->setMsgCodeHigh(1);
	mh2->setOnMessage(&event2);
	mh2->setEnabled(true);

	mq2->setQueueName("SendNote2");
	mq2->setMessenger(mc);

	//Creating Messenger dispatcher thread
	boost::thread dp(dispatcher, mc);

	Message.MsgCode = 1;
	Message.Param1 = 0;
	Message.Param2 = 0;
	Message.DataType = bdtVar;

	vt_dstm::ObjectAccessMsg oamsg("3-1",true);
	vt_dstm::HyflowMessage hmsg;
	hmsg.msg_t = vt_dstm::MSG_ACCESS_OBJECT;
	hmsg.setMsg(&oamsg);

	std::ostringstream ostream;
	boost::archive::text_oarchive oa(ostream);
	oa << hmsg;
	std::string msg = ostream.str();

	Message.Data = (void*)msg.c_str();
	Message.DataSize = msg.size();
	
	char dest[] = "Socket:127.0.0.1:14584|SendNote2";

	mc->SendMessageCallback(dest, &Message, &callback1, 0, NULL);

	std::cout<<"Callback message is sent"<<std::endl;

	for(long ii = 0;ii < 5;ii++)
	{
		Sleep(1000);
	}
	hyShutdown = true;
	dp.join();
	printf("Quitting.\n");
	delete mc;
}

void MSCtest::test() {
	MCBaseInitialization();

	int myId = NetworkManager::getNodeId();

	printf("My Id is %d\n", myId);

	NetworkManager::NetworkInit();

	// Give some time for callback to be send
	for (int i=0; i <9 ; i++) {
		Sleep(1000);
	}

	std::cout<<"MultiNode communication Test Passed\n"<<std::endl;
	return;
}

}	/*namespace vt_dstm */
