#include "gherkin.h"
#include "gherkin.lex.h"
#include <reflex/matcher.h>
#include <fstream>
#include <codecvt>

std::vector<GherkinKeword> GherkinProvider::keywords;

void GherkinProvider::setKeywords(const std::string& text)
{
	auto json = JSON::parse(text);
	keywords.clear();
	for (auto lang = json.begin(); lang != json.end(); ++lang) {
		std::string language = lang.key();
		if ((language != "en") && (language != "ru")) continue;
		auto& types = lang.value();
		for (auto type = types.begin(); type != types.end(); ++type) {
			auto& words = type.value();
			if (words.is_array()) {
				for (auto word = words.begin(); word != words.end(); ++word) {
					keywords.push_back({ language, type.key(), *word });
				}
			}
		}
	}
	std::sort(keywords.begin(), keywords.end(),
		[](const GherkinKeword& a, const GherkinKeword& b) -> bool {
			return a.words.size() > b.words.size();
		}
	);
}

#ifdef USE_BOOST

#include <boost/algorithm/string.hpp>

static bool comparei(const std::wstring& a, const std::wstring& b)
{
	return boost::iequals(a, b);
}

#else//USE_BOOST

#ifdef _WINDOWS

static bool comparei(const std::wstring& a, const std::wstring& b)
{
	static _locale_t locale = _create_locale(LC_ALL, "ru-RU");
	auto res = _wcsnicmp_l(a.c_str(), b.c_str(), std::max(a.size(), b.size()), locale);
	return res == 0;
}

#include <string.h>

#else//_WINDOWS

static bool comparei(std::wstring a, std::wstring b)
{
	transform(a.begin(), a.end(), a.begin(), toupper);
	transform(b.begin(), b.end(), b.begin(), toupper);
	return (a == b);
}

#endif//_WINDOWS

#endif//USE_BOOST

GherkinKeword* GherkinProvider::matchKeyword(const GherkinLine& line)
{
	for (auto& keyword : keywords) {
		auto matched = keyword.matchKeyword(line);
		if (matched) return matched;
	}
	return nullptr;
}

std::string GherkinProvider::ParseFile(const std::wstring& filename)
{
#ifdef _WINDOWS
	FILE* file = _wfopen(filename.c_str(), L"rb");
#else
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	FILE* file = fopen(converter.to_bytes(filename).c_str(), "rb");
#endif
	reflex::Input input = file;
	GherkinLexer lexer(input);
	lexer.lex();
	fclose(file);
	return lexer.dump();
}

GherkinKeword::GherkinKeword(const std::string& lang, const std::string& type, const std::string& word)
	: lang(lang), type(type), text(word)
{
	static const std::string regex = reflex::Matcher::convert("\\w+", reflex::convert_flag::unicode);
	static const reflex::Pattern pattern(regex);
	auto matcher = reflex::Matcher(pattern, word);
	while (matcher.find() != 0) {
		words.push_back(matcher.wstr());
	}
}

GherkinKeword* GherkinKeword::matchKeyword(const GherkinLine& line)
{
	if (words.size() > line.tokens.size()) return nullptr;
	for (size_t i = 0; i < words.size(); ++i) {
		if (line.tokens[i].type != Gherkin::Operator) return nullptr;
		if (!comparei(words[i], line.tokens[i].wstr)) return nullptr;
	}
	return this;
}

GherkinKeword::operator JSON() const
{
	JSON json;
	json["lang"] = lang;
	json["text"] = text;
	json["type"] = type;
	return json;
}

std::string GherkinToken::trim(const std::string& text)
{
	static const std::string regex = reflex::Matcher::convert("\\S[\\s\\S]*\\S|\\S", reflex::convert_flag::unicode);
	static const reflex::Pattern pattern(regex);
	auto matcher = reflex::Matcher(pattern, text);
	return matcher.find() ? matcher.text() : std::string();
}

GherkinToken::GherkinToken(Gherkin::TokenType t, GherkinLexer& l)
	: type(t), wstr(l.wstr()), text(l.text()), columno(l.columno()) 
{
	text = trim(text);
};

GherkinToken::operator JSON() const
{
	JSON json;
	json["type"] = type2str();
	json["text"] = text;
	return json;
}

std::string GherkinToken::type2str() const
{
	switch (type) {
	case Gherkin::Operator: return "operator";
	case Gherkin::Comment: return "comment";
	case Gherkin::Number: return "number";
	case Gherkin::Colon: return "colon";
	case Gherkin::Param: return "param";
	case Gherkin::Table: return "table";
	case Gherkin::Cell: return "cell";
	case Gherkin::Date: return "date";
	case Gherkin::Text: return "text";
	case Gherkin::Tag: return "tag";
	case Gherkin::Symbol: return "symbol";
	default: return "none";
	}
}

GherkinLine::GherkinLine(GherkinLexer& l)
	: lineNumber(l.lineno()) {}

void GherkinLine::push(Gherkin::TokenType t, GherkinLexer& l)
{
	tokens.push_back({ t, l });
}

JSON& GherkinLine::dump(JSON& json, GherkinKeword* keyword) const
{
	JSON js;
	json["keyword"] = *keyword;
	auto& words = keyword->words;
	int keynum = words.end() - words.begin();
	for (auto& t : tokens) {
		JSON j = t;
		if (keynum > 0) {
			j["type"] = "keyword";
		}
		else if (keynum == 0) {
			if (t.type == Gherkin::Colon) {
				json["toplevel"] = true;
			}
		}
		js.push_back(j);
		--keynum;
	}
	json["tokens"] = js;
	return json;
}

JSON& GherkinLine::dump(JSON& json) const
{
	JSON js;
	for (auto& t : tokens) {
		js.push_back(t);
	}
	json["tokens"] = js;
	return json;
}

GherkinLine::operator JSON() const
{
	JSON json, js;
	json["text"] = text;
	json["line"] = lineNumber;
	if (tokens.size() == 0) return json;
	auto keyword = GherkinProvider::matchKeyword(*this);
	if (keyword) return dump(json, keyword);
	return dump(json);
}

Gherkin::TokenType GherkinLine::type() const
{
	return tokens.empty() ? Gherkin::None : tokens.begin()->type;
}

JSON GherkinDocument::tags2json() const
{
	JSON json;
	for (auto& tag : tags()) {
		JSON j;
		j["key"] = tag.first;
		if (!tag.second.empty()) j["value"] = tag.second;
		json.push_back(j);
	}
	return json;
}

void GherkinDocument::push(Gherkin::TokenType t, GherkinLexer& l)
{
	if (current == nullptr) {
		lines.push_back({l});
		current = &lines.back();
		current->text = l.matcher().line();
	}
	current->push(t, l);
}

std::string GherkinDocument::dump() const
{
	JSON json, j;
	for (auto& line : lines) {
		j.push_back(line);
	}
	json["lines"] = j;
	json["tags"] = tags2json();
	return json.dump();
}

GherkinTags GherkinDocument::tags() const
{
	GherkinTags result;
	for (auto& line : lines) {
		if (line.type() == Gherkin::Tag) {
			std:: string key, value;
			for (auto& token : line.tokens) {
				switch (token.type) {
					case Gherkin::Operator: key = token.text; break;
					case Gherkin::Text: value = token.text; break;
				}
			}
			result.push_back({ key, value });
		}
	}
	return result;
}
