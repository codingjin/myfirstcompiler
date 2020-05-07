#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "attr.h"
#include "instruction.h"

int hash(char *str)
{
	if (!str || strlen(str)==0) {
		fprintf(stderr, "invalid string in hash()\n");
		exit(0);
	}
	int hashcode = 0;
	int len = strlen(str);
	for (int i=0;i<len;++i)
		hashcode = ((hashcode<<SHIFT) + str[i]) % TABLESIZE;
	return hashcode;
}

HashNode *lookup(HashNode **table, char *name)
{
	if (!table) {
		fprintf(stderr, "invalid table in lookup()\n");
		exit(0);
	}
	if (!name || strlen(name)==0) {
		fprintf(stderr, "invalid name in lookup()\n");
		exit(0);
	}

	int index = hash(name);
	if (index < 0)	index = -index;
	
	HashNode *node = table[index];
	int namelen = strlen(name);
	while (node) {
		if (namelen==strlen(node->name) && strcmp(name, node->name)==0)
			return node;
		node = node->next;
	}
	return NULL;
}

void insert(HashNode **table, HashNode *newnode)
{
	if (!table) {
		fprintf(stderr, "invalid table call insert()\n");
		exit(0);
	}
	if (!newnode) {
		fprintf(stderr, "insert null node\n");
		exit(0);
	}
	
	int index = hash(newnode->name);
	if (index < 0)	index = -index;
	if (!table[index])	table[index] = newnode;
	else {
		HashNode *node = table[index]->next;
		table[index]->next = newnode;
		newnode->next = node;
	}
}

Node *getnewnode()
{
	Node *node = (Node*)calloc(1, sizeof(Node));
	if (!node) {
		fprintf(stderr, "failed to calloc new Node\n");
		exit(0);
	}
	return node;
}

Node *gettailnode(Node *node)
{
	if (!node)	return node;
	Node *current = node;
	while (current->next)
		current = current->next;
	return current;
}

void printast(Node *ast)
{
	while (ast) {
		Node *c;
		Node *current = ast;
		printf("name[%s]\n", current->name);
		c = current->children[0];
		while (c) {
			printf("%s\n", c->info);
			c = c->next;
		}

		c = current->children[1];
		while (c) {
			printf("%s\n", c->info);
			c = c->next;
		}

		c = current->children[2];
		while (c) {
			printf("stmt:%s\n", c->info);
			c = c->next;
		}

		current = current->next;
		ast = ast->next;
	}
}

void build_symtable(Node *ast)
{
	init_gsymtable(ast);
	while (ast) {
		if (ast->nodekind == FUNCKIND) {
			// params
			Node *param = ast->children[0];
			HashNode **symtable = ast->symtable;
			HashNode *hashnode;
			while (param) {
				hashnode = lookup(symtable, param->name);
				if (hashnode) {
					fprintf(stderr, "duplicate param[%s] in function[%s]\n",
						param->name, ast->name);
					exit(0);
				}
				hashnode = calloc(1, sizeof(HashNode));
				if (!hashnode) {
					fprintf(stderr, "fail to calloc for hashnode\n");
					exit(0);
				}
				strcpy(hashnode->name, param->name);
				hashnode->valuetype = param->valuetype;
				hashnode->offset = NewOffset();
				hashnode->symtype = IDSYM;
				insert(symtable, hashnode);
				param = param->next;
			}
			// vardecls
			Node *vardecl = ast->children[1];
			while (vardecl) {
				hashnode = lookup(symtable, vardecl->name);
				if (hashnode) {
					fprintf(stderr, "duplicated vardecl[%s] in function[%s]\n",
						vardecl->name, ast->name);
					exit(0);
				}
				hashnode = calloc(1, sizeof(HashNode));
				if (!hashnode) {
					fprintf(stderr, "fail to calloc for hashnode\n");
					exit(0);
				}
				strcpy(hashnode->name, vardecl->name);
				hashnode->valuetype = vardecl->valuetype;
				hashnode->offset = NewOffset();
				hashnode->symtype = IDSYM;
				insert(symtable, hashnode);
				vardecl= vardecl->next;
			}
		
		}else { // ast->nodekind==MAINKIND
			Node *vardecl = ast->children[0];
			HashNode **symtable = ast->symtable;
			HashNode *hashnode;
			while (vardecl) {
				hashnode = lookup(symtable, vardecl->name);
				if (hashnode) {
					fprintf(stderr, "duplicate definition of %s\n", vardecl->name);
					exit(0);
				}

				hashnode = (HashNode*)calloc(1, sizeof(HashNode));
				strcpy(hashnode->name, vardecl->name);
				hashnode->valuetype = vardecl->valuetype;
				hashnode->offset = NewOffset();
				hashnode->symtype = IDSYM;
				insert(symtable, hashnode);
				vardecl = vardecl->next;
			}
		}
		ast = ast->next;
	}
}

