#ifndef ATTR_H
#define ATTR

#define CHILDRENNUM	4
#define NAMELEN 32
#define TABLESIZE 391
#define SHIFT 4
#define MAXSTRINGLEN 512

typedef enum ValueType {
	NOTYPE = 0,
	BOOLTYPE = 1,
	INTTYPE = 2,
}ValueType;

typedef enum SymType {
	FUNCSYM=1,
	MAINSYM=2,
	IDSYM=3,
}SymType;

typedef struct HashNode {
	char name[NAMELEN];
	ValueType valuetype;
	SymType symtype;
	int label;
	int offset;
	int returnlabel;
	struct HashNode *next;
}HashNode;

HashNode *gsymtable[TABLESIZE];
void init_gsymtable();

HashNode *calltable[TABLESIZE];

HashNode *lookup(HashNode **ptr, char *name);
void intsert(HashNode **table, HashNode *hashnode);
int hash(char *str);


typedef enum NodeKind {
	CONSTKIND = 1,
	IDKIND = 2,
	EXPKIND = 3,
	PRINTKIND = 4, // stmt
	ASSIGNKIND = 5, // stmt
	STMTKIND = 6,
	VARDECLKIND = 7,
	MAINKIND = 8,
	FUNCKIND = 9,
	PROGRAMKIND = 10,
	IFKIND = 11, // stmt
	IFELSEKIND = 12, // stmt
	WHILEKIND = 13, // stmt
	FORKIND = 14, // stmt
	PARAMKIND = 15,
	ARGSKIND = 16,
	CALLKIND = 17,
	RETURNKIND = 18, // stmt
}NodeKind;

typedef enum OpKind {
	NOTK = 0,
	ADDK = 1,
	MINUSK = 2,
	MULK = 3,
	DIVK = 4,
	ANDK = 5,
	ORK = 6,
	XORK = 7,
	LTK = 8,
	LEK = 9,
	EQK = 10,
	NEQK = 11,
	GEK = 12,
	GTK = 13
}OpKind;

typedef struct Token {
	char name[NAMELEN];
	NodeKind nodekind;
	ValueType valuetype;
	int value;
}Token;

typedef struct Node {
	NodeKind nodekind;
	// IDKIND MAINKIND FUNCKIND
	char name[NAMELEN];
	// CONSTKIND
	int value;
	// CONSTKIND, IDKIND, EXPKIND, ASSIGNKIND, VARDECLKIND, FUNCKIND
	ValueType valuetype;
	// EXPKIND
	OpKind opkind;
	// FUNCKIND and MAINKIND
	int label;
	int returnlabel;
	int param_num;
	int decl_num;

	//FUNCKIND, MAINKIND
	HashNode *symtable[TABLESIZE];

	// debug infomation
	char info[MAXSTRINGLEN];

	struct Node *next;

	int childrennum;
	struct Node *children[CHILDRENNUM];
}Node;
extern Node *gast;
Node *getnewnode();
Node *gettailnode(Node *node);

void printast(Node *ast);
void build_symtable(Node *ast);
void print_symtable(Node *ast);
void set_idtype(Node *ast);
void set_expidtype(HashNode **symtable, Node *exp);
void set_stmttype(HashNode **symtable, Node *stmt);
void typecheck(Node *ast);
void typecheck_stmt(Node *stmt);
void typecheck_exp(Node *exp);
void print_gsymtable();
void gencode(Node *ast);
int gencode_exp(Node *exp, Node *funcnode);
void gencode_stmt(Node *stmt, Node *funcnode);

#endif
