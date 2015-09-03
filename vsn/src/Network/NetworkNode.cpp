#include <Network/NetworkNode.h>
#include <string>
#include <opencv2/core/core.hpp>
#include "TestbedTypes.h"

using namespace std;

/* Define static members */
NetworkNode* NetworkNode::_gui = NULL;
NetworkNode* NetworkNode::_sink = NULL;
NetworkNode* NetworkNode::_myself = NULL;
std::vector<NetworkNode*> NetworkNode::_cameras;
std::vector<NetworkNode*> NetworkNode::_cooperators;

NetworkNode::NetworkNode(const NodeType type, const int id, const int am,
		const string ipStr, const uint16_t port) {
	_type = type;
	_id = id;
	_am = am;
	_ip = NetworkNode::ipStringToInt(ipStr);
	_port = port;
	_session = NULL;
}

std::string NetworkNode::ipIntToString(const uint32_t ip) {
	stringstream os;
	os << ((ip >> 24) & 0xFF) << "." << ((ip >> 16) & 0xFF) << "."
			<< ((ip >> 8) & 0xFF) << "." << (ip & 0xFF);
	//cout << "NetworkNode::ipIntToString: " << os << endl;
	return os.str();
}

uint32_t NetworkNode::ipStringToInt(const std::string ipStr) {
	int octets[4];
	uint32_t ipInt = 0;
	char dot;
	istringstream ipStream(ipStr);
	ipStream >> octets[3] >> dot >> octets[2] >> dot >> octets[1] >> dot
			>> octets[0];
	ipInt |= octets[3];
	ipInt <<= 8;
	ipInt |= octets[2];
	ipInt <<= 8;
	ipInt |= octets[1];
	ipInt <<= 8;
	ipInt |= octets[0];

	return ipInt;
}

/* Network nodes database management methods */
NetworkNode* NetworkNode::setGui(const std::string ipStr, const uint16_t port) {
	if (_gui != NULL) {
		delete _gui;
	}
	_gui = new NetworkNode(NODETYPE_GUI, 0, 0, ipStr, port);
	return _gui;
}
NetworkNode* NetworkNode::setSink(const int id, const int am,
		const std::string ipStr, const uint16_t port) {
	if (_sink != NULL) {
		delete _sink;
	}
	_sink = new NetworkNode(NODETYPE_SINK, id, am, ipStr, port);
	return _sink;
}
NetworkNode* NetworkNode::addCamera(const int id, const int am,
		const std::string ipStr, const uint16_t port) {
	NetworkNode* camera = NetworkNode::getCameraById(id);
	if (camera == NULL) {
		camera = new NetworkNode(NODETYPE_CAMERA, id, am, ipStr, port);
		_cameras.push_back(camera);
	}
	return camera;
}
NetworkNode* NetworkNode::addCooperator(const int id) {
	NetworkNode* cooperator = NetworkNode::getCooperatorById(id);
	if (cooperator == NULL) {
		cooperator = new NetworkNode(NODETYPE_COOPERATOR, id);
		_cooperators.push_back(cooperator);
	}
	return cooperator;
}
NetworkNode* NetworkNode::getNodeById(const uint8_t id) {
	if (_gui && (_gui->getId() == id)) {
		return _gui;
	}
	if (_sink && (_sink->getId() == id)) {
		return _sink;
	}
	NetworkNode* node = getCameraById(id);
	if (node) {
		return node;
	}
	node = getCooperatorById(id);
	if (node) {
		return node;
	}
	return NULL;
}
NetworkNode* NetworkNode::getNodeByIpPort(const std::string ip,
		const uint16_t port) {
	if (_gui && (_gui->getIpAddrString() == ip)
			&& (_gui->getIpPort() == port)) {
		return _gui;
	}
	if (_sink && (_sink->getIpAddrString() == ip)
			&& (_sink->getIpPort() == port)) {
		return _sink;
	}
	NetworkNode* node = getCameraByIpPort(ip, port);
	if (node) {
		return node;
	}
	/* Nobody should search for a cooperator with the tuple (ip,port) since the port is not a priori fixed */
	return NULL;
}
NetworkNode* NetworkNode::getNodeBySession(Session* const session) {
	if (!session) {
		return NULL;
	}
	if (_gui && _gui->getSession() == session) {
		return _gui;
	}
	if (_sink && _sink->getSession() == session) {
		return _sink;
	}
	NetworkNode* node = getCameraBySession(session);
	if (node) {
		return node;
	}
	node = getCooperatorBySession(session);
	if (node) {
		return node;
	}
	return NULL;
}
NetworkNode* NetworkNode::getCameraById(const uint8_t id) {
	if (id == 0xFF) {
		return NULL;
	}
	for (vector<NetworkNode*>::iterator it = _cameras.begin();
			it != _cameras.end(); ++it) {
		if ((*it)->getId() == id) {
			return *it;
		}
	}
	return NULL;
}
NetworkNode* NetworkNode::getCooperatorById(const uint8_t id) {
	if (id == 0xFF) {
		return NULL;
	}
	for (vector<NetworkNode*>::iterator it = _cooperators.begin();
			it != _cooperators.end(); ++it) {
		if ((*it)->getId() == id) {
			return *it;
		}
	}
	return NULL;
}

NetworkNode* NetworkNode::getCameraBySession(Session* const session) {
	if (!session) {
		return NULL;
	}
	for (vector<NetworkNode*>::iterator it = _cameras.begin();
			it != _cameras.end(); ++it) {
		if ((*it)->getSession() == session) {
			return *it;
		}
	}
	return NULL;
}
NetworkNode* NetworkNode::getCooperatorBySession(Session* const session) {
	if (!session) {
		return NULL;
	}
	for (vector<NetworkNode*>::iterator it = _cooperators.begin();
			it != _cooperators.end(); ++it) {
		if ((*it)->getSession() == session) {
			return *it;
		}
	}
	return NULL;
}
NetworkNode* NetworkNode::getCameraByIpPort(const string ip,
		const uint16_t port) {
	for (vector<NetworkNode*>::iterator it = _cameras.begin();
			it != _cameras.end(); ++it) {
		if (((*it)->getIpAddrString() == ip) && ((*it)->getIpPort() == port)) {
			return *it;
		}
	}
	return NULL;
}

void NetworkNode::printNetwork() {
	cout << "---- Network Topology ----" << endl;
	if (_gui) {
		cout << (*_gui) << endl;
	}
	if (_sink) {
		cout << (*_sink) << endl;
	}
	for (vector<NetworkNode*>::iterator it = _cameras.begin();
			it != _cameras.end(); ++it) {
		cout << (**it) << endl;
	}
	for (vector<NetworkNode*>::iterator it = _cooperators.begin();
			it != _cooperators.end(); ++it) {
		cout << (**it) << endl;
	}
}

//TODO delete
vector<unsigned char> NetworkNode::_getCooperatorsIds() {
	vector<unsigned char> coopsIds;
	for (unsigned char idx = 0; idx < _cooperators.size(); ++idx) {
		coopsIds.push_back(_cooperators[idx]->getId());
	}
	return coopsIds;
}

ostream& operator <<(ostream& os, const NetworkNode& node) {
	os << node._type << " id:" << (int) node._id;
	return os;
}
