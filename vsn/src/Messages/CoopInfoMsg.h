#ifndef SRC_MESSAGES_COOPINFOMSG_H_
#define SRC_MESSAGES_COOPINFOMSG_H_

#include <Messages/Message.h>
#include <Network/LinkType.h>

#include <opencv2/core/core.hpp>

#include <string>
#include <vector>

class Header;
class NetworkNode;

class CoopInfoMsg: public Message {
private:

	friend class cereal::access;
	template<typename Archive>
	void serialize(Archive &ar);

	std::vector<unsigned char> _cooperatorsIds;

public:
	CoopInfoMsg(NetworkNode* const src, NetworkNode* const dst,
			const LinkType linkType,
			const std::vector<unsigned char>& coopsIds);

	CoopInfoMsg(Header* const header, Bitstream* const bitstream);

	Bitstream* getBitStream() const;

	std::vector<unsigned char> getCooperators() const {
		return _cooperatorsIds;
	}

};

#endif /* COOPINFOMSG_H_ */
