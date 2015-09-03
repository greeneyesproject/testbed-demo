#include <Network/MessageParser.h>
#include <Messages/Header.h>
#include <Messages/Message.h>
#include <Messages/StartCTAMsg.h>
#include <Messages/StartATCMsg.h>
#include <Messages/DataATCMsg.h>
#include <Messages/DataCTAMsg.h>
#include <Messages/StopMsg.h>
#include <Messages/CoopInfoMsg.h>
#include <Messages/CoopInfoReqMsg.h>
#include <Messages/NodeInfoMsg.h>
#include <Messages/AckMsg.h>
#include <cassert>
#include <iostream>
#include <boost/date_time.hpp>

using namespace std;
using namespace boost::posix_time;

/* Parse the bitstream according to the message type contained in the header.
 * Returned pointer is NULL on decode errors or on source/destination NULL. No exceptions are thrown
 * header and bitstream are deleted
 */
Message* MessageParser::parseMessage(Header* header, Bitstream* bitstream) {

	Message* msg = NULL;

	time_facet* facet = new time_facet("%d-%m-%Y %H:%M:%S");
	locale loc(cout.getloc(),facet);
	basic_stringstream<char> wss;
	wss.imbue(loc);
	wss << second_clock::local_time();
	string nowString = wss.str();

	if (_DEBUG){
		cout << "MessageParser::parseMessage: " << nowString << endl;
	}

	if (!header->getDst()) {
		if (_DEBUG)
			cerr << "MessageParser::parseMessage: invalid dst " << endl;
	} else if ((!header->getSrc() && header->getMsgType() != MESSAGETYPE_NODE_INFO)) {
		if (_DEBUG)
			cerr << "MessageParser::parseMessage: invalid src " << endl;
	} else {

		try {
			switch (header->getMsgType()) {
			case MESSAGETYPE_START_CTA:
				msg = new StartCTAMsg(header, bitstream);
				break;
			case MESSAGETYPE_START_ATC:
				msg = new StartATCMsg(header, bitstream);
				break;
			case MESSAGETYPE_STOP:
				msg = new StopMsg(header);
				break;
			case MESSAGETYPE_COOP_INFO:
				msg = new CoopInfoMsg(header, bitstream);
				break;
			case MESSAGETYPE_COOP_INFO_REQ:
				msg = new CoopInfoReqMsg(header);
				break;
			case MESSAGETYPE_DATA_CTA:
				msg = new DataCTAMsg(header, bitstream);
				break;
			case MESSAGETYPE_DATA_ATC:
				msg = new DataATCMsg(header, bitstream);
				break;
			case MESSAGETYPE_NODE_INFO:
				msg = new NodeInfoMsg(header, bitstream);
				break;
			case MESSAGETYPE_ACK:
				msg = new AckMsg(header, bitstream);
				break;
			default:
				break;
			}
		} catch (exception &e) {
			cerr
					<< "Session::handleReadMessage: unable to deserialize message of type " << header->getMsgType()
					<< endl;
		}

	}

	delete bitstream;
	delete header;

	return msg;
}

void MessageParser::printBitstream(Bitstream* const bitstream) {
	cout << "bitstream->size() = " << bitstream->size() << endl;
	cout << "bitstream: " << endl;
	cout << "0x" << setfill('0') << setw(2) << hex;
	for (Bitstream::iterator it = bitstream->begin(); it != bitstream->end();
			++it) {
		cout << (unsigned short) (*it) << " ";
	}
	cout << dec << endl;
}
