#ifndef SRC_NETWORK_TCP_CLIENT_H_
#define SRC_NETWORK_TCP_CLIENT_H_

#include <Network/Tcp/Session.h>
#include <Network/Tcp/ServerClientInterface.h>
#include <boost/asio.hpp>

class Client: public SessionInterface {
public:
	Client(boost::asio::io_service& ioService, ServerClientInterface* interface,
			std::string remoteIp, unsigned short remotePort,
			unsigned short localPort = 0);
	virtual ~Client();

	void flushOutgoingMessages() {
		_session.flushOutgoingMessages();
	}

	Session* getSession() {
		return &_session;
	}

	void errorHandler(Session* session, const boost::system::error_code& error);

	void messageHandler(Session* session, Message* msg) {
		_interface->clientMessageHandler(session, msg);
	}

private:

	static const uchar _DEBUG = 2;

	void _connect(const boost::system::error_code& error =
			boost::system::errc::make_error_code(boost::system::errc::success));
	void _handleConnect(const boost::system::error_code& error);
	void _reconnect();

	/**
	 * io_service provided by the caller
	 */
	boost::asio::io_service& _ioService;

	ServerClientInterface* _interface;

	boost::asio::ip::tcp::endpoint _remoteEndpoint;

	boost::asio::deadline_timer _retryTimer;

	unsigned short _localPort;

	Session _session;

};

#endif /* SRC_TCPSYSTEM_CLIENT_H_ */
