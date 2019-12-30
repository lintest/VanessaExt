#pragma once

#ifndef __JSON_EXT_H__
#define __JSON_EXT_H__

#include "stdafx.h"

#include "json.hpp"

class JSON : public nlohmann::json
{
public:
	operator std::wstring() const {
		if (empty()) return {};
		return MB2WC(dump());
	}
};

#endif//__JSON_EXT_H__
