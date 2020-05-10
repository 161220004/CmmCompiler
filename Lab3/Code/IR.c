#include "IRUtils.h"

void generateIR(char* fileName) {
  if (!isError() && isLab(3)) {
    if (root != NULL) {
      translateProgram();
      printInterCodes(fileName);
    }
  }
}

/* Program: 分析全部 */
void translateProgram() {
  // 初始化变量符号表
  interVarNum = getCertainNum(root, NTN_VARDEC);
  interVarList = (SymElem*)malloc(interVarNum * sizeof(SymElem));
  for (int i = 0; i < interVarNum; i++) { interVarList[i].isNull = true; }
  // 开始逐层分析
  IRList = translateExtDefList(getCertainChild(root, 1), NULL);
}

/* ExtDefList: 检查一系列全局变量、结构体或函数的定义 */
InterCode* translateExtDefList(Node* extDefListNode, InterCode* tail) {
  if (extDefListNode->child == NULL) { // 没了
    return getInterCodeHead(tail);
  } else { // 还存在未处理的定义
    InterCode* extDefCode = translateExtDef(getCertainChild(extDefListNode, 1));
    InterCode* newTail = addCodeToTail(extDefCode, tail); // 添加函数中间代码
    return translateExtDefList(getCertainChild(extDefListNode, 2), newTail);
  }
}

/* ExtDef: 检查一个全局变量、结构体或函数的定义，返回一整段中间代码的头部 */
InterCode* translateExtDef(Node* extDefNode) {
  if (childrenMatch(extDefNode, 3, NTN_COMPST)) {
    // 函数定义
    Node* funDecNode = getCertainChild(extDefNode, 2);
    InterCode* funcDecCode = translateFunDec(funDecNode);
    Node* compStNode = getCertainChild(extDefNode, 3);
    InterCode* compStCode = translateCompSt(compStNode);
    return linkInterCodeHeadToHead(funcDecCode, compStCode);
  } else { // Lab3中，禁止定义全局变量（任意类型全局变量(包括结构体变量)声明）
    // 结构体定义对中间代码生成没有影响，不必理会
    return NULL;
  }
}

/* FunDec: 检查对一个函数头的定义，返回一整段中间代码的头部 */
InterCode* translateFunDec(Node* funDecNode) {
  // 对每一个函数，参数从0算起
  paramCount = 0;
  Node* idNode = getCertainChild(funDecNode, 1);
  InterCode* funcIDCode = createInterCodeName(IR_FUNCTION, idNode->cval);
  InterCode* varListCode = NULL;
  if (childrenMatch(funDecNode, 3, NTN_VARLIST)) { // 有参数
    Node* varListNode = getCertainChild(funDecNode, 3);
    varListCode = translateVarList(varListNode, NULL); // 全部参数中间代码头部
  } // 无参数不处理
  return linkInterCodeHeadToHead(funcIDCode, varListCode);
}

/* VarList: 检查函数的一个或多个形参，返回全部参数代码的头部 */
InterCode* translateVarList(Node* varListNode, InterCode* tail) {
  Node* paramDecNode = getCertainChild(varListNode, 1);
  InterCode* paramDecCode = translateParamDec(paramDecNode); // 单个参数中间代码头部
  InterCode* newTail = addCodeToTail(paramDecCode, tail); // 添加到参数代码tail
  if (paramDecNode->nextSibling == NULL) { // 最后一个
    return getInterCodeHead(newTail);
  } else { // 后面还有形参
    return translateVarList(getCertainChild(varListNode, 3), newTail); // 其他参数中间代码头部
  }
}

/* ParamDec: 检查对一个形参的定义，返回该参数代码的头部 */
InterCode* translateParamDec(Node* paramDecNode) {
  paramCount += 1; // 当前函数参数个数累加1
  // 加入变量符号表
  TypeNode* paramTypeNode = handleParamDec(paramDecNode);
  addToVarList(paramTypeNode, interVarList, interVarNum);
  // 返回 PARAM 语句
  Node* varDecNode = getCertainChild(paramDecNode, 2);
  Operand* op = translateVarDec(varDecNode);
  return createInterCodeOne(IR_PARAM, op);
}

