#ifndef SRC_NETWORK_TCP_SERVER_H_
#define SRC_NETWORK_TCP_SERVER_H_

#include <Network/Tcp/Session.h>
#include <Network/Tcp/ServerClientInterface.h>
#include <boost/asio.hpp>

#include <set>

class Server: public SessionInterface {
public:
	Server(boost::asio::io_service& ioService, ServerClientInterface* interface,
			unsigned short localPort);
	virtual ~Server();

	void errorHandler(Session* session, const boost::system::error_code& error);

	void flushOutgoingMessages();

	void messageHandler(Session* session, Message* msg) {
		_interface->serverMessageHandler(session, msg);
	}

	void startAccept(const boost::system::error_code& error =
			boost::system::errc::make_error_code(boost::system::errc::success));

private:

	static const uchar _DEBUG = 2;

	void _bind(const boost::system::error_code& error =
			boost::system::errc::make_error_code(boost::system::errc::success));

	void _handleAccept(Session* session,
			const boost::system::error_code& error);
	void _stop(Session* session);

	/**
	 * io_service provided by the caller
	 */
	boost::asio::io_service& _ioService;
	boost::asio::ip::tcp::acceptor _acceptor;

	ServerClientInterface* _interface;

	unsigned short _localPort;

	std::set<Session*> _sessions;

	boost::asio::deadline_timer _retryTimer;

};

#endif /* SRC_TCPSYSTEM_SERVER_H_ */
