%option noinput
%option nounput
%option noyywrap
%option yylineno

%{
#include "ast/tree.hpp"
#include "parser/parser.tab.hh"
#include <iostream>
extern FILE *input;
%}

digit [0-9]
blank [ \t\n]
letter [_a-zA-Z]
linecomment \/\/[^\n]*
comment \/\*([^*]|\*[^/])*\*\/
identifier {letter}({letter}|{digit})*

%%

"0"             { yylval.int_val = 0; return INTCONST; }
[1-9]{digit}*   { yylval.int_val = atoi(yytext); return INTCONST; }
"0"[0-7]*   { yylval.int_val = strtol(yytext, NULL, 8); return INTCONST; }
"0X"[0-9a-fA-F]+  { yylval.int_val = strtol(yytext, NULL, 16); return INTCONST; }
"0x"[0-9a-fA-F]+  { yylval.int_val = strtol(yytext, NULL, 16); return INTCONST; }
"+"             { return ADD; }
"-"             { return SUB; }
"*"             { return MUL; }
"/"             { return DIV; }
"%"             { return MOD; }
"!"             { return NOT; }
"("             { return LPAREN; }
")"             { return RPAREN; }
"{"             { return LBRACE; }
"}"             { return RBRACE; }
"["             { return LBRACKET; }
"]"             { return RBRACKET; }
"="             { return ASSIGN; }
"=="            { return EQU; }
"!="            { return NEQ; }
">="            { return GEQ; }
"<="            { return LEQ; }
">"             { return GRE; }
"<"             { return LES; }
"&&"            { return AND; }
"||"            { return OR; }
","             { return COMMA; }
";"             { return SEMICOLON; }
"if"            { return IF; }
"else"          { return ELSE; }
"while"          { return WHILE; }
"return"        { return RETURN; }
"int"           { return INT; }
"void"          { return VOID; }
{identifier}    { yylval.str_val = strdup(yytext); return IDENT; }
{linecomment}   { }
{comment}       { }
{blank}         { }
.               { throw std::runtime_error("Unknown token '" + std::string(yytext) + "' at line " + std::to_string(yylineno)); }

%%

