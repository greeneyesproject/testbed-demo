#include <Messages/Message.h>
#include <Messages/Header.h>
#include <Messages/DataATCMsg.h>
#include <Messages/DataCTAMsg.h>
#include <Messages/AckMsg.h>
#include <Network/NetworkNode.h>

#ifndef GUI
#include <Network/NodeNetworkSystem.h>
#endif

#include <boost/bind.hpp>
#include <Network/Tcp/Session.h>

using namespace std;
using namespace boost::asio::ip;

Session::Session(boost::asio::io_service& ioService,
		SessionInterface* interface) :
		_messagesQueue(1000) {

	if (_DEBUG)
		cout << "Session::Session" << endl;

	_interface = interface;
	_socket = boost::shared_ptr<tcp::socket>(new tcp::socket(ioService));
	_headerBitstream = NULL;
	_header = NULL;
	_messageBitstream = NULL;

	_sendThread = NULL;

	_deleted = false;

	_txTimeATC = 0;
	_txTimeCTA = 0;

}

Session::~Session() {
	if (_DEBUG)
		cout << "Session::~Session" << endl;

	_deleted = true;

	if (_DEBUG)
		cout << "Session::~Session: disable messages queue" << endl;
	_messagesQueue.setActive(false);

	if (_DEBUG)
		cout << "Session::~Session: flush messages queue" << endl;
	_messagesQueue.flush();

	if (_DEBUG)
		cout << "Session::~Session: notify" << endl;
	_messagesQueue.notify();

	closeSocket();

	/*if (_DEBUG)
	 cout << "Session::~Session: deleting the socket" << endl;
	 try {
	 //_socket.reset();
	 } catch (exception& e) {
	 if (_DEBUG)
	 cout << "Session::~Session: " << e.what() << endl;
	 }*/

	if (_sendThread) {
		if (_DEBUG)
			cout << "Session::~Session: join the send thread" << endl;
		try {
			_sendThread->join();
		} catch (exception& e) {
			if (_DEBUG)
				cout << "Session::~Session: " << e.what() << endl;
		}
	}

}

void Session::closeSocket() {
	if (_DEBUG)
		cout << "Session::closeSocket" << endl;

	boost::system::error_code ec;
	if (_DEBUG)
		cout << "Session::closeSocket: cancel" << endl;
	_socket->cancel(ec);
	if (_DEBUG)
		cout << "Session::closeSocket: shutdown" << endl;
	_socket->shutdown(tcp::socket::shutdown_both, ec);
	if (_DEBUG)
		cout << "Session::closeSocket: close" << endl;
	if (_socket->is_open())
		_socket->close(ec);

	if (_DEBUG)
		cout << "Session::closeSocket: end" << endl;
}

bool Session::startReceiver() {
	if (_DEBUG)
		cout << "Session::startReceiver" << endl;
	if (_socket->is_open()) {
		_readHeader();
		_sendThread = new boost::thread(&Session::_sendMessageThread, this);
		return true;
	}
	return false;
}

