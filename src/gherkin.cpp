#include "gherkin.h"
#include "gherkin.lex.h"
#include <stdafx.h>
#include <codecvt>
#include <locale>
#include <stdio.h>
#include <reflex/matcher.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>

#ifdef USE_BOOST

#include <boost/algorithm/string.hpp>

static bool comparei(const std::wstring& a, const std::wstring& b)
{
	static const std::locale locale_ru("ru_RU.UTF-8");
	return boost::iequals(a, b, locale_ru);
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

static FILE* fileopen(const std::wstring& filepath)
{
#ifdef _WINDOWS
	return _wfopen(filepath.c_str(), L"rb");
#else
	return fopen(WC2MB(filepath).c_str(), "rb");
#endif
}

namespace Gherkin {

	static std::string trim(const std::string& text)
	{
		static const std::string regex = reflex::Matcher::convert("\\S[\\s\\S]*\\S|\\S", reflex::convert_flag::unicode);
		static const reflex::Pattern pattern(regex);
		auto matcher = reflex::Matcher(pattern, text);
		return matcher.find() ? matcher.text() : std::string();
	}

	GherkinProvider::Keyword::Keyword(KeywordType type, const std::string& text)
		:type(type), text(text)
	{
		static const std::string regex = reflex::Matcher::convert("\\w+", reflex::convert_flag::unicode);
		static const reflex::Pattern pattern(regex);
		auto matcher = reflex::Matcher(pattern, text);
		while (matcher.find() != 0) {
			words.push_back(matcher.wstr());
		}
	}

	GherkinKeyword* GherkinProvider::Keyword::match(GherkinTokens& tokens) const
	{
		if (words.size() > tokens.size())
			return nullptr;

		for (size_t i = 0; i < words.size(); ++i) {
			if (tokens[i].type != TokenType::Operator)
				return nullptr;

			if (!comparei(words[i], tokens[i].wstr))
				return nullptr;
		}

		bool toplevel = false;
		size_t keynum = words.end() - words.begin();
		for (auto& t : tokens) {
			if (keynum > 0) {
				t.type = TokenType::Keyword;
				keynum--;
			}
			else {
				if (t.type == TokenType::Colon)
					toplevel = true;
				break;
			}
		}

		return new GherkinKeyword(*this, toplevel);
	}

	std::string GherkinProvider::getKeywords() const
	{
		JSON json;
		for (auto& language : keywords) {
			JSON js;
			for (auto& keyword : language.second) {
				auto type = GherkinKeyword::type2str(keyword.type);
				js[type].push_back(keyword.text);
			}
			json[language.first] = js;
		}
		return json.dump();
	}

	void GherkinProvider::setKeywords(const std::string& text)
	{
		auto json = JSON::parse(text);
		keywords.clear();
		for (auto lang = json.begin(); lang != json.end(); ++lang) {
			std::string language = lang.key();
			if ((language != "en") && (language != "ru")) continue; // TODO: remove this line
			auto& vector = keywords[language];
			auto& types = lang.value();
			for (auto type = types.begin(); type != types.end(); ++type) {
				KeywordType t = GherkinKeyword::str2type(type.key());
				auto& words = type.value();
				if (words.is_array()) {
					for (auto word = words.begin(); word != words.end(); ++word) {
						std::string text = trim(*word);
						if (text == "*") continue;
						vector.push_back({ t, *word });
					}
				}
			}
			std::sort(vector.begin(), vector.end(),
				[](const Keyword& a, const Keyword& b) -> bool { return a.comp(b); }
			);
		}
	}

	GherkinKeyword* GherkinProvider::matchKeyword(const std::string& lang, GherkinTokens& tokens) const
	{
		std::string language = lang.empty() ? std::string("ru") : lang;
		for (auto& keyword : keywords.at(language)) {
			auto matched = keyword.match(tokens);
			if (matched) return matched;
		}
		return nullptr;
	}

	std::string GherkinProvider::ParseFolder(const std::wstring& root) const
	{
		JSON json;
		const std::wstring mask = L"^.+\\.feature$";
		boost::wregex pattern(mask, boost::regex::icase);
		boost::filesystem::recursive_directory_iterator end_itr; // Default ctor yields past-the-end
		for (boost::filesystem::recursive_directory_iterator i(root); i != end_itr; ++i) {
			if (boost::filesystem::is_regular_file(i->status())) {
				boost::wsmatch what;
				std::wstring path = i->path().wstring();
				std::wstring name = i->path().filename().wstring();
				if (!boost::regex_match(name, what, pattern)) continue;
				std::unique_ptr<FILE, decltype(&fclose)> file(fileopen(path), &fclose);
				reflex::Input input(file.get());
				GherkinDocument doc(*this);
				GherkinLexer lexer(input);
				auto j = lexer.parse(doc);
				j["filepath"] = WC2MB(path);
				json.push_back(j);
			}
		}
		return json.dump();
	}

	std::string GherkinProvider::ParseFile(const std::wstring& path) const
	{
		std::unique_ptr<FILE, decltype(&fclose)> file(fileopen(path), &fclose);
		reflex::Input input(file.get());
		GherkinDocument doc(*this);
		GherkinLexer lexer(input);
		return lexer.parse(doc).dump();
	}

	std::string GherkinProvider::ParseText(const std::string& text) const
	{
		reflex::Input input(text);
		GherkinDocument doc(*this);
		GherkinLexer lexer(input);
		return lexer.parse(doc).dump();
	}

	KeywordType GherkinKeyword::str2type(const std::string& text)
	{
		std::string type = text;
		transform(type.begin(), type.end(), type.begin(), tolower);
		static std::map<std::string, KeywordType> types{
			{ "and", KeywordType::And},
			{ "background", KeywordType::Background },
			{ "but", KeywordType::But },
			{ "examples", KeywordType::Examples },
			{ "feature", KeywordType::Feature },
			{ "given", KeywordType::Given },
			{ "rule", KeywordType::Rule },
			{ "scenario", KeywordType::Scenario },
			{ "scenariooutline", KeywordType::ScenarioOutline },
			{ "then", KeywordType::Then },
			{ "when", KeywordType::When },
		};
		return types.count(type) ? types[type] : KeywordType::None;
	}

	std::string GherkinKeyword::type2str(KeywordType type)
	{
		switch (type) {
		case KeywordType::And: return "And";
		case KeywordType::Background: return "Background";
		case KeywordType::But: return "But";
		case KeywordType::Examples: return "Then";
		case KeywordType::Feature: return "Feature";
		case KeywordType::Given: return "Given";
		case KeywordType::Scenario: return "Scenario";
		case KeywordType::ScenarioOutline: return "ScenarioOutline";
		case KeywordType::Rule: return "Rule";
		case KeywordType::Then: return "Then";
		case KeywordType::When: return "When";
		default: return {};
		}
	}

	GherkinKeyword::operator JSON() const
	{
		JSON json;
		json["text"] = text;
		json["type"] = type2str(type);
		if (toplevel) json["toplevel"] = toplevel;
		return json;
	}

	GherkinToken::GherkinToken(GherkinLexer& lexer, TokenType type, char ch)
		: type(type), wstr(lexer.wstr()), text(lexer.text()), column(lexer.columno()), symbol(ch)
	{
		if (ch != 0) {
			bool escaping = false;
			std::wstringstream ss;
			for (auto it = wstr.begin(); it != wstr.end(); ++it) {
				if (it == wstr.begin() || (it + 1) == wstr.end())
					continue;

				if (escaping) {
					escaping = false;
					wchar_t wc = *it;
					switch (wc) {
					case L'\"': ss << L'\"'; break;
					case L'\'': ss << L'\''; break;
					case L't': ss << L'\t'; break;
					case L'n': ss << L'\n'; break;
					case L'r': ss << L'\r'; break;
					default:
						if (lexer.isPrimitiveEscaping())
							ss << L'\\';
						ss << wc;
					}
				}
				else {
					if (*it == L'\\')
						escaping = true;
					else
						ss << *it;
				}
			}
			wstr = ss.str();
			text = WC2MB(wstr);
		}
		else {
			text = trim(text);
			wstr = MB2WC(text);
		}
	};

	GherkinToken::operator JSON() const
	{
		JSON json;
		json["text"] = text;
		json["column"] = column;
		json["type"] = type2str();

		if (symbol != 0)
			json["symbol"] = std::string(1, symbol);

		return json;
	}

	std::string GherkinToken::type2str() const
	{
		switch (type) {
		case TokenType::Language: return "Language";
		case TokenType::Encoding: return "Encoding";
		case TokenType::Asterisk: return "Asterisk";
		case TokenType::Operator: return "Operator";
		case TokenType::Comment: return "Comment";
		case TokenType::Keyword: return "Keyword";
		case TokenType::Number: return "Number";
		case TokenType::Colon: return "Colon";
		case TokenType::Param: return "Param";
		case TokenType::Table: return "Table";
		case TokenType::Cell: return "Cell";
		case TokenType::Line: return "Line";
		case TokenType::Date: return "Date";
		case TokenType::Text: return "Text";
		case TokenType::Tag: return "Tag";
		case TokenType::Symbol: return "Symbol";
		case TokenType::Multiline: return "Multiline";
		default: return "None";
		}
	}

	GherkinLine::GherkinLine(GherkinLexer& l)
		: lineNumber(l.lineno()), text(l.matcher().line())
	{
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		wstr = converter.from_bytes(text);
	}

	GherkinLine::GherkinLine(size_t lineNumber)
		: lineNumber(lineNumber)
	{
	}

	void GherkinLine::push(GherkinLexer& lexer, TokenType type, char ch)
	{
		tokens.emplace_back(lexer, type, ch);
	}

	GherkinKeyword* GherkinLine::matchKeyword(GherkinDocument& document)
	{
		if (tokens.size() == 0) return nullptr;
		if (tokens.begin()->type != TokenType::Operator) return nullptr;
		keyword.reset(document.matchKeyword(tokens));
		//TODO: check does colon exists for top level keywords: Feature, Background, Scenario...
		return keyword.get();
	}

	GherkinLine::operator JSON() const
	{
		JSON json, js;
		for (auto& t : tokens) {
			js.push_back(t);
		}
		json["tokens"] = js;
		json["text"] = text;
		json["line"] = lineNumber;
		if (keyword) json["keyword"] = *keyword;
		return json;
	}

	Gherkin::TokenType GherkinLine::getType() const
	{
		return tokens.empty() ? TokenType::None : tokens.begin()->type;
	}

	int GherkinLine::getIndent() const
	{
		int indent = 0;
		const int tabSize = 4;
		for (auto ch : text) {
			switch (ch) {
			case ' ':
				indent++;
				break;
			case '\t':
				indent = indent + tabSize - (indent % tabSize);
				break;
			default:
				return indent;
			}
		}
		return INT_MAX;
	}

	GherkinTable::GherkinTable(const GherkinLine& line)
		: lineNumber(line.getLineNumber())
	{
		for (auto& token : line.getTokens()) {
			if (token.getType() == TokenType::Cell)
				head.emplace_back(token.getText());
		}
	}

	void GherkinTable::push(const GherkinLine& line)
	{
		body.push_back({});
		auto& row = body.back();
		for (auto& token : line.getTokens()) {
			if (token.getType() == TokenType::Cell)
				row.emplace_back(token.getText());
		}
	}

	GherkinTable::operator JSON() const
	{
		JSON json;
		json["line"] = lineNumber;
		json["head"] = head;
		json["body"] = body;
		return json;
	}

	GherkinElement::GherkinElement(GherkinLexer& lexer, const GherkinLine& line)
		: wstr(line.getWstr()), text(line.getText()), lineNumber(line.getLineNumber())
	{
		comments = std::move(lexer.commentStack);
		tags = std::move(lexer.tagStack);
	}

	GherkinElement* GherkinElement::push(GherkinLexer& lexer, const GherkinLine& line)
	{
		GherkinElement* element = nullptr;
		switch (line.getType()) {
		case TokenType::Keyword:
			element = new GherkinStep(lexer, line);
			break;
		case TokenType::Asterisk:
		case TokenType::Operator:
		case TokenType::Symbol:
			element = new GherkinGroup(lexer, line);
			break;
		default:
			return nullptr;
		}
		items.emplace_back(element);
		return element;
	}

	GherkinTable* GherkinElement::pushTable(const GherkinLine& line)
	{
		tables.emplace_back(line);
		return &tables.back();
	}

	GherkinElement::operator JSON() const
	{
		JSON json;

		json["line"] = lineNumber;
		json["text"] = text;

		if (!items.empty()) {
			JSON js;
			for (auto& item : items)
				js.push_back(JSON(*item));

			json["items"] = js;
		}

		if (!tags.empty())
			json["tags"] = tags;

		if (!comments.empty())
			json["comments"] = comments;

		if (!tables.empty())
			json["tables"] = tables;

		return json;
	}

	GherkinFeature::GherkinFeature(GherkinLexer& lexer, const GherkinLine& line)
		: GherkinDefinition(lexer, line), keyword(*line.getKeyword())
	{
		std::string text = line.getText();
		static const std::string regex = reflex::Matcher::convert("[^:]+:\\s*", reflex::convert_flag::unicode);
		static const reflex::Pattern pattern(regex);
		auto matcher = reflex::Matcher(pattern, text);
		if (matcher.find() && matcher.size() < text.size()) {
			name = trim(text.substr(matcher.size()));
		}
	}

	GherkinElement* GherkinFeature::push(GherkinLexer& lexer, const GherkinLine& line)
	{
		description.emplace_back(trim(line.getText()));
		return nullptr;
	}

	GherkinFeature::operator JSON() const
	{
		JSON json = GherkinElement::operator JSON();

		if (!name.empty())
			json["name"] = name;

		if (!description.empty())
			json["description"] = description;

		json["keyword"] = keyword;

		return json;
	}

	GherkinDefinition::GherkinDefinition(GherkinLexer& lexer, const GherkinLine& line)
		: GherkinElement(lexer, line), keyword(*line.getKeyword())
	{
	}

	GherkinDefinition::operator JSON() const
	{
		JSON json = GherkinElement::operator JSON();
		json["keyword"] = keyword;

		if (!tokens.empty())
			json["tokens"] = tokens;

		return json;
	}

	GherkinStep::GherkinStep(GherkinLexer& lexer, const GherkinLine& line)
		: GherkinElement(lexer, line), keyword(*line.getKeyword()), tokens(line.getTokens())
	{
	}

	GherkinStep::operator JSON() const
	{
		JSON json = GherkinElement::operator JSON();
		json["keyword"] = keyword;

		if (!tokens.empty())
			json["tokens"] = tokens;

		return json;
	}

	GherkinGroup::GherkinGroup(GherkinLexer& lexer, const GherkinLine& line)
		: GherkinElement(lexer, line), name(trim(line.getText()))
	{
	}

	GherkinGroup::operator JSON() const
	{
		JSON json = GherkinElement::operator JSON();
		json["name"] = name;
		return json;
	}

	GherkinError::GherkinError(GherkinLexer& lexer, const std::string& message)
		: line(lexer.lineno()), column(lexer.columno()), message(message)
	{
	}

	GherkinError::GherkinError(size_t line, const std::string& message)
		: line(line), column(0), message(message)
	{
	}

	GherkinError::operator JSON() const
	{
		JSON json;
		json["line"] = line;
		json["text"] = message;
		if (column) json["column"] = column;
		return json;
	}

	void GherkinDocument::setLanguage(GherkinLexer& lexer)
	{
		if (language.empty())
			language = trim(lexer.text());
		else
			error(lexer, "Language key duplicate error");
	}

	void GherkinDocument::resetElementStack(GherkinLexer& lexer, GherkinElement& element)
	{
		lexer.lastElement = &element;
		lexer.elementStack.clear();
		lexer.elementStack.emplace_back(-1, &element);
	}

	void GherkinDocument::setDefinition(std::unique_ptr<GherkinDefinition>& definition, GherkinLexer& lexer, GherkinLine& line)
	{
		if (definition) {
			auto keyword = line.getKeyword();
			if (keyword) {
				std::string type = GherkinKeyword::type2str(keyword->getType());
				error(line, type + " keyword duplicate error");
			}
			else
				error(line, "Unknown keyword type");
		}
		else {
			GherkinDefinition* def =
				line.getKeyword()->getType() == KeywordType::Feature
				? (GherkinDefinition*) new GherkinFeature(lexer, line)
				: new GherkinDefinition(lexer, line);
			definition.reset(def);
			resetElementStack(lexer, *def);
		}
	}

	void GherkinDocument::addScenarioDefinition(GherkinLexer& lexer, GherkinLine& line)
	{
		scenarios.emplace_back(new GherkinDefinition(lexer, line));
		resetElementStack(lexer, *scenarios.back().get());
	}

	GherkinKeyword* GherkinDocument::matchKeyword(GherkinTokens& line)
	{
		return provider.matchKeyword(language, line);
	}

	void GherkinDocument::exception(GherkinLexer& lexer, const char* message)
	{
		std::stringstream stream_message;
		stream_message << (message != NULL ? message : "lexer error") << " at " << lexer.lineno() << ":" << lexer.columno();
		throw MB2WCHAR(stream_message.str());
	}

	void GherkinDocument::error(GherkinLexer& lexer, const std::string& error)
	{
		errors.emplace_back(lexer, error);
	}

	void GherkinDocument::error(GherkinLine& line, const std::string& error)
	{
		errors.emplace_back(line.getLineNumber(), error);
	}

	void GherkinDocument::push(GherkinLexer& lexer, TokenType type, char ch)
	{
		if (lexer.currentLine == nullptr) {
			lexer.lines.push_back({ lexer });
			lexer.currentLine = &lexer.lines.back();
		}
		lexer.currentLine->push(lexer, type, ch);
		switch (type) {
		case TokenType::Language:
			setLanguage(lexer);
			break;
		case TokenType::Comment:
			lexer.commentStack.push_back(lexer.text());
			break;
		case TokenType::Tag:
			lexer.tagStack.push_back(lexer.text());
			break;
		}
	}

	void GherkinDocument::addTableLine(GherkinLexer& lexer, GherkinLine& line)
	{
		if (lexer.lastElement) {
			if (lexer.currentTable)
				lexer.currentTable->push(line);
			else {
				lexer.currentTable = lexer.lastElement->pushTable(line);
			}
		}
		else {
			//TODO: save error to error list
		}
	}

	void GherkinDocument::addElement(GherkinLexer& lexer, GherkinLine& line)
	{
		auto indent = line.getIndent();
		while (!lexer.elementStack.empty()) {
			if (lexer.elementStack.back().first < indent) break;
			lexer.elementStack.pop_back();
		}
		if (lexer.elementStack.empty()) {
			throw u"Element statck is empty";
		}
		auto parent = lexer.elementStack.back().second;
		if (auto element = parent->push(lexer, line)) {
			lexer.elementStack.emplace_back(indent, element);
			lexer.lastElement = element;
		}
	}

	void GherkinDocument::processLine(GherkinLexer& lexer, GherkinLine& line)
	{
		if (line.getType() != TokenType::Table)
			lexer.currentTable = nullptr;

		auto keyword = line.matchKeyword(*this);
		if (keyword) {
			switch (keyword->getType()) {
			case KeywordType::Feature:
				setDefinition(feature, lexer, line);
				break;
			case KeywordType::ScenarioOutline:
				setDefinition(outline, lexer, line);
				break;
			case KeywordType::Background:
				setDefinition(background, lexer, line);
				break;
			case KeywordType::Scenario:
				addScenarioDefinition(lexer, line);
				break;
			default:
				addElement(lexer, line);
			}
		}
		else {
			switch (line.getType()) {
			case TokenType::Asterisk:
			case TokenType::Operator:
			case TokenType::Symbol:
				addElement(lexer, line);
				break;
			case TokenType::Table:
				addTableLine(lexer, line);
				break;
			case TokenType::Multiline:
				//TODO: add multy line
				break;
			}
		}
	}

	void GherkinDocument::next(GherkinLexer& lexer)
	{
		if (lexer.currentLine) {
			processLine(lexer, *lexer.currentLine);
			lexer.currentLine = nullptr;
		}
		else {
			auto lineNumber = lexer.lineno();
			if (lineNumber > 1) {
				lexer.lines.push_back({ lineNumber });
				processLine(lexer, lexer.lines.back());
			}
			return;
		}
	}

	const GherkinTags& GherkinDocument::getTags() const
	{
		static const GherkinTags empty;
		return feature ? feature->getTags() : empty;
	}

	std::string GherkinDocument::dump() const
	{
		return JSON(*this).dump();
	}

	GherkinDocument::operator JSON() const
	{
		JSON json;
		json["language"] = language;

		if (feature)
			json["feature"] = JSON(*feature);

		if (outline)
			json["outline"] = JSON(*outline);

		if (background)
			json["background"] = JSON(*background);

		if (!scenarios.empty()) {
			JSON js;
			for (auto& scen : scenarios)
				js.push_back(*scen);

			json["scenarios"] = js;
		}

		if (!errors.empty())
			json["errors"] = JSON(errors);

		return json;
	}
}
