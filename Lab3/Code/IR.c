#include "IRUtils.h"

void generateIR(char* fileName) {
  if (!isError() && isLab(3)) {
      translateProgram();
      printInterCode(fileName);
  }
}

/* Program: 分析全部 */
void translateProgram() {
  // 初始化中间代码头
  IRList = createInterCodeNull();
  IRTail = IRList;
  // 初始化变量符号表
  interVarNum = getCertainNum(root, NTN_VARDEC);
  interVarList = (SymElem*)malloc(interVarNum * sizeof(SymElem));
  // 开始逐层分析
  translateExtDefList(getCertainChild(root, 1));
}

/* ExtDefList: 检查一系列全局变量、结构体或函数的定义 */
void translateExtDefList(Node* extDefListNode) {
  if (extDefListNode != NULL && extDefListNode->child != NULL) { // 还存在未处理的定义
    translateExtDef(getCertainChild(extDefListNode, 1));
    translateExtDefList(getCertainChild(extDefListNode, 2));
  }
}

/* ExtDef: 检查一个全局变量、结构体或函数的定义 */
void translateExtDef(Node* extDefNode) {
  if (childrenMatch(extDefNode, 2, NTN_EXTDECLIST)) {
    // Lab3中，禁止定义全局变量（任意类型全局变量(包括结构体变量)声明）
  } else if (childrenMatch(extDefNode, 2, TN_SEMI)) {
    // 结构体定义，这里对中间代码生成没有影响，不必理会
  } else if (childrenMatch(extDefNode, 3, NTN_COMPST)) {
    // 函数定义
    Node* funDecNode = getCertainChild(extDefNode, 2);
    Node* compStNode = getCertainChild(extDefNode, 3);
    translateFunDec(funDecNode); // 添加函数头中间代码
    translateCompSt(compStNode); // 添加函数体中间代码
  }
}

/* FunDec: 检查对一个函数头的定义 */
void translateFunDec(Node* funDecNode) {
  // 对每一个函数，参数从0算起
  paramCount = 0;
  Node* idNode = getCertainChild(funDecNode, 1);
  InterCode* funcCode = createInterCodeName(IR_FUNCTION, idNode->cval);
  addCodeToTail(funcCode); // 添加 FUNCTION 语句
  if (childrenMatch(funDecNode, 3, NTN_VARLIST)) { // 有参数
    Node* varListNode = getCertainChild(funDecNode, 3);
    translateVarList(varListNode); // 添加全部参数中间代码
  } // 无参数不处理
}

/* VarList: 检查函数的一个或多个形参 */
void translateVarList(Node* varListNode) {
  Node* paramDecNode = getCertainChild(varListNode, 1);
  translateParamDec(paramDecNode); // 添加单个参数中间代码
  if (paramDecNode->nextSibling != NULL) { // 后面还有形参
    translateVarList(getCertainChild(varListNode, 3)); // 添加其他参数中间代码
  }
}

/* ParamDec: 检查对一个形参的定义，为类型描述符+变量名 */
void translateParamDec(Node* paramDecNode) {
  paramCount += 1; // 当前函数参数个数累加1
  // 加入变量符号表
  TypeNode* paramTypeNode = handleParamDec(paramDecNode);
  addToVarList(paramTypeNode, interVarList, interVarNum);
  // 添加 PARAM 语句
  Node* varDecNode = getCertainChild(paramDecNode, 2);
  Operand* op = translateVarDec(varDecNode);
  InterCode* paramCode = createInterCodeOne(IR_PARAM, op);
  addCodeToTail(paramCode); // 添加 PARAM 语句
}

/* VarDec: 检查变量定义（基本/数组） */
Operand* translateVarDec(Node* varDecNode) {
  // 这里并不操作数组
  return createOperand(OP_VAR, varDecNode->cval);
}

/* CompSt: 检查一个由一对花括号括起来的语句块，其中全部局部变量的定义必须在全部语句之前 */
void translateCompSt(Node* compStNode) {

}