/* VarDec: 检查变量定义（基本/数组） */
Operand* translateVarDec(Node* varDecNode) {
  // 这里并不操作数组
  return createOperand(OP_VAR, getVarDecName(varDecNode));
}

/* CompSt: 检查一个由一对花括号括起来的语句块，返回该语句块代码的头部 */
InterCode* translateCompSt(Node* compStNode) {
  InterCode* defListCode = translateDefList(getCertainChild(compStNode, 2), NULL);
  InterCode* stmtListCode = translateStmtList(getCertainChild(compStNode, 3), NULL);
  return linkInterCodeHeadToHead(defListCode, stmtListCode);
}

/* DefList: 检查一连串定义（可能为空），返回一连串定义代码的头部 */
InterCode* translateDefList(Node* defListNode, InterCode* tail) {
  if (defListNode->child == NULL) { // 没了
    return getInterCodeHead(tail);
  } else { // 还有
    InterCode* defCode = translateDef(getCertainChild(defListNode, 1)); // 一条定义代码的头部
    InterCode* newTail = addCodeToTail(defCode, tail);
    return translateDefList(getCertainChild(defListNode, 2), newTail);
  }
}

/* Def: 检查一条局部定义，返回一条定义代码的头部 */
InterCode* translateDef(Node* defNode) {
  // 获取一串变量的类型
  Node* specNode = getCertainChild(defNode, 1);
  Type* defType = handleSpecifier(specNode, false);
  // 交给下层处理
  return translateDecList(getCertainChild(defNode, 2), defType, NULL);
}

/* DecList: 检查每一组逗号分割的变量名，只有结构体/数组类型才有中间代码，返回一连串定义代码的头部 */
InterCode* translateDecList(Node* decListNode, Type* defType, InterCode* tail) {
  Node* decNode = getCertainChild(decListNode, 1);
  InterCode* decCode = translateDec(decNode, defType); // 一条定义代码的头部
  InterCode* newTail = addCodeToTail(decCode, tail); // 添加到定义代码tail
  if (decNode->nextSibling == NULL) { // 最后一个
    return getInterCodeHead(newTail);
  } else { // 还有
    return translateDecList(getCertainChild(decListNode, 3), defType, newTail);
  }
}

/* Dec: 检查一个变量的声明（初始化分情况），仅为结构体/数组开辟空间，返回一条定义代码的头部 */
InterCode* translateDec(Node* decNode, Type* defType) {
  // 加入变量符号表
  TypeNode* decTypeNode = handleDec(decNode, defType, false);
  addToVarList(decTypeNode, interVarList, interVarNum);
  Type* varType = decTypeNode->type; // 变量类型
  // 仅处理结构体/数组
  InterCode* decCode = NULL;
  Node* varDecNode = getCertainChild(decNode, 1);
  if (varType->kind == T_STRUCT || varType->kind == T_ARRAY) {
    Operand* op1 = translateVarDec(varDecNode);
    Operand* op2 = createConst(getVarMemory(varType));
    decCode = createInterCodeTwo(IR_DEC, op1, op2); // 有 DEC 语句
  }
  InterCode* expCode = NULL;
  if (varDecNode->nextSibling != NULL) { // 有初始化语句
    // 创建临时变量以获取表达式的值
    Operand* opTmp = newTemp();
    expCode = translateExp(getCertainChild(decNode, 3), opTmp); // 已经包含代码：表达式的值赋值给该变量
  }
  return linkInterCodeHeadToHead(decCode, expCode);
}

