#include <Network/Tcp/Client.h>

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <iostream>

using namespace std;
using namespace boost::asio::ip;

Client::Client(boost::asio::io_service& ioService,
		ServerClientInterface* interface, std::string remoteIp,
		unsigned short remotePort, unsigned short localPort) :
		_ioService(ioService), _remoteEndpoint(
				boost::asio::ip::address_v4::from_string(remoteIp), remotePort), _retryTimer(
				_ioService, boost::posix_time::seconds(RETRY_SECONDS)), _session(
				ioService, this) {
	if (_DEBUG)
		cout << "Client::Client" << endl;
	_interface = interface;
	_localPort = localPort;
	_connect();
}

/**
 * Close session
 */
Client::~Client() {
	if (_DEBUG)
		cout << "Client::~Client" << endl;
	/* _session is deleted since is an attribute of Client */
}

void Client::_connect(const boost::system::error_code& error) {
	if (_DEBUG)
		cout << "Client::_connect" << endl;
	if (error) {
		if (_DEBUG)
			cerr << "Client::_connect: " << error.message() << endl;
		if (error == boost::system::errc::operation_canceled){
			return;
		}
	}
	if (_localPort) {
		try {
			/* Bind to local port */
			if (!_session.getSocket()->is_open()) {
				_session.getSocket()->open(tcp::v4());
			}
			if (_DEBUG)
				cout << "Client::_connect: binding to local port " << _localPort
						<< endl;
			_session.getSocket()->set_option(tcp::socket::reuse_address(true));
			_session.getSocket()->bind(tcp::endpoint(tcp::v4(), _localPort));
		} catch (exception &e) {
			if (_DEBUG)
				cerr << "Client::_connect: " << e.what() << endl;
			_retryTimer.expires_from_now(
					boost::posix_time::seconds(RETRY_SECONDS));
			_retryTimer.async_wait(
					boost::bind(&Client::_connect, this,
							boost::asio::placeholders::error));
			return;
		}
	}
	if (_DEBUG)
		cout << "Client::_connect: Connecting to " << _remoteEndpoint << endl;
	_session.getSocket()->async_connect(_remoteEndpoint,
			boost::bind(&Client::_handleConnect, this,
					boost::asio::placeholders::error));
}

void Client::_handleConnect(const boost::system::error_code& error) {
	if (_DEBUG)
		cout << "Client::_handleConnect" << endl;
	if (error) {
		if (_DEBUG)
			cerr << "Client::_handleConnect: " << error.message() << endl;
		_reconnect();
		return;
	}
	if (_session.startReceiver()) {
		if (_DEBUG)
			cout << "Client::_handleConnect: Connected to "
					<< _session.getSocket()->remote_endpoint() << " from "
					<< _session.getSocket()->local_endpoint() << endl;
		_interface->clientConnectHandler(&_session);
	} else {
		_reconnect();
	}
}

void Client::_reconnect() {
	if (_DEBUG)
		cout << "Client::_reconnect" << endl;
	_session.closeSocket();
	boost::system::error_code ec;
	_retryTimer.cancel(ec);
	_retryTimer.expires_from_now(boost::posix_time::seconds(RETRY_SECONDS), ec);
	_retryTimer.async_wait(
			boost::bind(&Client::_connect, this,
					boost::asio::placeholders::error));
}

void Client::errorHandler(Session* session,
		const boost::system::error_code& error) {
	if (_DEBUG)
		cerr << "Client::errorHandler: " << error.message() << endl;
	switch (error.value()) {
	case boost::system::errc::operation_canceled:
		break;
	default:
		_reconnect();
	}
}
