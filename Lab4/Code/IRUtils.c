#include "IRUtils.h"

/** 全部中间代码双向链表 */
InterCode* IRList = NULL;

/** 变量个数（需要初始化以计算符号表长度） */
int interVarNum = 0;
/** 中间代码变量符号表 */
SymElem* interVarList = NULL;

/* 参数个数 */
int paramCount = 0;
/* 临时变量个数 */
int tempCount = 0;
/* Label个数 */
int labelCount = 0;

/* 是否调用了read函数 */
bool hasRead = false;
/* 是否调用了write函数 */
bool hasWrite = false;

/** 打印一条中间代码 */
void printInterCode(FILE* file, InterCode* IRCode, bool toNewLine) {
  if (IRCode == NULL) {
    if (yyget_debug()) printf("NULL");
  } else if (IRCode->kind == IR_ASSIGN) {
    // 赋值
    if (IRCode->two.op1 == NULL || IRCode->two.op2 == NULL) return;
    printOperand(IRCode->two.op1, file);
    fprintf(file, " := ");
    printOperand(IRCode->two.op2, file);
  } else if (IRCode->kind == IR_ADD || IRCode->kind == IR_SUB ||
             IRCode->kind == IR_MUL || IRCode->kind == IR_DIV) {
    // 加减乘除
    if (IRCode->three.op1 == NULL || IRCode->three.op2 == NULL || IRCode->three.op3 == NULL) return;
    printOperand(IRCode->three.op1, file);
    fprintf(file," := ");
    printOperand(IRCode->three.op2, file);
    if (IRCode->kind == IR_ADD) fprintf(file, " + ");
    else if (IRCode->kind == IR_SUB) fprintf(file, " - ");
    else if (IRCode->kind == IR_MUL) fprintf(file, " * ");
    else if (IRCode->kind == IR_DIV) fprintf(file, " / ");
    printOperand(IRCode->three.op3, file);
  } else if (IRCode->kind == IR_LABEL) {
    // Label
    fprintf(file, "LABEL %s :", IRCode->name);
  } else if (IRCode->kind == IR_FUNCTION) {
    // Function
    fprintf(file, "FUNCTION %s :", IRCode->name);
  } else if (IRCode->kind == IR_RETURN || IRCode->kind == IR_PARAM ||
             IRCode->kind == IR_READ || IRCode->kind == IR_WRITE ) {
    // 关键字 + 变量名
    if (IRCode->one.op == NULL) return;
    if (IRCode->kind == IR_RETURN) fprintf(file, "RETURN ");
    else if (IRCode->kind == IR_PARAM) fprintf(file, "PARAM ");
    else if (IRCode->kind == IR_READ) fprintf(file, "READ ");
    else if (IRCode->kind == IR_WRITE) fprintf(file, "WRITE ");
    printOperand(IRCode->one.op, file);
  } else if (IRCode->kind == IR_ARG) {
    if (IRCode->one.op == NULL) return;
    fprintf(file, "ARG ");
    if (IRCode->one.op->kind == OP_ADDR) fprintf(file, "&");
    printOperand(IRCode->one.op, file);
  } else if (IRCode->kind == IR_CALL) {
    // Call 语句
    if (IRCode->call.op == NULL) return;
    printOperand(IRCode->call.op, file);
    fprintf(file, " := CALL ");
    fprintf(file, "%s", IRCode->call.funcName);
  } else if (IRCode->kind == IR_IF) {
    // IF 语句
    fprintf(file, "IF ");
    printOperand(IRCode->ifcode.op1, file);
    switch(IRCode->ifcode.relop){
      case EQ: fprintf(file, " == "); break;
      case NE: fprintf(file, " != "); break;
      case GE: fprintf(file, " >= "); break;
      case GT: fprintf(file, " > "); break;
      case LE: fprintf(file, " <= "); break;
      case LT: fprintf(file, " < "); break;
    }
    printOperand(IRCode->ifcode.op2, file);
    fprintf(file, " GOTO %s", IRCode->ifcode.label);
  } else if (IRCode->kind == IR_GOTO) {
    // Goto 语句
    fprintf(file, "GOTO %s", IRCode->name);
  } else if (IRCode->kind == IR_DEC) {
    // 开辟空间
    if (IRCode->two.op1 == NULL || IRCode->two.op2 == NULL) return;
    fprintf(file, "DEC ");
    printOperand(IRCode->two.op1, file);
    fprintf(file, " %d", IRCode->two.op2->val);
  } else {
    // 不应该出现
    fprintf(stderr, "Error in printInterCode(): Undefined Kind %d.\n", IRCode->kind);
    return;
  }
  if (toNewLine) {
    fprintf(file, "\n");
  }
}

