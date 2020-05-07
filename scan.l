%option noyywrap
%option nodefault
%option yylineno
%{
#include <string.h>
#include <stdlib.h>
#include "attr.h"
#include "parse.tab.h"
int fileno(FILE *stream);
%}

id [_a-zA-Z][_a-zA-Z0-9]*

%%
[0-9]+	{
			yylval.token.valuetype = INTTYPE;
			yylval.token.value = atoi(yytext);
			yylval.token.nodekind = CONSTKIND;
			return CONST;
		}
"true"	{
			yylval.token.valuetype = BOOLTYPE;
			yylval.token.value = 1;
			yylval.token.nodekind = CONSTKIND;
			return CONST;
		}
"false"	{
			yylval.token.valuetype = BOOLTYPE;
			yylval.token.value = 0;
			yylval.token.nodekind = CONSTKIND;
			return CONST;
		}
"def"	{ return DEF; }
"main"	{ return MAIN; }
"end"	{ return END; }
"print"	{ return PRINT; }
"if"	{ return IF; }
"do"	{ return DO; }
"else"	{ return ELSE; }
"for"	{ return FOR; }
"while"	{ return WHILE; }
"return"	{ return RETURN; }
"int"	{ return INT; }
"bool"	{ return BOOL; }
"<="	{ return LE; }
"=="	{ return EQ; }
"!="	{ return NEQ; }
">="	{ return GE; }
"&&"	{ return AND; }
"||"	{ return OR; }
"="	{ return yytext[0]; }
"<"	{ return yytext[0]; }
">"	{ return yytext[0]; }
"+"	{ return yytext[0]; }
"-"	{ return yytext[0]; }
"*"	{ return yytext[0]; }
"/"	{ return yytext[0]; }
"!"	{ return yytext[0]; }
"("	{ return yytext[0]; }
")"	{ return yytext[0]; }
";"	{ return yytext[0]; }
":"	{ return yytext[0]; }
","	{ return yytext[0]; }

{id}	{
			yylval.token.nodekind = IDKIND;
			strcpy(yylval.token.name, yytext);
			return ID;
		}

"/*"	{
			char in;
			while (1) {
				in = input();
				if (in == '*') {
					in = input();
					if (in == '/')	break;
				}
			}
		}

[ \t\n]+	{}
"//".*\n	{}
.	{ fprintf(stderr, "%c is an invalid character\n", yytext[0]); }
%%