/* StmtList: 检查零个或多个Stmt的组合，返回全部中间代码的头部 */
InterCode* translateStmtList(Node* stmtListNode, InterCode* tail) {
  if (stmtListNode->child == NULL) { // 没了
    return getInterCodeHead(tail);
  } else { // 还有
    InterCode* stmtCode = translateStmt(getCertainChild(stmtListNode, 1)); // 语句代码的头部
    InterCode* newTail = addCodeToTail(stmtCode, tail);
    return translateStmtList(getCertainChild(stmtListNode, 2), newTail);
  }
}

/* Stmt: 检查一条语句，返回一条语句中间代码的头部 */
InterCode* translateStmt(Node* stmtNode) {
  if (childrenMatch(stmtNode, 1, NTN_EXP)) { // 普通语句
    return translateExp(getCertainChild(stmtNode, 1), NULL);
  } else if (childrenMatch(stmtNode, 1, NTN_COMPST)) { // 新的语句块
    return translateCompSt(getCertainChild(stmtNode, 1));
  } else if (childrenMatch(stmtNode, 1, TN_RETURN)) { // RETURN语句
    // 新建return的临时变量
    Operand* opTmp = newTemp();
    InterCode* expCode = translateExp(getCertainChild(stmtNode, 2), opTmp); // 表达式代码的头部
    InterCode* returnCode = createInterCodeOne(IR_RETURN, opTmp);
    return linkInterCodeHeadToHead(expCode, returnCode);
  } else { // IF-ELSE条件语句 / WHILE循环语句
    InterCode* ifExpCode = NULL; // 条件部分的全部代码的头部
    // 如果是 while语句，开始前给一个Label
    char* whileLabel = NULL;
    if (childrenMatch(stmtNode, 1, TN_WHILE)) {
      whileLabel = newLabel();
      ifExpCode = createInterCodeName(IR_LABEL, whileLabel);
    }
    // 新建 if (true) 对应的 Label 名字
    char* ifLabel = newLabel();
    // 分别获取条件表达式左侧和右侧代码的头部，再与IF代码连接起来
    Node* expNode = getCertainChild(stmtNode, 3);
    Node* relopNode = getCertainChild(expNode, 2);
    if (relopNode->name == TN_EQ || relopNode->name == TN_NE || relopNode->name == TN_LE ||
        relopNode->name == TN_GE || relopNode->name == TN_LT || relopNode->name == TN_GT) { // op1 [relop] op2
      // 新建条件表达式的两个临时变量（Lab3默认Exp表达式必然是“x [relop] y”格式）
      Operand* opTmp1 = newTemp();
      Operand* opTmp2 = newTemp();
      // 分别获取条件表达式左侧和右侧代码的头部：expCode1, expCode2
      InterCode* expCode1 = translateExp(getCertainChild(expNode, 1), opTmp1);
      InterCode* expCode2 = translateExp(getCertainChild(expNode, 3), opTmp2);
      Relop relop;
      switch (relopNode->name) {
        case TN_EQ: relop = EQ; break;
        case TN_NE: relop = NE; break;
        case TN_LE: relop = LE; break;
        case TN_GE: relop = GE; break;
        case TN_LT: relop = LT; break;
        case TN_GT: relop = GT; break;
        default: relop = EQ;
      }
      InterCode* ifCondCode = createInterCodeIf(opTmp1, relop, opTmp2, ifLabel);
      // 连接：ifExpCode + expCode1 + expCode2 + ifCondCode -> ifExpCode
      InterCode* expCode = linkInterCodeHeadToHead(expCode1, expCode2);
      expCode = linkInterCodeHeadToHead(expCode, ifCondCode);
      ifExpCode = linkInterCodeHeadToHead(ifExpCode, expCode);
    } else {
      if (yyget_debug()) fprintf(stderr, "Only Support IF Condition as \"x [relop] y\".\n");
      ifExpCode = createInterCodeIf(NULL, EQ, NULL, ifLabel);
    }
    // 新建整个IFELSE/WHILE语句结束后面的 Label
    char* endLabel = newLabel();
    InterCode* endLabelCode = createInterCodeName(IR_LABEL, endLabel);
    // 获取 if (true) 对应的 Stmt 内容
    InterCode* ifStmtLabCode = createInterCodeName(IR_LABEL, ifLabel);
    InterCode* ifStmtBodyCode = translateStmt(getCertainChild(stmtNode, 5));
    InterCode* ifStmtGotoCode = NULL;
    if (childrenMatch(stmtNode, 1, TN_WHILE)) { // WHILE
      ifStmtGotoCode = createInterCodeName(IR_GOTO, whileLabel);
    } else { // IF / IF-ELSE
      ifStmtGotoCode = createInterCodeName(IR_GOTO, endLabel);
    }
    // 连接：ifStmtLabCode + ifStmtBodyCode + ifStmtGotoCode -> ifStmtCode
    InterCode* ifStmtCode = linkInterCodeHeadToHead(ifStmtBodyCode, ifStmtGotoCode);
    ifStmtCode = linkInterCodeHeadToHead(ifStmtLabCode, ifStmtCode);
    // 如果有，处理IF-ELSE还有的 Else 语句，Goto 地址是 elseLabel；如果没有，Goto 地址是 endLabel
    InterCode* elseGotoCode = NULL;
    InterCode* elseStmtCode = NULL;
    if (childrenMatch(stmtNode, 6, TN_ELSE)) {
      // 新建 ELSE 语句对应的的 Label
      char* elseLabel = newLabel();
      elseGotoCode = createInterCodeName(IR_GOTO, elseLabel);
      InterCode* elseStmtLabCode = createInterCodeName(IR_LABEL, elseLabel);
      InterCode* elseStmtBodyCode = translateStmt(getCertainChild(stmtNode, 7));
      InterCode* elseStmtGotoCode = createInterCodeName(IR_GOTO, endLabel);
      // 连接：elseStmtLabCode + elseStmtBodyCode + elseStmtGotoCode -> elseStmtCode
      elseStmtCode = linkInterCodeHeadToHead(elseStmtBodyCode, elseStmtGotoCode);
      elseStmtCode = linkInterCodeHeadToHead(elseStmtLabCode, elseStmtCode);
    } else {
      elseGotoCode = createInterCodeName(IR_GOTO, endLabel);
    }
    // 连接：ifExpCode + elseGotoCode + ifStmtCode + elseStmtCode + endLabelCode
    InterCode* ifAllCode = linkInterCodeHeadToHead(elseStmtCode, endLabelCode);
    ifAllCode = linkInterCodeHeadToHead(ifStmtCode, ifAllCode);
    ifAllCode = linkInterCodeHeadToHead(elseGotoCode, ifAllCode);
    return linkInterCodeHeadToHead(ifExpCode, ifAllCode);
  }
}