/** 打印全部中间代码 */
void printInterCodes(char* fileName) {
  FILE* file;
  if (fileName == NULL) {
    file = stdout;
  } else {
    file = fopen(fileName, "w");
    if (file == NULL) {
      fprintf(stderr, "Failed to Open File %s\n", fileName);
      return;
    }
  }
  // 开始写入
  InterCode* IRCode = IRList;
  while (IRCode != NULL) {
    printInterCode(file, IRCode, true);
    IRCode = IRCode->next;
  }
  // 关闭文件
	if (fileName != NULL) fclose(file);
}

/** 打印一个操作数 */
void printOperand(Operand* op, FILE* file) {
  if (op == NULL) return;
  if (op->kind == OP_CONST) { // 常量取值，前面加 #
    fprintf(file, "#%d", op->val);
  } else { // 取变量名
    if (op->kind == OP_GETADDR) { // 取址，前面加 &
      fprintf(file, "&");
    } else if (op->kind == OP_GETCONT) { // 从地址取值，前面加 *
      fprintf(file, "*");
    }
    fprintf(file, "%s", op->name);
  }
}

/** 获取中间代码头部 */
InterCode* getInterCodeHead(InterCode* tail) {
  if (tail == NULL) return NULL;
  InterCode* head = tail;
  while (head->prev != NULL) {
    head = head->prev;
  }
  return head;
}

/** 获取中间代码尾部 */
InterCode* getInterCodeTail(InterCode* head) {
  if (head == NULL) return NULL;
  InterCode* tail = head;
  while (tail->next != NULL) {
    tail = tail->next;
  }
  return tail;
}

/** 连接已知头部的两段中间代码，返回第一段中间代码的头部 */
InterCode* linkInterCodeHeadToHead(InterCode* prevCode, InterCode* nextCode) {
  if (prevCode == NULL) {
    return nextCode;
  }
  if (nextCode == NULL) {
    return prevCode;
  }
  InterCode* prevCodeTail = getInterCodeTail(prevCode);
  // 衔接
  prevCodeTail->next = nextCode;
  nextCode->prev = prevCodeTail;
  return prevCode;
}

/** 连接已知尾部和头部的两段中间代码，返回第一段中间代码的头部 */
InterCode* linkInterCodeTailToHead(InterCode* prevCode, InterCode* nextCode) {
  if (prevCode == NULL) {
    return nextCode;
  }
  if (nextCode == NULL) {
    return prevCode;
  }
  InterCode* prevCodeHead = getInterCodeHead(prevCode);
  // 衔接
  prevCode->next = nextCode;
  nextCode->prev = prevCode;
  return prevCodeHead;
}

/** 在当前位置tail后面插入一段中间代码，后面接上原来的尾部，返回新代码的尾部 */
InterCode* addCodeToTail(InterCode* newCode, InterCode* tail) {
  if (newCode == NULL) {
    return tail;
  }
  InterCode* newCodeTail = getInterCodeTail(newCode);
  if (tail == NULL) { // 返回新代码尾部
    return newCodeTail;
  }
  // 最后面的部分
  InterCode* afterTail = tail->next;
  // 开始衔接：前半部分 + 新代码头部
  tail->next = newCode;
  newCode->prev = tail;
  // 开始衔接：新代码尾部 + 后半部分
  newCodeTail->next = afterTail;
  if (afterTail != NULL) {
    afterTail->prev = newCodeTail;
  }
  if (yyget_debug()) {
    printf("Add Code to Tail: ");
    printInterCode(stdout, newCode, false);
    if (newCode != newCodeTail) {
      printf(" -> ... -> ");
      printInterCode(stdout, newCodeTail, false);
    }
    printf("\n  Context: ");
    printInterCode(stdout, newCode->prev, false);
    printf(" -> (Added) -> ");
    printInterCode(stdout, newCodeTail->next, true);
  }
  return newCodeTail;
}

/** 获取一个操作数名字（用于临时变量/Label等取名） */
char* getOpName(char* body, int num) {
  char* result = (char*)malloc(64 * sizeof(char));
  result[0] = '\0';
  strcat(result, body);
  strcat(result, itoa(num));
  return result;
}

/** 检查一个Exp是否是Condition */
bool isCondition(Node* expNode) {
  if (childrenMatch(expNode, 1, TN_NOT)) {
    return true;
  } else {
    Node* opNode = getCertainChild(expNode, 2);
    if (opNode == NULL) {
      return false;
    } else if (isRelop(opNode->name) || opNode->name == TN_AND || opNode->name == TN_OR) {
      return true;
    }
  }
  return false;
}

