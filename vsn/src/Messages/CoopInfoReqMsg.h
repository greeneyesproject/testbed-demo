/*
 * CoopInfoReqMsg.h
 *
 *  Created on: 18/set/2014
 *      Author: greeneyes
 */

#ifndef SRC_MESSAGES_COOPINFOREQMSG_H_
#define SRC_MESSAGES_COOPINFOREQMSG_H_

#include <Messages/Message.h>

class CoopInfoReqMsg: public Message {

private:

public:

	CoopInfoReqMsg(NetworkNode* const src, NetworkNode* const dst,
			const LinkType linkType) :
			Message(src, dst, linkType) {
		_msg_type = MESSAGETYPE_COOP_INFO_REQ;
	}
	CoopInfoReqMsg(Header* const header) :
			Message(header) {
		_msg_type = MESSAGETYPE_COOP_INFO_REQ;
	}

};

#endif /* CoopInfoReqMsg_H_ */
