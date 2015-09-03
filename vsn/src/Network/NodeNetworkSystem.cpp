#include <Network/MessageParser.h>
#include <Network/Telosb/TelosbRadioSystem.h>
#include <Network/Tcp/Server.h>
#include <Network/Tcp/Client.h>
#include <Network/Tcp/Session.h>
#include <Network/NetworkNode.h>
#include <Messages/NodeInfoMsg.h>
#include <Network/OffloadingManager.h>

#include <boost/filesystem.hpp>
#include <Network/NodeNetworkSystem.h>
#include <Network/Telosb/IncomingMessageQueue.h>
#include <Messages/CoopInfoMsg.h>
#include <cstddef>

using namespace std;

NodeNetworkSystem* NodeNetworkSystem::_instance = NULL;
boost::mutex NodeNetworkSystem::_countMutex;
boost::condition_variable NodeNetworkSystem::_countContVar;
unsigned short NodeNetworkSystem::_queuedMessages = 0;
BlackLib::BlackGPIO** NodeNetworkSystem::_gpios = NULL;

NodeNetworkSystem* NodeNetworkSystem::getInstance(
		NodeProcessingSystem* nodeProcessingSystem,
		boost::asio::io_service& ioService,
		const std::vector<std::string> serverIps,
		const std::vector<uint16_t> serverPorts, const std::string telosDevPath,
		BlackLib::BlackGPIO** gpios) {
	if (_DEBUG)
		cout << "NodeNetworkSystem::getInstance" << endl;
	if (_instance == NULL) {
		_instance = new NodeNetworkSystem(nodeProcessingSystem, ioService,
				serverIps, serverPorts, telosDevPath, gpios);
	}
	return _instance;
}

NodeNetworkSystem::NodeNetworkSystem(NodeProcessingSystem* nodeProcessingSystem,
		boost::asio::io_service& ioService,
		const std::vector<std::string> remoteIps,
		const std::vector<uint16_t> remotePorts, const std::string telosDevPath,
		BlackLib::BlackGPIO** gpios) :
		_ioService(ioService) {

	_gpios = gpios;

	if (_DEBUG)
		cout << "NodeNetworkSystem::NodeNetworkSystem" << endl;

	/*
	 * Aruments checker
	 */
	if (!(remoteIps.size() == remotePorts.size())) {
		if (_DEBUG)
			cerr
					<< "NodeNetworkSystem::NodeNetworkSystem: The number of remote ips must be equal to the number of remote ports "
					<< endl;
		exit(-1);
	}

	_nodeProcessingSystem = nodeProcessingSystem;
	_nodeProcessingSystem->setNodeNetworkSystem(this);

	_offloadingManager = OffloadingManager::getInstance(_nodeProcessingSystem,
			this);

	/*
	 * Instantiate the Telosb Radio System
	 */
	if (telosDevPath != "null" && telosDevPath != ""
			&& boost::filesystem::exists(telosDevPath)) {
		if (_DEBUG)
			cout
					<< "NodeNetworkSystem::NodeNetworkSystem: Instantiate Telosb Radio System"
					<< endl;
		_telosbRadioSystem = new TelosbRadioSystem(this, telosDevPath.c_str());
	} else {
		_telosbRadioSystem = NULL;
	}

	/*
	 * Instantiate the server
	 */
	unsigned short incomingPort = NetworkNode::getMyself()->getIpPort();
	if (incomingPort) {
		if (_DEBUG)
			cout
					<< "NodeNetworkSystem::NodeNetworkSystem: Opening server on port "
					<< incomingPort << endl;
		_server = new Server(_ioService, this, incomingPort);
	} else {
		_server = NULL;
	}

	/*
	 * Instantiate the clients
	 */
	for (size_t clientIdx = 0; clientIdx < remoteIps.size(); clientIdx++) {
		string serverIp = remoteIps.at(clientIdx);
		uint16_t serverPort = remotePorts.at(clientIdx);
		Client* client = new Client(_ioService, this, serverIp, serverPort);
		_clients.push_back(client);
		NetworkNode::getNodeByIpPort(serverIp, serverPort)->setSession(
				client->getSession());
	}

	_wifiBandwidth = 55296;

}