void print_symtable(Node *ast)
{
	while (ast) {
		if (!ast->symtable)	continue;
		printf("function[%s]\n", ast->name);
		HashNode **symtable = ast->symtable;
		HashNode *node;
		for (int i=0;i<TABLESIZE;++i) {
			if (symtable[i]) {
				node = symtable[i];
				while(node) {
					printf("name[%s] symtype[%d] valuetype[%d] offset[%d]\n",
					node->name, node->symtype, node->valuetype, node->offset);
					node = node->next;
				}
			}
		}
		ast = ast->next;
	}
}

void set_idtype(Node *ast)
{
	while (ast) {
		HashNode **symtable = ast->symtable;
		if (!symtable)	continue;

		if (ast->nodekind == FUNCKIND)
			set_stmttype(symtable, ast->children[2]);
		else if (ast->nodekind == MAINKIND)
			set_stmttype(symtable, ast->children[1]);

		ast = ast->next;
	}
}

void set_stmttype(HashNode **symtable, Node *stmt)
{ // PRINTKIND(noaction here), ASSIGNKIND, IFKIND, IFELSEKIND
	HashNode *node;
	while (stmt) {
		if (stmt->nodekind == ASSIGNKIND) {
			node = lookup(symtable, stmt->name);
			if (!node) {
				fprintf(stderr, "ID[%s] in assignstmt undeclared\n", stmt->name);
				exit(0);
			}
			stmt->valuetype = node->valuetype;
			set_expidtype(symtable, stmt->children[0]);
			if (stmt->valuetype != stmt->children[0]->valuetype) {
				fprintf(stderr, "valuetype mismatch in assignstmt\n");
				exit(0);
			}
		}else if (stmt->nodekind == IFKIND) {
			// IF exp DO stmts END, (1)exp part
			set_expidtype(symtable, stmt->children[0]);
			if (stmt->children[0]->valuetype != BOOLTYPE) {
				fprintf(stderr, "exp valutype[%d] of IFKIND is not BOOLTYPE[%d]\n",
					stmt->children[0]->valuetype, BOOLTYPE);
				exit(0);
			}
			// (2)stmts part
			set_stmttype(symtable, stmt->children[1]);
		}else if (stmt->nodekind == IFELSEKIND) {
			set_expidtype(symtable, stmt->children[0]);
			if (stmt->children[0]->valuetype != BOOLTYPE) {
				fprintf(stderr, "exp vauletype[%d] of IFELSEKIND is not BOOLTYPE[%d]\n",
					stmt->children[0]->valuetype, BOOLTYPE);
				exit(0);
			}
			set_stmttype(symtable, stmt->children[1]);
			set_stmttype(symtable, stmt->children[2]);
		}else if (stmt->nodekind == WHILEKIND) {
			set_expidtype(symtable, stmt->children[0]);
			if (stmt->children[0]->valuetype != BOOLTYPE) {
				fprintf(stderr, "exp vauletype[%d] of IFELSEKIND is not BOOLTYPE[%d]\n",
					stmt->children[0]->valuetype, BOOLTYPE);
				exit(0);
			}
			set_stmttype(symtable, stmt->children[1]);
		}else if (stmt->nodekind == FORKIND) {
			set_expidtype(symtable, stmt->children[0]);
			if (stmt->children[0]->valuetype != BOOLTYPE) {
				fprintf(stderr, "exp vauletype[%d] of IFELSEKIND is not BOOLTYPE[%d]\n",
					stmt->children[0]->valuetype, BOOLTYPE);
				exit(0);
			}
			set_stmttype(symtable, stmt->children[1]);
			set_stmttype(symtable, stmt->children[2]);
			set_stmttype(symtable, stmt->children[3]);
		}else if (stmt->nodekind == RETURNKIND) {
			set_expidtype(symtable, stmt->children[0]);
		}

		stmt = stmt->next;
	}
}

