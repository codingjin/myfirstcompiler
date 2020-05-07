%{
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "attr.h"
#include "instruction.h"

extern int yylineno;
FILE *outfile;
void yyerror(const char *s, ...);
int yyparse();
int yylex();
Node *gast;
extern void insert(HashNode **table, HashNode *hashnode);
%}

%union {
	Node *node;
	ValueType valuetype;
	Token token;
}

%token DEF MAIN END
%token <token> ID CONST
%token PRINT INT BOOL
%token LE EQ NEQ GE
%token AND OR
%token IF DO ELSE FOR WHILE
%token RETURN

%type <valuetype> type
%type <node> program main vardecls vardecl stmts stmt
%type <node> assign print exp
%type <node> ifstmt whilestmt forstmt returnstmt
%type <node> functions function params someparams param moreparams
%type <node> args someargs moreargs

%right '='
%left OR
%left AND '^'
%left EQ NEQ
%left '<' '>' LE GE
%left '+' '-'
%left '*' '/'
%nonassoc '!'

%start program

%%

program	:	functions { gast = $1; }
		;

functions	:	function functions
{
	$1->next = $2;
	$$ = $1;
}
			|	main
			;

function	:	DEF ID '(' params ')' ':' type vardecls stmts END
{
	Node *n = getnewnode();
	n->nodekind = FUNCKIND;
	strcpy(n->name, $2.name);
	n->valuetype = $7;
	n->childrennum = 3;
	n->children[0] = $4;
	n->children[1] = $8;
	n->children[2] = $9;
	n->label = NewLabel();

	Node *np = $4;
	while (np) {
		++n->param_num;
		np = np->next;
	}
	Node *dp = $8;
	while (dp) {
		++n->decl_num;
		dp = dp->next;
	}

	// check the returnstmt
	Node *last = $9;
	if (!last)
		yyerror("invalid function definition, return stmt missing");

	while (last->next) {
		if (last->nodekind == RETURNKIND)	yyerror("invalid definition, returnstmt should be the last stmt");
		last = last->next;
	}
	if (last->nodekind != RETURNKIND)
		yyerror("invalid function definition, return stmt should be the last stmt");
	
	// return type
	if (last->valuetype == NOTYPE) {
		last->valuetype=$7;
		last->children[0]->valuetype = $7;
	}else if (last->valuetype != $7)
		yyerror("return valuetype[%d] and functype[%d] donot match", last->valuetype, $7);

	$$ = n;
}
			;

params	:	{ $$ = NULL; }
		|	someparams
		;

someparams	:	param moreparams
{
	$1->next = $2;
	if ($2)
		sprintf($1->info, "%s, %s", $1->info, $2->info);
	$$ = $1;
}
			;

param	:	type ID
{
	Node *n = getnewnode();
	n->nodekind = PARAMKIND;
	strcpy(n->name, $2.name);
	n->valuetype = $1;
	if ($1 == INTTYPE)
		sprintf(n->info, "int %s", $2.name);
	else if ($1 == BOOLTYPE)
		sprintf(n->info, "bool %s", $2.name);

	$$ = n;
}
		;

moreparams	:	',' someparams { $$ = $2; }
			|	{ $$ = NULL; }
			;

main	:	DEF MAIN vardecls stmts END
{
	Node *n = getnewnode();
	n->nodekind = MAINKIND;
	strcpy(n->name, "main");
	n->childrennum = 2;
	n->children[0] = $3;
	n->children[1] = $4;
	n->label = NewLabel();
	n->valuetype = NOTYPE;
	
	Node *dp = $3;
	while (dp) {
		++n->decl_num;
		dp = dp->next;
	}

	// check stmts, we do not allow return stmt in main
	Node *s = $4;
	while (s) {
		if (s->nodekind == RETURNKIND)	yyerror("main should not contain return stmt");
		s = s->next;
	}

	$$ = n;
}
		;

vardecls	:	/*empty*/ { $$ = NULL; }
			|	vardecl ';' vardecls
{
	$1->next = $3;
	if ($3)
		sprintf($1->info, "%s;%s", $1->info, $3->info);
	$$ = $1;
}
			;

vardecl	:	type ID
{
	Node *n = getnewnode();
	n->nodekind = VARDECLKIND;
	strcpy(n->name, $2.name);
	n->valuetype = $1;
	sprintf(n->info, "type[%d] %s", $1, $2.name);
	$$ = n;
}
		;

