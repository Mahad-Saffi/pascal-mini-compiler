/* Flex scanner for the Subset of Pascal - the lexer half of the Bison cross-check
 * oracle (tools/bison/pascal.y). It deliberately mirrors src/lexer/lexer.cpp so the
 * token stream Bison's LALR(1) parser sees matches the one our three parsers see:
 *   - keywords are reserved and case-insensitive  (%option caseless)
 *   - identifiers are letter (letter|digit)*
 *   - num is an unsigned integer or real constant
 *   - { ... } brace comments are skipped (no nesting, no (* *) form), exactly as
 *     our hand-written lexer does
 *   - the longest-match rule gives :=, .., <=, >=, <> priority over :, ., <, >, =
 * This file has no actions beyond returning token codes; the grammar decides the
 * accept/reject verdict. Build + run via tools/bison/cross_check.sh (or
 * `make cross-check`), which gates on bison/flex being installed.
 */
%option noyywrap
%option caseless
%option nounput noinput

%{
#include "pascal.tab.h"
%}

ws      [ \t\r\n]+
digit   [0-9]
letter  [A-Za-z]
id      {letter}({letter}|{digit})*
int     {digit}+
real    {digit}+"."{digit}+([eE][+-]?{digit}+)?|{digit}+[eE][+-]?{digit}+

%%
{ws}            { /* whitespace between tokens is optional and ignored */ }
"{"[^}]*"}"     { /* Pascal brace comment - skipped like the real lexer */ }

"program"       return PROGRAM;
"var"           return VAR;
"array"         return ARRAY;
"of"            return OF;
"integer"       return INTEGER;
"real"          return REAL;
"function"      return FUNCTION;
"procedure"     return PROCEDURE;
"begin"         return BEGINT;
"end"           return ENDT;
"if"            return IFT;
"then"          return THEN;
"else"          return ELSE;
"while"         return WHILE;
"do"            return DO;
"not"           return NOT;
"or"            return OR;
"div"           return DIV;
"mod"           return MOD;
"and"           return AND;

":="            return ASSIGNOP;
".."            return DOTDOT;
"<="            return LE;
">="            return GE;
"<>"            return NE;

{real}          return NUM;
{int}           return NUM;
{id}            return ID;

"<"             return '<';
">"             return '>';
"="             return '=';
"+"             return '+';
"-"             return '-';
"*"             return '*';
"/"             return '/';
"("             return '(';
")"             return ')';
"["             return '[';
"]"             return ']';
";"             return ';';
","             return ',';
":"             return ':';
"."             return '.';

.               { return yytext[0]; }  /* any stray char -> unexpected token -> reject */
%%
