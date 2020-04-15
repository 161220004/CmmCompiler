#include "SemUtils.h"

/* 语义错误的检测过程中，涉及到的推导过程包括：
 * 定义声明类：
 * 记 $(GlobalVar) = "Program -> ExtDefList -> ExtDef... -> Specifier ExtDecList SEMI -> Specifier VarDec... SEMI"
 * 记 $(LocalVar) = "Program -> ExtDefList -> ExtDef... -> Specifier FunDec CompSt -> Specifier FunDec LC DefList StmtList RC
                    (Part:) DefList -> Def... -> Specifier DecList SEMI -> Specifier Dec... SEMI"
  1.1 基本变量全局声明
      $(GlobalVar) -> TYPE("int"|"float") VarDec... SEMI -> TYPE ID... SEMI
  1.2 基本变量局部声明
      $(LocalVar) -> Specifier VarDec... SEMI -> TYPE ID... SEMI
  1.3 基本变量局部定义
      $(LocalVar) -> Specifier VarDec ASSIGNOP Exp... SEMI -> TYPE ID ASSIGNOP Exp... SEMI
  2.1 数组变量全局声明
      $(GlobalVar) -> TYPE VarDec... SEMI -> TYPE VarDec LB INT RB... SEMI -> TYPE ID LB INT RB... SEMI
  2.2 数组变量局部声明
      $(LocalVar) -> Specifier VarDec... SEMI ->
      TYPE VarDec... SEMI -> TYPE VarDec LB INT RB... SEMI -> TYPE ID LB INT RB... SEMI
  2.3 数组变量局部定义
      $(LocalVar) -> Specifier VarDec ASSIGNOP Exp... SEMI -> TYPE VarDec ASSIGNOP Exp... SEMI ->
      TYPE VarDec LB INT RB ASSIGNOP Exp... SEMI -> TYPE ID LB INT RB ASSIGNOP Exp... SEMI
  3.1 结构体全局定义/结构体变量全局声明
      $(GlobalVar) -> StructSpecifier VarDec... SEMI
      --1-> STRUCT Tag VarDec SEMI [ERROR]
      --2-> STRUCT OptTag LC DefList RC VarDec SEMI -> STRUCT ""|ID LC Def... RC VarDec SEMI ->
      STRUCT ""|ID LC Specifier DecList SEMI RC VarDec SEMI -> STRUCT ""|ID LC Specifier Dec... SEMI RC VarDec SEMI
      --1-> STRUCT ""|ID LC Specifier VarDec SEMI RC VarDec SEMI
      --2-> STRUCT ""|ID LC Specifier VarDec ASSIGNOP Exp SEMI RC VarDec SEMI [ERROR]
  3.2 结构体局部定义/结构体变量局部声明
      $(LocalVar) -> StructSpecifier VarDec... SEMI -> ... (同 3.1)
  3.3 结构体变量局部定义（不允许）
      $(LocalVar) -> StructSpecifier VarDec ASSIGNOP Exp... SEMI [ERROR]
  4.1 函数全局声明
      Program -> ExtDefList -> ExtDef... -> Specifier FunDec SEMI
      --1-> Specifier ID LP VarList RP SEMI
      --2-> Specifier ID LP RP SEMI
  4.2 函数全局定义
      Program -> ExtDefList -> ExtDef... -> Specifier FunDec CompSt
      --1-> Specifier ID LP VarList RP CompSt
      --2-> Specifier ID LP RP CompSt
 * 运算访问类：
  1 变量运算
    (Part:) Exp --1-> (Exp) --2-> !Exp --3-> -Exp --4-> Exp =,==,!=,>,<,>=,<=,+,-,*,\,&&,|| Exp
  2 数组变量访问
    (Part:) Exp -> Exp LB Exp RB
  3 结构体变量域访问
    (Part:) Exp -> Exp DOT ID
  4 函数调用
    (Part:) Exp --1-> ID LP RP --2-> ID LP Args RP
*/

/* 语义分析 */
void semanticAnalysis() {
  if (isLab(2)) {
    handleProgram();
  }
}

