#include "Tree.h"

Node* root = NULL;

int errorflag = 0;

int labflag = 1; // 第几次实验

int processflag = 1; // 步骤（ 1: Lab1， 2: Lab2， 3: Lab3， 4: Lab4）

char* getStrncpy(char* str) {
  int len = strlen(str);
  char* cp = (char*)malloc(sizeof(char) * len);
  strncpy(cp, str, len);
  cp[len] = '\0';
  return cp;
}

char* NodeSymbolStr[53] = {
  "Program", "ExtDefList", "ExtDef", "ExtDecList", "Specifier", "StructSpecifier",
  "OptTag", "Tag", "VarDec", "FunDec", "VarList", "ParamDec", "CompSt",
  "StmtList", "Stmt", "DefList", "Def", "DecList", "Dec", "Exp", "Args",
  ";", ",", "==", "!=", "<=", ">=", "<", ">", "=", "+", "-",
  "*", "/", "&&", "||", "!", "(", ")", "[", "]", "{", "}", ".",
  "TYPE", "ID", "INT", "FLOAT", "struct", "return", "if", "else", "while"
};

Node* createTerminalNode(NodeName tName, int lineno, int colno, ValueType valType, ...) {
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
    terminal->cval = getStrncpy(va_arg(val, char*));
  } else if (valType == FLOAT_VAL) {
    terminal->fval = va_arg(val, double);
    terminal->cval = getStrncpy(va_arg(val, char*));
  } else if (valType == ID_VAL || valType == TYPE_VAL) {
    char* cval = va_arg(val, char*);
    terminal->cval = getStrncpy(cval);
  } else { // NO_Val, 为其他的cval里填入对应符号
    terminal->cval = NodeSymbolStr[tName];
  }
  va_end(val);
  terminal->child = NULL;
  terminal->nextSibling = NULL;
  return terminal;
}

Node* createNonTerminalNode(NodeName ntName, int lineno, int colno, int num, ...) {
  Node *nonterminal = malloc(sizeof(Node));
  nonterminal->name = ntName;
  nonterminal->isTerminal = false;
  nonterminal->lineno = lineno;
  nonterminal->colno = colno;
  nonterminal->valType = NO_VAL;
  nonterminal->cval = NodeSymbolStr[ntName];
  nonterminal->child = NULL;
  // 子节点依次连接到父节点
  va_list children;
	va_start(children, num);
  Node* lastSibling = nonterminal;
  while(num > 0) {
    Node* cnode = va_arg(children, Node*);
    if (nonterminal->child == NULL) {
      nonterminal->child = cnode;
      lastSibling = nonterminal->child;
    } else {
      lastSibling->nextSibling = cnode;
      lastSibling = lastSibling->nextSibling;
    }
    num -= 1;
  }
  va_end(children);
  return nonterminal;
}

char* NodeNameStr[53] = {
  "Program", "ExtDefList", "ExtDef", "ExtDecList", "Specifier", "StructSpecifier",
  "OptTag", "Tag", "VarDec", "FunDec", "VarList", "ParamDec", "CompSt",
  "StmtList", "Stmt", "DefList", "Def", "DecList", "Dec", "Exp", "Args",
  "SEMI", "COMMA", "EQ", "NE", "LE", "GE", "LT", "GT", "ASSIGNOP", "PLUS", "MINUS",
  "STAR", "DIV", "AND", "OR", "NOT", "LP", "RP", "LB", "RB", "LC", "RC", "DOT",
  "TYPE", "ID", "INT", "FLOAT", "STRUCT", "RETURN", "IF", "ELSE", "WHILE"
};

void printNode(Node* node, int lev) {
  if (node == NULL) return;
  // 没有子节点的非终结符节点，跳过
  if (!node->isTerminal && node->child == NULL) {
    printNode(node->nextSibling, lev);
    return;
  }
  // 缩进
  for (int i = 0; i < lev; i++) {
    printf("  ");
  }
  printf("%s", NodeNameStr[node->name]);
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
  if (labflag == 1 && errorflag == 0) {
    printNode(root, 0);
  }
}

void setLabFlag(int n) {
  labflag = n;
}
bool isLab(int n) {
  return (labflag == n);
}

void setProcessFlag(int n) {
  processflag = n;
}
bool isProcess(int n) {
  return (processflag == n);
}

void setError() {
  errorflag = 1;
}

bool isError() {
  return (errorflag > 0);
}
