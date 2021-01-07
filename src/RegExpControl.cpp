#ifdef USE_BOOST

#include "RegExpControl.h"

#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>

using namespace boost;

std::vector<std::u16string> RegExpComponent::names = {
	AddComponent(u"RegExp1C", []() { return new RegExpComponent; }),
};

RegExpComponent::RegExpComponent()
{
	AddFunction(u"Replace", u"Заменить",
		[&](VH text, VH search, VH replace) { this->result = this->Replace(text, search, replace); }
	);
	AddFunction(u"Split", u"Разделить",
		[&](VH text, VH search) { this->result = this->Split(text, search); }
	);
}

std::wstring RegExpComponent::Replace(const std::wstring& source, const std::wstring& search, const std::wstring& replace)
{
	return regex_replace(source, basic_regex(search), replace);
}

std::wstring RegExpComponent::Split(const std::wstring& source, const std::wstring& search)
{
	JSON json;
	std::vector<std::wstring> result;
	split_regex(result, source, basic_regex(search));
	for (auto& line : result) json.push_back(WC2MB(line));
	return MB2WC(json.dump());
}

#endif //USE_BOOST
