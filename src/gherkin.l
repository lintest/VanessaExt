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
  GherkinTags tags() const {
    return document.tags();
  }
  std::string dump() const {
    return document.dump();
  }
}

%option fast nodefault unicode
%xstate TABLE
%xstate TAG
%xstate TEXT

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
text          [^\r\n]+
tag           @

%%

<INITIAL>{new_line}   { next(); start(INITIAL); }
<INITIAL>{space}      // ignore
<INITIAL>{colon}      { push(Gherkin::Colon); }
<INITIAL>{operator}   { push(Gherkin::Operator); }
<INITIAL>{param}      { push(Gherkin::Param); }
<INITIAL>{date}       { push(Gherkin::Date); }
<INITIAL>{number}     { push(Gherkin::Number); }
<INITIAL>{comment}    { push(Gherkin::Comment); }
<INITIAL>{table}      { push(Gherkin::Table); start(TABLE); }
<INITIAL>{tag}        { push(Gherkin::Tag); start(TAG); }
<INITIAL>.            { push(Gherkin::Symbol); }

<TABLE>{new_line}     { next(); start(INITIAL); }
<TABLE>{space}        // ignore
<TABLE>{table}        { push(Gherkin::Table); }
<TABLE>{cell}         { push(Gherkin::Cell); }

<TAG>{new_line}       { next(); start(INITIAL); }
<TAG>{space}          // ignore
<TAG>{operator}       { push(Gherkin::Operator); }
<TAG>{colon}          { push(Gherkin::Colon); start(TEXT); }

<TEXT>{new_line}       { next(); start(INITIAL); }
<TEXT>{text}           { push(Gherkin::Text); }

%%
