#ifndef SEMANALYSIS_H
#define SEMANALYSIS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

/* 类型：包括 int，float，数组，结构体 */
typedef enum { T_INT, T_FLOAT, T_ARRAY, T_STRUCT } Kind;

/* 作用域类型：包括全局作用域，局部作用域（函数/条件/循环/其他） */
typedef enum {
  F_GLOBAL, F_FUNCTION, F_CONDITION, F_LOOP, F_OTHER
} FieldType;

typedef struct Type Type;
typedef struct Function Function;
typedef struct TypeNode TypeNode;
typedef struct FieldNode FieldNode;
typedef struct SymElem SymElem;

/* 类型信息 */
struct Type {
  Kind kind; // 类型，int/float/array/struct
  union {
    bool isRight; // int/float专用，是否是右值
    struct { Type* eleType; int length; } array; // array专用，元素类型和元素个数
    struct { TypeNode* node; char* name; } structure; // struct专用，域的链表和结构体名以及是否定义内部
  };
};

/* 函数信息 */
struct Function {
  char* name; // 函数名
  bool isDefined; // 是否定义（一旦存在，默认已经声明）
  Type* returnType; // 返回类型
  TypeNode* paramNode; // 参数链表
};

/* 结构体的一个域，或函数的一个参数，或作用域内的定义语句 */
struct TypeNode {
  Type* type; // 域/参数的类型
  char* name; // 域/参数的id
  int lineno; // 所在行数
  TypeNode* next;
};

/* 作用域 */
struct FieldNode {
  FieldType type; // 作用域种类
  FieldNode* parent; // 其外部作用域（一个）
  Function* func; // 对于函数作用域，记录函数信息（一个）
  int varListLen; // 本作用域内的变量符号表长度
  SymElem* varSymList; // 本作用域内的变量符号表（已按id排序的有序数组）（不包括外部作用域的符号）
};

/* 变量/函数/结构体符号表数组元素 */
struct SymElem {
  bool isNull; // 是否空
  char* name; // 变量名/函数名/结构体名
  union {
    Type* type; // 变量类型或结构体结构
    Function* func; // 函数信息
  };
};

void handleProgram();
void handleExtDefList(Node* extDefListNode);
void handleExtDef(Node* extDefNode);
TypeNode* handleExtDecList(Node* extDecListNode, TypeNode* inhTypeNode, Type* inhType);
Type* handleSpecifier(Node* specNode, bool isSemi);
Type* handleStructSpecifier(Node* structSpecNode, bool isSemi);
Type* handleVarDec(Node* varDecNode, Type* inhType);
char* getVarDecName(Node* varDecNode);
TypeNode* handleVarList(Node* varListNode, TypeNode* inhTypeNode);
TypeNode* handleParamDec(Node* paramDecNode);
void handleCompSt(Node* compStNode);
TypeNode* handleDefList(Node* defListNode, TypeNode* inhTypeNode, bool inStruct);
TypeNode* handleDef(Node* defNode, bool inStruct);
TypeNode* handleDecList(Node* decListNode, TypeNode* inhTypeNode, Type* inhType, bool inStruct);
TypeNode* handleDec(Node* decNode, Type* inhType, bool inStruct);

#endif