type	:	INT { $$ = INTTYPE; }
		|	BOOL { $$ = BOOLTYPE; }
		;

stmts	:	stmt ';' stmts
{
	$1->next = $3;
	if ($3)
		sprintf($1->info, "%s;%s", $1->info, $3->info);
	$$ = $1;
}
		|	stmt ';'
		;

stmt	:	assign
		|	print
		|	ifstmt
		|	whilestmt
		|	forstmt
		|	returnstmt
		;

whilestmt	:	WHILE '(' exp ')' DO stmts END
{
	if ($3->nodekind==CONSTKIND || $3->nodekind==EXPKIND) {
		if ($3->valuetype != BOOLTYPE)
			yyerror("cond exp in whilestmt is not BOOLTYPE[%d]", $3->valuetype);
	}else if ($3->nodekind == IDKIND)	$3->valuetype = BOOLTYPE;
	else	yyerror("cond exp is invalid nodekind in whilestmt");

	Node *n = getnewnode();
	n->nodekind = WHILEKIND;
	n->childrennum = 2;
	n->children[0] = $3;
	n->children[1] = $6;
	sprintf(n->info, "while (%s) DO %s END", $3->info, $6->info);
	$$ = n;
}
			;

forstmt	:	FOR '(' stmt ';' exp ';' stmt ')' DO stmts END
{
	if ($5->nodekind==CONSTKIND || $5->nodekind==EXPKIND) {
		if ($5->valuetype != BOOLTYPE)
			yyerror("invalid valuetype of cond exp in forstmt");
	}else if ($5->nodekind == IDKIND)	$5->valuetype = BOOLTYPE;
	else	yyerror("invalid nodekind of cond exp in forstmt");

	Node *n = getnewnode();
	n->nodekind = FORKIND;
	n->childrennum = 4;
	n->children[0] = $5;
	n->children[1] = $3;
	n->children[2] = $7;
	n->children[3] = $10;
	sprintf(n->info, "for (%s ; %s ; %s ) DO %s END",
						$3->info, $5->info, $7->info, $10->info);
	$$ = n;
}
		;

ifstmt	:	IF '(' exp ')' DO stmts END
{
	if ($3->nodekind==CONSTKIND || $3->nodekind==EXPKIND) {
		if ($3->valuetype != BOOLTYPE)
			yyerror("exp in ifstmt is invalid valuetype[%d]", $3->valuetype);
	}else if ($3->nodekind == IDKIND)	$3->valuetype = BOOLTYPE;
	else	yyerror("invalid nodekind[%d] of cond exp in ifstmt", $3->nodekind);

	Node *n = getnewnode();
	n->nodekind = IFKIND;
	n->childrennum = 2;
	n->children[0] = $3;
	n->children[1] = $6;
	sprintf(n->info, "if (%s) DO %s END", $3->info, $6->info);
	$$ = n;
}
		|	IF '(' exp ')' DO stmts ELSE stmts END
{
	if ($3->nodekind==CONSTKIND || $3->nodekind==EXPKIND) {
		if ($3->valuetype != BOOLTYPE)
			yyerror("exp in ifstmt is invalid valuetype[%d]", $3->valuetype);
	}else if ($3->nodekind == IDKIND)	$3->valuetype = BOOLTYPE;
	else	yyerror("invalid nodekind of cond exp in ifstmt");

	Node *n = getnewnode();
	n->nodekind = IFELSEKIND;
	n->childrennum = 3;
	n->children[0] = $3;
	n->children[1] = $6;
	n->children[2] = $8;
	sprintf(n->info, "if (%s) DO %s ELSE %s END", $3->info, $6->info, $8->info);
	$$ = n;
}
		;

assign	:	ID '=' exp
{
	Node *n = getnewnode();
	n->nodekind = ASSIGNKIND;
	strcpy(n->name, $1.name);
	n->valuetype = NOTYPE;
	n->childrennum = 1;
	n->children[0] = $3;
	sprintf(n->info, "%s=%s", $1.name, $3->info);
	$$ = n;
}
		;

