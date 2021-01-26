#include "GherkinParser.h"
#include "gherkin.lex.h"
#include <fstream>
#include <stdio.h>

std::vector<std::u16string> GherkinParser::names = {
	AddComponent(u"GherkinParser", []() { return new GherkinParser; }),
};

GherkinParser::GherkinParser()
{
	AddProperty(u"Keywords", u"КлючевыеСлова", nullptr, [&](VH value) { GherkinProvider::setKeywords(value); });
	AddFunction(u"Parse", u"Прочитать", [&](VH filename) { this->result = this->Parse(filename); });
}

std::string GherkinParser::Parse(const std::string& filename)
{
	reflex::Input input = fopen(filename.c_str(), "r");
	GherkinLexer lexer(input);
	lexer.lex();
	fclose(input.file());
	return lexer.dump();
}
