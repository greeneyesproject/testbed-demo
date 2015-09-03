#include <Messages/CoopInfoMsg.h>
#include <Messages/Message.h>
#include <TestbedTypes.h>
#include <cassert>
#include <vector>
using namespace std;

CoopInfoMsg::CoopInfoMsg(NetworkNode* const src, NetworkNode* const dst,
		const LinkType linkType, const std::vector<unsigned char>& coopsIds) :
		Message(src, dst, linkType) {
	_msg_type = MESSAGETYPE_COOP_INFO;
	_cooperatorsIds = coopsIds;
}

CoopInfoMsg::CoopInfoMsg(Header* const header, Bitstream* const bitstream) :
		Message(header) {
	_msg_type = MESSAGETYPE_COOP_INFO;
	/* Dummy initialization to avoid warnings */
	stringstream ss;
	ss.rdbuf()->pubsetbuf((char*) bitstream->data(), bitstream->size());
	cereal::BinaryInputArchive ia(ss);
	ia >> (*this);
}

Bitstream* CoopInfoMsg::getBitStream() const {
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
void CoopInfoMsg::serialize(Archive &ar) {
	ar & _cooperatorsIds;
}

