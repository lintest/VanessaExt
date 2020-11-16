#include "WebSocket.h"

#ifdef _WINDOWS
#define _WIN32_WINNT 0x0601
#endif //_WINDOWS

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/regex.hpp>
#include <cstdlib>
#include <iostream>
#include <string>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

class WebSocket
	: public WebSocketBase
{
private:
	net::io_context ioc;
	tcp::resolver resolver{ ioc };
	websocket::stream<tcp::socket> ws{ ioc };
public:
	virtual ~WebSocket() {}
	bool open(const std::string& url, std::string& res) override;
	bool send(const std::string& msg, std::string& res) override;
};

WebSocketBase* WebSocketBase::create()
{
	return new WebSocket;
}

#ifdef _WINDOWS

std::string cp1251_to_utf8(const char* str) {
	std::string res;
	int result_u, result_c;
	result_u = MultiByteToWideChar(1251, 0, str, -1, 0, 0);
	if (!result_u) { return 0; }
	wchar_t* ures = new wchar_t[result_u];
	if (!MultiByteToWideChar(1251, 0, str, -1, ures, result_u)) {
		delete[] ures;
		return 0;
	}
	result_c = WideCharToMultiByte(65001, 0, ures, -1, 0, 0, 0, 0);
	if (!result_c) {
		delete[] ures;
		return 0;
	}
	char* cres = new char[result_c];
	if (!WideCharToMultiByte(65001, 0, ures, -1, cres, result_c, 0, 0)) {
		delete[] cres;
		return 0;
	}
	delete[] ures;
	res.append(cres);
	delete[] cres;
	return res;
}

#else

std::string cp1251_to_utf8(const char* str)
{
	return str;
}

#endif //_WINDOWS

bool WebSocket::open(const std::string& url, std::string& res)
{
	try {
		boost::regex ex("(ws)://([^/ :]+):?([^/ ]*)(/?[^ #?]*)");
		boost::cmatch what;
		if (!regex_match(url.c_str(), what, ex)) {
			std::stringstream ss;
			ss << "Wrong URL: " << url;
			res = ss.str();
			return false;
		}

		auto const host = std::string(what[2].first, what[2].second);
		auto const port = std::string(what[3].first, what[3].second);
		auto const path = std::string(what[4].first, what[4].second);

		// Look up the domain name
		auto const results = resolver.resolve(host, port);

		// Make the connection on the IP address we get from a lookup
		net::connect(ws.next_layer(), results.begin(), results.end());

		// Set a decorator to change the User-Agent of the handshake
		ws.set_option(websocket::stream_base::decorator(
			[](websocket::request_type& req)
			{
				req.set(http::field::user_agent,
					std::string(BOOST_BEAST_VERSION_STRING) +
					" websocket-client-coro");
			}));

		// Perform the websocket handshake
		ws.handshake(host, path);
	}
	catch (std::exception const& e)
	{
		std::stringstream ss;
		ss << "Error: " << cp1251_to_utf8(e.what());
		res = ss.str();
		return false;
	}
	return true;
}

bool WebSocket::send(const std::string& msg, std::string& res)
{
	try {
		// Send the message
		ws.write(net::buffer(msg));

		// This buffer will hold the incoming message
		beast::flat_buffer buffer;

		// Read a message into our buffer
		ws.read(buffer);

		// The make_printable() function helps print a ConstBufferSequence
		std::stringstream ss;
		ss << beast::make_printable(buffer.data());
		res = ss.str();
	}
	catch (std::exception const& e)
	{
		std::stringstream ss;
		ss << "Error: " << cp1251_to_utf8(e.what());
		res = ss.str();
		return false;
	}
	return true;
}

// Sends a WebSocket message 
bool doWebSocket(const std::string& url, std::string& msg, std::string& res)
{
	WebSocket ws;
	return ws.open(url, res) && ws.send(msg, res);
}