/* Exp: 检查条件表达式，返回条件表达式代码的头部 */
InterCode* translateCond(Node* expNode, InterCode* trueLab, InterCode* falseLab) {
  if (childrenMatch(expNode, 1, TN_NOT)) {
    return translateCond(getCertainChild(expNode, 2), falseLab, trueLab);
  } else if (childrenMatch(expNode, 1, NTN_EXP)) {
    Node* opNode = getCertainChild(expNode, 2);
    if (opNode->name == TN_AND) {
      // 新的Label
      InterCode* newLab = createInterCodeName(IR_LABEL, newLabel());
      InterCode* code1 = translateCond(getCertainChild(expNode, 1), newLab, falseLab);
      InterCode* code2 = translateCond(getCertainChild(expNode, 3), trueLab, falseLab);
      InterCode* code = linkInterCodeHeadToHead(newLab, code2);
      return linkInterCodeHeadToHead(code1, code);
    } else if (opNode->name == TN_OR) {
      // 新的Label
      InterCode* newLab = createInterCodeName(IR_LABEL, newLabel());
      InterCode* code1 = translateCond(getCertainChild(expNode, 1), trueLab, newLab);
      InterCode* code2 = translateCond(getCertainChild(expNode, 3), trueLab, falseLab);
      InterCode* code = linkInterCodeHeadToHead(newLab, code2);
      return linkInterCodeHeadToHead(code1, code);
    } else if (isRelop(opNode->name)) {
      Relop relop = getExpRelop(opNode->name);
      // 两个新的临时变量
      Operand* tmpOp1 = newTemp();
      Operand* tmpOp2 = newTemp();
      InterCode* code1 = translateExp(getCertainChild(expNode, 1), tmpOp1);
      InterCode* code2 = translateExp(getCertainChild(expNode, 3), tmpOp2);
      InterCode* code3 = createInterCodeIf(tmpOp1, relop, tmpOp2, trueLab->name);
      InterCode* code4 = createInterCodeName(IR_GOTO, falseLab->name);
      InterCode* code = linkInterCodeHeadToHead(code3, code4);
      code = linkInterCodeHeadToHead(code2, code);
      return linkInterCodeHeadToHead(code1, code);
    }
  }
  // 不是上述情况，条件变成：Exp > #0
  Operand* tmpOp = newTemp();
  InterCode* code1 = translateExp(expNode, tmpOp);
  Operand* zeroOp = createConst(0);
  InterCode* code2 = createInterCodeIf(tmpOp, NE, zeroOp, trueLab->name);
  InterCode* code3 = createInterCodeName(IR_GOTO, falseLab->name);
  InterCode* code = linkInterCodeHeadToHead(code2, code3);
  return linkInterCodeHeadToHead(code1, code);
}