/** 判断是否是 Relop */
bool isRelop(NodeName name) {
  switch (name) {
    case TN_EQ: case TN_NE: case TN_LE:
    case TN_GE: case TN_LT: case TN_GT:
    return true;
    default: return false;
  }
}

/** 获取 Exp 的 Relop */
Relop getExpRelop(NodeName name) {
  switch (name) {
    case TN_EQ: return EQ;
    case TN_NE: return NE;
    case TN_LE: return LE;
    case TN_GE: return GE;
    case TN_LT: return LT;
    case TN_GT: return GT;
    default: return EQ;
  }
}

/** 把某变量设置成参数模式 */
void setVarToParam(char* name) {
  int index = findInSymList(name, 0, interVarNum, interVarList);
  if (index < 0) { // 不应该这样
    if (yyget_debug()) fprintf(stderr, "Error in Lab3, Undefined Var to Param: %s.\n", name);
  } else {
    interVarList[index].isParam = true;
  }
  if (yyget_debug()) printf("Set \"%s\" to Param\n", name);
}

/** 检查某变量是否是参数模式 */
bool varIsParam(char* name) {
  int index = findInSymList(name, 0, interVarNum, interVarList);
  if (index < 0) { // 不应该这样
    if (yyget_debug()) fprintf(stderr, "Error in Lab3, Undefined Var or Param: %s.\n", name);
    return false;
  } else {
    return interVarList[index].isParam;
  }
}

/** 检查一个Exp节点是否是纯ID */
bool isPureID(Node* expNode) {
  Node* idNode = getCertainChild(expNode, 1);
  if (idNode->name == TN_ID && idNode->nextSibling == NULL) {
    return true;
  } else return false;
}

/** 获取纯ID的Exp节点的纯ID */
char* getPureID(Node* expNode) {
  return getCertainChild(expNode, 1)->cval;
}

/** 检查一个Exp节点是否是纯INT */
bool isPureInt(Node* expNode) {
  if (childrenMatch(expNode, 1, TN_INT)) return true;
  else return false;
}

/** 获取纯INT的Exp节点的纯INT */
int getPureInt(Node* expNode) {
  return getCertainChild(expNode, 1)->ival;
}

/** 检查一个Exp节点是否是纯ARRAY/STRUCT */
bool isPureArrayStruct(Node* expNode) {
  Type* expType = handleExp(expNode);
  if (expType->kind == T_ARRAY || expType->kind == T_STRUCT) {
    if (isPureID(expNode)) return true;
    else return false;
  } else {
    return false;
  }
}

/** 从符号表获取变量 */
Operand* lookUpVar(char* name) {
  int index = findInSymList(name, 0, interVarNum, interVarList);
  if (index < 0) { // 不应该这样
    if (yyget_debug()) fprintf(stderr, "Error in Lab3, Undefined Var: %s.\n", name);
    return NULL;
  } else {
    return createOperand(OP_VAR, name);
  }
}

/** 从符号表获取数组 */
Type* lookUpArrayType(char* name) {
  int index = findInSymList(name, 0, interVarNum, interVarList);
  if (index < 0) { // 不应该这样
    if (yyget_debug()) fprintf(stderr, "Error in Lab3, Undefined Var: %s.\n", name);
    return NULL;
  } else {
    return interVarList[index].type;
  }
}

/** 从符号表获取结构体变量类型 */
Type* lookUpStructType(char* name) {
  int index = findInSymList(name, 0, interVarNum, interVarList);
  if (index < 0) { // 不应该这样
    if (yyget_debug()) fprintf(stderr, "Error in Lab3, Undefined Structure Var: %s.\n", name);
    return NULL;
  } else {
    return interVarList[index].type;
  }
}

/** 从符号表获取函数 */
Operand* lookUpFunc(char* name) {
  int index = findInSymList(name, 0, funcSymListLen, funcSymList);
  if (index < 0) { // 不应该这样
    if (yyget_debug()) fprintf(stderr, "Error in Lab3, Undefined Function: %s.\n", name);
    return NULL;
  } else {
    return createOperand(OP_VAR, name);
  }
}

/** 创建一个临时变量 */
Operand* newTemp() {
  tempCount += 1;
  return createOperand(OP_TEMP, getOpName("t", tempCount));
}

/** 创建一个Label */
char* newLabel() {
  labelCount += 1;
  return getOpName("label", labelCount);
}

