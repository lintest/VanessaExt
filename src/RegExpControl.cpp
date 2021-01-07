#ifdef USE_BOOST

#include "RegExpControl.h"

#include <boost/regex.hpp>
#include <boost/algorithm/string/replace.hpp>

using namespace boost;

std::vector<std::u16string> RegExpComponent::names = {
	AddComponent(u"RegExp1C", []() { return new RegExpComponent; }),
};

RegExpComponent::RegExpComponent()
{
	AddFunction(u"Replace", u"Заменить", 
		[&](VH text, VH search, VH replace) { this->result = this->Replace(text, search, replace); }
	);
}

std::wstring RegExpComponent::Replace(const std::wstring& source, const std::wstring& search, const std::wstring& replace)
{
	return regex_replace(source, basic_regex(search), replace);
}

#endif //USE_BOOST