/* Program: 分析全部 */
void handleProgram() {
  Node* extDefListNode = getCertainChild(root, 1);
  // 创建全局函数表
  funcSymListLen = getRoughFuncNum(extDefListNode);
  funcSymList = (SymElem*)malloc(funcSymListLen * sizeof(SymElem));
  for (int i = 0; i < funcSymListLen; i++) { funcSymList[i].isNull = true; }
  // 创建全局结构体表
  structSymListLen = getRoughStructNum(extDefListNode);
  structSymList = (SymElem*)malloc(structSymListLen * sizeof(SymElem));
  for (int i = 0; i < structSymListLen; i++) { structSymList[i].isNull = true; }
  // 创建全局作用域
  globalField = createChildField(F_GLOBAL, getRoughGloVarNum(extDefListNode), NULL);
  currentField = globalField; // 当前处于全局作用域
  // 开始逐层分析
  handleExtDefList(extDefListNode);
  // 最后检查有没有只声明没定义的函数
  for (int i = 0; i < funcSymListLen; i++) {
    if (!funcSymList[i].isNull && !funcSymList[i].func->isDefined) { // 非空且未定义，报错18
      reportError(18, funcSymList[i].func->lineno, funcSymList[i].func->name, NULL);
    }
  }
  if (yyget_debug()) {
    printf("Global Field: \n  ");
    printFieldNode(globalField);
    printf("Function Symbol List: \n  ");
    printSymList(funcSymListLen, funcSymList, true);
    printf("Struct Symbol List: \n  ");
    printSymList(structSymListLen, structSymList, true);
  }
}

/* ExtDefList: 检查一系列全局变量、结构体或函数的定义 */
void handleExtDefList(Node* extDefListNode) {
  if (extDefListNode->child != NULL) { // 还存在未处理的定义
    handleExtDef(getCertainChild(extDefListNode, 1));
    handleExtDefList(getCertainChild(extDefListNode, 2));
  }
}

/* ExtDef: 检查一个全局变量、结构体或函数的定义 */
void handleExtDef(Node* extDefNode) {
  if (childrenMatch(extDefNode, 2, NTN_EXTDECLIST)) { // 任意类型全局变量(包括结构体变量)声明
    Type* specType = handleSpecifier(getCertainChild(extDefNode, 1), false); // 获取该全局变量的类型
    // 获取定义的全部变量
    TypeNode* varTypeNodeList = handleExtDecList(getCertainChild(extDefNode, 2), NULL, specType);
    // 添加到全局变量符号表，若有重复的则报错
    TypeNode* varTypeNode = varTypeNodeList;
    while (varTypeNode != NULL) {
      if (findInVarList(varTypeNode->name, 0, globalField->varListLen, globalField->varSymList) < 0) { // 无重复
        addToVarList(varTypeNode, globalField->varSymList, globalField->varListLen);
      } else { // 发现重复，报错3
        reportError(3, varTypeNode->lineno, varTypeNode->name, NULL);
      }
      varTypeNode = varTypeNode->next;
    }
  } else if (childrenMatch(extDefNode, 2, TN_SEMI)) { // 结构体定义
    // 处理结构体定义（非变量）
    handleSpecifier(getCertainChild(extDefNode, 1), true);
  } else { // 函数定义/声明
    // 获取函数的返回类型
    Type* returnType = handleSpecifier(getCertainChild(extDefNode, 1), false);
    Node* funcNode = getCertainChild(extDefNode, 2);
    Node* idNode = getCertainChild(funcNode, 1);
    // 查询此函数名是否已经记录
    int index = findInSymList(idNode->cval, 0, funcSymListLen, true);
    if (childrenMatch(extDefNode, 3, TN_SEMI)) { // 函数声明
      Function* decFunc = handleFunDec(funcNode, returnType, false);
      if (index < 0) { // 首次出现，添加到函数符号表
        addToFuncList(decFunc);
      } else { // 重复声明，检测是否一致
        if (!typeEquals(decFunc->returnType, funcSymList[index].func->returnType) ||
            !paramEquals(decFunc->paramNode, funcSymList[index].func->paramNode)) { // 存在冲突，报错19
          reportError(19, funcNode->lineno, idNode->cval, NULL);
        }
      }
      if(yyget_debug()) {
        printf("Declare Function: \n  ");
        printFunction(decFunc, true);
      }
    } else { // if (childrenMatch(extDefNode, 3, NTN_COMPST)) { // 函数定义
      Function* defFunc = handleFunDec(funcNode, returnType, true);
      if (index < 0) { // 首次出现，添加到函数符号表
        addToFuncList(defFunc);
      } else { // 再次出现，检测是否重复定义/是否与声明一致
        Function* preDefFunc = funcSymList[index].func;
        if (preDefFunc->isDefined) { // 重复定义，报错4
          // 此时不影响下面创建函数作用域，且作用域对应的函数是新定义的这个
          reportError(4, funcNode->lineno, idNode->cval, NULL);
        } else { // 声明过，检测声明与定义是否一致
          if (!typeEquals(defFunc->returnType, preDefFunc->returnType) ||
              !paramEquals(defFunc->paramNode, preDefFunc->paramNode)) { // 存在冲突，报错19
            // 此时不影响下面创建函数作用域，且作用域对应的函数是新定义的这个（不是声明的那个）
            reportError(19, funcNode->lineno, idNode->cval, NULL);
          }
          preDefFunc->isDefined = true; // 无论是否一致，都视为已定义
        }
      }
      if(yyget_debug()) {
        printf("Define Function: \n  ");
        printFunction(defFunc, true);
      }
      // 创建函数作用域（重复定义的按本次定义的算），进入作用域处理完退回到当前作用域
      Node* compStNode = getCertainChild(extDefNode, 3);
      FieldNode* funcField = createChildField(F_FUNCTION, getRoughLocVarNum(getCertainChild(compStNode, 2)), defFunc);
      currentField = funcField; // 当前作用域置为函数作用域
      handleCompSt(compStNode); // 开始进入函数作用域内部进行分析
      currentField = currentField->parent; // 解决完函数内部后重置作用域到外部
      if (yyget_debug()) {
        printf("Function Field: \n  ");
        printFieldNode(funcField);
      }
    }
  }
}

