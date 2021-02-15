%top{
	#include "gherkin.h"
	using namespace Gherkin;
%}

%class{
public:
	TokenType current_token;
	GherkinDocument* document;
	std::vector<GherkinLine> lines;
	GherkinLine* currentLine = nullptr;
	GherkinTable* currentTable = nullptr;
	GherkinMultiline* currentMultiline = nullptr;
	StringLines tagStack;
	StringLines commentStack;
	std::vector<std::pair<int, GherkinElement*>> elementStack;
	GherkinElement* lastElement = nullptr;
public:
	void parse(GherkinDocument& doc) {
		document = &doc;
		lex();
	}
	void next() {
		document->next(*this);
	}
	void push(TokenType tt, char ch = 0) {
		document->push(*this, tt, ch);
	}
	void tail(TokenType type) {
		current_token = type;
		start(TAIL);
	}
	bool isPrimitiveEscaping() const {
		return document ? document->isPrimitiveEscaping() : false;
	}
	virtual void lexer_error(const char *message = NULL) override {
		document->exception(*this, message);
	}
}

%option fast nodefault unicode

%xstate LINE
%xstate TAG
%xstate TAIL
%xstate TABLE
%xstate STR_A
%xstate STR_D

new_line      \r?\n
language      #\s*(?i)(language)\s*:\s*
encoding      #\s*(?i)(encoding)\s*:\s*
asterisk      \*
operator      \w+
number        -?(\d*[\.\,])?\d+
date          \d+\.\d+\.\d+|\d+\/\d+\/\d+
param_d       \"([^\\\"\n\r]|\\.)*\"
param_a       `([^\\`\n\r]|\\.)*`
param_b       <([^\\>\n\r]|\\.)*>
param_s       '([^\\'\n\r]|\\.)*'
comment       #|\/\/
space         \h+
colon         :
table         \|
cell          (\\\||[^\s\|])(\\\||[^\|\n\r\\])*
text          [^\s][^\r\n]*
str_a         \`\`\`[^\n\r]*
str_d         \"\"\"[^\n\r]*
tag           @

%%

<INITIAL>{new_line}   { next(); start(INITIAL); }
<INITIAL>{space}      // ignore
<INITIAL>{asterisk}   { tail(TokenType::Asterisk); }
<INITIAL>{language}   { tail(TokenType::Language); }
<INITIAL>{encoding}   { tail(TokenType::Encoding); }
<INITIAL>{comment}    { tail(TokenType::Comment); }
<INITIAL>{tag}        { tail(TokenType::Tag); }
<INITIAL>{operator}   { start(LINE);  push(TokenType::Operator); }
<INITIAL>{table}      { start(TABLE); push(TokenType::Table); }
<INITIAL>{str_a}      { start(STR_A); push(TokenType::Multiline); }
<INITIAL>{str_d}      { start(STR_D); push(TokenType::Multiline); }
<INITIAL>.            { start(LINE);  push(TokenType::Symbol); }

<LINE>{new_line}      { next(); start(INITIAL); }
<LINE>{space}         // ignore
<LINE>{comment}       { tail(TokenType::Comment); }
<LINE>{colon}         { push(TokenType::Colon); }
<LINE>{date}          { push(TokenType::Date); }
<LINE>{number}        { push(TokenType::Number); }
<LINE>{operator}      { push(TokenType::Operator); }
<LINE>{param_a}       { push(TokenType::Param, '`'); }
<LINE>{param_b}       { push(TokenType::Param, '>'); }
<LINE>{param_d}       { push(TokenType::Param, '\"'); }
<LINE>{param_s}       { push(TokenType::Param, '\''); }
<LINE>.               { push(TokenType::Symbol); }

<TAIL>{new_line}      { next(); start(INITIAL); }
<TAIL>{text}          { push(current_token); }
<TAIL>{space}         // ignore

<TABLE>{new_line}     { next(); start(INITIAL); }
<TABLE>{table}        { push(TokenType::Table); }
<TABLE>{date}         { push(TokenType::Date); }
<TABLE>{number}       { push(TokenType::Number); }
<TABLE>{param_a}      { push(TokenType::Param, '`'); }
<TABLE>{param_b}      { push(TokenType::Param, '>'); }
<TABLE>{param_d}      { push(TokenType::Param, '\"'); }
<TABLE>{param_s}      { push(TokenType::Param, '\''); }
<TABLE>{cell}         { push(TokenType::Param); }
<TABLE>{space}        // ignore

<STR_A>{new_line}     { next(); }
<STR_A>{str_a}        { push(TokenType::Multiline); start(INITIAL); }
<STR_A>{text}         { push(TokenType::Line); }
<STR_A>{space}         // ignore

<STR_D>{new_line}     { next(); }
<STR_D>{str_d}        { push(TokenType::Multiline); start(INITIAL); }
<STR_D>{text}         { push(TokenType::Line); }
<STR_D>{space}         // ignore

<<EOF>>               { next(); return 0; }

%%
