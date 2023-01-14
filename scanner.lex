%{

/* Declarations section */
#include <stdio.h>
#include "types.h"
#include "parser.tab.hpp"
#include "hw3_output.hpp"

%}

%option yylineno
%option noyywrap
digit   		([0-9])
letter  		([a-zA-Z])
whitespace		([\t\n \r])
relop           ([<>!]=?|==)
%%

0|[1-9]{digit}*      		yylval.numNode = new NumNode(yytext); return NUM;
void                        return VOID;
int                         return INT;
byte                        return BYTE;
b                           return B;
bool                        return BOOL;
and                         return AND;
or                          return OR;
not                         return NOT;
true                        yylval.boolNode = new BoolNode(true); return TRUE;
false                       yylval.boolNode = new BoolNode(false);return FALSE;
return                      return RETURN;
if                          return IF;
else                        return ELSE;
while                       return WHILE;
break                      return BREAK;
continue                   return CONTINUE;
;                           return SC;
,                           return COMMA;
\(                          return LPAREN;
\)                          return RPAREN;
\{                          return LBRACE;
\}                          return RBRACE;
{relop}                     yylval.relopNode = new RelopNode(yytext); return RELOP;
=                           return ASSIGN;
[+-]                          yylval.binopNode = new BinopNode(yytext[0]); return ADD_MIN;
[*/]                         yylval.binopNode = new BinopNode(yytext[0]); return MUL_DIV;

{letter}({letter}|{digit})* yylval.idNode = new IdNode(yytext); return ID;
\"([^\n\r\"\\]|\\[rnt"\\])+\" yylval.stringNode = new StringNode(yytext); return STRING;
{whitespace}                ;
\/\/[^\r\n]*[ \r|\n|\r\n]? 	;
.                           {output::errorLex(yylineno); exit(0);};

%%
