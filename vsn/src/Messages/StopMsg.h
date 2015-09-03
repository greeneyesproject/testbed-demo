#ifndef SRC_MESSAGES_STOPMSG_H_
#define SRC_MESSAGES_STOPMSG_H_

#include <Messages/Message.h>

class Header;

class StopMsg: public Message {
private:

public:

	StopMsg(NetworkNode* const src, NetworkNode* const dst,
			LinkType const linkType) :
			Message(src, dst, linkType) {
		_msg_type = MESSAGETYPE_STOP;
	}
	StopMsg(Header* const header) :
			Message(header) {
		_msg_type = MESSAGETYPE_STOP;
	}

};

#endif /* STOPMSG_H_ */