void set_expidtype(HashNode **symtable, Node *exp)
{
	if (!symtable || !exp) {
		fprintf(stderr, "set_expidtype exception\n");
		exit(0);
	}

	NodeKind nodekind = exp->nodekind;
	if (nodekind == CONSTKIND)	return;
	else if (nodekind == IDKIND) {
		HashNode *node = lookup(symtable, exp->name);
		if (!node) {
			fprintf(stderr, "undeclared ID[%s]\n", exp->name);
			exit(0);
		}
		if (exp->valuetype == NOTYPE)	exp->valuetype = node->valuetype;
		else if (exp->valuetype != node->valuetype) {
			fprintf(stderr, "type mismatch: symtable:s[%d] ID:s[%d]\n",
				node->name, node->valuetype, exp->name, exp->valuetype);
			exit(0);
		}
	}else if (nodekind == CALLKIND) {
		HashNode *fnode = lookup(gsymtable, exp->name);
		if (!fnode) {
			fprintf(stderr, "undeclared function[%s] is called\n", exp->name);
			exit(0);
		}
		if (exp->valuetype == NOTYPE)	exp->valuetype = fnode->valuetype;
		else if (exp->valuetype != fnode->valuetype) {
			fprintf(stderr, "function[%s] type mismatch\n", exp->name);
			exit(0);
		}
		// deal with call args
		Node *argexp = exp->children[0];
		if (argexp) {
			argexp = argexp->children[0];
			while (argexp) {
				set_expidtype(symtable, argexp);
				argexp = argexp->next;
			}
		}
		// check the types of args with the function params definition
		Node *i = gast;
		while (i) {
			if (strlen(i->name)==strlen(exp->name) 
				&& strcmp(i->name, exp->name)==0) {
				argexp = exp->children[0];
				if (argexp)	argexp=argexp->children[0];
				Node *param = i->children[0];
				while (param) {
					if (!argexp || argexp->valuetype!=param->valuetype) {
						fprintf(stderr, "call args and function[%s] params not match\n", exp->name);
						exit(0);
					}
					param = param->next;
					argexp = argexp->next;
				}
				if (argexp) {
					fprintf(stderr, "call args are more than function[%s] params\n",
						exp->name);
					exit(0);
				}
				break;
			}
			i = i->next;
		}
	}else if (nodekind == EXPKIND) {
		if (exp->opkind == NOTK)	set_expidtype(symtable, exp->children[0]);
		else {
			set_expidtype(symtable, exp->children[0]);
			set_expidtype(symtable, exp->children[1]);
			if (exp->opkind == EQK || exp->opkind == NEQK)
				if (exp->children[0]->valuetype != exp->children[1]->valuetype) {
					fprintf(stderr, "2 operands' valuetype mismatch in EQK/NEQK\n");
					exit(0);
				}
		}
	}else {
		fprintf(stderr, "exp invalid nodekind[%d]\n", nodekind);
		exit(0);
	}
}

void typecheck(Node *ast)
{
	while (ast) {
		if (ast->nodekind == FUNCKIND)
			typecheck_stmt(ast->children[2]);
		else if (ast->nodekind == MAINKIND)
			typecheck_stmt(ast->children[1]);

		ast = ast->next;
	}
}

void typecheck_stmt(Node *stmt)
{
	while (stmt) {
		if (stmt->nodekind == ASSIGNKIND)
			typecheck_exp(stmt->children[0]);
		else if (stmt->nodekind == IFKIND) {
			typecheck_exp(stmt->children[0]);
			typecheck_stmt(stmt->children[1]);
		}else if (stmt->nodekind == IFELSEKIND) {
			typecheck_exp(stmt->children[0]);
			typecheck_stmt(stmt->children[1]);
			typecheck_stmt(stmt->children[2]);
		}else if (stmt->nodekind == WHILEKIND) {
			typecheck_exp(stmt->children[0]);
			typecheck_stmt(stmt->children[1]);
		}else if (stmt->nodekind == FORKIND) {
			typecheck_exp(stmt->children[0]);
			typecheck_stmt(stmt->children[1]);
			typecheck_stmt(stmt->children[2]);
			typecheck_stmt(stmt->children[3]);
		}else if (stmt->nodekind == RETURNKIND) {
			typecheck_exp(stmt->children[0]);
		}

		stmt = stmt->next;
	}
}

