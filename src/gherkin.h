#ifndef GHERKIN_H
#define GHERKIN_H

#include <vector>
#include <string>
#include <memory>
#include <set>
#include <sstream>
#include <boost/filesystem.hpp>
#include "json.hpp"

using JSON = nlohmann::json;

class GherkinLexer;
class GherkinParser;

namespace Gherkin {

	enum class TokenType {
		Language,
		Encoding,
		Multiline,
		Asterisk,
		Operator,
		Keyword,
		Comment,
		Number,
		Symbol,
		Colon,
		Param,
		Table,
		Cell,
		Line,
		Text,
		Date,
		Tag,
		None
	};

	enum class KeywordType {
		Feature,
		Background,
		Scenario,
		ScenarioOutline,
		Examples,
		And,
		But,
		If,
		Given,
		Rule,
		Then,
		When,
		None
	};

	class GherkinProvider;
	class GherkinDocument;
	class GherkinFilter;
	class GherkinElement;
	class AbsractDefinition;
	class GherkinDefinition;
	class ExportScenario;
	class GherkinKeyword;
	class GherkinToken;
	class GherkinGroup;
	class GherkinLine;
	class GherkinStep;

	using GherkinSnippet = std::wstring;
	using GherkinTags = std::vector<std::string>;
	using GherkinComments = std::vector<std::string>;
	using GherkinTokens = std::vector<GherkinToken>;
	using SnippetStack = std::set<GherkinSnippet>;
	using AbsractDef = std::unique_ptr<AbsractDefinition>;
	using GherkinDef = std::unique_ptr<GherkinDefinition>;
	using ScenarioMap = std::map<GherkinSnippet, std::unique_ptr<ExportScenario>>;
	using GherkinParams = std::map<std::wstring, GherkinToken>;

	class AbstractProgress {
	public:
		virtual void Start(const std::string& dir, size_t max, const std::string& info) = 0;
		virtual void Step(const boost::filesystem::path& path) = 0;
		virtual void Send(const std::string& msg) = 0;
	};

	class GherkinProvider {
	public:
		class Keyword {
		private:
			KeywordType type;
			std::string text;
			std::vector<std::wstring> words;
			friend class GherkinProvider;
			friend class GherkinKeyword;
		public:
			Keyword(KeywordType type, const std::string& text);
			GherkinKeyword* match(GherkinTokens& tokens) const;
			bool comp(const Keyword& other) const {
				return words.size() > other.words.size();
			}
		};
		using BoostPath = boost::filesystem::path;
		using BoostPaths = std::vector<BoostPath>;
		using Keywords = std::map<std::string, std::vector<Keyword>>;
		using FileCache = std::map<boost::filesystem::path, std::unique_ptr<GherkinDocument>>;
	private:
		class ScanParams;
		Keywords keywords;
		size_t identifier = 0;
		GherkinParser* parser = nullptr;
		ScenarioMap snippets;
		BoostPaths GetDirFiles(size_t id, const BoostPath& root) const;
		void ScanFolder(size_t id, AbstractProgress* progress, const BoostPath& root, ScanParams& params);
		void DumpFolder(size_t id, AbstractProgress* progress, const BoostPath& root, ScanParams& params);
	public:
		bool primitiveEscaping = false;
		std::string getKeywords() const;
		void setKeywords(const std::string& text);
		GherkinKeyword* matchKeyword(const std::string& lang, GherkinTokens& line) const;
		std::string ParseFolder(const std::string& dirs, const std::string& libs, const std::string& filter, AbstractProgress* progress = nullptr);
		std::string ParseFile(const std::wstring& path, const std::string& libs, AbstractProgress* progress = nullptr);
		std::string ParseText(const std::string& text);
		void ClearSnippets(const BoostPath& path);
		void ClearSnippets() { snippets.clear(); };
		void AbortScan() { ++identifier; };
	};

	class GherkinKeyword {
	private:
		KeywordType type;
		std::string text;
		bool toplevel;
	public:
		static KeywordType str2type(const std::string& text);
		static std::string type2str(KeywordType type);
		GherkinKeyword(const GherkinProvider::Keyword& source, bool toplevel)
			: type(source.type), text(source.text), toplevel(toplevel) {}
		GherkinKeyword(const GherkinKeyword& source)
			: type(source.type), text(source.text), toplevel(source.toplevel) {}
		KeywordType getType() const { return type; }
		operator JSON() const;
	};
 
