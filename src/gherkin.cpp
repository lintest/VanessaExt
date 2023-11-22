﻿#include "gherkin.h"
#include "gherkin.lex.h"
#include <stdafx.h>
#include <codecvt>
#include <locale>
#include <stdio.h>
#include <reflex/matcher.h>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

template<typename T>
static bool comparei(const T& a, const T& b)
{
	static const std::locale locale_ru("ru_RU.UTF-8");
	return boost::iequals(a, b, locale_ru);
}

static FILE* fileopen(const BoostPath& path)
{
#ifdef _WINDOWS
	return _wfopen(path.wstring().c_str(), L"rb");
#else
	return fopen(path.string().c_str(), "rb");
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

	void set(JSON& json, const std::string& key, size_t value) {
		if (value != 0)
			json[key] = value;
	}

	void set(JSON& json, const std::string& key, const FileCache& value) {
		if (!value.empty()) {
			JSON js;
			for (const auto& [key, val] : value) {
				char buf[24];
				strftime(buf, 20, "%Y-%m-%dT%H:%M:%S", localtime(&val.second));
				js.push_back(JSON({ {"file", WC2MB(key.wstring())}, {"id", val.first }, { "date", std::string(buf) } }));
			}
			json[key] = js;
		}
	}

	void set(JSON& json, const std::string& key, const ScenarioMap& value) {
		if (!value.empty()) {
			JSON js;
			for (auto& [k, v] : value) {
				auto filename = WC2MB(v.filepath.wstring());
				js.push_back({ {"filename", filename }, { "snippet", v } });
			}
			json[key] = js;
		}
	}

	void set(JSON& json, const std::string& key, const std::string& value) {
		if (!value.empty())
			json[key] = value;
	}

	void set(JSON& json, const std::string& key, const std::wstring& value) {
		if (!value.empty())
			json[key] = WC2MB(value);
	}

	void set(JSON& json, const std::string& key, const JSON& value) {
		if (!value.empty())
			json[key] = value;
	}

	void set(JSON& json, const std::string& key, const std::vector<std::string>& value) {
		if (!value.empty())
			json[key] = value;
	}

	void set(JSON& json, const std::string& key, const GherkinParams& value) {
		if (!value.empty()) {
			JSON js;
			for (const auto& [i, v] : value)
				js.push_back(JSON({ {"key", i.text}, {"value", v } }));

			json[key] = js;
		}
	}

	template<typename T>
	void set(JSON& json, const std::string& key, const std::unique_ptr<T>& value) {
		if (value)
			json[key] = *value;
	}

	template<typename T>
	void set(JSON& json, const std::string& key, const std::vector<std::unique_ptr<T>>& value) {
		if (!value.empty()) {
			JSON js;
			for (auto& i : value)
				js.push_back(JSON(*i));

			json[key] = js;
		}
	}

	static inline bool notspace(wchar_t ch) {
		return !std::isspace(ch);
	}

	static inline std::wstring& ltrim(std::wstring& s) {
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), notspace));
		return s;
	}

	static inline std::wstring& rtrim(std::wstring& s) {
		s.erase(std::find_if(s.rbegin(), s.rend(), notspace).base(), s.end());
		return s;
	}

	static inline std::wstring& trim(std::wstring& s) {
		return rtrim(ltrim(s));
	}

	static int indent(const std::string& text)
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

	template <typename T>
	bool replace(T& obj, const GherkinParams& params)
	{
		bool changed = false;
		std::wstringstream ss;
		static const boost::wregex expression(L"\\[[^\\]]+\\]");
		std::wstring::const_iterator start = obj.wstr.begin();
		std::wstring::const_iterator end = obj.wstr.end();
		boost::match_results<std::wstring::const_iterator> what;
		boost::match_flag_type flags = boost::match_default;
		while (regex_search(start, end, what, expression, flags)) {
			auto& match = what[0];
			if (match.first > start)
				ss << std::wstring(start, match.first);
			start = match.second;
			auto key = std::wstring(match.begin() + 1, match.end() - 1);
			auto par = GherkinToken(TokenType::Param, WC2MB(key), key);
			auto it = params.find(par);
			if (it == params.end())
				ss << match;
			else {
				ss << it->second.getWstr();
				changed = true;
			}
		}
		if (changed) {
			ss << std::wstring(start, end);
			obj.wstr = ss.str();
			obj.text = WC2MB(obj.wstr);
		}
		return changed;
	}

	enum class MatchType {
		Include,
		Exclude,
		Unknown
	};

	bool GherkinTokenComparator::operator()(const GherkinToken& a, const GherkinToken& b) const {
		if (a.getType() == b.getType())
			return lower(a.getWstr()) < lower(b.getWstr());
		else
			return a.getType() < b.getType();
	}

	class GherkinFilter {
	private:
		std::set<std::wstring> include;
		std::set<std::wstring> exclude;
	public:
		GherkinFilter(const std::string& text) {
			if (text.empty())
				return;

			auto json = JSON::parse(text);

			for (auto& tag : json["include"]) {
				include.emplace(lower(MB2WC(tag)));
			}

			for (auto& tag : json["exclude"]) {
				exclude.emplace(lower(MB2WC(tag)));
			}
		}
		MatchType match(const StringLines& tags) const {
			if (!exclude.empty())
				for (auto& tag : tags) {
					if (exclude.find(lower(tag.wstr)) != exclude.end())
						return MatchType::Exclude;
				}
			if (include.empty())
				return MatchType::Include;
			else {
				for (auto& tag : tags) {
					if (include.find(lower(tag.wstr)) != include.end())
						return MatchType::Include;
				}
				return MatchType::Unknown;
			}
		}
		bool match(const GherkinDocument& doc) const {
			switch (match(doc.getTags())) {
			case MatchType::Exclude: return false;
			case MatchType::Include: return true;
			default: return include.empty();
			}
		}
	};

	GherkinProvider::Keyword::Keyword(KeywordType type, const std::string& name, const std::string& text)
		:type(type), name(name), text(text)
	{
		static const std::string regex = reflex::Matcher::convert("\\w+", reflex::convert_flag::unicode);
		static const reflex::Pattern pattern(regex);
		auto matcher = reflex::Matcher(pattern, text);
		while (matcher.find() != 0) {
			words.push_back(matcher.wstr());
		}
	}

	bool GherkinProvider::Keyword::isTopLevel() const
	{
		switch (type) {
		case KeywordType::Import:
		case KeywordType::Step:
		case KeywordType::None:
			return false;
		default:
			return true;
		}
	}

	GherkinKeyword* GherkinProvider::Keyword::match(GherkinTokens& tokens) const
	{
		bool toplevel = isTopLevel();
		size_t count = words.size();
		if (tokens.size() < count + (toplevel ? 1 : 0))
			return nullptr;

		if (toplevel && tokens[count].type != TokenType::Colon)
			return nullptr;

		for (size_t i = 0; i < count; ++i) {
			if (tokens[i].type != TokenType::Operator)
				return nullptr;

			if (!comparei(words[i], tokens[i].wstr))
				return nullptr;
		}

		for (size_t i = 0; i < count; ++i) {
			tokens[i].type = TokenType::Keyword;
		}

		return new GherkinKeyword(*this, toplevel);
	}

	std::string GherkinProvider::getKeywords() const
	{
		JSON json;
		for (auto& language : keywords) {
			JSON js;
			for (auto& keyword : language.second) {
				js[keyword.name].push_back(keyword.text);
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
			auto& vector = keywords[language];
			auto& types = lang.value();
			for (auto type = types.begin(); type != types.end(); ++type) {
				KeywordType t = GherkinKeyword::str2type(type.key());
				auto& words = type.value();
				if (words.is_array()) {
					for (auto word = words.begin(); word != words.end(); ++word) {
						if (text == "*") continue;
						vector.push_back({ t, type.key(), *word });
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

	template<typename T, typename V>
	void remove(T& cache, const V& key) {
		bool exists = true;
		while (exists) {
			exists = false;
			for (auto it = cache.begin(); it != cache.end(); ++it) {
				if (it->second.filepath == key) {
					cache.erase(it);
					exists = true;
					break;
				}
			}
		}
	}

	void GherkinProvider::ClearCashe(const BoostPath& path)
	{
		if (path.empty()) {
			fileCache.clear();
			variables.clear();
			snippets.clear();
			return;
		}

		auto it = fileCache.find(path);
		if (it != fileCache.end())
			fileCache.erase(it);

		remove(variables, path);
		remove(snippets, path);
	}

	JSON to_json(const std::pair<std::wstring, VariablesFile>& pair) {
		return JSON({
			{"name", WC2MB(pair.first)},
			{"path", WC2MB(pair.second.filepath.wstring())},
			{"items", pair.second.variables}
			});
	}

	std::string GherkinProvider::GetVariables(const std::string& text) const
	{
		JSON json;
		if (text.empty())
			for (auto& var : variables)
				json.push_back(to_json(var));
		else {
			try {
				JSON names = JSON::parse(text);
				for (auto& name : names) {
					auto key = lower(MB2WC(name));
					auto it = variables.find(key);
					if (it != variables.end())
						json.push_back(to_json(*it));
				}
			}
			catch (std::exception& e) {
				json["error"] = e.what();
			}
		}
		return json.dump();
	}

	std::string GherkinProvider::GetCashe() const
	{
		JSON json;
		set(json, "files", fileCache);
		set(json, "snippets", snippets);
		set(json, "variables", variables);
		return json.dump();
	}

	class GherkinProvider::ScanParams {
	public:
		using DocumentCache = std::map<BoostPath, std::unique_ptr<GherkinDocument>>;
		ScanParams(const std::string& filter) : filter(filter) {}
		std::set<BoostPath> ready;
		GherkinFilter filter;
		DocumentCache cashe;
		JSON json;
	};

	struct sort_functor
	{
		bool operator ()(const BoostPath& a, const BoostPath& b)
		{
			return a < b;// or some custom code
		}
	};	

	BoostPaths GherkinProvider::GetDirFiles(size_t id, const BoostPath& root) const
	{
		std::vector<std::wstring> files;
		const std::wstring mask = L"^.+\\.feature$";
		boost::wregex pattern(mask, boost::regex::icase);
		boost::filesystem::recursive_directory_iterator end_itr;
		for (boost::filesystem::recursive_directory_iterator i(root); i != end_itr; ++i) {
			if (id != identifier) return {};
			if (boost::filesystem::is_regular_file(i->status())) {
				boost::wsmatch what;
				std::wstring path = i->path().wstring();
				std::wstring name = i->path().filename().wstring();
				if (boost::regex_match(name, what, pattern))
					files.push_back(path);
			}
		}
		std::sort(files.begin(), files.end());
		BoostPaths result;
		for (auto &file : files) {  
			result.push_back(file);
		}		
		return result;
	}

#ifdef _WINDOWS

	static std::string cp1251_to_utf8(const char* str) {
		int size_u = MultiByteToWideChar(1251, 0, str, -1, 0, 0);
		if (!size_u) return {};
		std::wstring ures((size_t)size_u, 0);
		if (!MultiByteToWideChar(1251, 0, str, -1, ures.data(), size_u)) return {};
		int size_c = WideCharToMultiByte(65001, 0, ures.data(), -1, 0, 0, 0, 0);
		if (!size_c) return {};
		std::string cres((size_t)size_c, 0);
		if (!WideCharToMultiByte(65001, 0, ures.data(), -1, cres.data(), size_c, 0, 0)) return {};
		return cres.data();
	}

#else

	static std::string cp1251_to_utf8(const char* str)
	{
		return str;
	}

#endif

	static void push(JSON& json, const std::string& message, const BoostPath& path) {
		json.push_back({ {"filename", WC2MB(path.wstring()) }, {"error",  message} });
	}

	void GherkinProvider::ScanFile(const BoostPath& path, ScanParams& params)
	{
		try {
			auto it = fileCache.find(path);
			time_t time = 0;
			if (it != fileCache.end()) {
				auto& info = it->second;
				if (info.first == identifier) return;
				time = boost::filesystem::last_write_time(path);
				if (time == info.second) return;
			}
			ClearCashe(path);
			if (time == 0) time = boost::filesystem::last_write_time(path);
			auto doc = std::make_unique<GherkinDocument>(*this, path);
			doc->getExportSnippets(snippets);
			doc->getVariables(variables);
			fileCache[path] = { identifier, time };
			params.cashe.emplace(path, doc.release());
		}
		catch (std::exception& e) {
			push(params.json, e.what(), path);
		}
	}

	void GherkinProvider::DumpFile(const BoostPath& path, ScanParams& params)
	{
		try {
			std::unique_ptr<GherkinDocument> doc;
			auto it = params.cashe.find(path);
			if (it == params.cashe.end())
				doc.reset(new GherkinDocument(*this, path));
			else {
				doc.reset(it->second.release());
				params.cashe.erase(it);
			}
			doc->generate(snippets);
			auto js = doc->dump(params.filter, variables);
			if (!js.empty())
				params.json.push_back(js);
		}
		catch (std::exception& e) {
			push(params.json, e.what(), path);
		}
	}

	void GherkinProvider::ScanFolder(size_t id, AbstractProgress* progress, const BoostPath& root, ScanParams& params)
	{
		try {
			if (root.empty()) return;
			auto files = GetDirFiles(id, root);
			if (progress) progress->Start(WC2MB(root.wstring()), files.size(), "scan");

			for (auto& path : files) {
				if (id != identifier) return;
				if (progress) progress->Step(path);
				ScanFile(path, params);
			}
		}
		catch (boost::filesystem::filesystem_error& e) {
			push(params.json, cp1251_to_utf8(e.what()), root);
		}
		catch (std::exception& e) {
			push(params.json, e.what(), root);
		}
	}

	void GherkinProvider::DumpFolder(size_t id, AbstractProgress* progress, const BoostPath& root, ScanParams& params)
	{
		try {
			if (root.empty()) return;
			auto files = GetDirFiles(id, root);
			if (progress) progress->Start(WC2MB(root.wstring()), files.size(), "dump");

			for (auto& path : files) {
				if (id != identifier) return;
				if (progress) progress->Step(path);
				if (params.ready.count(path) != 0) continue;
				params.ready.insert(path);
				DumpFile(path, params);
			}
		}
		catch (boost::filesystem::filesystem_error& e) {
			push(params.json, cp1251_to_utf8(e.what()), root);
		}
		catch (std::exception& e) {
			push(params.json, e.what(), root);
		}
	}

	std::string GherkinProvider::ParseFolder(const std::string& dirs, const std::string& libs, const std::string& tags, AbstractProgress* progress)
	{
		if (dirs.empty()) return {};
		size_t id = ++identifier;
		ScanParams params(tags);
		JSON directories, libraries;

		try {
			directories = JSON::parse(dirs);
		}
		catch (...) {
			directories.push_back(dirs);
		}

		try {
			if (!libs.empty())
				libraries = JSON::parse(libs);
		}
		catch (...) {
			libraries.push_back(libs);
		}

		for (auto& dir : libraries) {
			ScanFolder(id, progress, MB2WC(dir), params);
		}

		for (auto& dir : directories) {
			DumpFolder(id, progress, MB2WC(dir), params);
		}

		return params.json.dump();
	}

	std::string GherkinProvider::ParseFile(const std::wstring& path, const std::string& libs, AbstractProgress* progress)
	{
		if (path.empty()) return {};
		size_t id = ++identifier;
		ScanParams params({});
		JSON libraries, result;

		try {
			if (!libs.empty())
				libraries = JSON::parse(libs);
		}
		catch (...) {
			libraries.push_back(libs);
		}

		for (auto& dir : libraries) {
			ScanFolder(id, progress, MB2WC(dir), params);
		}

		try {
			std::unique_ptr<GherkinDocument> doc;
			auto it = params.cashe.find(path);
			if (it == params.cashe.end())
				doc.reset(new GherkinDocument(*this, path));
			else {
				doc.reset(it->second.release());
			}
			doc->generate(snippets);
			GherkinFilter filter({});
			result = doc->dump(filter, variables);
		}
		catch (boost::filesystem::filesystem_error& e) {
			auto message = cp1251_to_utf8(e.what());
			push(result["errors"], message, path);
		}
		catch (std::exception& e) {
			push(result["errors"], e.what(), path);
		}

		for (auto& e : params.json)
			if (!e["error"].empty())
				result["errors"].push_back(e);

		return result.dump();
	}

	std::string GherkinProvider::ParseText(const std::string& text)
	{
		if (text.empty()) return {};
		GherkinDocument doc(*this, text);
		doc.generate(snippets);
		return JSON(doc).dump();
	}

	KeywordType GherkinKeyword::str2type(const std::string& text)
	{
		std::string type = text;
		transform(type.begin(), type.end(), type.begin(), tolower);
		static std::map<std::string, KeywordType> types{
			{ "background", KeywordType::Background },
			{ "variables", KeywordType::Variables },
			{ "import", KeywordType::Import },
			{ "examples", KeywordType::Examples },
			{ "feature", KeywordType::Feature },
			{ "scenario", KeywordType::Scenario },
			{ "scenariooutline", KeywordType::ScenarioOutline },
		};
		auto it = types.find(type);
		return it == types.end() ? KeywordType::Step : it->second;
	}

	GherkinKeyword::operator JSON() const
	{
		JSON json;
		json["type"] = name;
		json["text"] = text;
		if (toplevel)
			json["toplevel"] = toplevel;

		return json;
	}

	StringLine::StringLine(const GherkinLexer& lexer)
		: lineNumber(lexer.lineno())
		, text(trim(lexer.text()))
		, wstr(MB2WC(text))
	{
	}

	StringLine::StringLine(const GherkinLine& line)
		: lineNumber(line.lineNumber)
		, text(trim(line.text))
		, wstr(MB2WC(text))
	{
	}

	StringLine::StringLine(const StringLine& src)
		: lineNumber(src.lineNumber), text(src.text), wstr(src.wstr)
	{
	}

	StringLine::StringLine(size_t lineNumber)
		: lineNumber(lineNumber)
	{
	}

	bool StringLine::replace(const GherkinParams& params)
	{
		return ::replace(*this, params);
	}

	StringLine::operator JSON() const
	{
		JSON json;
		set(json, "text", text);
		set(json, "line", lineNumber);
		return json;
	}

	GherkinToken& GherkinToken::operator=(const GherkinToken& src)
	{
		type = src.type;
		wstr = src.wstr;
		text = src.text;
		column = src.column;
		if (src.type != TokenType::Param || src.symbol) symbol = src.symbol;
		return *this;
	}

	void GherkinToken::unescape(GherkinLexer& lexer)
	{
		if (wstr.empty()) return;
		bool escaping = false;
		std::wstringstream ss;
		auto start = wstr.begin();
		auto finish = wstr.end();
		if (symbol) {
			++start;
			--finish;
		}
		for (auto& it = start; it != finish; ++it) {
			if (escaping) {
				escaping = false;
				wchar_t wc = *it;
				if (lexer.escaped(wc))
					switch (wc) {
					case L'a': wc = L'\a'; break;
					case L'b': wc = L'\b'; break;
					case L'f': wc = L'\f'; break;
					case L'n': wc = L'\n'; break;
					case L'r': wc = L'\r'; break;
					case L't': wc = L'\t'; break;
					case L'v': wc = L'\v'; break;
					case L'0': wc = L'\0'; break;
					}
				else ss << L'\\';
				ss << wc;
			}
			else {
				if (*it == L'\\') {
					if (it + 1 == finish) ss << *it;
					else escaping = true;
				}
				else ss << *it;
			}
		}
		wstr = ss.str();
		text = WC2MB(wstr);
	}

	GherkinToken::GherkinToken(GherkinLexer& lexer, TokenType type, char ch)
		: type(type), wstr(lexer.wstr()), text(lexer.text()), column(lexer.columno()), symbol(ch)
	{
		if (getType() == TokenType::Param) {
			if (ch == 0) wstr = trim(wstr);
			unescape(lexer);
		}
	}

	GherkinToken::GherkinToken(GherkinLexer& lexer, std::wstring wstr)
		: type(TokenType::Symbol), wstr(wstr), text(WC2MB(wstr)), column(lexer.columno()), symbol(0)
	{
	}

	bool GherkinToken::isParam() const
	{
		switch (type) {
		case TokenType::Param:
		case TokenType::Number:
		case TokenType::Date:
			return true;
		default:
			return false;
		}
	}

	bool GherkinToken::replace(const GherkinParams& params)
	{
		bool changed = false;
		if (isParam()) {
			auto it = params.find(*this);
			if (it != params.end()) {
				*this = it->second;
				changed = true;
			}
			else {
				changed = ::replace(*this, params);
			}
		}
		return changed;
	}

	std::wstringstream& operator<<(std::wstringstream& os, const GherkinToken& token)
	{
		if (token.symbol)
			os << MB2WC(std::string(1, (token.symbol == '>' ? '<' : token.symbol)));

		os << token.wstr;

		if (token.symbol)
			os << MB2WC(std::string(1, (token.symbol == '<' ? '>' : token.symbol)));

		return os;
	}

	std::wstringstream& operator<<(std::wstringstream& os, const GherkinTokens& tokens)
	{
		bool split = false;
		const wchar_t splitter = L' ';
		for (auto& token : tokens) {
			if (split) {
				if (token.getType() != TokenType::Symbol)
					os << splitter;
			}
			else {
				split = true;
			}
			os << token;
		}
		return os;
	}

	static double str2dec(const std::string& text, double numb, double sign, std::stringstream& ss) {
		double mult = 0.1;
		for (auto it = text.begin(); it != text.end(); ++it) {
			numb += (*it - '0') * mult;
			mult *= 0.1;
			ss << *it;
		}
		return numb * sign;
	}

	static double str2num(const std::string& text, std::stringstream& ss) {
		bool zeroPrefix = true;
		double numb = 0, sign = 1;
		for (auto it = text.begin(); it != text.end(); ++it) {
			switch (*it) {
			case '+':
				break;
			case '-':
				ss << *it;
				sign = -1;
				break;
			case ',':
			case '.':
				if (zeroPrefix) ss << '0';
				ss << '.';
				return str2dec(std::string(it + 1, text.end()), numb, sign, ss);
			default:
				ss << *it;
				zeroPrefix = false;
				numb = numb * 10 + int64_t(*it - '0');
			}
		}
		return numb * sign;
	}

	GherkinToken::operator JSON() const
	{
		JSON json;
		json["text"] = text;
		json["column"] = column;
		json["type"] = type2str();

		if (type == TokenType::Number) {
			try {
				std::stringstream ss;
				double number = str2num(text, ss);
				std::pair<double, std::string> data(number, ss.str());
				json["text"] = JSON(data);
			}
			catch (std::exception& e) {
				json["error"] = e.what();
			}
		}

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
		case TokenType::Line: return "Line";
		case TokenType::Date: return "Date";
		case TokenType::Tag: return "Tag";
		case TokenType::Symbol: return "Symbol";
		case TokenType::Multiline: return "Multiline";
		default: return "None";
		}
	}

	GherkinLine::GherkinLine(GherkinLexer& l)
		: lineNumber(l.lineno()),
		text(l.matcher().line()),
		wstr(l.matcher().wline()),
		indent(::indent(text))
	{
	}

	GherkinLine::GherkinLine(size_t lineNumber)
		: lineNumber(lineNumber)
		, indent(0)
	{
	}

	void GherkinLine::push(GherkinLexer& lexer, TokenType type, char ch)
	{
		if (type == TokenType::Symbol)
			tokens.emplace_back(lexer, lexer.wstr());
		else
			tokens.emplace_back(lexer, type, ch);
	}

	GherkinKeyword* GherkinLine::matchKeyword(GherkinDocument& document)
	{
		if (tokens.size() == 0) return nullptr;
		if (tokens.begin()->type != TokenType::Operator) return nullptr;
		keyword.reset(document.matchKeyword(tokens));
		return keyword.get();
	}

	GherkinSnippet snippet(const GherkinTokens& tokens)
	{
		std::wstringstream ss;
		for (auto& token : tokens) {
			switch (token.getType()) {
				case TokenType::Operator: {
					auto wstr = token.getWstr();
					for (auto& ch : wstr) {
						switch (ch) {
							case L'.':
								break;
							case L'-':
								ss << L'_';
								break;
							default:
								ss << std::tolower(ch, locale_ru);
						}
					}
				} break;
				case TokenType::Symbol: {
					if (token.getText() == "-") ss << L'_';
				} break;
			}
		}
		return ss.str();
	}

	GherkinLine::operator JSON() const
	{
		JSON json;
		set(json, "text", text);
		set(json, "line", lineNumber);
		set(json, "tokens", tokens);
		set(json, "keyword", keyword);
		return json;
	}

	Gherkin::TokenType GherkinLine::getType() const
	{
		return tokens.empty() ? TokenType::None : tokens.begin()->type;
	}

	GherkinTable::TableRow::TableRow(const GherkinLine& line)
		: lineNumber(line.lineNumber), text(line.text), wstr(line.wstr)
	{
		bool first = true;
		GherkinToken current = TokenType::None;
		for (auto& token : line.getTokens()) {
			if (first)
				first = false;
			else {
				if (token.getType() == TokenType::Table) {
					tokens.emplace_back(current);
					current = TokenType::None;
				}
				else
					current = token;
			}
		}
	}

	GherkinTable::TableRow::TableRow(const TableRow& src)
		: lineNumber(src.lineNumber), text(src.text), wstr(src.wstr)
	{
		for (auto& token : src.tokens) {
			tokens.emplace_back(token);
		}
	}

	GherkinTable::TableRow::TableRow(const TableRow& src, const GherkinParams& params)
		: lineNumber(src.lineNumber), text(src.text), wstr(src.wstr)
	{
		for (auto& token : src.tokens) {
			tokens.emplace_back(token);
			tokens.back().replace(params);
		}
		::replace(*this, params);
	}

	void GherkinTable::TableRow::push(const GherkinToken& token, const GherkinParams& params)
	{
		tokens.emplace_back(token);
		tokens.back().replace(params);
	}

	GherkinTable::TableRow& GherkinTable::TableRow::operator=(const TableRow& src)
	{
		lineNumber = src.lineNumber;
		tokens = src.tokens;
		text = src.text;
		return *this;
	}

	GherkinTable::TableRow::operator JSON() const
	{
		JSON json;
		set(json, "line", lineNumber);
		set(json, "text", text);
		set(json, "tokens", tokens);
		set(json, "script", script);
		return json;
	}

	GherkinTable::GherkinTable(const GherkinLine& line)
		: lineNumber(line.lineNumber), head(line)
	{
		if (head.tokens.size() == 1 && head.tokens[0].getType() == TokenType::None) {
			head.tokens.clear();
		}
	}

	GherkinTable::GherkinTable(const GherkinTable& src)
		: lineNumber(0), head(src.head)
	{
		for (auto& row : src.body) {
			body.emplace_back(row);
		}
	}

	GherkinTable::GherkinTable(const GherkinTable& src, const GherkinParams& params)
		: lineNumber(0), head(src.head, params)
	{
		for (auto& row : src.body) {
			body.emplace_back(row, params);
		}
	}

	GherkinTable& GherkinTable::operator=(const GherkinTable& src)
	{
		if (std::addressof(*this) == std::addressof(src))
			throw "Assign object to himself";

		head = src.head;
		body.clear();
		for (auto& row : src.body) {
			body.emplace_back(row);
		}
		return *this;
	}

	void GherkinTable::push(const GherkinLine& line)
	{
		body.emplace_back(line);
	}

	GherkinTable::operator JSON() const
	{
		JSON json;
		set(json, "line", lineNumber);
		json["head"] = head;
		json["body"] = body;
		return json;
	}

	GherkinMultiline::GherkinMultiline(const GherkinLine& line)
		: lineNumber(line.lineNumber)
		, lastNumber(line.lineNumber)
		, endNumber(0)
		, header(trim(line.text))
	{
	}

	GherkinMultiline::GherkinMultiline(const GherkinMultiline& src)
		: lineNumber(src.lineNumber)
		, lastNumber(src.lastNumber)
		, endNumber(src.endNumber)
		, header(src.header)
		, footer(src.footer)
		, lines(src.lines)
	{
	}

	GherkinMultiline::GherkinMultiline(const GherkinMultiline& src, const GherkinParams& params)
		: lineNumber(src.lineNumber)
		, lastNumber(src.lastNumber)
		, endNumber(src.endNumber)
		, header(src.header)
		, footer(src.footer)
		, lines(src.lines)
	{
		replace(params);
	}

	GherkinMultiline& GherkinMultiline::operator=(const GherkinMultiline& src)
	{
		if (std::addressof(*this) == std::addressof(src))
			throw "Assign object to himself";

		lines.clear();
		for (auto& line : src.lines) {
			lines.push_back(line);
		}
		return *this;
	}

	void GherkinMultiline::push(const GherkinLine& line)
	{
		while (line.lineNumber > lastNumber + 1) {
			lines.emplace_back(++lastNumber);
		}
		lastNumber = line.lineNumber;
		lines.emplace_back(line);
	}

	void GherkinMultiline::close(const GherkinLine& line)
	{
		while (line.lineNumber > lastNumber + 1) {
			lines.emplace_back(++lastNumber);
		}
		endNumber = line.lineNumber;
		footer = trim(line.text);
	}

	bool GherkinMultiline::replace(const GherkinParams& params)
	{
		bool changed = false;
		for (auto& line : lines) {
			changed |= line.replace(params);
		}
		return changed;
	}

	GherkinMultiline::operator JSON() const
	{
		JSON json;
		json["header"] = { {"line", lineNumber}, {"text", header} };
		json["footer"] = { {"line", endNumber}, {"text", footer} };
		set(json, "lines", lines);
		return json;
	}

	static void fill(GherkinParams& params, const GherkinTokens& owner, const GherkinTokens& definition)
	{
		GherkinTokens source, target;
		for (auto& token : owner) {
			switch (token.getType()) {
			case TokenType::Param:
			case TokenType::Number:
			case TokenType::Date:
				source.push_back(token);
				break;
			}
		}
		for (auto& token : definition) {
			switch (token.getType()) {
			case TokenType::Param:
			case TokenType::Number:
			case TokenType::Date:
				target.push_back(token);
				break;
			}
		}
		auto s = source.begin();
		auto t = target.begin();
		for (; s != source.end() && t != target.end(); ++s, ++t) {
			if (t->isParam()) {
				if (params.count(*t) == 0)
					params.emplace(*t, *s);
				else
					throw GherkinException("Duplicate param keys");
			}
		}
	}

	GeneratedScript* GeneratedScript::generate(const GherkinStep& owner, const GherkinDocument& doc, const ScenarioMap& map, const SnippetStack& stack)
	{
		auto snippet = owner.getSnippet();
		if (snippet.empty()) return nullptr;
		if (stack.count(snippet)) return nullptr;

		std::unique_ptr<GeneratedScript> result;
		std::unique_ptr<ExportScenario> definition(doc.find(snippet, owner));
		if (definition) {
			GherkinParams params;
			fill(params, owner.getTokens(), definition->getTokens());
			result.reset(new GeneratedScript(*definition, params));
		} else {
			auto it = map.find(snippet);
			if (it == map.end()) return nullptr;
			definition.reset(new ExportScenario(it->second));
			result.reset(new GeneratedScript(owner, *definition));
		}
		SnippetStack next = stack;
		next.insert(snippet);
		for (auto& step : result->steps)
			step->generate(doc, map, next);

		return result.release();
	}

	ExportScenario* GherkinDocument::find(const GherkinSnippet& snippet, const GherkinStep& owner) const
	{
		for (auto& def : scenarios) {
			if (def->getSnippet() == snippet) {
				return new ExportScenario({ *this, *def });
			}
		}
		return nullptr;
	}

	GeneratedScript::GeneratedScript(const GherkinStep& owner, const ExportScenario& definition)
		: filename(WC2MB(definition.filepath.wstring()))
		, snippet(definition.getSnippet())
	{
		if (auto t = definition.getExamples()) {
			examples.reset(new GherkinTable(*t));
		}
		fill(params, owner.getTokens(), definition.getTokens());
		for (auto& step : definition.steps) {
			steps.emplace_back(step->copy(params));
		}
	}

	GeneratedScript::GeneratedScript(const GherkinDefinition& definition, const GherkinParams& params)
	{
		for (auto& step : definition.steps) {
			steps.emplace_back(step->copy(params));
		}
	}

	GeneratedScript::GeneratedScript(const GherkinSteps& src, const GherkinParams& params)
	{
		for (auto& step : src) {
			steps.emplace_back(step->copy(params));
		}
	}

	void GeneratedScript::generate(const GherkinDocument& doc, const ScenarioMap& map, const SnippetStack& stack)
	{
		for (auto& step : steps) {
			step->generate(doc, map, stack);
		}
	}

	void GeneratedScript::replace(GherkinTables& tabs, GherkinMultilines& mlns)
	{
		for (auto& step : steps) {
			step->replace(tabs, mlns);
			for (auto& table : step->tables)
				table = GherkinTable(table, params);
		}
		if (examples) {
			if (!tabs.empty()) {
				*examples = GherkinTable(tabs.back(), params);
				tabs.pop_back();
			}
			auto& table = *examples;
			for (auto& row : table.body) {
				auto p = table.params(params, row);
				row.script.reset(new GeneratedScript(steps, p));
			}
		}
	}

	GeneratedScript::operator JSON() const
	{
		JSON json;
		set(json, "key", snippet);
		set(json, "filename", filename);
		set(json, "params", params);
		set(json, "steps", steps);
		set(json, "examples", examples);
		return json;
	}

	static void set_params(JSON& json, const GherkinTokens& tokens)
	{
		JSON params;
		for (auto& token : tokens) {
			switch (token.getType()) {
			case TokenType::Param:
			case TokenType::Number:
			case TokenType::Date:
				params.push_back(token);
			}
		}
		set(json, "params", params);
	}

	GherkinElement::GherkinElement(const GherkinElement& src, const GherkinParams& params)
		: wstr(src.wstr), text(src.text), lineNumber(src.lineNumber), tags(src.tags)
	{
		for (auto& step : src.steps) {
			steps.emplace_back(step->copy(params));
		}
		for (auto& table : src.tables) {
			tables.emplace_back(table, params);
		}
		for (auto& lines : src.multilines) {
			multilines.emplace_back(lines, params);
		}
	}

	GherkinElement::GherkinElement(GherkinLexer& lexer, const GherkinLine& line)
		: wstr(line.wstr), text(line.text), lineNumber(line.lineNumber)
	{
		comments = std::move(lexer.commentStack);
		tags = std::move(lexer.tagStack);
	}

	void GherkinElement::generate(const GherkinDocument& doc, const ScenarioMap& map, const SnippetStack& stack)
	{
		for (auto& it : steps)
			it->generate(doc, map, stack);
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
		steps.emplace_back(element);
		return element;
	}

	GherkinTable* GherkinElement::pushTable(const GherkinLine& line)
	{
		tables.emplace_back(line);
		return &tables.back();
	}

	GherkinMultiline* GherkinElement::pushMultiline(const GherkinLine& line)
	{
		multilines.emplace_back(line);
		return &multilines.back();
	}

	void GherkinElement::replace(GherkinTables& tabs, GherkinMultilines& mlns)
	{
		for (auto& table : tables) {
			if (tabs.empty()) return;
			auto& t = tabs.back();
			if (!t.empty()) table = t;
			tabs.pop_back();
		}
		for (auto& line : multilines) {
			if (mlns.empty()) return;
			auto& m = mlns.back();
			if (!m.empty()) line = m;
			mlns.pop_back();
		}
		for (auto& it : steps)
			it->replace(tabs, mlns);
	}

	GherkinElement* GherkinElement::copy(const GherkinParams& params) const
	{
		return new GherkinElement(*this, params);
	}

	GherkinElement::operator JSON() const
	{
		JSON json;
		set(json, "text", text);
		set(json, "line", lineNumber);
		set(json, "snippet", getSnippet());
		set(json, "steps", steps);
		set(json, "tables", tables);
		set(json, "multilines", multilines);
		set(json, "comments", comments);
		set(json, "tags", tags);
		return json;
	}

	AbsractDefinition::AbsractDefinition(GherkinLexer& lexer, const GherkinLine& line)
		: GherkinElement(lexer, line), keyword(*line.getKeyword())
	{
		std::string text = line.text;
		static const std::string regex = reflex::Matcher::convert("[^:]+:\\s*", reflex::convert_flag::unicode);
		static const reflex::Pattern pattern(regex);
		auto matcher = reflex::Matcher(pattern, text);
		if (matcher.find() && matcher.size() < text.size()) {
			name = trim(text.substr(matcher.size()));
		}
	}

	AbsractDefinition::AbsractDefinition(const GherkinDocument& doc, const AbsractDefinition& def)
		: GherkinElement(def, {}), name(def.name), keyword(def.keyword)
	{
	}

	AbsractDefinition::AbsractDefinition(const AbsractDefinition& src, const GherkinParams& params)
		: GherkinElement(src, params), name(src.name), keyword(src.keyword)
	{
	}

	GherkinElement* AbsractDefinition::push(GherkinLexer& lexer, const GherkinLine& line)
	{
		return GherkinElement::push(lexer, line);
	}

	AbsractDefinition::operator JSON() const
	{
		JSON json = GherkinElement::operator JSON();
		set(json, "name", name);
		return json;
	}

	GherkinFeature::GherkinFeature(GherkinLexer& lexer, const GherkinLine& line)
		: AbsractDefinition(lexer, line)
	{
	}

	GherkinElement* GherkinFeature::push(GherkinLexer& lexer, const GherkinLine& line)
	{
		description.emplace_back(line);
		return nullptr;
	}

	GherkinFeature::operator JSON() const
	{
		JSON json = AbsractDefinition::operator JSON();
		json["keyword"] = keyword;
		set(json, "description", description);
		return json;
	}

	GherkinVariable::GherkinVariable(const GherkinVariable& src)
		: lineNumber(src.lineNumber), name(src.name), text(src.text)
	{
		if (src.value) value.reset(new GherkinToken(*src.value.get()));
		if (src.table) table.reset(new GherkinTable(*src.table.get()));
		if (src.lines) lines.reset(new GherkinMultiline(*src.lines.get()));
	}

	GherkinVariable::GherkinVariable(const GherkinLine& line)
		: lineNumber(line.lineNumber), text(line.text)
	{
		auto& tokens = line.getTokens();
		switch (line.getType()) {
		case TokenType::Asterisk:
			name = trim(tokens[0].getText().substr(1));
			break;
		case TokenType::Keyword:
		case TokenType::Operator:
			if (tokens.size() > 1) {
				name = tokens[0].getText();
			}
			if (tokens.size() > 2
				&& tokens[1].getText() == "="
				&& tokens[1].getType() == TokenType::Symbol) {
				switch (tokens[2].getType()) {
				case TokenType::Param:
				case TokenType::Number:
				case TokenType::Date:
					value.reset(new GherkinToken(tokens[2]));
				}
			}
			break;
		default:
			throw GherkinException("Wrong variable definition");
		}
	}

	GherkinTable* GherkinVariable::pushTable(const GherkinLine& line)
	{
		table.reset(new GherkinTable(line));
		return table.get();
	}

	GherkinMultiline* GherkinVariable::pushMultiline(const GherkinLine& line)
	{
		lines.reset(new GherkinMultiline(line));
		return lines.get();
	}

	GherkinVariable::operator JSON() const
	{
		JSON json;
		set(json, "line", lineNumber);
		set(json, "name", name);
		set(json, "text", text);
		set(json, "value", value);
		set(json, "table", table);
		set(json, "lines", lines);
		return json;
	}

	VariablesFile::operator JSON() const
	{
		JSON json;
		set(json, "path", filepath.string());
		set(json, "items", variables);
		return json;
	}

	GherkinImport::GherkinImport(const GherkinLine& line)
		: lineNumber(line.lineNumber), text(line.text)
	{
		auto tokens = line.getTokens();
		if (tokens.size() > 0 && tokens[1].getType() == TokenType::Param) {
			filename = tokens[1].wstr;
		}
	}

	GherkinImport::GherkinImport(const GherkinImport& src)
		: lineNumber(src.lineNumber), text(src.text), filename(src.filename)
	{
	}

	JSON GherkinImport::dump(const VariableCache& cache) const
	{
		JSON json;
		set(json, "name", filename);
		set(json, "line", lineNumber);
		set(json, "text", text);

		auto it = cache.find(lower(filename));
		if (it != cache.end()) {
			set(json, "path", it->second.filepath.wstring());
			set(json, "items", it->second.variables);
		}

		return json;
	}

	GherkinImport::operator JSON() const
	{
		JSON json;
		set(json, "name", filename);
		set(json, "line", lineNumber);
		set(json, "text", text);
		return json;
	}

	GherkinVariables::GherkinVariables(GherkinLexer& lexer, const GherkinLine& line)
		: AbsractDefinition(lexer, line)
	{
	}

	GherkinElement* GherkinVariables::push(GherkinLexer& lexer, const GherkinLine& line)
	{
		auto& tokens = line.getTokens();
		switch (line.getType()) {
		case TokenType::Asterisk:
			current.reset(new GherkinVariable(line));
			return nullptr;
		case TokenType::Keyword:
			if (line.getKeyword()->getType() == KeywordType::Import) {
				imports.emplace_back(line);
				return nullptr;
			}
			[[fallthrough]];
		case TokenType::Operator:
			if (tokens.size() == 2) {
				current.reset(new GherkinVariable(line));
				return nullptr;
			}
			if (tokens.size() > 2
				&& tokens[1].getText() == "="
				&& tokens[1].getType() == TokenType::Symbol) {
				switch (tokens[2].getType()) {
				case TokenType::Param:
				case TokenType::Number:
				case TokenType::Date:
					variables.emplace_back(line);
					return nullptr;
				case TokenType::Comment:
					current.reset(new GherkinVariable(line));
					return nullptr;
				}
			}
		}
		throw GherkinException("Wrong variable definition");
	}

	GherkinTable* GherkinVariables::pushTable(const GherkinLine& line)
	{
		std::unique_ptr<GherkinVariable> var(current.release());
		if (!var) var.reset(new GherkinVariable());
		variables.emplace_back(*var.get());
		return variables.back().pushTable(line);
	}

	GherkinMultiline* GherkinVariables::pushMultiline(const GherkinLine& line)
	{
		std::unique_ptr<GherkinVariable> var(current.release());
		if (!var) current.reset(new GherkinVariable());
		variables.emplace_back(*var.get());
		return variables.back().pushMultiline(line);
	}

	JSON GherkinVariables::dump(const VariableCache& cache) const
	{
		JSON json = AbsractDefinition::operator JSON();
		set(json, "name", name);
		set(json, "keyword", keyword);
		set(json, "items", variables);
		for (auto& it : imports) {
			json["import"].push_back(it.dump(cache));
		}
		return json;
	}

	GherkinVariables::operator JSON() const
	{
		JSON json = AbsractDefinition::operator JSON();
		set(json, "name", name);
		set(json, "keyword", keyword);
		set(json, "import", imports);
		set(json, "items", variables);
		return json;
	}

	GherkinDefinition::GherkinDefinition(GherkinLexer& lexer, const GherkinLine& line)
		: AbsractDefinition(lexer, line), tokens(line.getTokens())
		, snippet(getType() == KeywordType::Background ? std::wstring() : ::snippet(tokens))
	{
	}

	GherkinDefinition::GherkinDefinition(const GherkinDocument& doc, const GherkinDefinition& def)
		: AbsractDefinition(doc, def), tokens(def.tokens), snippet(def.snippet)
	{
		if (def.examples)
			examples.reset((GherkinStep*)def.examples->copy({}));
	}

	GherkinDefinition::GherkinDefinition(const GherkinDefinition& src, const GherkinParams& params)
		: AbsractDefinition(src, params), tokens(src.tokens), snippet(src.snippet)
	{
		if (src.examples)
			examples.reset((GherkinStep*)src.examples->copy(params));
	}

	GherkinParams GherkinTable::params(const GherkinParams& src, const TableRow& row) const
	{
		GherkinParams params;
		auto i = head.tokens.begin();
		auto j = row.tokens.begin();
		for (; i != head.tokens.end() && j != row.tokens.end(); ++i, ++j) {
			if (params.count(*i)) {
				throw GherkinException("Duplicate param keys");
			}
			if (j->isParam()) {
				if (src.count(*j)) {
					auto& token = src.at(*j);
					params.emplace(*i, token);
					continue;
				}
			}
			params.emplace(*i, *j);
		}
		return params;
	}

	void GherkinDefinition::generate(const GherkinDocument& doc, const ScenarioMap& map, const SnippetStack& stack)
	{
		SnippetStack nextStack(stack);
		if (!snippet.empty())
			nextStack.insert(snippet);

		AbsractDefinition::generate(doc, map, nextStack);
		if (examples && !examples->tables.empty()) {
			auto& table = examples->tables[0];
			for (auto& row : table.body) {
				auto params = table.params({}, row);
				row.script.reset(new GeneratedScript(*this, params));
				row.script->generate(doc, map, stack);
			}
		}
	}

	void GherkinDefinition::replace(GherkinTables& tabs, GherkinMultilines& mlns)
	{
		if (!examples) return;
	}

	GherkinElement* GherkinDefinition::push(GherkinLexer& lexer, const GherkinLine& line)
	{
		auto keyword = line.getKeyword();
		if (keyword && keyword->getType() == KeywordType::Examples) {
			if (examples)
				throw GherkinException(lexer, "Examples duplicate error");

			examples.reset(new GherkinStep(lexer, line));
			return examples.get();
		}
		else
			return GherkinElement::push(lexer, line);
	}

	GherkinElement* GherkinDefinition::copy(const GherkinParams& params) const
	{
		return new GherkinDefinition(*this, params);
	}

	GherkinDefinition::operator JSON() const
	{
		JSON json = AbsractDefinition::operator JSON();
		json["keyword"] = keyword;
		set(json, "tokens", tokens);
		if (examples) {
			if (!examples->getTables().empty()) {
				json["examples"] = examples->getTables()[0];
			}
			set(json["examples"], "line", examples->lineNumber);
			set(json["examples"], "keyword", examples->keyword);
		}
		set_params(json, tokens);
		return json;
	}

	GherkinStep::GherkinStep(GherkinLexer& lexer, const GherkinLine& line)
		: GherkinElement(lexer, line)
		, keyword(*line.getKeyword())
		, tokens(line.getTokens())
		, snippet(::snippet(tokens))
	{
	}

	GherkinStep::GherkinStep(const GherkinStep& src, const GherkinParams& params)
		: GherkinElement(src, params)
		, keyword(src.keyword)
		, tokens(src.tokens)
		, snippet(src.snippet)
	{
		if (!params.empty()) {
			bool changed = false;
			for (auto& token : tokens) {
				changed |= token.replace(params);
			}
			if (changed) {
				std::wstringstream ss;
				for (auto& ch : wstr)
					if (std::isspace(ch)) ss << ch; else break;

				ss << tokens;
				wstr = ss.str();
				text = WC2MB(wstr);
			}
		}
	}

	GherkinElement* GherkinStep::copy(const GherkinParams& params) const
	{
		return new GherkinStep(*this, params);
	}

	template<typename T>
	static std::vector<T> reverse(const std::vector<T>& src)
	{
		std::vector<T> tabs;
		for (auto it = src.rbegin(); it != src.rend(); ++it) {
			tabs.push_back(*it);
		}
		return tabs;
	}

	void GherkinStep::generate(const GherkinDocument& doc, const ScenarioMap& map, const SnippetStack& stack)
	{
		if (stack.count(snippet))
			error = "Recursive call error";
		else
			script.reset(GeneratedScript::generate(*this, doc, map, stack));

		GherkinElement::generate(doc, map, stack);
		if (script) {
			auto tabs = reverse(tables);
			auto mlns = reverse(multilines);
			script->replace(tabs, mlns);
			if (script->examples) {
				SnippetStack next = stack;
				next.insert(script->snippet);
				auto& table = *script->examples;
				for (auto& row : table.body)
					for (auto& step : row.script->steps)
						step->generate(doc, map, next);
			}
		}
	}

	void GherkinStep::replace(GherkinTables& tabs, GherkinMultilines& mlns)
	{
		GherkinElement::replace(tabs, mlns);
		if (script) {
			auto tabs = reverse(tables);
			auto mlns = reverse(multilines);
			script->replace(tabs, mlns);
		}
	}

	GherkinStep::operator JSON() const
	{
		JSON json = GherkinElement::operator JSON();
		json["keyword"] = keyword;
		set(json, "tokens", tokens);
		set(json, "snippet", script);
		set(json, "error", error);
		set_params(json, tokens);
		return json;
	}

	GherkinGroup::GherkinGroup(const GherkinGroup& src, const GherkinParams& params)
		: GherkinElement(src, params), name(src.name)
	{
	}

	GherkinGroup::GherkinGroup(GherkinLexer& lexer, const GherkinLine& line)
		: GherkinElement(lexer, line), name(trim(line.text))
	{
		if (line.getType() == TokenType::Asterisk) {
			name = trim(name.substr(1));
		}
	}

	GherkinElement* GherkinGroup::copy(const GherkinParams& params) const
	{
		return new GherkinGroup(*this, params);
	}

	GherkinGroup::operator JSON() const
	{
		JSON json = GherkinElement::operator JSON();
		json["name"] = name;
		return json;
	}

	ExportScenario::ExportScenario(const ScenarioRef& ref)
		: GherkinDefinition(ref.first, ref.second), filepath(ref.first.filepath)
	{
	}

	ExportScenario::ExportScenario(const ExportScenario& src)
		: GherkinDefinition(src, {}), filepath(src.filepath)
	{
	}

	const GherkinTable* ExportScenario::getExamples() const
	{
		if (!examples) return nullptr;
		auto& tables = examples->getTables();
		return tables.empty() ? nullptr : &tables[0];
	}

	GherkinException::GherkinException(GherkinLexer& lexer, const std::string& message)
		: std::runtime_error(message.c_str()), line(lexer.lineno()), column(lexer.columno())
	{
	}

	GherkinException::GherkinException(GherkinLexer& lexer, char const* const message)
		: std::runtime_error(message), line(lexer.lineno()), column(lexer.columno())
	{
	}

	GherkinException::GherkinException(const GherkinException& src)
		: std::runtime_error(*this), line(src.line), column(src.column)
	{
	}

	GherkinException::GherkinException(size_t line, char const* const message)
		: std::runtime_error(message), line(line), column(0)
	{
	}

	GherkinException::GherkinException(char const* const message)
		: std::runtime_error(message), line(0), column(0)
	{
	}

	GherkinException::operator JSON() const
	{
		JSON json;
		std::wstring wstr = MB2WC(what());
		set(json, "line", line);
		set(json, "column", column);
		json["message"] = WC2MB(wstr);
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
		set(json, "line", line);
		set(json, "column", column);
		set(json, "text", message);
		return json;
	}

	GherkinDocument::GherkinDocument(GherkinProvider& provider, const BoostPath& path)
		: provider(provider), filepath(path), filetime(boost::filesystem::last_write_time(path))
	{
		std::unique_ptr<FILE, decltype(&fclose)> file(fileopen(path), &fclose);
		if (file.get() == NULL)
			throw std::runtime_error("Failed to open file");

		try {
			reflex::Input input(file.get());
			GherkinLexer lexer(input);
			lexer.parse(*this);
		}
		catch (const std::exception& e) {
			errors.emplace_back(e);
		}
	}

	GherkinDocument::GherkinDocument(GherkinProvider& provider, const std::string& text)
		: provider(provider), filetime(0)
	{
		if (text.empty())
			throw std::runtime_error("Feature file is empty");

		try {
			reflex::Input input(text);
			GherkinLexer lexer(input);
			lexer.parse(*this);
		}
		catch (const std::exception& e) {
			errors.emplace_back(e);
		}
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
		lexer.elementStack.emplace_back(-2, &element);
	}

	void GherkinDocument::setDefinition(AbsractDef& definition, GherkinLexer& lexer, GherkinLine& line)
	{
		if (definition) {
			auto keyword = line.getKeyword();
			if (keyword)
				error(line, keyword->getName() + " keyword duplicate error");
			else
				error(line, "Unknown keyword type");
		}
		else {
			GherkinDefinition* def = nullptr;
			switch (line.getKeyword()->getType()) {
			case KeywordType::Feature:
				def = (GherkinDefinition*) new GherkinFeature(lexer, line);
				break;
			case KeywordType::Variables:
				def = (GherkinDefinition*) new GherkinVariables(lexer, line);
				break;
			default:
				def = (GherkinDefinition*) new GherkinDefinition(lexer, line);
			}
			definition.reset(def);
			resetElementStack(lexer, *def);
		}
	}

	void GherkinDocument::addScenarioDefinition(GherkinLexer& lexer, GherkinLine& line)
	{
		scenarios.emplace_back(std::make_unique<GherkinDefinition>(lexer, line));
		resetElementStack(lexer, *scenarios.back().get());
	}

	void GherkinDocument::addScenarioExamples(GherkinLexer& lexer, GherkinLine& line)
	{
		auto it = lexer.elementStack.begin();
		if (it == lexer.elementStack.end() || it->second->getType() != KeywordType::ScenarioOutline)
			throw GherkinException(lexer, "Parent element <Scenario outline> not found for <Examples>");

		while (lexer.elementStack.size() > 1)
			lexer.elementStack.pop_back();

		auto parent = lexer.elementStack.back().second;
		if (auto element = parent->push(lexer, line)) {
			lexer.elementStack.emplace_back(-1, element);
			lexer.lastElement = element;
		}
	}

	GherkinKeyword* GherkinDocument::matchKeyword(GherkinTokens& line)
	{
		return provider.matchKeyword(language, line);
	}

	void GherkinDocument::exception(GherkinLexer& lexer, const char* message)
	{
		std::stringstream stream_message;
		stream_message << (message != NULL ? message : "lexer error") << " at " << lexer.lineno() << ":" << lexer.columno();
		throw GherkinException(lexer, stream_message.str());
	}

	void GherkinDocument::error(GherkinLexer& lexer, const std::string& error)
	{
		errors.emplace_back(lexer, error);
	}

	void GherkinDocument::error(GherkinLine& line, const std::string& error)
	{
		errors.emplace_back(line.lineNumber, error);
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
			lexer.commentStack.emplace_back(lexer);
			break;
		case TokenType::Tag:
			lexer.tagStack.emplace_back(lexer);
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

	void GherkinDocument::addMultiline(GherkinLexer& lexer, GherkinLine& line)
	{
		auto type = line.getType();
		if (lexer.lastElement) {
			if (lexer.currentMultiline) {
				switch (type) {
				case TokenType::Line:
					lexer.currentMultiline->push(line);
					return;
				case TokenType::Multiline:
					lexer.currentMultiline->close(line);
					lexer.currentMultiline = nullptr;
					return;
				}
			}
			else if (type == TokenType::Multiline) {
				lexer.currentMultiline = lexer.lastElement->pushMultiline(line);
				return;
			}
		}
		//TODO: save error to error list
	}

	void GherkinDocument::addElement(GherkinLexer& lexer, GherkinLine& line)
	{
		while (!lexer.elementStack.empty()) {
			if (lexer.elementStack.back().first < line.indent) break;
			lexer.elementStack.pop_back();
		}
		if (lexer.elementStack.empty()) {
			throw GherkinException(lexer, "Element statck is empty");
		}
		auto parent = lexer.elementStack.back().second;
		if (auto element = parent->push(lexer, line)) {
			lexer.elementStack.emplace_back(line.indent, element);
			lexer.lastElement = element;
		}
	}

	void GherkinDocument::processLine(GherkinLexer& lexer, GherkinLine& line)
	{
		auto type = line.getType();
		if (type != TokenType::Table && type != TokenType::Comment) {
			lexer.currentTable = nullptr;
		}
		if (auto keyword = line.matchKeyword(*this)) {
			switch (keyword->getType()) {
			case KeywordType::Feature:
				setDefinition(feature, lexer, line);
				break;
			case KeywordType::Variables:
				setDefinition(variables, lexer, line);
				break;
			case KeywordType::Background:
				setDefinition(background, lexer, line);
				break;
			case KeywordType::Scenario:
			case KeywordType::ScenarioOutline:
				addScenarioDefinition(lexer, line);
				break;
			case KeywordType::Examples:
				addScenarioExamples(lexer, line);
				break;
			default:
				addElement(lexer, line);
			}
		}
		else {
			switch (type) {
			case TokenType::Asterisk:
			case TokenType::Operator:
			case TokenType::Symbol:
				addElement(lexer, line);
				break;
			case TokenType::Table:
				addTableLine(lexer, line);
				break;
			case TokenType::Line:
			case TokenType::Multiline:
				addMultiline(lexer, line);
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

	static bool hasExportSnippets(const StringLines& tags)
	{
		const std::string test = "ExportScenarios";
		for (auto& tag : tags) {
			if (comparei(tag.text, test)) {
				return true;
			}
		}
		return false;
	}

	void GherkinDocument::getExportSnippets(ScenarioMap& snippets) const
	{
		bool all = hasExportSnippets(getTags());
		for (auto& def : scenarios) {
			if (all || hasExportSnippets(def->getTags())) {
				auto snippet = def->getSnippet();
				auto it = snippets.find(snippet);
				if (it != snippets.end()) snippets.erase(it);
				snippets.emplace(snippet, ScenarioRef(*this, *def));
			}
		}
	}

	void GherkinDocument::getVariables(VariableCache& cache) const
	{
		if (variables) {
			auto vars = reinterpret_cast<GherkinVariables*>(variables.get());
			auto filename = lower(filepath.filename().wstring());
			auto it = cache.find(filename);
			if (it != cache.end()) cache.erase(filename);
			cache.emplace(filename, VariablesFile(filepath, vars->variables));
		}
	}

	void GherkinDocument::generate(const ScenarioMap& map)
	{
		try {
			if (background)
				background->generate(*this, map, {});

			for (auto& def : scenarios)
				def->generate(*this, map, {});
		}
		catch (const std::exception& e) {
			errors.emplace_back(e);
		}
	}

	bool GherkinDocument::isEscapedChar(wchar_t ch) const
	{
		if (provider.escapedCharacters.empty()) return true;
		return provider.escapedCharacters.find(ch) != std::string::npos;
	}

	const StringLines& GherkinDocument::getTags() const
	{
		static const StringLines empty;
		return feature ? feature->getTags() : empty;
	}

	JSON GherkinDocument::dump(const GherkinFilter& filter, const VariableCache& cache) const
	{
		JSON json;
		json["language"] = language;
		set(json, "filename", filepath.wstring());

		try {
			auto match = MatchType::Unknown;
			if (feature) {
				match = filter.match(feature->getTags());
				if (match == MatchType::Exclude)
					return JSON();
			}
			if (scenarios.empty()) {
				if (match == MatchType::Unknown)
					return JSON();
			}
			else {
				for (auto& scen : scenarios) {
					switch (filter.match(scen->getTags())) {
					case MatchType::Exclude:
						continue;
					case MatchType::Include:
						break;
					default:
						if (match == MatchType::Unknown)
							continue;
						else
							break;
					}
					json["scenarios"].push_back(*scen);
				}
				if (json["scenarios"].empty())
					return JSON();
			}
			set(json, "feature", feature);
			set(json, "background", background);
			set(json, "errors", errors);

			if (variables) {
				auto vars = reinterpret_cast<GherkinVariables*>(variables.get());
				set(json, "variables", vars->dump(cache));
			}
		}
		catch (const std::exception& e) {
			json["errors"].push_back(
				JSON({ "text" }, e.what())
			);
		}
		return json;
	}

	GherkinDocument::operator JSON() const
	{
		GherkinFilter filter({});
		return dump(filter, {});
	}
}