/* ExtDecList: 检查零个或多个对一个全局变量的定义VarDec；返回定义后的链表 */
TypeNode* handleExtDecList(Node* extDecListNode, TypeNode* inhTypeNode, Type* inhType) {
  Node* varDecNode = getCertainChild(extDecListNode, 1);
  Type* varDecType = handleVarDec(varDecNode, inhType);
  TypeNode* varTypeNode = createTypeNode(varDecType, getVarDecName(varDecNode), varDecNode->lineno, inhTypeNode);
  if (varDecNode->nextSibling == NULL) { // 最后一个VarDec
    return varTypeNode;
  } else { // 后面还有VarDec
    return handleExtDecList(getCertainChild(extDecListNode, 3), varTypeNode, inhType);
  }
}

/* Specifier: 检查类型定义（基本/结构体），isSemi特指“Specifier SEMI”的特殊情况 */
Type* handleSpecifier(Node* specNode, bool isSemi) {
  if (childrenMatch(specNode, 1, TN_TYPE)) { // 子节点为基本类型（int/float）
    char* kindStr = getCertainChild(specNode, 1)->cval;
    Kind kind = T_INT;
    if (strcmp("float", kindStr) == 0) kind = T_FLOAT;
    return createBasicType(kind);
  } else { // if (childrenMatch(specNode, 1, NTN_STRUCTSPECIFIER)) { // 子节点为结构体
    return handleStructSpecifier(getCertainChild(specNode, 1), isSemi);
  }
}