void typecheck_exp(Node *exp)
{
	if (!exp) {
		fprintf(stderr, "NULL exp calls typecheck_exp()\n");
		exit(0);
	}

	NodeKind nodekind = exp->nodekind;
	if (nodekind==CONSTKIND || nodekind==IDKIND || nodekind==CALLKIND)	return;
	else if (nodekind == EXPKIND) {
		switch (exp->opkind) {
			case NOTK:
				if (exp->children[0]->valuetype != BOOLTYPE) {
					fprintf(stderr, "invalid !exp valuetype: %s\n", exp->info);
					exit(0);
				}
				typecheck_exp(exp->children[0]);
				break;
			case ANDK:
			case ORK:
			case XORK:
				if (exp->children[0]->valuetype != BOOLTYPE
					|| exp->children[1]->valuetype != BOOLTYPE) {
					fprintf(stderr, "invalid exp valuetype: %s\n", exp->info);
					exit(0);
				}
				typecheck_exp(exp->children[0]);
				typecheck_exp(exp->children[1]);
				break;
			case ADDK:
			case MINUSK:
			case MULK:
			case DIVK:
			case LTK:
			case LEK:
			case GEK:
			case GTK:
				if (exp->children[0]->valuetype != INTTYPE
					|| exp->children[1]->valuetype != INTTYPE) {
					fprintf(stderr, "invalid exp valuetype: %s\n", exp->info);
					exit(0);
				}
				typecheck_exp(exp->children[0]);
				typecheck_exp(exp->children[1]);
				break;
			case EQK:
			case NEQK:
				if (exp->children[0]->valuetype != exp->children[1]->valuetype) {
					fprintf(stderr, "invalid exp valuetype: %s\n", exp->info);
					exit(0);
				}
				typecheck_exp(exp->children[0]);
				typecheck_exp(exp->children[1]);
				break;
			default:
				fprintf(stderr, "invalid opkind[%d]\n", exp->opkind);
				exit(0);
		}
	}else {
		fprintf(stderr, "invalid nodekind calls typecheck_exp()\n");
		exit(0);
	}
}

void init_gsymtable(Node *ast)
{
	memset(gsymtable, 0, TABLESIZE * sizeof(HashNode*));
	while (ast) {
		if (ast->nodekind == FUNCKIND) {
			HashNode *n = lookup(gsymtable, ast->name);
			if (n) {
				fprintf(stderr, "duplicated function[%s] definition\n", ast->name);
				exit(0);
			}
			n = calloc(1, sizeof(HashNode));
			if (!n) {
				fprintf(stderr, "failed to calloc for function[%s] in global symtable\n", ast->name);
				exit(0);
			}
			strcpy(n->name, ast->name);
			n->valuetype = ast->valuetype;
			n->symtype = FUNCSYM;
			n->label = ast->label;

			HashNode *calln = lookup(calltable, ast->name);
			if (calln) {
				n->returnlabel = calln->returnlabel;
				ast->returnlabel = calln->returnlabel;
			}else {
				n->returnlabel = NONE;
				ast->returnlabel = NONE;
			}

			insert(gsymtable, n);
		}else if (ast->nodekind == MAINKIND) {
			HashNode *n = calloc(1, sizeof(HashNode));
			if (!n) {
				fprintf(stderr, "fail to calloc for main in globaltable\n");
				exit(0);
			}
			strcpy(n->name, "main");
			n->valuetype = NOTYPE;
			n->symtype = MAINSYM;
			n->label = ast->label;
			n->returnlabel = NONE;
			insert(gsymtable, n);
		}else {
			fprintf(stderr, "invalid ast nodekind in init_gsymtable[%d]\n", ast->nodekind);
			exit(0);
		}

		ast = ast->next;
	}
}

void print_gsymtable()
{
	HashNode *node;
	for (int i=0;i<TABLESIZE;++i) {
		if (gsymtable[i]) {
			node = gsymtable[i];
			while (node) {
				printf("name[%s] symtype[%d] valuetype[%d] label[%d] returnlabel[%d]\n",
					node->name, node->symtype, node->valuetype, node->label, node->returnlabel);
				node = node->next;
			}
		}
	}
}

void gencode(Node *ast)
{
	emitcomment("goes to main entry");
	HashNode *mainnode = lookup(gsymtable, "main");
	if (!mainnode) {
		fprintf(stderr, "main is unfound in gsymtable\n");
		exit(0);
	}
	emit(NONE, BR, mainnode->label, NONE, NONE);

	while (ast) {
		// function label goes first
		emitcomment("%s function entry", ast->name);
		emit(ast->label, NOP, NONE, NONE, NONE);

		if (ast->nodekind == FUNCKIND)
			gencode_stmt(ast->children[2], ast);
		else if (ast->nodekind == MAINKIND)
			gencode_stmt(ast->children[1], ast);
		else {
			fprintf(stderr, "invalid nodekind[%d] call gencode\n", ast->nodekind);
			exit(0);
		}

		ast = ast->next;
	}
}

