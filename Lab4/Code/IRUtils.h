#ifndef IRUTILS_H
#define IRUTILS_H

#include "SemUtils.h"
#include "IR.h"

/** 全部中间代码双向链表 */
extern InterCode* IRList;
/** 变量个数（需要初始化以计算符号表长度） */
extern int interVarNum;
/** 中间代码变量符号表 */
extern SymElem* interVarList;
/** 参数个数 */
extern int paramCount;
/** 临时变量个数 */
extern int tempCount;
/** Label个数 */
extern int labelCount;
/* 是否调用了read函数 */
extern bool hasRead;
/* 是否调用了write函数 */
extern bool hasWrite;

void printInterCode(FILE* file, InterCode* IRCode, bool toNewLine);
void printInterCodes(char* fileName);
void printOperand(Operand* op, FILE* file);
InterCode* getInterCodeHead(InterCode* tail);
InterCode* getInterCodeTail(InterCode* head);
InterCode* linkInterCodeHeadToHead(InterCode* prevCode, InterCode* nextCode);
InterCode* linkInterCodeTailToHead(InterCode* prevCode, InterCode* nextCode);
InterCode* addCodeToTail(InterCode* newCode, InterCode* tail);
char* getOpName(char* body, int num);
bool isCondition(Node* expNode);
bool isRelop(NodeName name);
Relop getExpRelop(NodeName name);
void setVarToParam(char* name);
bool varIsParam(char* name);
bool isPureID(Node* expNode);
char* getPureID(Node* expNode);
bool isPureInt(Node* expNode);
int getPureInt(Node* expNode);
bool isPureArrayStruct(Node* expNode);
Operand* lookUpVar(char* name);
Type* lookUpArrayType(char* name);
Type* lookUpStructType(char* name);
Operand* lookUpFunc(char* name);
Operand* newTemp();
char* newLabel();
Operand* createOperand(OpKind kind, char* name);
Operand* createConst(int val);
InterCode* createInterCodeName(IRKind kind, char* name);
InterCode* createInterCodeOne(IRKind kind, Operand* op);
InterCode* createInterCodeTwo(IRKind kind, Operand* op1, Operand* op2);
InterCode* createInterCodeThree(IRKind kind, Operand* op1, Operand* op2, Operand* op3);
InterCode* createInterCodeIf(Operand* op1, Relop relop, Operand* op2, char* label);
InterCode* createInterCodeCall(IRKind kind, Operand* op, char* funcName);
int getVarMemory(Type* type);
char* getArrayName(Node* expNode);
int getFieldPosInStruct(char* name, Type* type);
void printIRVarList();

#endif