/* StructSpecifier: 检查结构体类型定义 */
Type* handleStructSpecifier(Node* structSpecNode, bool isSemi) {
  if (childrenMatch(structSpecNode, 2, NTN_OPTTAG)) { // 有结构体具体定义
    // 获取结构体名
    char* structName = NULL;
    Node* optTagNode = getCertainChild(structSpecNode, 2);
    if (optTagNode->child == NULL) { // 为空，则把结构体行数作为匿名结构体的名
      structName = itoa(optTagNode->lineno);
    } else { // if (childrenMatch(optTagNode, 1, TN_ID)) { // 获取ID作为结构体名
      structName = getStrncpy(getCertainChild(optTagNode, 1)->cval);
    }
    // 获取域链表
    TypeNode* structNode = handleDefList(getCertainChild(structSpecNode, 4), NULL, true);
    // 创建结构体类型
    Type* structType = createStructType(structName, structNode);
    // 添加结构体名到结构体符号表数组
    if (findInSymList(structName, 0, structSymListLen, false) < 0) { // 第一次出现
      addToStructList(structType);
    } else { // 发现重名，报错16
      reportError(16, optTagNode->lineno, structName, NULL);
    }
    if (findTypeInAllVarList(structName, currentField) != NULL) { // 检查与变量重名，报错16
      reportError(16, optTagNode->lineno, structName, NULL);
    }
    // 查找域链表是否存在重名（重名不必删除）
    TypeNode* structField = structType->structure.node;
    while (structField != NULL) {
      TypeNode* dupStructField = findInTypeNode(structField->name, structField->next);
      if (dupStructField != NULL) { // 重名报错15
        reportError(15, structField->lineno, structField->name, NULL);
      }
      structField = structField->next;
    }
    return structType;
  } else { // if (childrenMatch(structSpecNode, 2, NTN_TAG)) { // 结构体仅声明（无内容定义）
    Node* idNode = getCertainChild(getCertainChild(structSpecNode, 2), 1);
    // 检查此结构体是否已经定义过
    int index = findInSymList(idNode->cval, 0, structSymListLen, false);
    if (index < 0) { // 出现未定义结构体
      if (!isSemi) { // 若不是特殊情况如“struct A;”，即定义变量，报错17
        reportError(17, idNode->lineno, idNode->cval, NULL);
      }
      return createStructType(idNode->cval, NULL); // 作为一个“空”结构体返回以便程序继续运行
    } else { // 已经定义过
      return structSymList[index].type;
    }
  }
}

/* VarDec: 检查变量定义（基本/数组），需借助上一次继承的类型 */
Type* handleVarDec(Node* varDecNode, Type* inhType)  {
  if (childrenMatch(varDecNode, 1, TN_ID)) { // 最后获得一个ID，直接返回前面的继承属性
    return typeShallowCopy(inhType);
  } else { // if (childrenMatch(varDecNode, 1, NTN_VARDEC)) { // 操作数组
    Type* arrayType = createArrayType(getCertainChild(varDecNode, 3)->ival, inhType);
    return handleVarDec(getCertainChild(varDecNode, 1), arrayType);
  }
}

/* VarDec: 获取VarDec的ID */
char* getVarDecName(Node* varDecNode) {
  if (childrenMatch(varDecNode, 1, TN_ID)) { // 获取ID
    return getStrncpy(getCertainChild(varDecNode, 1)->cval);
  } else {
    return getVarDecName(getCertainChild(varDecNode, 1));
  }
}

/* FunDec: 检查对一个函数头的定义 */
Function* handleFunDec(Node* funDecNode, Type* returnType, bool isDefined) {
  Node* idNode = getCertainChild(funDecNode, 1);
  if (childrenMatch(funDecNode, 3, NTN_VARLIST)) { // 有参数
    TypeNode* paramNode = handleVarList(getCertainChild(funDecNode, 3), NULL);
    // 检查参数名是否重名（重名不必删除）
    TypeNode* paramNode1 = paramNode;
    while (paramNode1 != NULL) {
      TypeNode* paramNode2 = paramNode1->next;
      while (paramNode2 != NULL) {
        if (strcmp(paramNode1->name, paramNode2->name) == 0) { // 重名报错3
          reportError(3, paramNode1->lineno, paramNode1->name, NULL);
        }
        paramNode2 = paramNode2->next;
      }
      paramNode1 = paramNode1->next;
    }
    return createFunction(idNode->cval, idNode->lineno, isDefined, returnType, paramNode);
  } else { // 无参数
    return createFunction(idNode->cval, idNode->lineno, isDefined, returnType, NULL);
  }
}

/* VarList: 检查函数的一个或多个形参；返回形参链表 */
TypeNode* handleVarList(Node* varListNode, TypeNode* inhTypeNode) {
  Node* paramDecNode = getCertainChild(varListNode, 1);
  TypeNode* paramTypeNode = handleParamDec(paramDecNode);
  paramTypeNode->next = inhTypeNode;
  if (paramDecNode->nextSibling == NULL) { // 最后一个形参
    return paramTypeNode;
  } else { // 后面还有
    return handleVarList(getCertainChild(varListNode, 3), paramTypeNode);
  }
}