int gencode_exp(Node *exp, Node *funcnode)
{
	if (!exp || !funcnode) {
		fprintf(stderr, "gencode_exp exception\n");
		exit(0);
	}
	HashNode **symtable = funcnode->symtable;
	if (!symtable) {
		fprintf(stderr, "function[%s] symtable is none\n", funcnode->name);
		exit(0);
	}

	int r = NewRegister();
	int r1, r2;
	if (exp->nodekind == CONSTKIND)
		emit(NONE, LOADI, exp->value, r, NONE);
	else if (exp->nodekind == IDKIND) {
		HashNode *idnode = lookup(symtable, exp->name);
		if (!idnode) {
			fprintf(stderr, "idnode[%s] unfound\n", exp->name);
			exit(0);
		}
		if (idnode->symtype != IDSYM) {
			fprintf(stderr, "invalid id symtype[%d]\n", idnode->symtype);
			exit(0);
		}
		emit(NONE, LOADAI, 0, idnode->offset, r);
	}else if (exp->nodekind == CALLKIND) {
		HashNode *fn = lookup(gsymtable, exp->name);
		if (!fn) {
			fprintf(stderr, "unfound function[%s] in gsymtable in call process\n", exp->name);
			exit(0);
		}

		// compute the params
		Node *callee = gast;
		while (callee) {
			if (strcmp(callee->name, exp->name)==0)	break;
			callee = callee->next;
		}
		if (!callee) {
			fprintf(stderr, "callee[%s] unfound in gast\n", exp->name);
			exit(0);
		}
		HashNode **callee_symtable = callee->symtable;
		Node *callee_param = callee->children[0];
		HashNode *callee_param_node;
		int rarg;

		Node *args = exp->children[0];
		Node *argexp;
		if (args) {
			argexp = args->children[0];
			while (argexp) {
				rarg = gencode_exp(argexp, funcnode);
				callee_param_node = lookup(callee->symtable, callee_param->name);
				emit(NONE, STOREAI, rarg, 0, callee_param_node->offset);
				argexp = argexp->next;
				callee_param = callee_param->next;
			}
		}
		// jump to the callee label
		emit(NONE, BR, callee->label, NONE, NONE);
		emit(callee->returnlabel, NOP, NONE, NONE, NONE);
		emit(NONE, I2I, RET, r, NONE);
		//printf("callee[%s] label[%d] returnlabel[%d]\n", callee->name, callee->label, callee->returnlabel);
	}else if (exp->nodekind == EXPKIND) {
		r1 = gencode_exp(exp->children[0], funcnode);
		if (exp->children[1])
			r2 = gencode_exp(exp->children[1], funcnode);

		switch (exp->opkind) {
			case NOTK:
				emit(NONE, XORI, r1, 1, r);
				break;
			case ADDK:
				emit(NONE, ADD, r1, r2, r);
				break;
			case MINUSK:
				emit(NONE, SUB, r1, r2, r);
				break;
			case MULK:
				emit(NONE, MULT, r1, r2, r);
				break;
			case DIVK:
				emit(NONE, DIV, r1, r2, r);
				break;
			case ANDK:
				emit(NONE, ANDOP, r1, r2, r);
				break;
			case ORK:
				emit(NONE, OROP, r1, r2, r);
				break;
			case XORK:
				emit(NONE, XOROP, r1, r2, r);
				break;
			case LTK:
				emit(NONE, CMPLT, r1, r2, r);
				break;
			case LEK:
				emit(NONE, CMPLE, r1, r2, r);
				break;
			case EQK:
				emit(NONE, CMPEQ, r1, r2, r);
				break;
			case NEQK:
				emit(NONE, CMPNE, r1, r2, r);
				break;
			case GEK:
				emit(NONE, CMPGE, r1, r2, r);
				break;
			case GTK:
				emit(NONE, CMPGT, r1, r2, r);
				break;
			default:
				fprintf(stderr, "invalid opkind[%d]\n", exp->opkind);
				exit(0);
		}
	}else {
		fprintf(stderr, "invalid exp nodekind[%d]\n", exp->nodekind);
		exit(0);
	}
	return r;
}

