
/*
 *
 * CS-252 
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */

%{

#include <cstring>
#include "y.tab.hh"

static  void yyunput (int c,char *buf_ptr  );

void myunputc(int c) {
  unput(c);
}

%}

%option noyywrap
%option stack
%%

\n {
  return NEWLINE;
}

[ \t] {
  /* Discard spaces and tabs */
}

">" {
  return GREAT;
}

">>" {
  return GREATGREAT;
}

"|" {
  return PIPE;
}

"<" {
  return LESS;
}

"&" {
  return AMPERSAND;
}

"2>" {
  return TWOGREAT;
}

">&" {
  return GREATAMPERSAND;
}

">>&" {
  return GREATGREATAMPERSAND;
}

\"[^\"]*\" {
  /* Remove quotes and return contents as a single WORD */
  yytext[yyleng-1] = '\0';        /* remove closing quote */
  yylval.cpp_string = new std::string(yytext + 1);  /* skip opening quote */
  return WORD;
}

"${"[a-zA-Z_][a-zA-Z0-9_]*"}" {
  /* variable expansion ${VAR} */
  std::string varName(yytext + 2, yyleng - 3);
  char* val = getenv(varName.c_str());
  yylval.cpp_string = new std::string(val ? val : "");
  return WORD;
}

(\\.|\`[^\`]*\`|[^ \t\n\"\`\\><!|&])+ {
  /* handle escape characters */
  std::string * s = new std::string();
  for (int i = 0; i < yyleng; i++) {
    if (yytext[i] == '\\') {
      i++;  // skip the backslash
      s->push_back(yytext[i]);  // keep the next character literally
    } else {
      s->push_back(yytext[i]);
    }
  }
  yylval.cpp_string = s;
  return WORD;
}

<<EOF>> {
    yypop_buffer_state();
    if (!YY_CURRENT_BUFFER) {
        yyterminate();
    }
}
