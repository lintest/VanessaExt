#pragma once

#ifdef USE_BOOST

#include <string>

class WebSocketBase {
public:
	virtual ~WebSocketBase() = default;
	static WebSocketBase* create();
	virtual bool open(const std::string& url, std::string& res) = 0;
	virtual bool send(const std::string& msg, std::string& res) = 0;
};

#endif//USE_BOOST
