#include <Network/Tcp/Server.h>

#include <boost/bind.hpp>
#include <boost/asio.hpp>

#include <iostream>

using namespace std;
using namespace boost::asio::ip;

Server::Server(boost::asio::io_service& ioService,
		ServerClientInterface* interface, unsigned short localPort) :
		_ioService(ioService), _acceptor(_ioService), _retryTimer(_ioService,
				boost::posix_time::seconds(RETRY_SECONDS)) {
	if (_DEBUG)
		cout << "Server::Server" << endl;
	_interface = interface;
	_localPort = localPort;
	_acceptor.open(tcp::v4());
	_bind();
}

/**
 * Close all connections
 */
Server::~Server() {
	if (_DEBUG)
		cout << "Server::~Server" << endl;
	boost::system::error_code ec;

	if (_DEBUG)
		cout << "Server::~Server: stop sessions" << endl;
	for (set<Session*>::iterator it = _sessions.begin(); it != _sessions.end();
			++it) {
		_stop(*it);
	}

	if (_DEBUG)
		cout << "Server::~Server: cancel" << endl;
	_acceptor.cancel(ec);
	if (_DEBUG)
		cout << "Server::~Server: close" << endl;
	_acceptor.close(ec);
}

void Server::_bind(const boost::system::error_code& error) {
	if (_DEBUG)
		cout << "Server::_bind" << endl;
	try {
		_acceptor.bind(tcp::endpoint(tcp::v4(), _localPort));
		_acceptor.set_option(tcp::acceptor::reuse_address(true));
		_acceptor.listen();
		startAccept();
	} catch (exception &e) {
		if (_DEBUG)
			cerr << "Server::_bind: " << e.what() << endl;
		_retryTimer.expires_from_now(boost::posix_time::seconds(RETRY_SECONDS));
		_retryTimer.async_wait(
				boost::bind(&Server::_bind, this,
						boost::asio::placeholders::error));
	}
}

/**
 * Started once at class instantiation
 * When the first client connects successfully this method must be called again to listen for other connections
 */
void Server::startAccept(const boost::system::error_code& error) {
	if (_DEBUG)
		cout << "Server::_startAccept" << endl;
	try {
		Session* newSession(new Session(_ioService, this));
		_acceptor.async_accept(*(newSession->getSocket()),
				boost::bind(&Server::_handleAccept, this, newSession,
						boost::asio::placeholders::error));
	} catch (exception &e) {
		if (_DEBUG)
			cerr << "Server::_startAccept: " << e.what() << endl;
		_retryTimer.expires_from_now(boost::posix_time::seconds(RETRY_SECONDS));
		_retryTimer.async_wait(
				boost::bind(&Server::_bind, this,
						boost::asio::placeholders::error));
	}
}

/**
 * Does not call startAccept again in case of successful connection
 */
void Server::_handleAccept(Session* session,
		const boost::system::error_code& error) {
	if (_DEBUG)
		cout << "Server::_handleAccept" << endl;
	if (error) {
		if (_DEBUG)
			cerr << "Server::_handleAccept: " << error.message() << endl;
		_retryTimer.expires_from_now(boost::posix_time::seconds(RETRY_SECONDS));
		_retryTimer.async_wait(
				boost::bind(&Server::_bind, this,
						boost::asio::placeholders::error));
	} else {
		if (session->startReceiver()) {
			_sessions.insert(session);
			_interface->serverAddSessionHandler(session);
			return;
		} else {
			if (_DEBUG)
				cerr << "Server::_handleAccept: can't start session's receiver"
						<< endl;
		}
	}
	startAccept();
}

void Server::errorHandler(Session* session,
		const boost::system::error_code& error) {
	if (_DEBUG)
		cerr << "Server::errorHandler: " << error.message() << endl;
	switch (error.value()) {
	case boost::system::errc::operation_canceled:
		break;
	default:
		if (_DEBUG)
			cout << "Server::errorHandler: Stop and remove session" << endl;
		_interface->serverRemoveSessionHandler(session);
		_sessions.erase(session);
		_stop(session);
	}
}

void Server::flushOutgoingMessages() {
	for (set<Session*>::iterator it = _sessions.begin(); it != _sessions.end();
			++it) {
		(*it)->flushOutgoingMessages();
	}
}

/**
 * Delete the session
 */
void Server::_stop(Session* session) {
	if (_DEBUG)
		cout << "Server::_stop" << endl;
	delete session;
}

