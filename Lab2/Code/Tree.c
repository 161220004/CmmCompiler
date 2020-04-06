#include "Tree.h"

Node* root = NULL;

int errorflag = 0;

Node* createTerminalNode(char* tName, int lineno, int colno, ValueType valType, ...) {
  Node *terminal = malloc(sizeof(Node));
  terminal->name = tName;
  terminal->isTerminal = true;
  terminal->lineno = lineno;
  terminal->colno = colno;
  terminal->valType = valType;
  // 未知参数不同类型分别赋值
  va_list val;
	va_start(val, valType);
  if (valType == INT_VAL) {
    terminal->ival = va_arg(val, int);
    if (yyget_debug()) printf("\t\t- Tree.createTerminalNode: get Value INT = %d\n", terminal->ival);
  } else if (valType == FLOAT_VAL) {
    terminal->fval = va_arg(val, double);
    if (yyget_debug()) printf("\t\t- Tree.createTerminalNode: get Value FLOAT = %f\n", terminal->fval);
  } else if (valType == ID_VAL || valType == TYPE_VAL) {
    char* cval = va_arg(val, char*);
    int clen = strlen(cval);
    terminal->cval = (char*)malloc(sizeof(char) * clen);
    strncpy(terminal->cval, cval, clen);
    terminal->cval[clen] = '\0';
    if (yyget_debug()) printf("\t\t- Tree.createTerminalNode: get Value ID/TYPE = %s (%lu Byte)\n", terminal->cval, sizeof(char) * clen);
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
  nonterminal->isTerminal = false;
  nonterminal->lineno = lineno;
  nonterminal->colno = colno;
  nonterminal->valType = NO_VAL;
  // 子节点依次连接到父节点
  va_list children;
	va_start(children, num);
  Node* lastSibling = nonterminal;
  if (yyget_debug()) printf("\t\t- Tree.createNonTerminalNode: %s", nonterminal->name);
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

void printNode(Node* node, int lev) {
  if (node == NULL) return;
  // 没有子节点的非终结符节点，跳过
  if (!node->isTerminal && node->child == NULL) {
    if (yyget_debug()) printf("\t\t- Tree.printNode: [%s] No Leaves Nonterminal\n", node->name);
    printNode(node->nextSibling, lev);
    return;
  }
  // 缩进
  for (int i = 0; i < lev; i++) {
    printf("  ");
  }
  printf("%s", node->name);
  if (node->valType == INT_VAL) {
    printf(": %d", node->ival);
  } else if (node->valType == FLOAT_VAL) {
    printf(": %f", node->fval);
  } else if (node->valType == ID_VAL || node->valType == TYPE_VAL) {
    printf(": %s", node->cval);
  } else {
    /* NO_Val */
  }
  if (!node->isTerminal) {
    printf(" (%d)", node->lineno);
  }
	printf("\n");
  // 下一个
  printNode(node->child, lev + 1);
  printNode(node->nextSibling, lev);
}

void printTree() {
  if (errorflag == 0) {
    printNode(root, 0);
  }
}

void setError() {
  errorflag = 1;
}
