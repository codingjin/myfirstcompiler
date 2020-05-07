#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "instruction.h"

int NewRegister()
{
	if (available_register > MAXRID)	available_register = 1;
	return available_register++;
}

int NewLabel()
{
	if (available_label < 0) {
		fprintf(stderr, "available_label[%d] error!\n", available_label);
		exit(0);
	}
	return available_label++;
}

int NewOffset()
{
	int tmp = available_offset;
	if (tmp > DATAAREA_END) {
		fprintf(stderr, "memoey is used up!\n");
		exit(0);
	}
	available_offset += 4;
	return tmp;
}

void emitcomment(char *comment, ...)
{
	va_list ap;
	va_start(ap, comment);
	fprintf(outfile, "//");
	vfprintf(outfile, comment, ap);
	fprintf(outfile, "\n");
}

void emit(int label_index, Opcode opcode,
			int op1, int op2, int op3)
{
	char label[LABEL_LEN] = {0};

	if (label_index > NONE)
		sprintf(label, "L%d:", label_index);

	switch (opcode) {
		case NOP:
			fprintf(outfile, "%s\t nop \n", label); break;
		case ADDI:
			fprintf(outfile, "%s\t addI \t r%d, %d \t => r%d \n", 
					label, op1, op2, op3);
			break;
		case ADD:
			fprintf(outfile, "%s\t add \t r%d, r%d \t => r%d \n", 
					label, op1, op2, op3);
			break;
		case SUBI:
			fprintf(outfile, "%s\t subI \t r%d, %d \t => r%d \n",
					label, op1, op2, op3);
			break;
		case RSUBI:
			fprintf(outfile, "%s\t rsubI \t r%d, %d \t => r%d \n",
					label, op1, op2, op3);
			break;
		case SUB:
			fprintf(outfile, "%s\t sub \t r%d, r%d \t => r%d \n",
					label, op1, op2, op3);
			break;
		case MULTI:
			fprintf(outfile, "%s\t multI \t r%d, %d \t => r%d \n",
					label, op1, op2, op3);
			break;
		case MULT:
			fprintf(outfile, "%s\t mult \t r%d, r%d \t => r%d \n",
					label, op1, op2, op3);
			break;
		case DIVI:
			fprintf(outfile, "%s\t divI \t r%d, %d \t => r%d \n",
					label, op1, op2, op3);
			break;
		case RDIVI:
			fprintf(outfile, "%s\t rdivI \t r%d, %d \t => r%d \n",
					label, op1, op2, op3);
			break;
		case DIV:
			fprintf(outfile, "%s\t div \t r%d, r%d \t => r%d \n",
					label, op1, op2, op3);
			break;
		case ANDOP:
			fprintf(outfile, "%s\t and \t r%d, r%d \t => r%d \n",
					label, op1, op2, op3);
			break;
		case ANDI:
			fprintf(outfile, "%s\t andI \t r%d, %d \t => r%d \n",
					label, op1, op2, op3);
			break;
		case OROP:
			fprintf(outfile, "%s\t or \t r%d, r%d \t => r%d \n",
					label, op1, op2, op3);
			break;
		case ORI:
			fprintf(outfile, "%s\t orI \t r%d, %d \t => r%d \n",
					label, op1, op2, op3);
			break;
		case XOROP:
			fprintf(outfile, "%s\t xor \t r%d, r%d \t => r%d \n",
					label, op1, op2, op3);
			break;
		case XORI:
			fprintf(outfile, "%s\t xorI \t r%d, %d \t => r%d \n",
					label, op1, op2, op3);
			break;
		case LOAD:
			fprintf(outfile, "%s\t load \t r%d \t => r%d \n", label, op1, op2); break;
		case LOADI:
			fprintf(outfile, "%s\t loadI \t %d \t => r%d \n", label, op1, op2); break;
		case LOADAI:
			fprintf(outfile, "%s\t loadAI \t r%d, %d \t => r%d \n",
					label, op1, op2, op3);
			break;
		case LOADAO:
			fprintf(outfile, "%s\t loadAO \t r%d, r%d \t => r%d \n",
					label, op1, op2, op3);
			break;
		case STORE:
			fprintf(outfile, "%s\t store \t r%d \t => r%d \n", label, op1, op2); break;
		case STOREAI:
			fprintf(outfile, "%s\t storeAI \t r%d \t => r%d, %d \n",
					label, op1, op2, op3);
			break;
		case STOREAO:
			fprintf(outfile, "%s\t storeAO \t r%d, r%d \t => r%d \n",
					label, op1, op2, op3);
			break;
		case BR: /* br	L1 */
			fprintf(outfile, "%s\t br \t L%d \n", label, op1); break;
		case CBR: /* cbr r1 => L1, L2 */
			fprintf(outfile, "%s\t cbr \t r%d \t => L%d, L%d \n",
					label, op1, op2, op3);
			break;
		case CMPLT: /* cmp_LT	r1, r2	=> r3 */
			fprintf(outfile, "%s\t cmp_LT \t r%d, r%d \t => r%d \n",
					label, op1, op2, op3);
			break;
		case CMPLE: /* cmp_TE	r1, r2	=> r3 */
			fprintf(outfile, "%s\t cmp_LE	\t r%d, r%d \t => r%d \n",
					label, op1, op2, op3);
			break;
		case CMPEQ:
			fprintf(outfile, "%s\t cmp_EQ \t r%d, r%d \t => r%d \n",
					label, op1, op2, op3);
			break;
		case CMPNE:
			fprintf(outfile, "%s\t cmp_NE \t r%d, r%d \t => r%d \n",
					label, op1, op2, op3);
			break;
		case CMPGE:
			fprintf(outfile, "%s\t cmp_GE \t r%d, r%d \t => r%d \n",
					label, op1, op2, op3);
			break;
		case CMPGT:
			fprintf(outfile, "%s\t cmp_GT \t r%d, r%d \t => r%d \n",
					label, op1, op2, op3);
			break;
		case OUTPUTAI:
			fprintf(outfile, "%s\t outputAI \t r%d, %d \n", label, op1, op2);
			break;
		case I2I:
			fprintf(outfile, "%s\t i2i \t r%d => r%d \n", label, op1, op2);
			break;
		default:
			fprintf(stderr, "Invalid instruction:emit(%d, %d, %d, %d, %d)\n",
						label_index, opcode, op1, op2, op3);
			exit(0);
	}
}

