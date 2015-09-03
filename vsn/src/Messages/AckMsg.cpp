#include <Messages/AckMsg.h>
#include <vector>
#include <string>
#include <iostream>

using namespace std;

AckMsg::AckMsg(NetworkNode* const src, NetworkNode* const dst,
		const LinkType linkType, const MessageType rcvMsgType,
		const uint32_t rcvPayloadSize, const int64 sendTxStartTick) :
		Message(src, dst, linkType) {
	_msg_type = MESSAGETYPE_ACK;
	_rcvMsgType = rcvMsgType;
	_rcvPayloadSize = rcvPayloadSize;
	_sendTxStartTick = sendTxStartTick;
}

AckMsg::AckMsg(Header* const header, Bitstream* const bitstream) :
		Message(header ) {
	_msg_type = MESSAGETYPE_ACK;
	/* Dummy initialization to avoid warnings */
	_rcvMsgType = MESSAGETYPE_NONE;
	_rcvPayloadSize = 0;
	_sendTxStartTick = 0;
	stringstream ss;
	ss.rdbuf()->pubsetbuf((char*) bitstream->data(), bitstream->size());
	cereal::BinaryInputArchive ia(ss);
	ia >> (*this);
}

template<typename Archive>
void AckMsg::serialize(Archive &ar){
	ar & _rcvMsgType;
	ar & _rcvPayloadSize;
	ar & _sendTxStartTick;
}

Bitstream* AckMsg::getBitStream() const {
	stringstream bitstream;
	cereal::BinaryOutputArchive oa(bitstream);
	oa << (*this);
	/* bitstreamString is needed since every call to bitstream.str() returns a different string*/
	string bitstreamString = bitstream.str();
	Bitstream* bitstreamVector(
			new vector<uchar>(bitstreamString.begin(), bitstreamString.end()));
	return bitstreamVector;
}