	class GherkinToken {
	private:
		std::string type2str() const;
	public:
		std::wstring wstr;
		std::string text;
		TokenType type;
		size_t column;
		char symbol;
	public:
		GherkinToken(const GherkinToken& src)
			: type(src.type), wstr(src.wstr), text(src.text), column(src.column), symbol(src.symbol) {}
		GherkinToken(GherkinLexer& lexer, TokenType type, char ch);
		GherkinToken& operator=(const GherkinToken& src);
		friend std::wstringstream& operator<<(std::wstringstream& os, const GherkinToken& dt);
		std::string getText() const { return text; }
		std::wstring getWstr() const { return wstr; }
		TokenType getType() const { return type; }
		operator JSON() const;
	};

	class GherkinLine {
	private:
		std::wstring wstr;
		GherkinTokens tokens;
		std::string text;
		size_t lineNumber;
	private:
		std::unique_ptr<GherkinKeyword> keyword;
	public:
		GherkinLine(GherkinLexer& l);
		GherkinLine(size_t lineNumber);
		void push(GherkinLexer& lexer, TokenType type, char ch);
		GherkinKeyword* matchKeyword(GherkinDocument& document);
		const GherkinTokens getTokens() const { return tokens; }
		const GherkinKeyword* getKeyword() const { return keyword.get(); }
		size_t getLineNumber() const { return lineNumber; }
		std::wstring getWstr() const { return wstr; }
		std::string getText() const { return text; }
		TokenType getType() const;
		int getIndent() const;
		operator JSON() const;
	};

	class GherkinTable {
	public:
	private:
		size_t lineNumber;
		GherkinTokens head;
		std::vector<GherkinTokens> body;
	public:
		GherkinTable(const GherkinLine& line);
		void push(const GherkinLine& line);
		operator JSON() const;
	};

	class GeneratedScript {
	private:
		GherkinTokens tokens;
		GherkinParams params;
		std::vector<std::unique_ptr<GherkinElement>> steps;
	public:
		static GeneratedScript* generate(const GherkinStep& owner, const ScenarioMap& map, const SnippetStack& stack);
		GeneratedScript(const GherkinStep& owner, const ExportScenario& definition);
		const std::string filename;
		const GherkinSnippet snippet;
		operator JSON() const;
	};

	class GherkinElement {
	protected:
		std::wstring wstr;
		std::string text;
		size_t lineNumber;
		GherkinTags tags;
		GherkinComments comments;
		std::vector<std::unique_ptr<GherkinElement>> steps;
		std::vector<GherkinTable> tables;
		friend class GeneratedScript;
	public:
		GherkinElement(GherkinLexer& lexer, const GherkinLine& line);
		GherkinElement(const GherkinElement& src, const GherkinParams& params);
		virtual void generate(const ScenarioMap& map, const SnippetStack &stack);
		virtual GherkinElement* push(GherkinLexer& lexer, const GherkinLine& line);
		GherkinTable* pushTable(const GherkinLine& line);
		const GherkinTags& getTags() const { return tags; }
		virtual KeywordType getType() const { return KeywordType::None; }
		virtual GherkinSnippet getSnippet() const { return {}; }
		virtual GherkinElement* copy(const GherkinParams& params) const;
		virtual operator JSON() const;
	};

	class GherkinGroup
		: public GherkinElement {
	private:
		std::string name;
	public:
		GherkinGroup(GherkinLexer& lexer, const GherkinLine& line);
		GherkinGroup(const GherkinGroup& src, const GherkinParams& params);
		virtual GherkinElement* copy(const GherkinParams& params) const override;
		virtual operator JSON() const override;
	};

	class GherkinStep
		: public GherkinElement {
	private:
		GherkinKeyword keyword;
		GherkinTokens tokens;
		std::unique_ptr<GeneratedScript> script;
	public:
		GherkinStep(GherkinLexer& lexer, const GherkinLine& line);
		GherkinStep(const GherkinStep& src, const GherkinParams& params);
		const GherkinTokens& getTokens() const { return tokens; }
		virtual void generate(const ScenarioMap& map, const SnippetStack &stack) override;
		virtual GherkinSnippet getSnippet() const override;
		virtual GherkinElement* copy(const GherkinParams& params) const override;
		virtual operator JSON() const override;
	};

