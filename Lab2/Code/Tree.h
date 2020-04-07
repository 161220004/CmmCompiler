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

typedef enum {
  NTN_PROGRAM = 0, NTN_EXTDEFLIST, NTN_EXTDEF, NTN_EXTDECLIST, NTN_SPECIFIER, NTN_STRUCTSPECIFIER,
  NTN_OPTTAG, NTN_TAG, NTN_VARDEC, NTN_FUNDEC, NTN_VARLIST, NTN_PARAMDEC, NTN_COMPST,
  NTN_STMTLIST, NTN_STMT, NTN_DEFLIST, NTN_DEF, NTN_DECLIST, NTN_DEC, NTN_EXP, NTN_ARGS,
  TN_SEMI, TN_COMMA, TN_EQ, TN_NE, TN_LE, TN_GE, TN_LT, TN_GT, TN_ASSIGNOP, TN_PLUS, TN_MINUS,
  TN_STAR, TN_DIV, TN_AND, TN_OR, TN_NOT, TN_LP, TN_RP, TN_LB, TN_RB, TN_LC, TN_RC, TN_DOT,
  TN_TYPE, TN_ID, TN_INT, TN_FLOAT, TN_STRUCT, TN_RETURN, TN_IF, TN_ELSE, TN_WHILE
} NodeName;

struct Node {
  NodeName name; /* 终结符/非终结符的名字 */
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
Node* createTerminalNode(NodeName tName, int lineno, int colno, ValueType valType, ...);

/** 为非终结符创建节点；有num个Node类型参数作为该非终结符的子节点 */
Node* createNonTerminalNode(NodeName ntName, int lineno, int colno, int num, ...);

/** 打印整棵语法树 */
void printTree();

/** 第几次实验 */
void setLabFlag(int n);
bool isLab(int n);

/** 判定出现语法/词法错误 */
void setError();

extern int yyget_debug(void);

#endif
