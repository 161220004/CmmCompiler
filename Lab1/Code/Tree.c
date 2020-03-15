#include "Tree.h"

Node* root = NULL;

Node* createTerminalNode(char* tName, int lineno, int colno, ValueType valType, ...) {
  Node *terminal = malloc(sizeof(Node));
  terminal->name = tName;
  terminal->lineno = lineno;
  terminal->colno = colno;
  terminal->valType = valType;
  // 未知参数不同类型分别赋值
  va_list val;
	va_start(val, valType);
  if (valType == INT_VAL) {
    terminal->ival = va_arg(val, int);
    if (yyget_debug()) printf("    Tree.createTerminalNode: get Value INT = %d\n", terminal->ival);
  } else if (valType == FLOAT_VAL) {
    terminal->fval = va_arg(val, double);
    if (yyget_debug()) printf("    Tree.createTerminalNode: get Value FLOAT = %f\n", terminal->fval);
  } else if (valType == ID_VAL) {
    char* idval = va_arg(val, char*);
    int idlen = strlen(idval);
    terminal->idval = (char*)malloc(sizeof(char) * idlen);
    strncpy(terminal->idval, idval, idlen);
    if (yyget_debug()) printf("    Tree.createTerminalNode: get Value ID = %s (%lu Byte)\n", terminal->idval, sizeof(char) * idlen);
  } else {
    /* NO_Val */
  }
  va_end(val);
  terminal->child = NULL;
  terminal->nextSibling = NULL;
  return terminal;
}

Node* createNonTerminalNode(char* ntName, int lineno, int colno, int num, ...) {
  Node *nonterminal = malloc(sizeof(Node));
  nonterminal->name = ntName;
  nonterminal->lineno = lineno;
  nonterminal->colno = colno;
  nonterminal->valType = NO_VAL;
  // 子节点依次连接到父节点
  va_list children;
	va_start(children, num);
  Node* lastSibling = nonterminal;
  if (yyget_debug()) printf("    Tree.createNonTerminalNode: %s", nonterminal->name);
  while(num > 0) {
    Node* cnode = va_arg(children, Node*);
    if (nonterminal->child == NULL) {
      nonterminal->child = cnode;
      lastSibling = nonterminal->child;
      if (yyget_debug()) printf(" -> (child) %s", lastSibling->name);
    } else {
      lastSibling->nextSibling = cnode;
      lastSibling = lastSibling->nextSibling;
      if (yyget_debug()) printf(" -> (next) %s", lastSibling->name);
    }
    num -= 1;
  }
  if (yyget_debug()) printf("\n");
  va_end(children);
  return nonterminal;
}
