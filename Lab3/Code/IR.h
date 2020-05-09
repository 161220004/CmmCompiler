#ifndef IR_H
#define IR_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

typedef struct Operand Operand;
typedef struct InterCode InterCode;
typedef enum {
  OP_VAR, OP_CONST, OP_TEMP, OP_ADDR,
  OP_GETADDR, OP_GETCONT
} OpKind;
typedef enum {
  IR_ASSIGN, IR_ADD, IR_SUB, IR_MUL, IR_DIV,
  IR_LABEL, IR_FUNCTION, IR_RETURN, IR_PARAM, IR_ARG, IR_CALL,
  IR_READ, IR_WRITE, IR_IF, IR_GOTO, IR_DEC, IR_NULL
} IRKind;
typedef enum { EQ, NE, GE, GT, LE, LT } Relop;

struct Operand {
	OpKind kind;
	union {
		char* name;
		int val; // Lab3只考虑整数
	};
};

struct InterCode {
  // 操作类型，即多个操作数的连接方式
  IRKind kind;
	union {
		struct { Operand* op; } one; // 包括：RETURN, PARAM, ARG, READ, WRITE
		struct { Operand* op1; Operand* op2; } two; // 包括：ASSIGN
		struct { Operand* op1; Operand* op2; Operand* op3; } three; // 包括：ADD, SUB, MUL, DIV, DEC
		struct { Operand* op; char* funcName; } call; // 包括：CALL
		struct { Operand* op1; Relop relop; Operand* op2; char* label; } ifcode; // 包括：IF
		char* name; // 包括：LABEL, FUNCTION, GOTO
	};
  InterCode* prev;
  InterCode* next;
};

void generateIR(char* fileName);
void translateProgram();
void translateExtDefList(Node* extDefListNode);
void translateExtDef(Node* extDefNode);
void translateFunDec(Node* funDecNode);
void translateVarList(Node* varListNode);
void translateParamDec(Node* paramDecNode);
Operand* translateVarDec(Node* varDecNode);
void translateCompSt(Node* compStNode);

#endif