void NodeNetworkSystem::shutdown(const int signal) {
	if (_DEBUG)
		cout << endl << "--------" << endl << "NodeNetworkSystem::shutdown"
				<< endl;
	if (_instance->_server) {
		delete _instance->_server;
	}
	for (vector<Client*>::iterator it = _instance->_clients.begin();
			it != _instance->_clients.end(); ++it) {
		delete (*it);
	}
	exit(0);
}

/* ServerInterface methods */

void NodeNetworkSystem::serverAddSessionHandler(Session* session) {
	if (_DEBUG)
		cout << "NodeNetworkSystem::serverAddSessionHandler" << endl;
	_server->startAccept();
}

void NodeNetworkSystem::serverRemoveSessionHandler(Session* session) {
	switch (_nodeProcessingSystem->getNodeType()) {
	case NODETYPE_SINK: {
		NetworkNode* camera = NetworkNode::getCameraBySession(session);
		if (camera) {
			if (_DEBUG)
				cout << "NodeNetworkSystem::serverAddSessionHandler: Camera "
						<< (unsigned short) camera->getId() << " disconnected "
						<< endl;
			camera->setSession(NULL);
		}
		break;
	}
	case NODETYPE_CAMERA: {
		NetworkNode* cooperator = NetworkNode::getCooperatorBySession(session);
		if (cooperator) {
			if (_DEBUG)
				cout
						<< "NodeNetworkSystem::serverAddSessionHandler: Cooperator "
						<< (unsigned short) cooperator->getId()
						<< " disconnected " << endl;
			cooperator->setSession(NULL);
			_offloadingManager->removeCooperator(cooperator);
			CoopInfoMsg* infoMsg = new CoopInfoMsg(NetworkNode::getMyself(),
					NetworkNode::getSink(), LINKTYPE_TCP,
					_offloadingManager->getCooperatorsIds());
			sendMessage(infoMsg);
		}
		break;
	}
	default: {
		if (_DEBUG)
			cerr << "NodeNetworkSystem::serverAddSessionHandler: Node of type "
					<< _nodeProcessingSystem->getNodeType()
					<< " can't reach this point" << endl;
	}
	}
}

/**
 * A client component has connected. Notify the server this node
 */
void NodeNetworkSystem::clientConnectHandler(Session* session) {
	if (_DEBUG)
		cout << "NodeNetworkSystem::clientConnectHandler" << endl;

	NetworkNode* serverNode = NetworkNode::getNodeBySession(session);
	NetworkNode* myself = NetworkNode::getMyself();

	NodeInfoMsg* msg = new NodeInfoMsg(myself, serverNode, LINKTYPE_TCP,
			myself->getId(), myself->getAmAddr(),
			session->getSocket()->local_endpoint().address().to_string(),
			session->getSocket()->local_endpoint().port(), myself->getType());

	session->writeMessage(msg);

	if (myself->getType() == NODETYPE_CAMERA) {
		CoopInfoMsg* infoMsg = new CoopInfoMsg(NetworkNode::getMyself(),
				NetworkNode::getSink(), LINKTYPE_TCP,
				_offloadingManager->getCooperatorsIds());
		sendMessage(infoMsg);
	}

}

void NodeNetworkSystem::serverMessageHandler(Session* session, Message* msg) {
	if (_DEBUG)
		cout << "NodeNetworkSystem::serverMessageHandler" << endl;

	if (msg->getType() == MESSAGETYPE_NODE_INFO) {
		_addNetworkNode(session, (NodeInfoMsg*) msg);
	} else {
		_nodeProcessingSystem->queueMessage(msg);
	}
}

/* ClientInterface methods */
void NodeNetworkSystem::clientMessageHandler(Session* session, Message* msg) {
	_nodeProcessingSystem->queueMessage(msg);
}

void NodeNetworkSystem::telosMessageHandler(Message* msg) {
	_nodeProcessingSystem->queueMessage(msg);
}

/**
 * Given a NodeInfoMsg inserts the node into NetworkNode database.
 * @param session can be null
 * Returns true if new node inserted, false if already present
 * Delete the message
 */