/* Exp: 检查表达式，返回一个表达式中间代码的头部 */
InterCode* translateExp(Node* expNode, Operand* place) {
  // 先检查是不是条件语句
  if (isCondition(expNode)) { // 条件
    // 新的标签
    InterCode* newLab1 = createInterCodeName(IR_LABEL, newLabel());
    InterCode* newLab2 = createInterCodeName(IR_LABEL, newLabel());
    // 如果条件表达式为真，那么为place赋值1；否则，为其赋值0
    Operand* zeroOp = createConst(0);
    InterCode* code1 = createInterCodeTwo(IR_ASSIGN, place, zeroOp);
    InterCode* code2 = translateCond(expNode, newLab1, newLab2);
    Operand* oneOp = createConst(1);
    InterCode* code3 = createInterCodeTwo(IR_ASSIGN, place, oneOp);
    InterCode* code = linkInterCodeHeadToHead(code3, newLab2);
    code = linkInterCodeHeadToHead(newLab1, code);
    code = linkInterCodeHeadToHead(code2, code);
    return linkInterCodeHeadToHead(code1, code);
  }
  // 返回的最后一条代码一定是赋值语句：place := Exp
  if (childrenMatch(expNode, 1, TN_LP)) { // 括号
    return translateExp(getCertainChild(expNode, 2), place);
  } else if (childrenMatch(expNode, 1, TN_INT)) { // int
    Operand* intOp = createConst(getCertainChild(expNode, 1)->ival);
    return createInterCodeTwo(IR_ASSIGN, place, intOp);
  } else if (childrenMatch(expNode, 1, TN_FLOAT)) { // float
    fprintf(stderr, "Not Support Float in Lab3\n");
    return NULL;
  } else if (childrenMatch(expNode, 1, TN_ID)) {
    Node* idNode = getCertainChild(expNode, 1);
    if (idNode->nextSibling == NULL) { // ID
      Operand* varOp = lookUpVar(idNode->cval);
      return createInterCodeTwo(IR_ASSIGN, place, varOp);
    } else { // 函数调用
      Operand* funcOp = lookUpFunc(idNode->cval);
      if (childrenMatch(expNode, 3, TN_RP)) { // 无参数
        if (strcmp(funcOp->name, "read") == 0) {
          return createInterCodeOne(IR_READ, funcOp);
        } else {
          return createInterCodeCall(IR_CALL, place, funcOp->name);
        }
      } else { // 有参数
        InterCode* argsCode = createInterCodeOne(IR_ARG, NULL); // 第一个是空节点
        InterCode* argsExpCode = translateArgs(getCertainChild(expNode, 3), NULL, argsCode);
        if (strcmp(funcOp->name, "write") == 0) {
          return linkInterCodeHeadToHead(argsExpCode, createInterCodeOne(IR_WRITE, funcOp));
        } else {
          argsCode = linkInterCodeHeadToHead(argsExpCode, argsCode);
          InterCode* callCode = createInterCodeCall(IR_CALL, place, funcOp->name);
          return linkInterCodeHeadToHead(argsCode, callCode);
        }
      }
    }
  } else if (childrenMatch(expNode, 1, TN_MINUS)) { // 取负（算术运算）
    // 新的临时变量
    Operand* tmpOp = newTemp();
    InterCode* code1 = translateExp(getCertainChild(expNode, 2), tmpOp);
    Operand* zeroOp = createConst(0);
    InterCode* code2 = createInterCodeThree(IR_SUB, place, zeroOp, tmpOp);
    return linkInterCodeHeadToHead(code1, code2);
  } else { // if (childrenMatch(expNode, 1, NTN_EXP)) {
    Node* opNode = getCertainChild(expNode, 2);
    if (opNode->name == TN_ASSIGNOP) { // 赋值
      // Exp1只能是三种情况之一（单个变量访问、数组元素访问或结构体特定域的访问）
      Node* expNode1 = getCertainChild(expNode, 1);
      if (childrenMatch(expNode1, 1, TN_ID)) { // 单个变量访问

      } else { // 数组元素访问或结构体特定域的访问

      }
    } else if (opNode->name == TN_PLUS || opNode->name == TN_MINUS ||
               opNode->name == TN_STAR || opNode->name == TN_DIV) { // 加减乘除
      // 新的临时变量
      Operand* tmpOp1 = newTemp();
      Operand* tmpOp2 = newTemp();
      InterCode* code1 = translateExp(getCertainChild(expNode, 1), tmpOp1);
      InterCode* code2 = translateExp(getCertainChild(expNode, 3), tmpOp2);
      InterCode* code3 = NULL;
      switch (opNode->name) {
        case TN_PLUS: code3 = createInterCodeThree(IR_ADD, place, tmpOp1, tmpOp2); break;
        case TN_MINUS: code3 = createInterCodeThree(IR_SUB, place, tmpOp1, tmpOp2); break;
        case TN_STAR: code3 = createInterCodeThree(IR_MUL, place, tmpOp1, tmpOp2); break;
        case TN_DIV: code3 = createInterCodeThree(IR_DIV, place, tmpOp1, tmpOp2); break;
        default: break;
      }
      InterCode* code = linkInterCodeHeadToHead(code2, code3);
      return linkInterCodeHeadToHead(code1, code);
    } else if (opNode->name == TN_DOT) {  // 结构体访问

    } else if (opNode->name == TN_LB) {  // 数组访问

    }
  }
}

/* Args: 检查实参列表，用于函数调用表达式，每个实参都可以变成一个表达式Exp；返回实参代码的头部 */
InterCode* translateArgs(Node* argsNode, InterCode* tail, InterCode* argsCode) {
  Node* expNode = getCertainChild(argsNode, 1);
  // 临时变量
  Operand* opTmp = newTemp();
  InterCode* expCode = translateExp(expNode, opTmp);
  InterCode* newTail = addCodeToTail(expCode, tail);
  // ARG语句连接上argsCode
  argsCode = linkInterCodeHeadToHead(createInterCodeOne(IR_ARG, opTmp), argsCode);
  if (expNode->nextSibling == NULL) { // 最后一个
    return getInterCodeHead(newTail);
  } else { // 接下来还有
    return translateArgs(getCertainChild(argsNode, 3), newTail, argsCode);
  }
}
