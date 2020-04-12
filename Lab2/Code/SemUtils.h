#ifndef SEMUTILS_H
#define SEMUTILS_H

#include "Tree.h"
#include "SemAnalysis.h"

/* 函数符号表（有序数组） */
extern SymElem* funcSymList;
extern int funcSymListLen;
/* 结构体符号表（有序数组） */
extern SymElem* structSymList;
extern int structSymListLen;
/* 全局作用域 */
extern FieldNode* globalField;
/* 当前所在的作用域 */
extern FieldNode* currentField;

void reportError(int errno, int lineno, char* val, char* addition);
bool childrenMatch(Node* node, int n, NodeName expectName);
Node* getCertainChild(Node* node, int n);
char* itoa(int num);
Type* typeShallowCopy(Type* type);
TypeNode* linkTypeNodeList(TypeNode* preList, TypeNode* addList);
int getRoughStructNum(Node* extDefListNode);
int getRoughFuncNum(Node* extDefListNode);
int getRoughGloVarNum(Node* extDefListNode);
void addToFuncList(Function* func);
void addToStructList(Type* type);
void addToVarList(TypeNode* addVar, SymElem* varList, int varListLen);
int findInSymList(char* name, int start, int end, bool isFunc);
int findInVarList(char* name, int start, int end, SymElem* varList);
bool isInVarList(char* name, FieldNode* field);
bool typeEquals(Type* type1, Type* type2);
bool paramEquals(TypeNode* param1, TypeNode* param2);

#endif