print	:	PRINT '(' ID ')'
{
	Node *n = getnewnode();
	n->nodekind = PRINTKIND;
	strcpy(n->name, $3.name);
	sprintf(n->info, "print(%s)", n->name);
	$$ = n;
}
		;

exp	:	CONST 
{
	Node *node = getnewnode();
	node->nodekind = CONSTKIND;
	node->valuetype = $1.valuetype;
	node->value = $1.value;
	sprintf(node->info, "CONST valuetype[%d] %d", node->valuetype, node->value);
	$$ = node;
}
	|	ID
{
	Node *node = getnewnode();
	node->nodekind = IDKIND;
	node->valuetype = NOTYPE;
	strcpy(node->name, $1.name);
	sprintf(node->info, "ID %s", node->name);
	$$ = node;
}
	|	'!' exp
{
	if ($2->nodekind==CONSTKIND) {
		if ($2->valuetype != BOOLTYPE)
			yyerror("! only works on BOOLTYPE, valuetype[%d] mismatches", $2->valuetype);
		
		$2->value = ! $2->value;
		sprintf($2->info, "CONST valuetype[%d] %d", $2->valuetype, $2->value);
		$$ = $2;
	}else {
		if ($2->nodekind==IDKIND || $2->nodekind==CALLKIND)
			$2->valuetype = BOOLTYPE;
		else if ($2->nodekind == EXPKIND) {
			if ($2->valuetype != BOOLTYPE) {
				yyerror("! only works on BOOLTYPE, exp valuetype[%d] mismatches",
					$2->valuetype);
			}
		}else
			yyerror("exp nodekind[%d] error", $2->nodekind);

		Node *node = getnewnode();
		node->nodekind = EXPKIND;
		node->valuetype = BOOLTYPE;
		node->opkind = NOTK;
		node->childrennum = 1;
		node->children[0] = $2;
		sprintf(node->info, "! %s", $2->info);
		$$ = node;
	}
}
	|	'(' exp ')' { $$ = $2; }
	|	exp '+' exp
{
	if ($1->nodekind==CONSTKIND && $3->nodekind==CONSTKIND) {
		if (!($1->valuetype==INTTYPE && $3->valuetype==INTTYPE))
			yyerror("+ only works on INTTYPE, valuetype mismatch");

		$1->value = $1->value + $3->value;
		sprintf($1->info, "CONST INT %d", $1->value);
		$$ = $1;
		free($3);
	}else {
		if ($1->nodekind==CONSTKIND || $1->nodekind==EXPKIND) {
			if ($1->valuetype != INTTYPE)
				yyerror("+ works on INTTYPE, valuetype[%d] mismatch", $1->valuetype);
		}else if ($1->nodekind==IDKIND || $1->nodekind==CALLKIND)
			$1->valuetype = INTTYPE;
		else	yyerror("invalid nodekind[%d]", $1->nodekind);

		if ($3->nodekind==CONSTKIND || $3->nodekind==EXPKIND) {
			if ($3->valuetype != INTTYPE)
				yyerror("+ works on INTTYPE, valuetype[%d] mismatch", $3->valuetype);
		}else if ($3->nodekind==IDKIND || $3->nodekind==CALLKIND)
			$3->valuetype = INTTYPE;
		else	yyerror("invalid nodekind[%d]", $3->nodekind);
		
		Node *node = getnewnode();
		node->nodekind = EXPKIND;
		node->valuetype = INTTYPE;
		node->opkind = ADDK;
		node->childrennum = 2;
		node->children[0] = $1;
		node->children[1] = $3;
		sprintf(node->info, "%s + %s", $1->info, $3->info);
		$$ = node;
	}
}
	|	exp '-' exp
{
	if ($1->nodekind==CONSTKIND && $3->nodekind==CONSTKIND) {
		if (!($1->valuetype==INTTYPE && $3->valuetype==INTTYPE))
			yyerror("- only works on INTTYPE, const node type mismatches");

		$1->value = $1->value - $3->value;
		free($3);
		sprintf($1->info, "CONST INT %d", $1->value);
		$$ = $1;
	}else {
		if ($1->nodekind==CONSTKIND || $1->nodekind==EXPKIND) {
			if ($1->valuetype != INTTYPE)
				yyerror("- works on INTTYPE, valuetype[%d] mismatch", $1->valuetype);
		}else if ($1->nodekind==IDKIND || $1->nodekind==CALLKIND)
			$1->valuetype = INTTYPE;
		else	yyerror("invalid nodekind[%d]", $1->nodekind);

		if ($3->nodekind==CONSTKIND || $3->nodekind==EXPKIND) {
			if ($3->valuetype != INTTYPE)
				yyerror("- only works on INTTYPE, valuetype[%d] mismatch", $3->valuetype);
		}else if ($3->nodekind==IDKIND || $3->nodekind==CALLKIND)
			$3->valuetype = INTTYPE;
		else	yyerror("invalid nodekind[%d]", $3->nodekind);

		Node *node = getnewnode();
		node->nodekind = EXPKIND;
		node->valuetype = INTTYPE;
		node->opkind = MINUSK;
		node->childrennum = 2;
		node->children[0] = $1;
		node->children[1] = $3;
		sprintf(node->info, "%s - %s", $1->info, $3->info);
		$$ = node;
	}
}
	|	exp '*' exp
{
	if ($1->nodekind==CONSTKIND && $3->nodekind==CONSTKIND) {
		if (!($1->valuetype==INTTYPE && $3->valuetype==INTTYPE))
			yyerror("* works on INTTYPE, valuetype mismatch");

		$1->value = $1->value * $3->value;
		free($3);
		sprintf($1->info, "CONST INT %d", $1->value);
		$$ = $1;
	}else {
		if ($1->nodekind == CONSTKIND || $1->nodekind==EXPKIND) {
			if ($1->valuetype != INTTYPE)
				yyerror("* works on INTTYPE, valuetype[%d] mismatch", $1->valuetype);
		}else if ($1->nodekind==IDKIND || $1->nodekind==CALLKIND)
			$1->valuetype = INTTYPE;
		else
			yyerror("invalid nodekind[%d]", $1->nodekind);

		if ($3->nodekind == CONSTKIND || $3->nodekind==EXPKIND) {
			if ($3->valuetype != INTTYPE)
				yyerror("* works on INTTYPE, valuetype[%d] mismatch", $3->valuetype);
		}else if ($3->nodekind==IDKIND || $3->nodekind==CALLKIND)
			$3->valuetype = INTTYPE;
		else
			yyerror("invalid nodekind[%d]", $3->nodekind);

		Node *node = getnewnode();
		node->nodekind = EXPKIND;
		node->valuetype = INTTYPE;
		node->opkind = MULK;
		node->childrennum = 2;
		node->children[0] = $1;
		node->children[1] = $3;
		sprintf(node->info, "%s * %s", $1->info, $3->info);
		$$ = node;
	}
}
	|	exp '/' exp
{
	if ($1->nodekind==CONSTKIND && $3->nodekind==CONSTKIND) {
		if (!($1->valuetype==INTTYPE && $3->valuetype==INTTYPE))
			yyerror("/ works on INTTYPE, valuetype mismatch");
		if ($3->value == 0)	yyerror("div 0 error");
		$1->value = $1->value / $3->value;
		free($3);
		sprintf($1->info, "CONST INT %d", $1->value);
		$$ = $1;
	}else {
		if ($1->nodekind == CONSTKIND || $1->nodekind==EXPKIND) {
			if ($1->valuetype != INTTYPE)
				yyerror("* works on INTTYPE, valuetype[%d] mismatch", $1->valuetype);
		}else if ($1->nodekind==IDKIND || $1->nodekind==CALLKIND)
			$1->valuetype = INTTYPE;
		else
			yyerror("invalid nodekind[%d]", $1->nodekind);

		if ($3->nodekind == CONSTKIND || $3->nodekind==EXPKIND) {
			if ($3->valuetype != INTTYPE)
				yyerror("* works on INTTYPE, valuetype[%d] mismatch", $3->valuetype);
		}else if ($3->nodekind==IDKIND || $3->nodekind==CALLKIND)
			$3->valuetype = INTTYPE;
		else
			yyerror("invalid nodekind[%d]", $3->nodekind);

		Node *node = getnewnode();
		node->nodekind = EXPKIND;
		node->valuetype = INTTYPE;
		node->opkind = DIVK;
		node->childrennum = 2;
		node->children[0] = $1;
		node->children[1] = $3;
		sprintf(node->info, "%s / %s", $1->info, $3->info);
		$$ = node;
	}
}
	|	exp AND exp
{
	if ($1->nodekind==CONSTKIND && $3->nodekind==CONSTKIND) {
		if (!($1->valuetype==BOOLTYPE && $3->valuetype==BOOLTYPE))
			yyerror("AND only works on BOOLTYPE, valuetype mismatch");

		$1->value = $1->value && $3->value;
		free($3);
		sprintf($1->info, "CONST BOOL %d", $1->value);
		$$ = $1;
	}else {
		if ($1->nodekind==CONSTKIND || $1->nodekind==EXPKIND) {
			if ($1->valuetype != BOOLTYPE)
				yyerror("AND only works on BOOLTYPE, valuetype mismatch");
		}else if ($1->nodekind==IDKIND || $1->nodekind==CALLKIND)
			$1->valuetype = BOOLTYPE;
		else
			yyerror("invalid nodekind");

		if ($3->nodekind==CONSTKIND || $3->nodekind==EXPKIND) {
			if ($3->valuetype != BOOLTYPE)
				yyerror("AND only works on BOOLTYPE, valuetype mismatch");
		}else if ($3->nodekind==IDKIND || $3->nodekind==CALLKIND)
			$3->valuetype = BOOLTYPE;
		else
			yyerror("invalid nodekind");
		
		Node *node = getnewnode();
		node->nodekind = EXPKIND;
		node->valuetype = BOOLTYPE;
		node->opkind = ANDK;
		node->childrennum = 2;
		node->children[0] = $1;
		node->children[1] = $3;
		sprintf(node->info, "%s AND %s", $1->info, $3->info);
		$$ = node;
	}
}
	|	exp OR	exp
{
	if ($1->nodekind==CONSTKIND && $3->nodekind==CONSTKIND) {
		if (!($1->valuetype==BOOLTYPE && $3->valuetype==BOOLTYPE))
			yyerror("OR only works on BOOLTYPE, valuetype mismatch");

		$1->value = $1->value || $3->value;
		free($3);
		sprintf($1->info, "CONST BOOL %d", $1->value);
		$$ = $1;
	}else {
		if ($1->nodekind==CONSTKIND || $1->nodekind==EXPKIND) {
			if ($1->valuetype != BOOLTYPE)
				yyerror("OR only works on BOOLTYPE, valuetype mismatch");
		}else if ($1->nodekind==IDKIND || $1->nodekind==CALLKIND)
			$1->valuetype = BOOLTYPE;
		else
			yyerror("invalid nodekind");

		if ($3->nodekind==CONSTKIND || $3->nodekind==EXPKIND) {
			if ($3->valuetype != BOOLTYPE)
				yyerror("OR only works on BOOLTYPE, valuetype mismatch");
		}else if ($3->nodekind==IDKIND || $3->nodekind==CALLKIND)
			$3->valuetype = BOOLTYPE;
		else
			yyerror("invalid nodekind");
		
		Node *node = getnewnode();
		node->nodekind = EXPKIND;
		node->valuetype = BOOLTYPE;
		node->opkind = ORK;
		node->childrennum = 2;
		node->children[0] = $1;
		node->children[1] = $3;
		sprintf(node->info, "%s OR %s", $1->info, $3->info);
		$$ = node;
	}
}
	|	exp '^' exp
{
	if ($1->nodekind==CONSTKIND && $3->nodekind==CONSTKIND) {
		if (!($1->valuetype==BOOLTYPE && $3->valuetype==BOOLTYPE))
			yyerror("XOR only works on BOOLTYPE, valuetype mismatch");

		$1->value = $1->value ^ $3->value;
		free($3);
		sprintf($1->info, "CONST BOOL %d", $1->value);
		$$ = $1;
	}else {
		if ($1->nodekind==CONSTKIND || $1->nodekind==EXPKIND) {
			if ($1->valuetype != BOOLTYPE)
				yyerror("XOR only works on BOOLTYPE, valuetype mismatch");
		}else if ($1->nodekind==IDKIND || $1->nodekind==CALLKIND)
			$1->valuetype = BOOLTYPE;
		else
			yyerror("invalid nodekind");

		if ($3->nodekind==CONSTKIND || $3->nodekind==EXPKIND) {
			if ($3->valuetype != BOOLTYPE)
				yyerror("XOR only works on BOOLTYPE, valuetype mismatch");
		}else if ($3->nodekind==IDKIND || $3->nodekind==CALLKIND)
			$3->valuetype = BOOLTYPE;
		else
			yyerror("invalid nodekind");
		
		Node *node = getnewnode();
		node->nodekind = EXPKIND;
		node->valuetype = BOOLTYPE;
		node->opkind = XORK;
		node->childrennum = 2;
		node->children[0] = $1;
		node->children[1] = $3;
		sprintf(node->info, "%s XOR %s", $1->info, $3->info);
		$$ = node;
	}
}
	|	exp	'<'	exp
{
	if ($1->nodekind==CONSTKIND && $3->nodekind==CONSTKIND) {
		if (!($1->valuetype==INTTYPE && $3->valuetype==INTTYPE))
			yyerror("< works on INTTYPE, valuetype mismatch");

		$1->value = $1->value < $3->value;
		$1->valuetype = BOOLTYPE;
		free($3);
		sprintf($1->info, "CONST BOOL %d", $1->value);
		$$ = $1;
	}else {
		if ($1->nodekind == CONSTKIND || $1->nodekind==EXPKIND) {
			if ($1->valuetype != INTTYPE)
				yyerror("< works on INTTYPE, valuetype[%d] mismatch", $1->valuetype);
		}else if ($1->nodekind==IDKIND || $1->nodekind==CALLKIND)
			$1->valuetype = INTTYPE;
		else
			yyerror("invalid nodekind[%d]", $1->nodekind);

		if ($3->nodekind == CONSTKIND || $3->nodekind==EXPKIND) {
			if ($3->valuetype != INTTYPE)
				yyerror("< works on INTTYPE, valuetype[%d] mismatch", $3->valuetype);
		}else if ($3->nodekind==IDKIND || $3->nodekind==CALLKIND)
			$3->valuetype = INTTYPE;
		else
			yyerror("invalid nodekind[%d]", $3->nodekind);

		Node *node = getnewnode();
		node->nodekind = EXPKIND;
		node->valuetype = BOOLTYPE;
		node->opkind = LTK;
		node->childrennum = 2;
		node->children[0] = $1;
		node->children[1] = $3;
		sprintf(node->info, "%s < %s", $1->info, $3->info);
		$$ = node;
	}
}
	|	exp	LE	exp
{
	if ($1->nodekind==CONSTKIND && $3->nodekind==CONSTKIND) {
		if (!($1->valuetype==INTTYPE && $3->valuetype==INTTYPE))
			yyerror("<= works on INTTYPE, valuetype mismatch");

		$1->value = $1->value <= $3->value;
		$1->valuetype = BOOLTYPE;
		free($3);
		sprintf($1->info, "CONST BOOL %d", $1->value);
		$$ = $1;
	}else {
		if ($1->nodekind == CONSTKIND || $1->nodekind==EXPKIND) {
			if ($1->valuetype != INTTYPE)
				yyerror("<= works on INTTYPE, valuetype[%d] mismatch", $1->valuetype);
		}else if ($1->nodekind==IDKIND || $1->nodekind==CALLKIND)
			$1->valuetype = INTTYPE;
		else
			yyerror("invalid nodekind[%d]", $1->nodekind);

		if ($3->nodekind == CONSTKIND || $3->nodekind==EXPKIND) {
			if ($3->valuetype != INTTYPE)
				yyerror("<= works on INTTYPE, valuetype[%d] mismatch", $3->valuetype);
		}else if ($3->nodekind==IDKIND || $3->nodekind==CALLKIND)
			$3->valuetype = INTTYPE;
		else
			yyerror("invalid nodekind[%d]", $3->nodekind);

		Node *node = getnewnode();
		node->nodekind = EXPKIND;
		node->valuetype = BOOLTYPE;
		node->opkind = LEK;
		node->childrennum = 2;
		node->children[0] = $1;
		node->children[1] = $3;
		sprintf(node->info, "%s <= %s", $1->info, $3->info);
		$$ = node;
	}
}
	|	exp	'>'	exp
{
	if ($1->nodekind==CONSTKIND && $3->nodekind==CONSTKIND) {
		if (!($1->valuetype==INTTYPE && $3->valuetype==INTTYPE))
			yyerror("> works on INTTYPE, valuetype mismatch");

		$1->value = $1->value > $3->value;
		$1->valuetype = BOOLTYPE;
		free($3);
		sprintf($1->info, "CONST BOOL %d", $1->value);
		$$ = $1;
	}else {
		if ($1->nodekind == CONSTKIND || $1->nodekind==EXPKIND) {
			if ($1->valuetype != INTTYPE)
				yyerror("> works on INTTYPE, valuetype[%d] mismatch", $1->valuetype);
		}else if ($1->nodekind==IDKIND || $1->nodekind==CALLKIND)
			$1->valuetype = INTTYPE;
		else
			yyerror("invalid nodekind[%d]", $1->nodekind);

		if ($3->nodekind == CONSTKIND || $3->nodekind==EXPKIND) {
			if ($3->valuetype != INTTYPE)
				yyerror("> works on INTTYPE, valuetype[%d] mismatch", $3->valuetype);
		}else if ($3->nodekind==IDKIND || $3->nodekind==CALLKIND)
			$3->valuetype = INTTYPE;
		else
			yyerror("invalid nodekind[%d]", $3->nodekind);

		Node *node = getnewnode();
		node->nodekind = EXPKIND;
		node->valuetype = BOOLTYPE;
		node->opkind = GTK;
		node->childrennum = 2;
		node->children[0] = $1;
		node->children[1] = $3;
		sprintf(node->info, "%s > %s", $1->info, $3->info);
		$$ = node;
	}
}
	|	exp	GE	exp
{
	if ($1->nodekind==CONSTKIND && $3->nodekind==CONSTKIND) {
		if (!($1->valuetype==INTTYPE && $3->valuetype==INTTYPE))
			yyerror(">= works on INTTYPE, valuetype mismatch");

		$1->value = $1->value >= $3->value;
		$1->valuetype = BOOLTYPE;
		free($3);
		sprintf($1->info, "CONST BOOL %d", $1->value);
		$$ = $1;
	}else {
		if ($1->nodekind == CONSTKIND || $1->nodekind==EXPKIND) {
			if ($1->valuetype != INTTYPE)
				yyerror(">= works on INTTYPE, valuetype[%d] mismatch", $1->valuetype);
		}else if ($1->nodekind==IDKIND || $1->nodekind==CALLKIND)
			$1->valuetype = INTTYPE;
		else
			yyerror("invalid nodekind[%d]", $1->nodekind);

		if ($3->nodekind == CONSTKIND || $3->nodekind==EXPKIND) {
			if ($3->valuetype != INTTYPE)
				yyerror(">= works on INTTYPE, valuetype[%d] mismatch", $3->valuetype);
		}else if ($3->nodekind==IDKIND || $3->nodekind==CALLKIND)
			$3->valuetype = INTTYPE;
		else
			yyerror("invalid nodekind[%d]", $3->nodekind);

		Node *node = getnewnode();
		node->nodekind = EXPKIND;
		node->valuetype = BOOLTYPE;
		node->opkind = GEK;
		node->childrennum = 2;
		node->children[0] = $1;
		node->children[1] = $3;
		sprintf(node->info, "%s >= %s", $1->info, $3->info);
		$$ = node;
	}
}
	|	exp	EQ	exp
{
	if ($1->nodekind==CONSTKIND && $3->nodekind==CONSTKIND) {
		if ($1->valuetype != $3->valuetype)
			yyerror("EQ works on the same valuetype");
		if ($1->value==$3->value)
			$1->value = 1;
		else
			$1->value = 0;

		$1->valuetype = BOOLTYPE;
		sprintf($1->info, "CONST BOOL %d", $1->value);
		$$ = $1;
		free($3);
	}else {
		if ($1->nodekind==CONSTKIND || $3->nodekind==CONSTKIND) {
			Node *n1, *n3;
			if ($1->nodekind == CONSTKIND) {
				n1 = $1;
				n3 = $3;
			}else {
				n1 = $3;
				n3 = $1;
			}

			if (n3->nodekind==IDKIND || n3->nodekind==CALLKIND)
				n3->valuetype = n1->valuetype;
			else if (n3->nodekind == EXPKIND) {
				if (n1->valuetype != n3->valuetype)
					yyerror("EQ works on the same valuetype");
			}else
				yyerror("invalid nodekind");
			
		} // if none of them is CONSTKIN, we do not do typechecking, do it later
		
		Node *n = getnewnode();
		n->nodekind = EXPKIND;
		n->valuetype = BOOLTYPE;
		n->opkind = EQK;
		n->childrennum = 2;
		n->children[0] = $1;
		n->children[1] = $3;
		sprintf(n->info, "%s == %s", $1->info, $3->info);
		$$ = n;
	}
}
	|	exp NEQ	exp
{
	if ($1->nodekind==CONSTKIND && $3->nodekind==CONSTKIND) {
		if ($1->valuetype != $3->valuetype)
			yyerror("NEQ works on the same valuetype");
		if ($1->value==$3->value)
			$1->value = 0;
		else
			$1->value = 1;

		$1->valuetype = BOOLTYPE;
		sprintf($1->info, "CONST BOOL %d", $1->value);
		$$ = $1;
		free($3);
	}else {
		if ($1->nodekind==CONSTKIND || $3->nodekind==CONSTKIND) {
			Node *n1, *n3;
			if ($1->nodekind == CONSTKIND) {
				n1 = $1;
				n3 = $3;
			}else {
				n1 = $3;
				n3 = $1;
			}

			if (n3->nodekind==IDKIND || n3->nodekind==CALLKIND)
				n3->valuetype = n1->valuetype;
			else if (n3->nodekind == EXPKIND) {
				if (n1->valuetype != n3->valuetype)
					yyerror("EQ works on the same valuetype");
			}else
				yyerror("invalid nodekind");
			
		} //if none is CONSTKIND, we do not typecheck, do it later

		Node *n = getnewnode();
		n->nodekind = EXPKIND;
		n->valuetype = BOOLTYPE;
		n->opkind = NEQK;
		n->childrennum = 2;
		n->children[0] = $1;
		n->children[1] = $3;
		sprintf(n->info, "%s != %s", $1->info, $3->info);
		$$ = n;
	}
}
	|	ID '(' args ')'
{
	Node *n = getnewnode();
	n->nodekind = CALLKIND;
	strcpy(n->name, $1.name);
	n->childrennum = 1;
	n->children[0] = $3;
	n->valuetype = NOTYPE;
	n->returnlabel = NewLabel();

	// insert it into calltable
	HashNode *calln = lookup(calltable, $1.name);
	if (calln)	yyerror("each function allows only 1 call, function[%s] call duplicated!", $1.name);

	calln = calloc(1, sizeof(HashNode));
	if (!calln)	yyerror("failed to calloc for calltable node!");
	strcpy(calln->name, $1.name);
	calln->returnlabel = n->returnlabel;
	insert(calltable, calln);

	if ($3)
		sprintf(n->info, "%s(%s)", $1.name, $3->info);
	$$ = n;
}
	;

