#ifndef TREE_H
#define TREE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

typedef enum {
  NO_VAL, INT_VAL, FLOAT_VAL, ID_VAL
} ValueType;

struct Node {
  char* name;
  int lineno;
  int colno;
  ValueType valType;
  union {
		int ival;
		double fval;
    char* idval;
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

extern int yyget_debug(void);

#endif
