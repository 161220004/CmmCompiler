#ifndef TREE_H
#define TREE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>

typedef enum {
  NO_VAL, INT_VAL, FLOAT_VAL, ID_VAL, TYPE_VAL
} ValueType;

struct Node {
  char* name; /* 终结符/非终结符的名字 */
  bool isTerminal; /* 是否是终结符 */
  int lineno; /* 行数 */
  int colno; /* 列数 */
  ValueType valType; /* 数据类型 */
  union { /* 根据不同的数据类型存储不同的值 */
		int ival; /* int */
		double fval; /* float */
    char* cval; /* ID或Type */
	};
  struct Node* child;
	struct Node* nextSibling;
};

typedef struct Node Node;

/** 根节点 */
extern Node* root;

/** 为终结符创建节点；有一个未知类型的参数存储值（int/double/char* ...） */
Node* createTerminalNode(char* tName, int lineno, int colno, ValueType valType, ...);

/** 为非终结符创建节点；有num个Node类型参数作为该非终结符的子节点 */
Node* createNonTerminalNode(char* ntName, int lineno, int colno, int num, ...);

/** 打印整棵语法树 */
void printTree();

/** 判定出现语法/词法错误 */
void setError();

extern int yyget_debug(void);

#endif
