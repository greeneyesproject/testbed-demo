#ifndef SRC_NETWORK_NETWORKNODE_H_
#define SRC_NETWORK_NETWORKNODE_H_
#include <TestbedTypes.h>
#include <iostream>
#include <vector>
#include <stdint.h>

class Connection;
class Session;

class NetworkNode {
public:

	static std::string ipIntToString(const uint32_t ipInt);
	static uint32_t ipStringToInt(const std::string ipStr);

	friend std::ostream& operator <<(std::ostream& os, const NetworkNode& node);

	static NetworkNode* setGui(const std::string ipStr, const uint16_t port);
	static NetworkNode* setSink(const int id, const int am = -1,
			const std::string ipStr = "", const uint16_t port = 0);
	static NetworkNode* setMyself(NetworkNode* const myself) {
		_myself = myself;
		return _myself;
	}
	static NetworkNode* addCamera(const int id, const int am = -1,
			const std::string ipStr = "", const uint16_t port = 0);
	static NetworkNode* addCooperator(const int id);

	static NetworkNode* getGui() {
		return _gui;
	}
	static NetworkNode* getSink() {
		return _sink;
	}
	static NetworkNode* getMyself() {
		return _myself;
	}
	static std::vector<NetworkNode*> getCameras() {
		return _cameras;
	}
	static std::vector<NetworkNode*> getCooperators() {
		return _cooperators;
	}

	static NetworkNode* getCameraById(uint8_t id);
	static NetworkNode* getCooperatorById(uint8_t id);
	static NetworkNode* getCameraBySession(Session* session);
	static NetworkNode* getCooperatorBySession(Session* session);
	static NetworkNode* getCameraByIpPort(std::string ip, uint16_t port);

	static NetworkNode* getNodeById(uint8_t id);
	static NetworkNode* getNodeByIpPort(std::string ip, uint16_t port);
	static NetworkNode* getNodeBySession(Session* session);

	static void printNetwork();

	std::string getIpAddrString() const {
		return NetworkNode::ipIntToString(_ip);
	}

	uint8_t getId() const {
		return _id;
	}
	uint16_t getAmAddr() const {
		return _am;
	}
	uint32_t getIpAddr() const {
		return _ip;
	}
	uint16_t getIpPort() const {
		return _port;
	}
	NodeType getType() const {
		return _type;
	}

	Session* getSession() const {
		return _session;
	}

	void setSession(Session* const session) {
		_session = session;
	}

private:
	NetworkNode(const NodeType type, const int id, const int am = -1,
			const std::string ipStr = "", const uint16_t por = 0);

	NodeType _type;
	/* Id of the node */
	uint8_t _id;
	/* Am addres of the node. 0xFFFF (-1) if not defined */
	uint16_t _am;
	/* Ip address of the node. 0xFFFFFFFF (-1) if not defined*/
	uint32_t _ip;
	/* Server port of the node. 0 if no server for this node*/
	uint16_t _port;

	/* Connection to the node */
	Session* _session;

	/* Local database of network nodes. Each node is uniquely identified by its id */
	static NetworkNode* _gui;
	static NetworkNode* _sink;
	static NetworkNode* _myself;
	static std::vector<NetworkNode*> _cameras;
	static std::vector<NetworkNode*> _cooperators;

	//TODO delete
	static std::vector<unsigned char> _getCooperatorsIds();
};

#endif /* SRC_RADIOSYSTEM_NETWORKNODE_H_ */
