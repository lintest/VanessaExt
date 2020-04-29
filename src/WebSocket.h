#pragma once

#ifndef __WEBSOCKET_H__
#define __WEBSOCKET_H__

#include <string>

class WebSocketBase {
public:
	virtual ~WebSocketBase() {}
	static WebSocketBase* create();
	virtual bool open(const std::string& url, std::string& res) = 0;
	virtual bool send(const std::string& msg, std::string& res) = 0;
};

bool doWebSocket(std::string& url, std::string& msg, std::string& res);

#endif //__WEBSOCKET_H__