/* ParamDec: 检查对一个形参的定义，为类型描述符+变量名；返回形参节点 */
TypeNode* handleParamDec(Node* paramDecNode) {
  Type* paramType = handleSpecifier(getCertainChild(paramDecNode, 1), false);
  Node* varDecNode = getCertainChild(paramDecNode, 2);
  Type* varDecType = handleVarDec(varDecNode, paramType);
  return createTypeNode(varDecType, getVarDecName(varDecNode), varDecNode->lineno, NULL);
}

/* CompSt: 检查一个由一对花括号括起来的语句块，其中全部局部变量的定义必须在全部语句之前 */
void handleCompSt(Node* compStNode) {
  // 处理定义部分，获取全部变量声明
  TypeNode* defTypeNode = handleDefList(getCertainChild(compStNode, 2), NULL, false);
  // 一个一个判断是否重复定义后加入当前作用域的变量符号表
  while (defTypeNode != NULL) {
    if (findInVarList(defTypeNode->name, 0, currentField->varListLen, currentField->varSymList) < 0) {
      // 本作用域内首次定义，则加入符号表
      addToVarList(defTypeNode, currentField->varSymList, currentField->varListLen);
    } else { // 重复定义，不加入符号表，报错3
      reportError(3, defTypeNode->lineno, defTypeNode->name, NULL);
    }
    defTypeNode = defTypeNode->next;
  }
  // 处理语句
  handleStmtList(getCertainChild(compStNode, 3));
}

/* StmtList: 检查零个或多个Stmt的组合 */
void handleStmtList(Node* stmtListNode) {
  if (stmtListNode->child != NULL) {
    handleStmt(getCertainChild(stmtListNode, 1), F_ANONY);
    handleStmtList(getCertainChild(stmtListNode, 2));
  }
}

/* Stmt: 检查一条语句，根据是否是条件/循环的子语句遇到CompSt则新建不同的作用域 */
void handleStmt(Node* stmtNode, FieldType extField) {
  if (childrenMatch(stmtNode, 1, NTN_EXP)) { // 普通语句
    handleExp(getCertainChild(stmtNode, 1)); // 不需要返回Exp类型
  } else if (childrenMatch(stmtNode, 1, NTN_COMPST)) { // 新的语句块（一定新建作用域）
    Node* compStNode = getCertainChild(stmtNode, 1);
    // 根据extField判断新建何种作用域（不可能是F_FUNCTION/F_GLOBAL）
    FieldNode* field = createChildField(extField, getRoughLocVarNum(getCertainChild(compStNode, 2)), NULL);
    currentField = field; // 当前作用域置为新建的作用域
    handleCompSt(compStNode); // 开始进入新建的作用域内部进行分析
    currentField = currentField->parent; // 解决完新建的作用域内部后重置作用域到外部
    if (yyget_debug()) {
      printf("Child Field: \n  ");
      printFieldNode(field);
    }
  } else if (childrenMatch(stmtNode, 1, TN_RETURN)) { // RETURN语句
    Node* expNode = getCertainChild(stmtNode, 2);
    Type* returnType = handleExp(expNode);
    FieldNode* funcField = currentField; // 查找其对应的函数
    while (funcField != NULL && funcField->type != F_FUNCTION) { funcField = funcField->parent; }
    if (funcField != NULL && !typeEquals(returnType, funcField->func->returnType)) { // 若返回类型不一致，报错8
      reportError(8, expNode->lineno, NULL, NULL);
    }
  } else { // IF-ELSE条件语句/WHILE循环语句
    Node* expNode = getCertainChild(stmtNode, 3);
    Type* condType = handleExp(expNode);
    // 判断循环语句判断类型是否符合语义
    if (condType->kind != T_INT) { // 不合语义，报错7
      reportError(7, expNode->lineno, NULL, NULL);
    }
    handleStmt(getCertainChild(stmtNode, 5), F_COND_LOOP);
  }
}

/* DefList: 检查一连串定义（可能为空），需借助上一次继承的定义链表 */
TypeNode* handleDefList(Node* defListNode, TypeNode* inhTypeNode, bool inStruct) {
  if (defListNode->child == NULL) { // 最后一次为空，直接返回之前的全部
    return inhTypeNode;
  } else { // if (childrenMatch(defListNode, 1, NTN_DEF)) { // 在继承的那部分中添加Def并传递下去
    TypeNode* defTypeNode = handleDef(getCertainChild(defListNode, 1), inStruct);
    inhTypeNode = linkTypeNodeList(inhTypeNode, defTypeNode);
    return handleDefList(getCertainChild(defListNode, 2), inhTypeNode, inStruct);
  }
}