void gencode_stmt(Node *stmt, Node *funcnode)
{
	if (!stmt || !funcnode) {
		fprintf(stderr, "gencode_stmt() exception");
		exit(0);
	}
	HashNode **symtable = funcnode->symtable;
	if (!symtable) {
		fprintf(stderr, "function[%s] symtable is null", funcnode->name);
		exit(0);
	}
	while (stmt) {
		if (stmt->nodekind == PRINTKIND) {
			emitcomment("print: %s", stmt->info);
			HashNode *id = lookup(symtable, stmt->name);
			if (!id) {
				fprintf(stderr, "id[%s] undeclared\n", stmt->name);
				exit(0);
			}
			emit(NONE, OUTPUTAI, 0, id->offset, NONE);
		}else if (stmt->nodekind == ASSIGNKIND) {
			emitcomment("assignment: %s", stmt->info);
			HashNode *id = lookup(symtable, stmt->name);
			if (!id) {
				fprintf(stderr, "id[%s] undeclared\n", stmt->name);
				exit(0);
			}
			int r_rhs = gencode_exp(stmt->children[0], funcnode);
			emit(NONE, STOREAI, r_rhs, 0, id->offset);
		}else if (stmt->nodekind == IFKIND) {
			emitcomment("ifstmt: %s", stmt->info);
			int r_exp = gencode_exp(stmt->children[0], funcnode);
			int l_true = NewLabel();
			int l_false = NewLabel();
			emit(NONE, CBR, r_exp, l_true, l_false);
			emit(l_true, NOP, NONE, NONE, NONE);
			gencode_stmt(stmt->children[1], funcnode);
			emit(l_false, NOP, NONE, NONE, NONE);
		}else if (stmt->nodekind == IFELSEKIND) {
			emitcomment("if-else stmt: %s", stmt->info);
			int r_exp = gencode_exp(stmt->children[0], funcnode);
			int l_true = NewLabel();
			int l_false = NewLabel();
			int l_end = NewLabel();
			emit(NONE, CBR, r_exp, l_true, l_false);
			emit(l_true, NOP, NONE, NONE, NONE);
			gencode_stmt(stmt->children[1], funcnode);
			emit(NONE, BR, l_end, NONE, NONE);

			emit(l_false, NOP, NONE, NONE, NONE);
			gencode_stmt(stmt->children[2], funcnode);
			emit(l_end, NOP, NONE, NONE, NONE);
		}else if (stmt->nodekind == WHILEKIND) {
			emitcomment("whilestmt: %s", stmt->info);
			int l_condexp = NewLabel();
			int l_body = NewLabel();
			int l_end = NewLabel();

			emit(l_condexp, NOP, NONE, NONE, NONE);
			int r_condexp = gencode_exp(stmt->children[0], funcnode);
			emit(NONE, CBR, r_condexp, l_body, l_end);
			
			emit(l_body, NOP, NONE, NONE, NONE);
			gencode_stmt(stmt->children[1], funcnode);
			emit(NONE, BR, l_condexp, NONE, NONE);

			emit(l_end, NOP, NONE, NONE, NONE);
		}else if (stmt->nodekind == FORKIND) {
			emitcomment("forstmt: %s", stmt->info);
			gencode_stmt(stmt->children[1], funcnode);
			
			int l_condexp = NewLabel();
			int l_body = NewLabel();
			int l_end = NewLabel();

			emit(l_condexp, NOP, NONE, NONE, NONE);
			int r_condexp = gencode_exp(stmt->children[0], funcnode);
			emit(NONE, CBR, r_condexp, l_body, l_end);

			emit(l_body, NOP, NONE, NONE, NONE);
			gencode_stmt(stmt->children[3], funcnode);
			gencode_stmt(stmt->children[2], funcnode);
			emit(NONE, BR, l_condexp, NONE, NONE);

			emit(l_end, NOP, NONE, NONE, NONE);
		}else if (stmt->nodekind == RETURNKIND) {
			emitcomment("returnstmt: %s", stmt->info);
			int ret = gencode_exp(stmt->children[0], funcnode);
			emit(NONE, I2I, ret, RET, NONE);
			HashNode *f = lookup(gsymtable, funcnode->name);
			if (!f) {
				fprintf(stderr, "function[%s] unfound in gsymtable\n", funcnode->name);
				exit(0);
			}
			emit(NONE, BR, funcnode->returnlabel, NONE, NONE);
		}else {
			fprintf(stderr, "invalid stmt\n");
			exit(0);
		}

		stmt = stmt->next;
	}
}


