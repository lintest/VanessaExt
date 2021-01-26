%top{
  #include "gherkin.h"
%}

%class{
private:
  GherkinDocument document;
public:
  void next() {
    document.next();
  }
  void push(Gherkin::TokenType type) {
    document.push(type, *this);
  }
  std::string dump() {
    return document.dump();
  }
}

%option fast nodefault unicode
%xstate TABLE

new_line      \r?\n
operator      \w+
number        -?(\\d*\\.)?\\d+
date          \d+\.\d+\.\d+|\d+\/\d+\/\d+
param         \"[^\"\n\r]*\"|'[^'\n\r]*'|<[^>\n\r]*>
comment       #[^\n\r]*|\/\/[^\n\r]*
space         \h+
colon         :
table         \|
cell          [^\n\r\|]+
tag           @

%%

<INITIAL>{new_line}   { next(); start(INITIAL); }
<INITIAL>{space}      // ignore
<INITIAL>{tag}        { push(Gherkin::Tag); }
<INITIAL>{colon}      { push(Gherkin::Colon); }
<INITIAL>{operator}   { push(Gherkin::Operator); }
<INITIAL>{param}      { push(Gherkin::Param); }
<INITIAL>{date}       { push(Gherkin::Date); }
<INITIAL>{number}     { push(Gherkin::Number); }
<INITIAL>{comment}    { push(Gherkin::Comment); }
<INITIAL>{table}      { push(Gherkin::Table); start(TABLE); }
<INITIAL>.            { push(Gherkin::Symbol); }

<TABLE>{new_line}     { next(); start(INITIAL); }
<TABLE>{space}        // ignore
<TABLE>{table}        { push(Gherkin::Table); }
<TABLE>{cell}         { push(Gherkin::Cell); }

%%