void Session::_readHeader() {
	if (_DEBUG)
		cout << "Session::_readHeader" << endl;
	_headerBitstream = new vector<unsigned char>(HEADER_SIZE_IP);
	async_read(*_socket, boost::asio::buffer(*_headerBitstream),
			boost::bind(&Session::_handleReadHeader, this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
}
void Session::_handleReadHeader(const boost::system::error_code& error,
		const size_t /*bytes_transferred*/) {
	if (_DEBUG)
		cout << "Session::_handleReadHeader" << endl;
	if (error) {
		_interface->errorHandler(this, error);
		return;
	}
	_header = Header::headerFromIpBitstream(_headerBitstream);
	_readMessage();
}

void Session::_readMessage() {
	cout << "Session::_readMessage" << endl;
	_messageBitstream = new vector<unsigned char>(_header->getPayloadSize());

	async_read(*_socket, boost::asio::buffer(*_messageBitstream),
			boost::bind(&Session::_handleReadMessage, this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
}

/**
 * Decode the received bitstream and call interface messageHandler if the message is correctly decoded.
 * An Ack is sent to the sender to notify the arrival and correct decoding of the message.
 * The Ack is also used by the sender to store the transmission time of the packet
 */
void Session::_handleReadMessage(const boost::system::error_code& error,
		const size_t /*bytes_transferred*/) {
	if (_DEBUG)
		cout << "Session::_handleReadMessage" << endl;
	if (error) {
		_interface->errorHandler(this, error);
		return;
	}

	/* Need to do this before header and messageBistream being deleted by the parser */
	NetworkNode* const src = _header->getSrc();
	const LinkType linkType = _header->getLinkType();
	const MessageType msgType = _header->getMsgType();
	const uint32_t rcvMsgSize = _messageBitstream->size();

	Message* message = MessageParser::parseMessage(_header, _messageBitstream);

	if (message) {

		switch (NetworkNode::getMyself()->getType()) {
		case NODETYPE_SINK: {
			/*
			 * If the sink correctly decodes the message ,it returns an ack to the camera.
			 */
			switch (message->getType()) {
			case MESSAGETYPE_DATA_ATC: {
				DataATCMsg* msg = (DataATCMsg*) message;
				if (msg->getBlockNumber() == msg->getNumBlocks() - 1) {
					AckMsg* ackMsg = new AckMsg(NetworkNode::getMyself(), src,
							linkType, msgType, rcvMsgSize, msg->getStartTick());
					writeMessage(ackMsg);
				}
				break;
			}
			case MESSAGETYPE_DATA_CTA: {
				DataCTAMsg* msg = (DataCTAMsg*) message;
				if (msg->getSliceNumber() == msg->getTotNumSlices() - 1) {
					AckMsg* ackMsg = new AckMsg(NetworkNode::getMyself(), src,
							linkType, msgType, rcvMsgSize, msg->getStartTick());
					writeMessage(ackMsg);
				}
				break;
			}
			default:
				break;
			}
			break;
		}
		case NODETYPE_CAMERA: {
			switch (message->getType()) {
			case MESSAGETYPE_ACK: {
				AckMsg* ackMsg = (AckMsg*) message;
				/* Store txTime for the transmission of current message type entire frame (more messages)*/
				if (_DEBUG)
					cout << "Session::_sendMessageThread: sendTxStartTick: "
							<< ackMsg->getSendTxStartTick() << endl;
				int64 nowTick = cv::getTickCount();
				float time = (double) (nowTick - ackMsg->getSendTxStartTick())
						/ cv::getTickFrequency();
				if (time > FLT_MIN) {
					switch (ackMsg->getReceivedMessageType()) {
					case MESSAGETYPE_DATA_ATC:
						if (_DEBUG)
							cout
									<< "Session::_sendMessageThread: storing ATC txTime: "
									<< time << endl;
						_txTimeATC = time;
						break;
					case MESSAGETYPE_DATA_CTA:
						if (_DEBUG)
							cout
									<< "Session::_sendMessageThread: storing CTA txTime: "
									<< time << endl;
						_txTimeCTA = time;
						break;
					default:
						break;
					}
				} else {
					if (_DEBUG) {
						cout
								<< "Session::_sendMessageThread: time = 0: nowTick "
								<< nowTick << endl;
						cout
								<< "Session::_sendMessageThread: time = 0: ackMsg->getSendTxStartTick() "
								<< ackMsg->getSendTxStartTick() << endl;
					}
				}
				break;
			}
			default:
				break;
			}
			break;
		}
		default:
			break;
		}

		if (message->getType() == MESSAGETYPE_ACK) {
			delete message;
		} else {
			_interface->messageHandler(this, message);
		}
	}

	_readHeader();
}

/**
 * Message is deleted after being sent by _sendMessages
 */
void Session::writeMessage(Message* msg) {
	if (_DEBUG)
		cout << "Session::writeMessage" << endl;
	if (msg == NULL) {
		throw "Session::writeMessage: empty message";
	}

	_messagesQueue.push(msg);

}

void Session::_sendMessageThread() {
	if (_DEBUG)
		cout << "Session::_sendMessageThread" << endl;

	Message* msg;
	double time;

	while (1) {
		_messagesQueue.wait_and_pop(msg);
		if (_deleted) {
			if (_DEBUG)
				cout << "Session::_sendMessageThread: deleted thread" << endl;
			break;
		}

		if (_DEBUG)
			cout << "Session::_sendMessageThread: Message: " << *msg << endl;

		/*
		 * If Camera set DataATC and DataCTA txTime from previous measurements
		 */
		switch (NetworkNode::getMyself()->getType()) {
		case NODETYPE_CAMERA: {
			switch (msg->getType()) {
			case MESSAGETYPE_DATA_ATC: {
				if (_DEBUG)
					cout
							<< "Session::_sendMessageThread: setting ATC send time: "
							<< _txTimeATC << endl;
				((DataATCMsg*) msg)->setTxTime(_txTimeATC);
				break;
			}
			case MESSAGETYPE_DATA_CTA: {
				if (_DEBUG)
					cout
							<< "Session::_sendMessageThread: setting CTA send time: "
							<< _txTimeCTA << endl;
				((DataCTAMsg*) msg)->setTxTime(_txTimeCTA);
				break;
			}
			default:
				break;
			}
			break;
		}
		default:
			break;
		}

		time = cv::getCPUTickCount();
		Bitstream* msgBitstream = msg->getBitStream();
		if (_DEBUG > 2)
			cout << "Session::_sendMessageThread: message serialization time: "
					<< (cv::getCPUTickCount() - time) / cv::getTickFrequency()
					<< " - message bitstream length: " << msgBitstream->size()
					<< " byte" << endl;

		Header header(msg->getSrc(), msg->getDst(), msg->getSeqNum(), 1, 0,
				msg->getType(), msg->getLinkType(), msgBitstream->size());

		bool notifySendEnd = msg->getType() == MESSAGETYPE_DATA_ATC
				|| msg->getType() == MESSAGETYPE_DATA_CTA;

		delete msg;

		time = cv::getCPUTickCount();
		Bitstream* headerBitstream = header.serializeForIpPacket();
		if (_DEBUG > 2)
			cout
					<< "Session::_sendMessageThread: IP header serialization time: "
					<< (cv::getCPUTickCount() - time) / cv::getTickFrequency()
					<< endl;

		vector<unsigned char> outputBitstream;
		outputBitstream.reserve(headerBitstream->size() + msgBitstream->size());
		outputBitstream.insert(outputBitstream.end(), headerBitstream->begin(),
				headerBitstream->end());
		outputBitstream.insert(outputBitstream.end(), msgBitstream->begin(),
				msgBitstream->end());

		boost::system::error_code error;

		write(*_socket,
				boost::asio::buffer(outputBitstream, outputBitstream.size()),
				error);

		delete msgBitstream;

#ifndef GUI
		if (notifySendEnd)
			NodeNetworkSystem::messageSent();
#endif
		if (error) {
			_interface->errorHandler(this, error);
			return;
		}
	}

	if (_DEBUG)
		cout << "Session::_sendMessageThread: End" << endl;

}

