#include "IRUtils.h"

/** 全部中间代码双向链表 */
InterCode* IRList = NULL;
/** 当前分析到的中间代码位置（总是指向最后一个不是NULL的中间代码） */
InterCode* IRTail = NULL;

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

/** 打印全部中间代码 */
void printInterCode(char* fileName) {
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
  while (IRList != NULL) {
    if (IRList->kind == IR_ASSIGN) {
      // 赋值
      printOperand(IRList->two.op1, file);
      fprintf(file, " := ");
      printOperand(IRList->two.op2, file);
      fprintf(file, "\n");
    } else if (IRList->kind == IR_ADD || IRList->kind == IR_SUB ||
               IRList->kind == IR_MUL || IRList->kind == IR_DIV) {
      // 加减乘除
      printOperand(IRList->three.op1, file);
      fprintf(file," := ");
      printOperand(IRList->three.op2, file);
      if (IRList->kind == IR_ADD) fprintf(file, " + ");
      else if (IRList->kind == IR_SUB) fprintf(file, " - ");
      else if (IRList->kind == IR_MUL) fprintf(file, " * ");
      else if (IRList->kind == IR_DIV) fprintf(file, " / ");
      printOperand(IRList->three.op3, file);
      fprintf(file, "\n");
    } else if (IRList->kind == IR_LABEL) {
      // Label
      fprintf(file, "LABEL %s :\n", IRList->name);
    } else if (IRList->kind == IR_FUNCTION) {
      // Function
      fprintf(file, "FUNCTION %s :\n", IRList->name);
    } else if (IRList->kind == IR_RETURN || IRList->kind == IR_ARG || IRList->kind == IR_PARAM ||
               IRList->kind == IR_READ || IRList->kind == IR_WRITE ) {
      // 关键字 + 变量名
      if (IRList->kind == IR_RETURN) fprintf(file, "RETURN ");
      else if (IRList->kind == IR_ARG) fprintf(file, "ARG ");
      else if (IRList->kind == IR_PARAM) fprintf(file, "PARAM ");
      else if (IRList->kind == IR_READ) fprintf(file, "READ ");
      else if (IRList->kind == IR_WRITE) fprintf(file, "WRITE ");
    	printOperand(IRList->one.op, file);
    } else if (IRList->kind == IR_CALL) {
      // Call 语句
      printOperand(IRList->call.op, file);
      fprintf(file, " := CALL ");
      fprintf(file, "%s\n", IRList->call.funcName);
    } else if (IRList->kind == IR_IF) {
      // IF 语句
      fprintf(file, "IF ");
      printOperand(IRList->ifcode.op1, file);
      switch(IRList->ifcode.relop){
        case EQ: fprintf(file, " == "); break;
        case NE: fprintf(file, " != "); break;
        case GE: fprintf(file, " >= "); break;
        case GT: fprintf(file, " > "); break;
        case LE: fprintf(file, " <= "); break;
        case LT: fprintf(file, " < "); break;
      }
      printOperand(IRList->ifcode.op2, file);
      fprintf(file, " GOTO %s\n", IRList->ifcode.label);
    } else if (IRList->kind == IR_GOTO) {
      // Goto 语句
      fprintf(file, "GOTO %s\n", IRList->name);
    } else if (IRList->kind == IR_DEC) {
      // 开辟空间
      fprintf(file, "DEC ");
      printOperand(IRList->two.op1, file);
      fprintf(file, " %d\n", IRList->two.op2->val);
    } else if (IRList->kind == IR_NULL) {
      // 空的，不输出
    } else {
      // 不应该出现
      fprintf(stderr, "Error in printInterCode(): Undefined Kind.\n");
    }
    IRList = IRList->next;
  }
  // 关闭文件
	if (fileName != NULL) fclose(file);
}

/** 打印一个操作数 */
void printOperand(Operand* op, FILE* file) {
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

/** 在当前位置尾部加一段中间代码，并置当前位置到新的尾部 */
void addCodeToTail(InterCode* newCode) {
  if (newCode == NULL) return;
  IRTail->next = newCode;
  newCode->prev = IRTail;
  while (newCode->next != NULL) {
    newCode = newCode->next;
  }
  IRTail = newCode; // 新的尾部
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

/** 创建一个空中间代码作为链表头 */
InterCode* createInterCodeNull() {
  InterCode* code = (InterCode*)malloc(sizeof(InterCode));
  code->kind = IR_NULL;
  code->prev = NULL;
  code->next = NULL;
  return code;
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
