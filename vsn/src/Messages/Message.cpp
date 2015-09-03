/*
 * Message.cpp
 *
 *  Created on: 04/feb/2015
 *      Author: luca
 */

#include <Messages/Message.h>
#include <Messages/Header.h>
#include <Network/NetworkNode.h>
#include <vector>
#include <string>

using namespace std;

const char* Message::_msgTypeStrVect[] = { "NONE", "START_CTA_MESSAGE",
		"START_ATC_MESSAGE", "DATA_CTA_MESSAGE",
		"DATA_ATC_MESSAGE", "STOP_MESSAGE", "COOP_INFO_MESSAGE",
		"COOP_INFO_REQ_MESSAGE", "NODE_INFO_MESSAGE","ACK_MESSAGE" };

Message::Message(Header* const header) {
	_src = header->getSrc();
	_dst = header->getDst();
	_msg_type = header->getMsgType();
	_seq_num = header->getSeqNum();
	_linkType = header->getLinkType();
	_bitstreamSize = 0;
}

Message::Message(NetworkNode* const src, NetworkNode* const dst,
		const LinkType linkType) {
	_src = src;
	_dst = dst;
	_msg_type = MESSAGETYPE_NONE;
	_seq_num = -1;
	_linkType = linkType;
	_bitstreamSize = 0;
}

ostream& operator <<(ostream& os, const Message& msg) {
	os << msg.getTypeStr();
	if (msg.getSrc())
		os << " from " << (*msg.getSrc());
	if (msg.getDst())
		os << " to " << (*msg.getDst());
	return os;
}
