#include "GherkinParser.h"
#include "gherkin.lex.h"

std::vector<std::u16string> GherkinParser::names = {
	AddComponent(u"GherkinParser", []() { return new GherkinParser; }),
};

GherkinParser::GherkinParser()
{
	AddProperty(u"Keywords", u"КлючевыеСлова", nullptr, [&](VH value) { GherkinProvider::setKeywords(value); });
	AddFunction(u"ParseFile", u"ПрочитатьФайл", [&](VH filename) {  this->result = GherkinProvider::ParseFile(filename); });
}
