#ifndef SRC_MESSAGES_ACKMSG_H_
#define SRC_MESSAGES_ACKMSG_H_

#include <Messages/Message.h>
#include <Network/LinkType.h>

#include <opencv2/core/core.hpp>

#include <string>
#include <vector>

class Header;

class AckMsg: public Message {
private:

	friend class cereal::access;
	template<typename Archive>
	void serialize(Archive &ar);

	MessageType _rcvMsgType;
	uint32_t _rcvPayloadSize;
	int64 _sendTxStartTick;

public:

	AckMsg(NetworkNode* const src, NetworkNode* const dst,
			const LinkType linkType, const MessageType rcvMsgType,
			const uint32_t rcvPayloadSize, const int64 sendTxStartTick);

	AckMsg(Header* const header, Bitstream* const bitstream);

	Bitstream* getBitStream() const;

	MessageType getReceivedMessageType() const {
		return _rcvMsgType;
	}

	uint32_t getReceivedPayloadSize() const {
		return _rcvPayloadSize;
	}

	int64 getSendTxStartTick() const {
		return _sendTxStartTick;
	}

};

#endif /* SRC_MESSAGES_ACKMSG_H_ */
