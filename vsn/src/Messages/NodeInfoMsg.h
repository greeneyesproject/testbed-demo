#ifndef SRC_MESSAGES_NODEINFOMSG_H_
#define SRC_MESSAGES_NODEINFOMSG_H_

#include <Messages/Message.h>
#include <Network/LinkType.h>
#include <TestbedTypes.h>
#include <opencv2/core/core.hpp>

#include <string>
#include <vector>

class Header;
class NetworkNode;

class NodeInfoMsg: public Message {
private:

	friend class cereal::access;
	template<typename Archive>
	void serialize(Archive &ar);

	uchar _id;
	ushort _amAddress;
	std::string _ipAddress;
	ushort _serverPort;
	NodeType _nodeType;

public:
	NodeInfoMsg(NetworkNode* const src, NetworkNode* const dst,
			const LinkType linkType, const uchar id, const ushort amAddress,
			const std::string& ipAddress, const ushort serverPort,
			const NodeType nodeType);

	NodeInfoMsg(Header* const header, Bitstream* const bitstream);

	Bitstream* getBitStream() const;

	uchar getId() const {
		return _id;
	}
	std::string getIpAddress() const {
		return _ipAddress;
	}
	ushort getServerPort() const {
		return _serverPort;
	}
	ushort getAmAddress() const {
		return _amAddress;
	}
	NodeType getNodeType() const {
		return _nodeType;
	}

};

#endif /* SRC_MESSAGES_NODEINFOMSG_H_ */
