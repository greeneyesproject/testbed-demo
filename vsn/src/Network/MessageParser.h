#ifndef SRC_NETWORK_MESSAGEPARSER_H_
#define SRC_NETWORK_MESSAGEPARSER_H_

#include <opencv2/core/core.hpp>

#include <vector>

typedef std::vector<unsigned char> Bitstream;

class Connection;
class Header;
class Message;

class MessageParser {
public:
	static Message* parseMessage(Header*, Bitstream*);
	static void printBitstream(Bitstream* const bitstream);
private:
	MessageParser() {
	}

	const static unsigned char _DEBUG = 0;

};

#endif /* MESSAGEPARSER_H_ */
