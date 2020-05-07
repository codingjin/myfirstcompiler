#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#define DATAAREA_BEGIN 0
#define DATAAREA_END 19999
#define MAXRID	254
#define NONE -1
#define LABEL_LEN 16
#define SP_BOUND 20000
#define RET 255


typedef enum Opcode {
	NOP=0, ADD, ADDI, SUB, SUBI, RSUBI, MULT, MULTI, DIV, DIVI, RDIVI,
	ANDOP, ANDI, OROP, ORI, XOROP, XORI,
	LOAD, LOADI, LOADAI, LOADAO, STORE, STOREAI, STOREAO, I2I,
	BR, CBR, CMPLT, CMPLE, CMPEQ, CMPNE, CMPGE, CMPGT, OUTPUTAI
}Opcode;

static int available_register = 1; // r0 is reserved for the begining of DATAAREA
static int available_label = 0;
static int available_offset = 0;

extern int NewRegister();
extern int NewLabel();
extern int NewOffset();
extern void emit(int label, Opcode opcode, int op1, int op2, int op3);
extern void emitcomment(char *comment, ...);

extern FILE *outfile;

#endif
