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
  IR_READ, IR_WRITE, IR_IF, IR_GOTO, IR_DEC
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
		struct { Operand* op1; Operand* op2; } two; // 包括：ASSIGN, DEC
		struct { Operand* op1; Operand* op2; Operand* op3; } three; // 包括：ADD, SUB, MUL, DIV
		struct { Operand* op; char* funcName; } call; // 包括：CALL
		struct { Operand* op1; Relop relop; Operand* op2; char* label; } ifcode; // 包括：IF
		char* name; // 包括：LABEL, FUNCTION, GOTO
	};
  InterCode* prev;
  InterCode* next;
};

void generateIR(char* fileName);
void translateProgram();
InterCode* translateExtDefList(Node* extDefListNode, InterCode* tail);
InterCode* translateExtDef(Node* extDefNode);
InterCode* translateFunDec(Node* funDecNode);
InterCode* translateVarList(Node* varListNode, InterCode* tail);
InterCode* translateParamDec(Node* paramDecNode);
Operand* translateVarDec(Node* varDecNode);
InterCode* translateCompSt(Node* compStNode);
InterCode* translateDefList(Node* defListNode, InterCode* tail);
InterCode* translateDef(Node* defNode);
InterCode* translateDecList(Node* decListNode, Type* defType, InterCode* tail);
InterCode* translateDec(Node* decNode, Type* defType);
InterCode* translateStmtList(Node* stmtListNode, InterCode* tail);
InterCode* translateStmt(Node* stmtNode);
InterCode* translateCond(Node* expNode, InterCode* trueLab, InterCode* falseLab);
InterCode* translateExp(Node* expNode, Operand* place);
InterCode* translateArgs(Node* argsNode, InterCode* tail, InterCode* argsCode);
InterCode* translateArrayAddr(Node* expNode, char* arrayName, bool isParam, Type* type, Operand* inhOp, Operand* place);
InterCode* translateStructAddr(Node* expNode, Operand* place);

#endif