/* Def: 检查一条局部定义；返回一个TypeNode链表 */
TypeNode* handleDef(Node* defNode, bool inStruct) {
  Node* specNode = getCertainChild(defNode, 1);
  return handleDecList(getCertainChild(defNode, 2), NULL, handleSpecifier(specNode, false), inStruct);
}

/* DecList: 检查每一组逗号分割的变量名；返回一个TypeNode链表 */
TypeNode* handleDecList(Node* decListNode, TypeNode* inhTypeNode, Type* inhType, bool inStruct) {
  Node* decNode = getCertainChild(decListNode, 1);
  TypeNode* decTypeNode = handleDec(decNode, inhType, inStruct);
  decTypeNode->next = inhTypeNode;
  if (decNode->nextSibling == NULL) { // 最后一个Dec
    return decTypeNode;
  } else { // 后面还有Dec
    return handleDecList(getCertainChild(decListNode, 3), decTypeNode, inhType, inStruct);
  }
}

/* Dec: 检查一个变量的声明（初始化分情况）；返回一个TypeNode节点 */
TypeNode* handleDec(Node* decNode, Type* inhType, bool inStruct) {
  Node* varDecNode = getCertainChild(decNode, 1);
  Type* varDecType = handleVarDec(varDecNode, inhType);
  TypeNode* varTypeNode = createTypeNode(varDecType, getVarDecName(varDecNode), varDecNode->lineno, NULL);
  // 根据是否初始化检查是否符合要求，不符合也照常返回
  if (varDecNode->nextSibling != NULL) { // 有初始化
    if (inStruct) { // 结构体内禁止初始化，报错15
      reportError(15, varDecNode->lineno, varTypeNode->name, NULL);
    } else { // 检查初始化是否符合要求
      Type* expType = handleExp(getCertainChild(decNode, 3));
      if (!typeEquals(varDecType, expType)) { // 初始化类型不一致，报错5
        reportError(5, varDecNode->lineno, NULL, NULL);
      }
    }
  }
  return varTypeNode;
}

