#ifndef SRC_NETWORK_NODENETWORKSYSTEM_H_
#define SRC_NETWORK_NODENETWORKSYSTEM_H_

#include <Network/Telosb/serialsource.h>
#include <Network/OffloadingManager.h>
#include <Network/Tcp/ServerClientInterface.h>
#include <NodeProcessingSystem.h>
#include <gpio/BlackGPIO.h>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include <stdint.h>
#include <string>
#include <set>

class NodeProcessingSystem;
class TelosbRadioSystem;
class WiFiRadioSystem;
class IncomingMessageQueue;
class Message;
class Connection;
class Session;
class Server;
class Client;
class NodeInfoMsg;

class NodeNetworkSystem: public ServerClientInterface {

public:

	static NodeNetworkSystem* getInstance(NodeProcessingSystem*,
			boost::asio::io_service& ioService,
			const std::vector<std::string> remoteIps,
			const std::vector<uint16_t> remotePorts,
			const std::string telosDevPath = "",
			BlackLib::BlackGPIO** gpios = NULL);

	static void shutdown(const int signal);

	static void stopOnSendEnd();

	static void messageSent();

	void flushOutgoingMessages();

	void sendMessage(Message*);

	float prepareWifiBandwidthControl(const string sinkIpAddr_, const ushort sinkPort_);

	float setWifiBandwidth(const ushort bw_);

	/* ServerInterface methods */
	void serverMessageHandler(Session* session, Message* msg);

	void serverAddSessionHandler(Session* session);

	void serverRemoveSessionHandler(Session* session);

	/* ClientInterface methods */
	void clientMessageHandler(Session* session, Message* msg);

	void clientConnectHandler(Session* session);

	void telosMessageHandler(Message* msg);

private:

	const static unsigned char _DEBUG = 1;

	NodeNetworkSystem(NodeProcessingSystem* nodeProcessingSystem,
			boost::asio::io_service& ioService,
			const std::vector<std::string> remoteIps,
			const std::vector<uint16_t> remotePorts,
			const std::string telosDevPath,
			BlackLib::BlackGPIO** gpios);

	bool _addNetworkNode(Session*, NodeInfoMsg*);

	static NodeNetworkSystem* _instance;

	static BlackLib::BlackGPIO** _gpios;

	boost::asio::io_service& _ioService;

	NodeProcessingSystem* _nodeProcessingSystem;
	OffloadingManager* _offloadingManager;
	TelosbRadioSystem* _telosbRadioSystem;

	Server* _server;
	std::vector<Client*> _clients;

	static boost::mutex _countMutex;
	static boost::condition_variable _countContVar;

	static unsigned short _queuedMessages;

	ushort _wifiBandwidth;

};

#endif