bool NodeNetworkSystem::_addNetworkNode(Session* session, NodeInfoMsg* msg) {
	if (_DEBUG)
		cout << "NodeNetworkSystem::_addNetworkNode" << endl;

	bool retval = false;
	NetworkNode* connectedNode = NetworkNode::getNodeById(msg->getId());
	if (connectedNode == NULL) {
		/* The connected node is unknown. Add it */
		retval = true;
		if (_DEBUG)
			cout
					<< "NodeNetworkSystem::_addNetworkNode: creating a new NetworkNode"
					<< endl;
		switch (msg->getNodeType()) {
		case NODETYPE_CAMERA:
			connectedNode = NetworkNode::addCamera(msg->getId(),
					msg->getAmAddress(), msg->getIpAddress(),
					msg->getServerPort());
			break;
		case NODETYPE_COOPERATOR:
			connectedNode = NetworkNode::addCooperator(msg->getId());
			break;
		default:
			if (_DEBUG)
				cerr
						<< "NodeNetworkSystem::_addNetworkNode: Error! Notification of a type "
						<< msg->getNodeType() << " node!" << endl;
			return false;
		}
		if (_DEBUG) {
			cout
					<< "NodeNetworkSystem::_addNetworkNode: Network Topology updated "
					<< endl;
			NetworkNode::printNetwork();
		}
	}
	if (session) {
		connectedNode->setSession(session);
	}
	if (connectedNode->getType() == NODETYPE_COOPERATOR) {
		switch (NetworkNode::getMyself()->getType()) {
		case NODETYPE_CAMERA: {
			_offloadingManager->addCooperator(connectedNode);
			CoopInfoMsg* infoMsg = new CoopInfoMsg(NetworkNode::getMyself(),
					NetworkNode::getSink(), LINKTYPE_TCP,
					_offloadingManager->getCooperatorsIds());
			sendMessage(infoMsg);
			break;
		}
		default:
			break;
		}
	}

	delete msg;

	return retval;
}

/**
 * Send message on the correct interface
 */
void NodeNetworkSystem::sendMessage(Message* msg) {
	if (!msg) {
		if (_DEBUG)
			cerr << "NodeNetworkSystem::sendMessage: Empty message" << endl;

		return;
	}

	NetworkNode* dst = msg->getDst();
	if (!dst) {
		if (_DEBUG)
			cerr << "NodeNetworkSystem::sendMessage: Empty destination" << endl;
		delete msg;
		return;
	}

	if (msg->getType() == MESSAGETYPE_DATA_ATC
			|| msg->getType() == MESSAGETYPE_DATA_CTA) {
		boost::mutex::scoped_lock lock(_countMutex);
		_queuedMessages++;
		_gpios[5]->setValue(BlackLib::high); //Set transmission pin
		lock.unlock();
	}

	switch (msg->getLinkType()) {
	case LINKTYPE_TCP: {
		Session* ses = dst->getSession();
		if (!ses) {
			if (_DEBUG)
				cerr << "NodeNetworkSystem::sendMessage: Empty session" << endl;
			delete msg;
			return;
		}
		ses->writeMessage(msg);
		break;
	}
	case LINKTYPE_TELOS: {
		_telosbRadioSystem->writeMessage(msg);
		break;
	}
	default: {
		if (_DEBUG)
			cout << "NodeNetworkSystem::sendMessage: Not yet implemented"
					<< endl;
		delete msg;
		break;
	}
	}

}

/* Flushes the queue of all outgoing messages: telos, server, client */
void NodeNetworkSystem::flushOutgoingMessages() {

	if (_DEBUG) {
		cout << "NodeNetworkSystem::flushOutgoingMessages" << endl;
	}
	if (_telosbRadioSystem)
		_telosbRadioSystem->flushOutgoingMessages();
	_server->flushOutgoingMessages();
	for (vector<Client*>::iterator it = _clients.begin(); it != _clients.end();
			++it) {

		(*it)->flushOutgoingMessages();
	}
}

void NodeNetworkSystem::stopOnSendEnd() {
	if (_DEBUG) {
		cout << "NodeNetworkSystem::stopOnSendEnd" << endl;
		cout << "NodeNetworkSystem::stopOnSendEnd: _queuedMessages = "
				<< _queuedMessages << endl;
	}
	boost::mutex::scoped_lock lock(_countMutex);
	while (_queuedMessages) {
		_countContVar.wait(lock);
		cout << "NodeNetworkSystem::stopOnSendEnd: WakeUp _queuedMessages = "
				<< _queuedMessages << endl;
	}
	shutdown(SIGQUIT);
}

