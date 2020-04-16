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
int getRoughLocVarNum(Node* defListNode);
void addToFuncList(Function* func);
void addToStructList(Type* type);
void addToVarList(TypeNode* addVar, SymElem* varList, int varListLen);
TypeNode* findInTypeNode(char* name, TypeNode* typeNode);
int findInSymList(char* name, int start, int end, SymElem* varList);
Type* findTypeInAllVarList(char* name, FieldNode* field);
Type* createUndefinedType(bool isRight);
Type* createRightType(Kind kind);
Type* createBasicType(Kind kind);
Type* createArrayType(int length, Type* eleType);
Type* createStructType(char* name);
TypeNode* createTypeNode(Type* type, char* name, int lineno, TypeNode* next);
Function* createFunction(char* name, int lineno, bool isDefined, Type* returnType, TypeNode* paramNode);
FieldNode* createChildField(FieldType type, int varListLen, Function* func);
char* getArgsString(TypeNode* paramNode, char* funcName);
char* getExpString(Node* expNode);
bool isBasicType(Type* type);
bool typeEquals(Type* type1, Type* type2);
bool paramEquals(TypeNode* param1, TypeNode* param2);
void printType(Type* type, bool toNewLine);
void printStruct(Type* type);
void printTypeNode(TypeNode* typeNode, bool toNewLine);
void printFunction(Function* func, bool toNewLine);
void printSymList(int symListLen, SymElem* symList, bool toNewLine);
void printFieldNode(FieldNode* field);

#endif