args	:	{ $$ = NULL; }
		|	someargs
{
	Node *n = getnewnode();
	n->nodekind = ARGSKIND;
	n->childrennum = 1;
	n->children[0] = $1;
	$$ = n;
}
		;

someargs	:	exp moreargs
{
	$1->next=$2;
	if ($2)
		sprintf($1->info, "%s, %s", $1->info, $2->info);
	$$=$1; 
}
			;

moreargs	:	',' someargs { $$=$2; }
			|	{ $$=NULL; }
			;

returnstmt	:	RETURN exp
{
	Node *n = getnewnode();
	n->nodekind = RETURNKIND;
	n->childrennum = 1;
	n->children[0] = $2;
	n->valuetype = $2->valuetype;
	sprintf(n->info, "return %s", $2->info);
	$$ = n;
}
			;

%%

int main()
{
	// Build Abstract Syntax Tree
	yyparse();
	//printast(ast);
	outfile = fopen("iloc.out", "w");
	// Build symble table
	build_symtable(gast);
	//print_gsymtable();
	//print_symtable(gast);
	// id typeset and typechecking
	set_idtype(gast);
	typecheck(gast);
	
	gencode(gast);
	return 0;
}

void yyerror(const char *s, ...)
{
	va_list ap;
	va_start(ap, s);

	fprintf(stderr, "Line[%d]: error ", yylineno);
	vfprintf(stderr, s, ap);
	fprintf(stderr, "\n");
	exit(0);
}

