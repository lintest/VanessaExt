#include "WebSocket.h"

#ifdef _WINDOWS
#define _WIN32_WINNT 0x0601
#endif

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

// Sends a WebSocket message 
bool doWebSocket(std::string& url, std::string& msg, std::string& res)
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

		// The io_context is required for all I/O
		net::io_context ioc;

		// These objects perform our I/O
		tcp::resolver resolver{ ioc };
		websocket::stream<tcp::socket> ws{ ioc };

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
		ss << "Error: " << e.what();
		res = ss.str();
		return false;
	}
	return true;
}
