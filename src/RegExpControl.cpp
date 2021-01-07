#ifdef USE_BOOST

#include "RegExpControl.h"

#include <boost/regex.hpp>
#include <boost/algorithm/string/replace.hpp>

std::vector<std::u16string> RegExpComponent::names = {
	AddComponent(u"RerExp1C", []() { return new RegExpComponent; }),
};

RegExpComponent::RegExpComponent()
{
	AddProperty(
		u"Text", u"Текст",
		[&](VH var) { var = this->getTestString(); },
		[&](VH var) { this->setTestString(var); });

	AddProperty(
		u"Number", u"Число",
		[&](VH var) { var = this->value; },
		[&](VH var) { this->value = var; });

	AddFunction(u"GetText", u"ПолучитьТекст", [&]() { this->result = this->getTestString(); });

	AddProcedure(u"SetText", u"УстановитьТекст", [&](VH par) { this->setTestString(par); }, {{0, u"default: "}});
}

std::u16string RegExpComponent::getTestString()
{
	return text;
}

void RegExpComponent::setTestString(const std::u16string &text)
{
	this->text = text;
}

#endif //USE_BOOST