/** 创建一个操作数（不是常数） */
Operand* createOperand(OpKind kind, char* name) {
  Operand* op = (Operand*)malloc(sizeof(Operand));
  op->kind = kind;
  op->name = name;
  return op;
}

/** 创建一个常数 */
Operand* createConst(int val) {
  Operand* op = (Operand*)malloc(sizeof(Operand));
  op->kind = OP_CONST;
  op->val = val;
  return op;
}

/** 创建一个无操作数的中间代码，包括：LABEL, FUNCTION, GOTO */
InterCode* createInterCodeName(IRKind kind, char* name) {
  InterCode* code = (InterCode*)malloc(sizeof(InterCode));
  code->kind = kind;
  code->name = name;
  code->prev = NULL;
  code->next = NULL;
  return code;
}

/** 创建单操作数的中间代码，包括：RETURN, PARAM, ARG, READ, WRITE */
InterCode* createInterCodeOne(IRKind kind, Operand* op) {
  InterCode* code = (InterCode*)malloc(sizeof(InterCode));
  code->kind = kind;
  code->one.op = op;
  code->prev = NULL;
  code->next = NULL;
  return code;
}

/** 创建双操作数的中间代码，包括：ASSIGN, DEC */
InterCode* createInterCodeTwo(IRKind kind, Operand* op1, Operand* op2) {
  InterCode* code = (InterCode*)malloc(sizeof(InterCode));
  code->kind = kind;
  code->two.op1 = op1;
  code->two.op2 = op2;
  code->prev = NULL;
  code->next = NULL;
  return code;
}

/** 创建三操作数的中间代码，包括：ADD, SUB, MUL, DIV */
InterCode* createInterCodeThree(IRKind kind, Operand* op1, Operand* op2, Operand* op3) {
  InterCode* code = (InterCode*)malloc(sizeof(InterCode));
  code->kind = kind;
  code->three.op1 = op1;
  code->three.op2 = op2;
  code->three.op3 = op3;
  code->prev = NULL;
  code->next = NULL;
  return code;
}

/** 创建If语句的中间代码 */
InterCode* createInterCodeIf(Operand* op1, Relop relop, Operand* op2, char* label) {
  InterCode* code = (InterCode*)malloc(sizeof(InterCode));
  code->kind = IR_IF;
  code->ifcode.op1 = op1;
  code->ifcode.relop = relop;
  code->ifcode.op2 = op2;
  code->ifcode.label = label;
  code->prev = NULL;
  code->next = NULL;
  return code;
}

/** 创建Call的中间代码 */
InterCode* createInterCodeCall(IRKind kind, Operand* op, char* funcName) {
  InterCode* code = (InterCode*)malloc(sizeof(InterCode));
  code->kind = kind;
  code->call.op = op;
  code->call.funcName = funcName;
  code->prev = NULL;
  code->next = NULL;
  return code;
}

/* 计算内存空间（Basic/结构体/数组） */
int getVarMemory(Type* type) {
  if (type->kind == T_STRUCT) { // 结构体
    TypeNode* structNode = type->structure.node;
    int mem = 0;
    while (structNode != NULL) {
      mem += getVarMemory(structNode->type);
      structNode = structNode->next;
    }
    return mem;
  } else if (type->kind == T_ARRAY) { // 数组
    return (type->array.length * getVarMemory(type->array.eleType));
  } else {
    return 4;
  }
}

/* 获取数组ID */
char* getArrayName(Node* expNode) {
  while (childrenMatch(expNode, 1, NTN_EXP)) {
    return getArrayName(getCertainChild(expNode, 1));
  }
  if (childrenMatch(expNode, 1, TN_ID)) {
    return getCertainChild(expNode, 1)->cval;
  } else {
    return NULL;
  }
}

/* 获取结构体某个域的位置 */
int getFieldPosInStruct(char* name, Type* type) {
  if (type->kind == T_STRUCT) {
    TypeNode* structNode = type->structure.node;
    int sum = 0;
    while (strcmp(structNode->name, name) != 0) {
      sum += getVarMemory(structNode->type);
      structNode = structNode->next;
    }
    return sum;
  } else {
    if (yyget_debug()) fprintf(stderr, "Var to Calculate Must be Structure\n");
    return 0;
  }
}

/* DEBUG: 打印IR符号表 */
void printIRVarList() {
  for (int i = 0; i < interVarNum; i++) {
    if (!interVarList[i].isNull) {
      printf("[\"%s\", ", interVarList[i].name);
      if (interVarList[i].isParam) printf("isParam");
      else printf("isVar");
      printf("] -> ");
    }
    else break;
  }
  printf("NULL\n");
}