	class AbsractDefinition
		: public GherkinElement {
	protected:
		std::string name;
		GherkinKeyword keyword;
	public:
		AbsractDefinition(GherkinLexer& lexer, const GherkinLine& line);
		AbsractDefinition(const GherkinDocument& doc, const AbsractDefinition& def);
		virtual GherkinElement* push(GherkinLexer& lexer, const GherkinLine& line) override;
		virtual KeywordType getType() const override { return keyword.getType(); };
		virtual operator JSON() const override;
	};

	class GherkinFeature
		: public AbsractDefinition {
	private:
		std::vector<std::string> description;
	public:
		GherkinFeature(GherkinLexer& lexer, const GherkinLine& line);
		virtual GherkinElement* push(GherkinLexer& lexer, const GherkinLine& line) override;
		virtual operator JSON() const override;
	};

	class GherkinDefinition
		: public AbsractDefinition {
	protected:
		GherkinTokens tokens;
		std::unique_ptr<GherkinStep> examples;
	public:
		GherkinDefinition(GherkinLexer& lexer, const GherkinLine& line);
		GherkinDefinition(const GherkinDocument& doc, const GherkinDefinition& def);
		const GherkinTokens& getTokens() const { return tokens; }
		virtual GherkinElement* push(GherkinLexer& lexer, const GherkinLine& line) override;
		virtual GherkinSnippet getSnippet() const override;
		virtual operator JSON() const override;
	};

	class ExportScenario
		: public GherkinDefinition {
	public:
		ExportScenario(const GherkinDocument& doc, const GherkinDefinition& def);
		const boost::filesystem::path filepath;
	};

	class GherkinException
		: public std::runtime_error {
	private:
		const size_t line = 0;
		const size_t column = 0;
	public:
		GherkinException(GherkinLexer& lexer, const std::string& message);
		GherkinException(GherkinLexer& lexer, char const* const message);
		GherkinException(const GherkinException& src);
		GherkinException(char const* const message);
		operator JSON() const;
	};

	class GherkinError {
	private:
		size_t line = 0;
		size_t column = 0;
		std::string message;
	public:
		GherkinError(const std::exception& e) : message(e.what()) {}
		GherkinError(GherkinLexer& lexer, const std::string& message);
		GherkinError(size_t line, const std::string& message);
		operator JSON() const;
	};

	class GherkinDocument {
	private:
		GherkinProvider& provider;
		std::string language;
		AbsractDef feature;
		AbsractDef background;
		std::vector<GherkinDef> scenarios;
		std::vector<GherkinError> errors;
	private:
		void setLanguage(GherkinLexer& lexer);
		void processLine(GherkinLexer& lexer, GherkinLine& line);
		void setDefinition(AbsractDef& definition, GherkinLexer& lexer, GherkinLine& line);
		void addScenarioDefinition(GherkinLexer& lexer, GherkinLine& line);
		void addScenarioExamples(GherkinLexer& lexer, GherkinLine& line);
		void resetElementStack(GherkinLexer& lexer, GherkinElement& element);
		void addTableLine(GherkinLexer& lexer, GherkinLine& line);
		void addElement(GherkinLexer& lexer, GherkinLine& line);
	public:
		GherkinDocument(GherkinProvider& provider, const boost::filesystem::path& path);
		GherkinDocument(GherkinProvider& provider, const std::string& text);
		const boost::filesystem::path filepath;
		const time_t filetime;
		void next(GherkinLexer& lexer);
		void push(GherkinLexer& lexer, TokenType type, char ch = 0);
		void exception(GherkinLexer& lexer, const char* message);
		void error(GherkinLexer& lexer, const std::string& error);
		void error(GherkinLine& line, const std::string& error);
		GherkinKeyword* matchKeyword(GherkinTokens& line);
		void getExportSnippets(ScenarioMap& snippets) const;
		bool isPrimitiveEscaping() const;
		void generate(const ScenarioMap& map);
		const GherkinTags& getTags() const;
		JSON dump(const GherkinFilter &filter) const;
		operator JSON() const;
	};
}

#ifdef _WINDOWS

#define WM_PARSING_PROGRESS (WM_USER + 3)
#define WM_PARSING_FINISHED (WM_USER + 4)

#endif//_WINDOWS

#endif//GHERKIN_H