/* Exp: 检查表达式，返回表达式类型 */
Type* handleExp(Node* expNode) {
  if (childrenMatch(expNode, 1, TN_LP)) { // 括号
    return handleExp(getCertainChild(expNode, 2));
  } else if (childrenMatch(expNode, 1, TN_INT)) { // 右值（int）
    return createRightType(T_INT);
  } else if (childrenMatch(expNode, 1, TN_FLOAT)) { // 右值（float）
    return createRightType(T_FLOAT);
  } else if (childrenMatch(expNode, 1, TN_ID)) {
    Node* idNode = getCertainChild(expNode, 1);
    Type* idType = findTypeInAllVarList(idNode->cval, currentField);
    if (idNode->nextSibling == NULL) { // 返回ID对应的类型
      if (idType == NULL) { // 发现未定义变量，报错1
        reportError(1, idNode->lineno, idNode->cval, NULL);
        return createUndefinedType(false); // 返回未定义类型
      } else {
        return typeShallowCopy(idType); // 返回最近定义域内定义的类型
      }
    } else { // 函数调用
      int index = findInSymList(idNode->cval, 0, funcSymListLen, true);
      if (index < 0) { // 发现未定义函数
        if (idType == NULL) { // 不是变量，报错2
          reportError(2, idNode->lineno, idNode->cval, NULL);
        } else { // 变量使用函数调用，报错11
          reportError(11, idNode->lineno, idNode->cval, NULL);
        }
        return createUndefinedType(true); // 返回未定义右值类型
      } else { // 函数已定义
        Function* func = funcSymList[index].func;
        TypeNode* argTypeNode = NULL; // 实参链表
        if (childrenMatch(expNode, 3, NTN_ARGS)) { // 含参函数调用
          argTypeNode = handleArgs(getCertainChild(expNode, 3), NULL);
        }
        if (!paramEquals(func->paramNode, argTypeNode)) { // 参数不匹配，报错9
          reportError(9, idNode->lineno, getArgsString(func->paramNode, func->name), getArgsString(argTypeNode, ""));
        }
        return typeShallowCopy(func->returnType);
      }
    }
  } else if (childrenMatch(expNode, 1, TN_MINUS)) { // 取负（算术运算，int/float），返回右值
    Type* expType = handleExp(getCertainChild(expNode, 2));
    if (expType->kind == T_UNDEFINED) { // Undefined是遗留错误，略过
      return expType;
    } if (isBasicType(expType)) { // 合法的算数运算
      return typeShallowCopy(expType);
    } else { // 不合法的算式，报错7
      reportError(7, expNode->lineno, NULL, NULL);
      return createUndefinedType(true); // 返回未定义右值类型
    }
  } else if (childrenMatch(expNode, 1, TN_NOT)) { // 取反（逻辑运算，仅int），返回int右值
    Type* expType = handleExp(getCertainChild(expNode, 2));
    if (expType->kind == T_UNDEFINED) { // Undefined是遗留错误，可进行类型推测，推测为int
      return createRightType(T_INT); // 返回int类型右值
    } if (expType->kind == T_INT) { // 合法的算数运算
      return typeShallowCopy(expType);
    } else { // 不合法的算式，报错7
      reportError(7, expNode->lineno, NULL, NULL);
      return createRightType(T_INT); // 返回int类型右值
    }
  } else { // if (childrenMatch(expNode, 1, NTN_EXP)) {
    Node* opNode = getCertainChild(expNode, 2); // 运算符
    Node* opExpNode1 = getCertainChild(expNode, 1); // 左侧运算数（Exp）
    Node* opExpNode2 = getCertainChild(expNode, 3); // 右侧运算数（Exp/ID）
    if (opNode->name == TN_ASSIGNOP) { // 赋值（小心右值）
      Type* leftType = handleExp(opExpNode1);
      Type* rightType = handleExp(opExpNode2);
      if (isBasicType(leftType) && leftType->isRight) { // 右值出现在左侧，报错6
        reportError(6, opExpNode1->lineno, NULL, NULL);
      }
      if (rightType->kind == T_UNDEFINED) { // 遗留错误，略过
        return typeShallowCopy(leftType);
      } else if (leftType->kind == T_UNDEFINED) { // 遗留错误，可进行类型推测，推测为rightType
        return typeShallowCopy(rightType);
      }
      if (!typeEquals(leftType, rightType)) { // 左右类型不同，报错5
        reportError(5, opExpNode2->lineno, NULL, NULL);
      }
      return typeShallowCopy(leftType); // 一律取左值作为最终值
    } else if (opNode->name == TN_PLUS || opNode->name == TN_MINUS ||
               opNode->name == TN_STAR || opNode->name == TN_DIV) { // 加减乘除，int/float（注意右值的“感染”），返回右值
      Type* opType1 = handleExp(opExpNode1);
      Type* opType2 = handleExp(opExpNode2);
      if (opType1->kind == T_UNDEFINED && opType2->kind == T_UNDEFINED) { // 遗留错误，略过
        return opType1;
      } else if (opType1->kind != T_UNDEFINED && opType2->kind == T_UNDEFINED) { // 遗留错误，可进行类型推测
        opType2 = typeShallowCopy(opType1);
      } else if (opType1->kind == T_UNDEFINED && opType2->kind != T_UNDEFINED) { // 遗留错误，可进行类型推测
        opType1 = typeShallowCopy(opType2);
      }
      if (isBasicType(opType1) && isBasicType(opType2) &&
          opType1->kind == opType2->kind) { // 必须操作数类型完全相同才合法（注意右值的“感染”）
        Type* expType = typeShallowCopy(opType1);
        if (opType2->isRight) expType->isRight = true;
        return expType;
      } else { // 不合法的算式，报错7
        Node* opNodeIll = opExpNode1; // 不合法的那个
        if (isBasicType(opType1)) opNodeIll = opExpNode2;
        reportError(7, opNodeIll->lineno, NULL, NULL);
        return createUndefinedType(true); // 返回未定义右值类型
      }
    } else if (opNode->name == TN_AND || opNode->name == TN_OR) { // 逻辑运算，仅int（注意右值的“感染”），返回int类型右值
      Type* opType1 = handleExp(opExpNode1);
      Type* opType2 = handleExp(opExpNode2);
      if (opType1->kind == T_UNDEFINED && opType2->kind == T_UNDEFINED) { // 遗留错误，可进行类型推测，推测为int
        return createRightType(T_INT); // 返回int类型右值
      } else if (opType1->kind != T_UNDEFINED && opType2->kind == T_UNDEFINED) { // 遗留错误，可进行类型推测
        opType2 = typeShallowCopy(opType1);
      } else if (opType1->kind == T_UNDEFINED && opType2->kind != T_UNDEFINED) { // 遗留错误，可进行类型推测
        opType1 = typeShallowCopy(opType2);
      }
      if (opType1->kind == T_INT && opType2->kind == T_INT) { // 合法的算数运算（注意右值的“感染”）
        Type* expType = typeShallowCopy(opType1);
        if (opType2->isRight) expType->isRight = true;
        return expType;
      } else { // 不合法的算式，报错7
        Node* opNodeIll = opExpNode1; // 不合法的那个
        if (opType1->kind == T_INT) opNodeIll = opExpNode2;
        reportError(7, opNodeIll->lineno, NULL, NULL);
        return createRightType(T_INT); // 返回int类型右值
      }
    } else if (opNode->name == TN_EQ || opNode->name == TN_NE || opNode->name == TN_LE ||
               opNode->name == TN_GE || opNode->name == TN_LT || opNode->name == TN_GT) { // RELOP，一定返回int
      Type* opType1 = handleExp(opExpNode1);
      Type* opType2 = handleExp(opExpNode2);
      if (opType1->kind == T_UNDEFINED && opType2->kind == T_UNDEFINED) { // 遗留错误，可进行类型推测为int
        return createRightType(T_INT); // 返回int类型右值
      } else if (opType1->kind != T_UNDEFINED && opType2->kind == T_UNDEFINED) { // 遗留错误，可进行类型推测
        opType2 = typeShallowCopy(opType1);
      } else if (opType1->kind == T_UNDEFINED && opType2->kind != T_UNDEFINED) { // 遗留错误，可进行类型推测
        opType1 = typeShallowCopy(opType2);
      }
      if (isBasicType(opType1) && isBasicType(opType2)) { // 合法的关系运算
        return createRightType(T_INT); // 返回int类型右值
      } else { // 不合法的算式，报错7
        Node* opNodeIll = opExpNode1; // 不合法的那个
        if (isBasicType(opType1)) opNodeIll = opExpNode2;
        reportError(7, opNodeIll->lineno, NULL, NULL);
        return createRightType(T_INT); // 返回int类型右值
      }
    } else if (opNode->name == TN_DOT) { // 结构体访问，返回非右值
      Type* structType = handleExp(opExpNode1);
      if (structType->kind != T_STRUCT) { // 非结构体，报错13
        reportError(13, opNode->lineno, NULL, NULL);
        return createUndefinedType(false);
      }
      TypeNode* structTypeNode = findInTypeNode(opExpNode2->cval, structType->structure.node);
      if (structTypeNode == NULL) { // 不存在此域名，报错14
        reportError(14, opExpNode2->lineno, opExpNode2->cval, NULL);
        return createUndefinedType(false);
      }
      return typeShallowCopy(structTypeNode->type);
    } else { // 数组访问，返回非右值
      Type* arrayType = handleExp(opExpNode1);
      Type* indexType = handleExp(opExpNode2);
      if (arrayType->kind != T_ARRAY) { // 非数组，报错10
        reportError(10, opNode->lineno, getExpString(opExpNode1), NULL);
        return createUndefinedType(false);
      }
      if (indexType->kind != T_INT) { // 非整数下标，报错12（返回类型推测为数组元素类型）
        reportError(12, opNode->lineno, getExpString(opExpNode2), NULL);
      }
      return typeShallowCopy(arrayType->array.eleType);
    }
  }
}

/* Args: 检查实参列表，用于函数调用表达式，每个实参都可以变成一个表达式Exp；返回的匿名的TypeNode链表 */
TypeNode* handleArgs(Node* argsNode, TypeNode* inhTypeNode) {
  Node* expNode = getCertainChild(argsNode, 1);
  Type* expType = handleExp(expNode);
  TypeNode* expTypeNode = createTypeNode(expType, "", expNode->lineno, inhTypeNode);
  if (expNode->nextSibling == NULL) { // 最后一个
    return expTypeNode;
  } else { // 接下来还有
    return handleArgs(getCertainChild(argsNode, 3), expTypeNode);
  }
}
