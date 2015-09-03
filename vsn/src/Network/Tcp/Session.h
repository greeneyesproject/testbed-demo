#ifndef SRC_NETWORK_TCP_SESSION_H_
#define SRC_NETWORK_TCP_SESSION_H_

#define RETRY_SECONDS 5

#include <Network/MessageParser.h>
#include <Network/Queue.h>

#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/smart_ptr.hpp>
#include <queue>

class Message;
class Session;

class SessionInterface {
public:

	virtual void errorHandler(Session* /*session*/,
			const boost::system::error_code& /*error*/) {

	}

	virtual void messageHandler(Session* /*session*/, Message* /*msg*/) {

	}

	virtual ~SessionInterface() {
	}

};

class Session {

public:
	Session(boost::asio::io_service& ioService, SessionInterface* interface);

	boost::shared_ptr<boost::asio::ip::tcp::socket> getSocket() {
		return _socket;
	}

	void closeSocket();

	bool startReceiver();

	void flushOutgoingMessages() {
		_messagesQueue.flush();
	}

	void writeMessage(Message* msg);

	virtual ~Session();

private:

	static const uchar _DEBUG = 2;

	void _readHeader();

	void _handleReadHeader(const boost::system::error_code& error,
			const size_t bytes_transferred);

	void _readMessage();
	void _handleReadMessage(const boost::system::error_code& error,
			const size_t bytes_transferred);

	void _sendMessageThread();

	/**
	 * socket created when the class is instantiated.
	 */
	boost::shared_ptr<boost::asio::ip::tcp::socket> _socket;

	boost::thread* _sendThread;

	long unsigned _tick;

	/**
	 * Class that implements session callbacks
	 */
	SessionInterface* _interface;

	Bitstream* _headerBitstream;
	Bitstream* _messageBitstream;

	Header* _header;

	Queue<Message> _messagesQueue;

	bool _deleted;

	float _txTimeCTA;
	float _txTimeATC;

};

#endif /* SESSION_H_ */
