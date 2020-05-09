#ifndef IRUTILS_H
#define IRUTILS_H

#include "SemUtils.h"
#include "IR.h"

/** 全部中间代码双向链表 */
extern InterCode* IRList;
/** 当前分析到的中间代码位置（总是指向最后一个不是NULL的中间代码） */
extern InterCode* IRTail;
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

void printInterCode(char* fileName);
void printOperand(Operand* op, FILE* file);
void addCodeToTail(InterCode* newCode);

Operand* createOperand(OpKind kind, char* name);
Operand* createConst(int val);
InterCode* createInterCodeNull();
InterCode* createInterCodeName(IRKind kind, char* name);
InterCode* createInterCodeOne(IRKind kind, Operand* op);

#endif