void NodeNetworkSystem::messageSent() {
	boost::mutex::scoped_lock lock(_countMutex);
	_queuedMessages--;
	if (!_queuedMessages) {
		_gpios[5]->setValue(BlackLib::low);  //Reset transmission pin
	}
	lock.unlock();
	_countContVar.notify_one();
}

float NodeNetworkSystem::prepareWifiBandwidthControl(const string sinkIpAddr_,
		const ushort sinkPort_) {

	float time = cv::getTickCount();

	stringstream ss;
	int ret;

	ss.str("");
	ss << "sudo tc qdisc del dev wlan0 root" << endl;
	if (_DEBUG)
		cout << "NodeProcessingSystem::_prepareWifiBandwidthControl: "
				<< ss.str();
	ret = system(ss.str().c_str());
	if (_DEBUG)
		cout << "NodeProcessingSystem::_prepareWifiBandwidthControl: ret: "
				<< ret << endl;

	ss.str("");
	ss
			<< "sudo tc qdisc add dev wlan0 root handle 1: cbq avpkt 1000 bandwidth 54mbit"
			<< endl;
	if (_DEBUG)
		cout << "NodeProcessingSystem::_prepareWifiBandwidthControl: "
				<< ss.str();
	ret = system(ss.str().c_str());
	if (_DEBUG)
		cout << "NodeProcessingSystem::_prepareWifiBandwidthControl: ret: "
				<< ret << endl;

	ss.str("");
	ss
			<< "sudo tc class replace dev wlan0 parent 1: classid 1:1 cbq rate 54mbit allot 1500 prio 5 bounded isolated"
			<< endl;
	if (_DEBUG)
		cout << "NodeProcessingSystem::_prepareWifiBandwidthControl: "
				<< ss.str();
	ret = system(ss.str().c_str());
	if (_DEBUG > 1)
		cout << "NodeProcessingSystem::_prepareWifiBandwidthControl: ret: "
				<< ret << endl;

	ss.str("");
	ss
			<< "sudo tc filter add dev wlan0 parent 1: protocol ip prio 16 handle 100 fw flowid 1:1"
			<< endl;
	if (_DEBUG)
		cout << "NodeProcessingSystem::_prepareWifiBandwidthControl: "
				<< ss.str();
	ret = system(ss.str().c_str());
	if (_DEBUG)
		cout << "NodeProcessingSystem::_prepareWifiBandwidthControl: ret: "
				<< ret << endl;

	ss.str("");
	ss << "sudo iptables -t mangle -A POSTROUTING -p tcp --dport " << sinkPort_
			<< " -d " << sinkIpAddr_ << " -j MARK --set-mark 100" << endl;
	if (_DEBUG)
		cout << "NodeProcessingSystem::_prepareWifiBandwidthControl: "
				<< ss.str();
	ret = system(ss.str().c_str());
	if (_DEBUG)
		cout << "NodeProcessingSystem::_prepareWifiBandwidthControl: ret: "
				<< ret << endl;

	return (cv::getTickCount() - time) / cv::getTickFrequency();

}

float NodeNetworkSystem::setWifiBandwidth(const ushort bw_) {

	float time = cv::getTickCount();

	if (bw_ == _wifiBandwidth)
		return 0;
	_wifiBandwidth = bw_;

	int ret;

	stringstream ss;
	ss.str("");
	ss << "sudo tc class replace dev wlan0 parent 1: classid 1:1 cbq rate "
			<< _wifiBandwidth << "kbit allot 1500 prio 5 bounded isolated"
			<< endl;

	if (_DEBUG)
		cout << "NodeProcessingSystem::_setWifiBandwidth: " << ss.str();
	ret = system(ss.str().c_str());
	if (_DEBUG)
		cout << "NodeProcessingSystem::_setWifiBandwidth: ret: " << ret << endl;

	return (cv::getTickCount() - time) / cv::getTickFrequency();
}
