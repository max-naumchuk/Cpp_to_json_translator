%option noyywrap
%option yylineno
%{
#include <string>
#include "parser.tab.hpp"
%}

%%

[0-9]+\.[0-9]+          { yylval.dval = atof(yytext); return TOK_REAL_LITERAL; }
[0-9]+                  { yylval.ival = atol(yytext); return TOK_INTEGER_LITERAL; }
\"([^\"\\]|\\.)*\"      { yylval.sval = new std::string(yytext + 1, yyleng - 2); return TOK_STRING_LITERAL; }

"break"                   { return TOK_BREAK; }
"case"                    { return TOK_CASE; }
"class"                   { return TOK_CLASS; }
"const"                   { return TOK_CONST; }
"continue"                { return TOK_CONTINUE; }
"default"                 { return TOK_DEFAULT; }
"delete"                  { return TOK_DELETE; }
"do"                      { return TOK_DO; }
"else"                    { return TOK_ELSE; }
"enum"                    { return TOK_ENUM; }
"for"                     { return TOK_FOR; }
"friend"                  { return TOK_FRIEND; }
"goto"                    { return TOK_GOTO; }
"if"                      { return TOK_IF; }
"new"                     { return TOK_NEW; }
"return"                  { return TOK_RETURN; }
"static_cast"             { return TOK_STATIC_CAST; }
"static"                  { return TOK_STATIC; }
"struct"                  { return TOK_STRUCT; }
"switch"                  { return TOK_SWITCH; }
"typedef"                 { return TOK_TYPEDEF; }
"union"                   { return TOK_UNION; }
"while"                   { return TOK_WHILE; }

"int"                     { return TOK_KW_INT; }
"double"                  { return TOK_KW_DOUBLE; }
"float"                   { return TOK_KW_FLOAT; }
"char"                    { return TOK_KW_CHAR; }
"bool"                    { return TOK_KW_BOOL; }
"void"                    { return TOK_KW_VOID; }
"long"                    { return TOK_KW_LONG; }
"short"                   { return TOK_KW_SHORT; }
"unsigned"                { return TOK_UNSIGNED; }


[a-zA-Z_][a-zA-Z_0-9]*   { yylval.sval = new std::string(yytext, yyleng); return TOK_IDENTIFIER; }


"<<="                    { return TOK_SHL_ASSIGN; }
">>="                    { return TOK_SHR_ASSIGN; }
"<<"                      { return TOK_SHL; }
">>"                      { return TOK_SHR; }
"<="                      { return TOK_LE; }
">="                      { return TOK_GE; }
"=="                      { return TOK_EQ; }
"!="                      { return TOK_NE; }
"&&"                      { return TOK_LOGICAL_AND; }
"||"                      { return TOK_LOGICAL_OR; }
"++"                      { return TOK_INCREMENT; }
"--"                      { return TOK_DECREMENT; }
"+="                      { return TOK_PLUS_ASSIGN; }
"-="                      { return TOK_MINUS_ASSIGN; }
"*="                      { return TOK_STAR_ASSIGN; }
"/="                      { return TOK_SLASH_ASSIGN; }
"%="                      { return TOK_PERCENT_ASSIGN; }
"&="                      { return TOK_AMP_ASSIGN; }
"|="                      { return TOK_PIPE_ASSIGN; }
"^="                      { return TOK_CARET_ASSIGN; }

"+"                        { return TOK_PLUS; }
"-"                        { return TOK_MINUS; }
"*"                        { return TOK_STAR; }
"/"                        { return TOK_SLASH; }
"%"                        { return TOK_PERCENT; }
"="                        { return TOK_ASSIGN; }
"&"                        { return TOK_AMP; }
"|"                        { return TOK_PIPE; }
"^"                        { return TOK_CARET; }
"~"                        { return TOK_TILDE; }
"!"                        { return TOK_LOGICAL_NOT; }
"<"                        { return TOK_LT; }
">"                        { return TOK_GT; }

"("                        { return TOK_LPAREN; }
")"                        { return TOK_RPAREN; }
"{"                        { return TOK_LBRACE; }
"}"                        { return TOK_RBRACE; }
"["                        { return TOK_LBRACKET; }
"]"                        { return TOK_RBRACKET; }
";"                        { return TOK_SEMICOLON; }
":"                        { return TOK_COLON; }
","                        { return TOK_COMMA; }
"?"                        { return TOK_QUESTION; }

[ \t\r\n]+                 {  }

.                           {}

%%
