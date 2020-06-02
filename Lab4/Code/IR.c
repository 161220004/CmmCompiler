#include "IRUtils.h"

/* 生成中间代码 */
void generateIR(char* fileName) {
  if (!isError() && (isLab(3) || isLab(4))) {
    if (root != NULL) {
      translateProgram();
      if (isLab(3)) printInterCodes(fileName);
    }
  }
}

/* Program: 分析全部 */
void translateProgram() {
  // 初始化变量符号表
  interVarNum = getCertainNum(root, NTN_VARDEC);
  interVarList = (SymElem*)malloc(interVarNum * sizeof(SymElem));
  for (int i = 0; i < interVarNum; i++) {
    interVarList[i].isNull = true;
    interVarList[i].isParam = false;
  }
  // 创建全局作用域
  createGlobalField(interVarNum, interVarList);
  // 开始逐层分析
  IRList = translateExtDefList(getCertainChild(root, 1), NULL);

  if (yyget_debug()) printIRVarList();
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
  // 设置为参数模式（因为数组/结构体参数，特别地，为地址）
  setVarToParam(paramTypeNode->name);
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
    Node* expNode2 = getCertainChild(decNode, 3);
    // 右侧的值赋给一个新的临时变量
    Operand* rightOp = NULL;
    if (isPureID(expNode2)) { // 右值是单个变量
      rightOp = createOperand(OP_VAR, getPureID(expNode2));
    } else if (isPureInt(expNode2)) { // 右值是int
      rightOp = createConst(getPureInt(expNode2));
    } else { // 都不是时才使用临时变量
      rightOp = newTemp();
      expCode = translateExp(expNode2, rightOp);
    }
    Operand* varOp = createOperand(OP_VAR, decTypeNode->name);
    InterCode* assignCode = createInterCodeTwo(IR_ASSIGN, varOp, rightOp);
    expCode = linkInterCodeHeadToHead(expCode, assignCode);
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
    Node* expNode = getCertainChild(stmtNode, 1);
    if (childrenMatch(expNode, 2, TN_LP)) { //函数调用
      Operand* resOp = newTemp();
      return translateExp(expNode, resOp);
    } else {
      return translateExp(expNode, NULL);
    }
  } else if (childrenMatch(stmtNode, 1, NTN_COMPST)) { // 新的语句块
    return translateCompSt(getCertainChild(stmtNode, 1));
  } else if (childrenMatch(stmtNode, 1, TN_RETURN)) { // RETURN语句
    // 新建return的临时变量
    Operand* opTmp = newTemp();
    InterCode* expCode = translateExp(getCertainChild(stmtNode, 2), opTmp); // 表达式代码的头部
    InterCode* returnCode = createInterCodeOne(IR_RETURN, opTmp);
    return linkInterCodeHeadToHead(expCode, returnCode);
  } else if (childrenMatch(stmtNode, 1, TN_IF)) { // IF-ELSE条件语句
    Node* expNode = getCertainChild(stmtNode, 3);
    char* newLab1 = newLabel();
    char* newLab2 = newLabel();
    InterCode* trueLab = createInterCodeName(IR_LABEL, newLab1);
    InterCode* falseLab = createInterCodeName(IR_LABEL, newLab2);
    InterCode* code1 = translateCond(expNode, trueLab, falseLab);
    InterCode* code2 = translateStmt(getCertainChild(stmtNode, 5));
    if (childrenMatch(stmtNode, 6, TN_ELSE)) { // 有 ELSE
      char* newLab3 = newLabel();
      InterCode* endLab = createInterCodeName(IR_LABEL, newLab3);
      InterCode* code3 = translateStmt(getCertainChild(stmtNode, 7));
      InterCode* code = linkInterCodeHeadToHead(code3, endLab);
      code = linkInterCodeHeadToHead(falseLab, code);
      code = linkInterCodeHeadToHead(createInterCodeName(IR_GOTO, newLab3), code);
      code = linkInterCodeHeadToHead(code2, code);
      code = linkInterCodeHeadToHead(trueLab, code);
      return linkInterCodeHeadToHead(code1, code);
    } else { // 无ELSE
      InterCode* code = linkInterCodeHeadToHead(code2, falseLab);
      code = linkInterCodeHeadToHead(trueLab, code);
      return linkInterCodeHeadToHead(code1, code);
    }
  } else { // WHILE循环语句
    Node* expNode = getCertainChild(stmtNode, 3);
    char* newLab1 = newLabel();
    char* newLab2 = newLabel();
    char* newLab3 = newLabel();
    InterCode* startLab = createInterCodeName(IR_LABEL, newLab1);
    InterCode* trueLab = createInterCodeName(IR_LABEL, newLab2);
    InterCode* falseLab = createInterCodeName(IR_LABEL, newLab3);
    InterCode* code1 = translateCond(expNode, trueLab, falseLab);
    InterCode* code2 = translateStmt(getCertainChild(stmtNode, 5));
    InterCode* code = linkInterCodeHeadToHead(createInterCodeName(IR_GOTO, newLab1), falseLab);
    code = linkInterCodeHeadToHead(code2, code);
    code = linkInterCodeHeadToHead(trueLab, code);
    code = linkInterCodeHeadToHead(code1, code);
    return linkInterCodeHeadToHead(startLab, code);
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
      if (childrenMatch(expNode, 3, NTN_ARGS)) { // 有参数
        InterCode* argsCode = createInterCodeOne(IR_ARG, NULL); // 第一个是空节点
        InterCode* argsExpCode = translateArgs(getCertainChild(expNode, 3), NULL, argsCode);
        // 此刻argsCode依然指向尾部的空节点，我们把空节点删除，然后把它指向头节点
        InterCode* argsCodeDel = argsCode;
        argsCode = argsCodeDel->prev;
        if (argsCode == NULL && yyget_debug()) {
          fprintf(stderr, "Failed to Receive Args.\n");
        } else {
          argsCodeDel->prev = NULL;
          argsCode->next = NULL;
        }
        argsCode = getInterCodeHead(argsCode);
        if (strcmp(funcOp->name, "write") == 0) {
          hasWrite = true;
          return linkInterCodeHeadToHead(argsExpCode, createInterCodeOne(IR_WRITE, argsCode->one.op));
        } else {
          argsCode = linkInterCodeHeadToHead(argsExpCode, argsCode);
          InterCode* callCode = createInterCodeCall(IR_CALL, place, funcOp->name);
          return linkInterCodeHeadToHead(argsCode, callCode);
        }
      } else { // 无参数
        if (strcmp(funcOp->name, "read") == 0) {
          hasRead = true;
          return createInterCodeOne(IR_READ, place);
        } else {
          return createInterCodeCall(IR_CALL, place, funcOp->name);
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
      Node* expNode1 = getCertainChild(expNode, 1); // 左值
      Node* expNode2 = getCertainChild(expNode, 3); // 右值
      // 右侧的值赋给一个新的临时变量
      Operand* rightOp = NULL;
      InterCode* rightCode = NULL;
      if (isPureID(expNode2)) { // 右值是单个变量
        rightOp = createOperand(OP_VAR, getPureID(expNode2));
      } else if (isPureInt(expNode2)) { // 右值是int
        rightOp = createConst(getPureInt(expNode2));
      } else { // 都不是时才使用临时变量
        rightOp = newTemp();
        rightCode = translateExp(expNode2, rightOp);
      }
      if (childrenMatch(expNode1, 1, TN_ID)) { // 左值是单个变量访问
        Operand* varOp = lookUpVar(getCertainChild(expNode1, 1)->cval);
        InterCode* assignCode = createInterCodeTwo(IR_ASSIGN, varOp, rightOp);
        if (place != NULL) {
          InterCode* placeCode = createInterCodeTwo(IR_ASSIGN, place, varOp);
          assignCode = linkInterCodeHeadToHead(assignCode, placeCode);
        }
        return linkInterCodeHeadToHead(rightCode, assignCode);
      } else if (childrenMatch(expNode1, 2, TN_LB)) { // 左值是数组元素访问
        char* arrayName = getArrayName(expNode1);
        if (arrayName == NULL) {
          if (yyget_debug()) fprintf(stderr, "Array Name is Not Valid.\n");
          return rightCode;
        }
        // 查符号表，得到数组类型
        Type* arrayType = lookUpArrayType(arrayName);
        // 新的临时变量：取到数组“[]”内的字节处的地址
        Operand* addrOp = newTemp();
        InterCode* getAddrCode = translateArrayAddr(expNode1, arrayName, arrayType, NULL, addrOp);
        // 右值赋给数组内容
        Operand* getContOp = createOperand(OP_GETCONT, addrOp->name);
        InterCode* setContCode = createInterCodeTwo(IR_ASSIGN, getContOp, rightOp);
        // 连接：rightCode + getAddrCode + setContCode ( + placeCode)
        InterCode* code = linkInterCodeHeadToHead(getAddrCode, setContCode);
        if (place != NULL) {
          InterCode* placeCode = createInterCodeTwo(IR_ASSIGN, place, rightOp);
          code = linkInterCodeHeadToHead(code, placeCode);
        }
        return linkInterCodeHeadToHead(rightCode, code);
      } else if (childrenMatch(expNode1, 2, TN_DOT)) { // 左值是结构体特定域的访问
        // 新的临时变量：取到结构体访问的字节处的地址
        Operand* addrOp = newTemp();
        InterCode* getAddrCode = translateStructAddr(expNode1, addrOp);
        Operand* getContOp = createOperand(OP_GETCONT, addrOp->name);
        InterCode* setContCode = createInterCodeTwo(IR_ASSIGN, getContOp, rightOp);
        // 连接：rightCode + getAddrCode + setContCode ( + placeCode)
        InterCode* code = linkInterCodeHeadToHead(getAddrCode, setContCode);
        if (place != NULL) {
          InterCode* placeCode = createInterCodeTwo(IR_ASSIGN, place, rightOp);
          code = linkInterCodeHeadToHead(code, placeCode);
        }
        return linkInterCodeHeadToHead(rightCode, code);
      } else {
        if (yyget_debug()) fprintf(stderr, "Not Support This as Left Value in Assign.");
        return rightCode;
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
      // 新的临时变量：取到结构体访问的字节处的地址
      Operand* addrOp = newTemp();
      InterCode* getAddrCode = translateStructAddr(expNode, addrOp);
      Operand* getContOp = createOperand(OP_GETCONT, addrOp->name);
      InterCode* getContCode = createInterCodeTwo(IR_ASSIGN, place, getContOp);
      return linkInterCodeHeadToHead(getAddrCode, getContCode);
    } else if (opNode->name == TN_LB) {  // 数组访问
      char* arrayName = getArrayName(expNode);
      if (arrayName == NULL) {
        if (yyget_debug()) fprintf(stderr, "Array Name is Not Valid.\n");
        return NULL;
      }
      // 查符号表，得到数组类型
      Type* arrayType = lookUpArrayType(arrayName);
      // 新的临时变量：取到数组“[]”内的字节处的地址
      Operand* addrOp = newTemp();
      InterCode* getAddrCode = translateArrayAddr(expNode, arrayName, arrayType, NULL, addrOp);
      // 取到该地址内的值，放入place
      Operand* getContOp = createOperand(OP_GETCONT, addrOp->name);
      InterCode* getContCode = createInterCodeTwo(IR_ASSIGN, place, getContOp);
      return linkInterCodeHeadToHead(getAddrCode, getContCode);
    } else {
      if (yyget_debug()) fprintf(stderr, "Should Not Enter Here.\n");
      return NULL;
    }
  }
}

/** Exp: 数组访问的位置代码，保证参数 Exp 是 Array */
InterCode* translateArrayAddr(Node* expNode, char* arrayName, Type* type, Operand* inhOp, Operand* place) {
  if (childrenMatch(expNode, 2, TN_LB)) { // 数组还有层数
    Node* arrayNode = getCertainChild(expNode, 1);
    // 先降级Type
    type = type->array.eleType;
    // 获取“[]”内的值
    Operand* elePosOp = newTemp();
    InterCode* elePosCode = translateExp(getCertainChild(expNode, 3), elePosOp);
    // 计算对应的字节位置
    Operand* bytePosOp = newTemp();
    Operand* sizeOp = createConst(getVarMemory(type));
    InterCode* bytePosCode = createInterCodeThree(IR_MUL, bytePosOp, elePosOp, sizeOp);
    // 连接：现在得到当前层的字节位置，接下来根据inhOp进行累加
    InterCode* arrayCode = linkInterCodeHeadToHead(elePosCode, bytePosCode);
    Operand* nextInhOp = bytePosOp; // 传给下一个的inhOp
    if (inhOp != NULL) { // 不是第一个，累加
      Operand* sumOp = newTemp();
      InterCode* sumCode = createInterCodeThree(IR_ADD, sumOp, bytePosOp, inhOp);
      arrayCode = linkInterCodeHeadToHead(arrayCode, sumCode);
      nextInhOp = sumOp;
    }
    // 现在nextInhOp为全部已知层叠加后的字节位置
    if (childrenMatch(arrayNode, 1, TN_ID)) { // 最后一层
      // 取到当前累加的字节位置的地址，放入place
      Operand* addrOp = newTemp();
      Operand* getAddrOp = NULL;
      if (varIsParam(arrayName)) { // 参数本身就是地址，不需要取址
        getAddrOp = createOperand(OP_VAR, arrayName);
      } else { // 变量需要额外取址
        getAddrOp = createOperand(OP_GETADDR, arrayName);
      }
      InterCode* getAddrCode = createInterCodeThree(IR_ADD, place, getAddrOp, nextInhOp);
      return linkInterCodeHeadToHead(arrayCode, getAddrCode);
    } else { // 还有好多层
      // 计算下一层的结果
      InterCode* lowerCode = translateArrayAddr(arrayNode, arrayName, type, nextInhOp, place);
      // 连接本层和下一层的代码
      return linkInterCodeHeadToHead(arrayCode, lowerCode);
    }
  } else {
    if (yyget_debug()) fprintf(stderr, "Only Translate Array Here.\n");
    return NULL;
  }
}

/** Exp: 结构体访问的位置代码，保证参数 Exp 是 Structure */
InterCode* translateStructAddr(Node* expNode, Operand* place) {
  Node* structID = getCertainChild(getCertainChild(expNode, 1), 1);
  if (structID->name != TN_ID || structID->nextSibling != NULL) {
    if (yyget_debug()) fprintf(stderr, "Structure to Access is Not Valid.\n");
    return NULL;
  }
  Type* structType = lookUpStructType(structID->cval);
  int fieldPos = getFieldPosInStruct(getCertainChild(expNode, 3)->cval, structType);
  // 结构体域地址放入place
  Operand* getAddrOp = NULL;
  if (varIsParam(structID->cval)) { // 参数本身就是地址，不需要取址
    getAddrOp = createOperand(OP_VAR, structID->cval);
  } else { // 变量需要额外取址
    getAddrOp = createOperand(OP_GETADDR, structID->cval);
  }
  if (fieldPos == 0) {
    return createInterCodeTwo(IR_ASSIGN, place, getAddrOp);
  } else {
    Operand* biaOp = createConst(fieldPos);
    return createInterCodeThree(IR_ADD, place, getAddrOp, biaOp);
  }
}

/* Args: 检查实参列表，用于函数调用表达式，每个实参都可以变成一个表达式Exp；返回实参代码的头部 */
InterCode* translateArgs(Node* argsNode, InterCode* tail, InterCode* argsCode) {
  Node* expNode = getCertainChild(argsNode, 1);
  Operand* opVar = NULL;
  InterCode* newTail = tail;
  // ARG语句，注意如果实参类型是数组/结构体，传地址
  if (isPureArrayStruct(expNode)) { // 数组/结构体，直接用
    opVar = lookUpVar(getCertainChild(expNode, 1)->cval);
    opVar->kind = OP_ADDR;
  } else { // 非数组/结构体，先算表达式放到临时变量
    opVar = newTemp();
    newTail = addCodeToTail(translateExp(expNode, opVar), tail);
  }
  // 连接ARG语句链表
  argsCode = linkInterCodeHeadToHead(createInterCodeOne(IR_ARG, opVar), argsCode);
  if (expNode->nextSibling == NULL) { // 最后一个
    return getInterCodeHead(newTail);
  } else { // 接下来还有
    return translateArgs(getCertainChild(argsNode, 3), newTail, argsCode);
  }
}
