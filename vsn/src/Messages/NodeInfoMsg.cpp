#include <Messages/NodeInfoMsg.h>
#include <Messages/Message.h>
#include <TestbedTypes.h>
#include <cassert>
#include <vector>
using namespace std;

NodeInfoMsg::NodeInfoMsg(NetworkNode* const src, NetworkNode* const dst,
		const LinkType linkType, const uchar id, const ushort amAddress,
		const std::string& ipAddress, const ushort serverPort,
		const NodeType nodeType) :
		Message(src, dst, linkType) {
	_msg_type = MESSAGETYPE_NODE_INFO;
	_id = id;
	_amAddress = amAddress;
	_ipAddress = ipAddress;

	_serverPort = serverPort;
	_nodeType = nodeType;
}

NodeInfoMsg::NodeInfoMsg(Header* const header, Bitstream* const bitstream) :
		Message(header) {
	_msg_type = MESSAGETYPE_NODE_INFO;
	/* Dummy initialization to avoid warnings */
	_id = -1;
	_amAddress = -1;
	_ipAddress = "";
	_serverPort = -1;
	_nodeType = NODETYPE_UNDEF;
	stringstream ss;
	ss.rdbuf()->pubsetbuf((char*) bitstream->data(), bitstream->size());
	cereal::BinaryInputArchive ia(ss);
	ia >> (*this);
}

Bitstream* NodeInfoMsg::getBitStream() const {
	stringstream bitstream;
	cereal::BinaryOutputArchive oa(bitstream);
	oa << (*this);
	/* bitstreamString is needed since every call to bitstream.str() returns a different string*/
	string bitstreamString = bitstream.str();
	Bitstream* bitstreamVector = new vector<uchar>(bitstreamString.begin(),
			bitstreamString.end());
	return bitstreamVector;
}

template<typename Archive>
void NodeInfoMsg::serialize(Archive &ar) {
	ar & _id;
	ar & _amAddress;
	ar & _ipAddress;
	ar & _serverPort;
	ar & _nodeType;
}

