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
  globalField = (FieldNode*)malloc(sizeof(FieldNode));
  globalField->type = F_GLOBAL;
  globalField->parent = NULL;
  globalField->func = NULL;
  globalField->varListLen = getRoughGloVarNum(extDefListNode);
  globalField->varSymList = (SymElem*)malloc(globalField->varListLen * sizeof(SymElem));
  for (int i = 0; i < globalField->varListLen; i++) { globalField->varSymList[i].isNull = true; }
  currentField = globalField; // 当前处于全局作用域
  // 开始逐层分析
  handleExtDefList(extDefListNode);
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
    }
  } else if (childrenMatch(extDefNode, 2, TN_SEMI)) { // 结构体定义
    // 处理结构体定义（非变量）
    handleSpecifier(getCertainChild(extDefNode, 1), true);
  } else { // 函数定义/声明
    // 获取函数的返回类型
    Type* returnType = handleSpecifier(getCertainChild(extDefNode, 1), false);
    Node* funcNode = getCertainChild(extDefNode, 2);
    Node* idNode = getCertainChild(funcNode, 1);
    TypeNode* varListTypeNode = NULL;
    if (childrenMatch(funcNode, 3, NTN_VARLIST)) { // 有参数
      varListTypeNode = handleVarList(getCertainChild(funcNode, 3), NULL);
    }
    // 查询此函数名是否已经记录
    int index = findInSymList(idNode->cval, 0, funcSymListLen, true);
    if (childrenMatch(extDefNode, 3, TN_SEMI)) { // 函数声明
      if (index < 0) { // 首次出现
        Function* decFunc = (Function*)malloc(sizeof(Function));
        decFunc->name = idNode->cval;
        decFunc->isDefined = false;
        decFunc->returnType = returnType;
        decFunc->paramNode = varListTypeNode;
        // 添加到函数符号表
        addToFuncList(decFunc);
      } else { // 重复声明，检测是否一致
        if (!typeEquals(returnType, funcSymList[index].func->returnType) ||
            !paramEquals(varListTypeNode, funcSymList[index].func->paramNode)) { // 存在冲突，报错19
          reportError(19, funcNode->lineno, idNode->cval, NULL);
        }
      }
    } else { // if (childrenMatch(extDefNode, 3, NTN_COMPST)) { // 函数定义
      if (index < 0) { // 首次出现
        Function* defFunc = (Function*)malloc(sizeof(Function));
        defFunc->name = idNode->cval;
        defFunc->isDefined = true;
        defFunc->returnType = returnType;
        defFunc->paramNode = varListTypeNode;
        // 添加到函数符号表
        addToFuncList(defFunc);
      } else { // 再次出现重复的
        Function* prefunc = funcSymList[index].func;
        if (prefunc->isDefined) { // 重复定义，报错4
          reportError(4, funcNode->lineno, idNode->cval, NULL);
        } else { // 声明过，检测声明与定义是否一致
          if (!typeEquals(returnType, funcSymList[index].func->returnType) ||
              !paramEquals(varListTypeNode, funcSymList[index].func->paramNode)) { // 存在冲突，报错19
            reportError(19, funcNode->lineno, idNode->cval, NULL);
          }
          prefunc->isDefined = true; // 无论是否一致，都视为已定义
        }
      }
      // 创建函数作用域
      // TODO: ...
      handleCompSt(getCertainChild(extDefNode, 3));
    }
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

/* ParamDec: 对一个形参的定义，为类型描述符+变量名；返回形参节点 */
TypeNode* handleParamDec(Node* paramDecNode) {
  Type* paramType = handleSpecifier(getCertainChild(paramDecNode, 1), false);
  Node* varDecNode = getCertainChild(paramDecNode, 2);
  Type* varDecType = handleVarDec(varDecNode, paramType);
  TypeNode* varTypeNode = (TypeNode*)malloc(sizeof(TypeNode));
  varTypeNode->type = varDecType;
  varTypeNode->name = getVarDecName(varDecNode);
  varTypeNode->lineno = varDecNode->lineno;
  varTypeNode->next = NULL;
  return varTypeNode;
}

/* CompSt: 一个由一对花括号括起来的语句块，其中全部局部变量的定义必须在全部语句之前 */
void handleCompSt(Node* compStNode) {

}

/* ExtDecList: 检查零个或多个对一个全局变量的定义VarDec；返回定义后的链表 */
TypeNode* handleExtDecList(Node* extDecListNode, TypeNode* inhTypeNode, Type* inhType) {
  Node* varDecNode = getCertainChild(extDecListNode, 1);
  Type* varDecType = handleVarDec(varDecNode, inhType);
  TypeNode* varTypeNode = (TypeNode*)malloc(sizeof(TypeNode));
  varTypeNode->type = varDecType;
  varTypeNode->name = getVarDecName(varDecNode);
  varTypeNode->lineno = varDecNode->lineno;
  varTypeNode->next = inhTypeNode;
  if (varDecNode->nextSibling == NULL) { // 最后一个VarDec
    return varTypeNode;
  } else { // 后面还有VarDec
    return handleExtDecList(getCertainChild(extDecListNode, 3), varTypeNode, inhType);
  }
}

/* Specifier: 检查类型定义（基本/结构体），isSemi特指“Specifier SEMI”的特殊情况 */
Type* handleSpecifier(Node* specNode, bool isSemi) {
  if (childrenMatch(specNode, 1, TN_TYPE)) { // 子节点为基本类型（int/float）
    Type* typeType = (Type*)malloc(sizeof(Type));
    Node* typeChild = getCertainChild(specNode, 1);
    typeType->isRight = false;
    if (strcmp("int", typeChild->cval) == 0) { // int
      typeType->kind = T_INT;
    } else if (strcmp("float", typeChild->cval) == 0) { // float
      typeType->kind = T_FLOAT;
    }
    return typeType;
  } else { // if (childrenMatch(specNode, 1, NTN_STRUCTSPECIFIER)) { // 子节点为结构体
    return handleStructSpecifier(getCertainChild(specNode, 1), isSemi);
  }
}

/* StructSpecifier: 检查结构体类型定义 */
Type* handleStructSpecifier(Node* structSpecNode, bool isSemi) {
  if (childrenMatch(structSpecNode, 2, NTN_OPTTAG)) { // 有结构体具体定义
    Type* structType = (Type*)malloc(sizeof(Type));
    structType->kind = T_STRUCT;
    Node* defListNode = getCertainChild(structSpecNode, 4);
    structType->structure.node = handleDefList(defListNode, NULL, true); // 获取域链表
    // 查找域链表是否存在重名
    TypeNode* structField = structType->structure.node;
    while (structField != NULL) {
      TypeNode* structFieldTmp = structField->next;
      while (structFieldTmp != NULL) {
        if (strcmp(structField->name, structFieldTmp->name) == 0) { // 重名报错15
          reportError(15, structField->lineno, structField->name, NULL);
        }
        structFieldTmp = structFieldTmp->next;
      }
      structField = structField->next;
    }
    // 获取结构体名
    Node* optTagNode = getCertainChild(structSpecNode, 2);
    if (optTagNode->child == NULL) { // 为空，则把结构体行数作为匿名结构体的名
      structType->structure.name = itoa(optTagNode->lineno);
    } else { // if (childrenMatch(optTagNode, 1, TN_ID)) { // 获取ID
      Node* idNode = getCertainChild(optTagNode, 1);
      structType->structure.name = getStrncpy(idNode->cval);
    }
    // 添加到结构体符号表数组
    if (findInSymList(structType->structure.name, 0, structSymListLen, false) < 0) { // 第一次出现
      addToStructList(structType);
    } else { // 发现重名，报错16
      reportError(16, optTagNode->lineno, structType->structure.name, NULL);
    }
    if (isInVarList(structType->structure.name, currentField)) { // 检查与变量重名，报错16
      reportError(16, optTagNode->lineno, structType->structure.name, NULL);
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
      // 作为一个“空”结构体返回以便程序继续运行
      Type* structType = (Type*)malloc(sizeof(Type));
      structType->kind = T_STRUCT;
      structType->structure.node = NULL;
      structType->structure.name = idNode->cval;
      return structType;
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
    Type* arrayType = (Type*)malloc(sizeof(Type));
    arrayType->kind = T_ARRAY;
    arrayType->array.length = getCertainChild(varDecNode, 3)->ival;
    arrayType->array.eleType = inhType;
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
  TypeNode* varTypeNode = (TypeNode*)malloc(sizeof(TypeNode));
  varTypeNode->type = varDecType;
  varTypeNode->name = getVarDecName(varDecNode);
  varTypeNode->lineno = varDecNode->lineno;
  varTypeNode->next = NULL;
  if (varDecNode->nextSibling == NULL) { // 无初始化
    return varTypeNode;
  } else { // 有初始化
    if (inStruct) { // 结构体内禁止初始化，报错15
      reportError(15, varDecNode->lineno, varTypeNode->name, NULL);
    } else { // 检查初始化是否符合要求
      // TODO: ...
    }
  }
}
