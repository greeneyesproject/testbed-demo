#ifndef SRC_MESSAGES_MESSAGE_H
#define SRC_MESSAGES_MESSAGE_H

#include <Network/MessageParser.h>
#include <TestbedTypes.h>
#include <Network/LinkType.h>

#include <opencv2/core/core.hpp>

#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>

class NetworkNode;
class Header;
class Connection;

class Message {
private:

	NetworkNode* _src;
	NetworkNode* _dst;

	static const char* _msgTypeStrVect[];

	uint8_t _seq_num;

protected:
	Message(Header* const header);
	Message(NetworkNode* const src, NetworkNode* const dst,
			const LinkType linkType);

	MessageType _msg_type;
	LinkType _linkType;
	size_t _bitstreamSize;

public:

	friend std::ostream& operator <<(std::ostream& os, const Message& node);

	uint8_t getSeqNum() const {
		return _seq_num;
	}

	void setSeqNum(const uint8_t seqnum) {
		_seq_num = seqnum;
	}

	MessageType getType() const {
		return _msg_type;
	}

	std::string getTypeStr() const {
		return std::string(_msgTypeStrVect[_msg_type]);
	}

	void setType(const MessageType type) {
		_msg_type = type;
	}

	LinkType getLinkType() const {
		return _linkType;
	}

	void setLinkType(LinkType const linkType) {
		_linkType = linkType;
	}

	NetworkNode* getSrc() const {
		return _src;
	}

	void setSrc(NetworkNode* const src) {
		_src = src;
	}

	NetworkNode* getDst() const {
		return _dst;
	}

	void setDst(NetworkNode* const dst) {
		_dst = dst;
	}

	size_t getBitstreamSize() const {
		return _bitstreamSize;
	}

	virtual Bitstream* getBitStream() const {
		return new Bitstream();
	}

	virtual ~Message() {
	}

};

#endif
