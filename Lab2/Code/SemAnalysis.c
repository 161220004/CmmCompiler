#include "Tree.h"
#include "SemAnalysis.h"

/* 可检测的语义错误如下：
  1) 错误类型1： 变量在使用时未经定义
  2) 错误类型2： 函数在调用时未经定义
  3) 错误类型3： 变量出现重复定义，或变量与前面定义过的结构体名字重复
  4) 错误类型4： 函数出现重复定义（即同样的函数名出现了不止一次定义）
  5) 错误类型5： 赋值号两边的表达式类型不匹配
  6) 错误类型6： 赋值号左边出现一个只有右值的表达式
  7) 错误类型7： 操作数类型不匹配或操作数类型与操作符不匹配（例如整型变量与数组变量相加减，或数组（或结构体）变量与数组（或结构体）变量相加减）
  8) 错误类型8： return语句的返回类型与函数定义的返回类型不匹配
  9) 错误类型9： 函数调用时实参与形参的数目或类型不匹配
  10) 错误类型10： 对非数组型变量使用“[…]” （数组访问）操作符
  11) 错误类型11： 对普通变量使用“(…)”或“()” （函数调用）操作符
  12) 错误类型12： 数组访问操作符“[…]” 中出现非整数（例如a[1.5]）
  13) 错误类型13： 对非结构体型变量使用“.” 操作符
  14) 错误类型14： 访问结构体中未定义过的域
  15) 错误类型15： 结构体中域名重复定义（指同一结构体中） ，或在定义时对域进行初始化（例如struct A { int a = 0; }）
  16) 错误类型16： 结构体的名字与前面定义过的结构体或变量的名字重复
  17) 错误类型17： 直接使用未定义过的结构体来定义变量
  18) 错误类型18： 函数进行了声明，但没有被定义
  19) 错误类型19： 函数的多次声明互相冲突（即函数名一致，但返回类型、形参数量或者形参类型不一致），或者声明与定义之间互相冲突
*/
void reportError(int errno, int lineno, char* val, char* addition) {
  switch (errno) {
    case 1: fprintf(stderr, "Error type 1 at Line %d: Undefined variable \"%s\".", lineno, val); break;
    case 2: fprintf(stderr, "Error type 2 at Line %d: Undefined function \"%s\".", lineno, val); break;
    case 3: fprintf(stderr, "Error type 3 at Line %d: Redefined variable \"%s\".", lineno, val); break;
    case 4: fprintf(stderr, "Error type 4 at Line %d: Redefined function \"%s\".", lineno, val); break;
    case 5: fprintf(stderr, "Error type 5 at Line %d: Type mismatched for assignment.", lineno); break;
    case 6: fprintf(stderr, "Error type 6 at Line %d: The left-hand side of an assignment must be a variable.", lineno); break;
    case 7: fprintf(stderr, "Error type 7 at Line %d: Type mismatched for operands.", lineno); break;
    case 8: fprintf(stderr, "Error type 8 at Line %d: Type mismatched for return.", lineno); break;
    case 9: fprintf(stderr, "Error type 9 at Line %d: Function \"%s\" is not applicable for arguments \"%s\".", lineno, val, addition); break;
    case 10: fprintf(stderr, "Error type 10 at Line %d: \"%s\" is not an array.", lineno, val); break;
    case 11: fprintf(stderr, "Error type 11 at Line %d: \"%s\" is not a function.", lineno, val); break;
    case 12: fprintf(stderr, "Error type 12 at Line %d: \"%s\" is not an integer.", lineno, val); break;
    case 13: fprintf(stderr, "Error type 13 at Line %d: Illegal use of \".\".", lineno); break;
    case 14: fprintf(stderr, "Error type 14 at Line %d: Non-existent field \"%s\".", lineno, val); break;
    case 15: fprintf(stderr, "Error type 15 at Line %d: Redefined field \"%s\".", lineno, val); break;
    case 16: fprintf(stderr, "Error type 16 at Line %d: Duplicated name \"%s\".", lineno, val); break;
    case 17: fprintf(stderr, "Error type 17 at Line %d: Undefined structure \"%s\".", lineno, val); break;
    case 18: fprintf(stderr, "Error type 18 at Line %d: Undefined function \"%s\".", lineno, val); break;
    case 19: fprintf(stderr, "Error type 19 at Line %d: Inconsistent declaration of function \"%s\".", lineno, val); break;
    default: fprintf(stderr, "Undefined Error !!!");
  }
}

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

/* 检查一个节点的子节点是否符合要求（只需检查第n个节点（从1开始数）是否是expectName），没有第n个则返回false */
bool childrenMatch(Node* node, int n, NodeName expectName) {
  Node* child = node->child;
  int restN = n - 1;
  while (child != NULL) {
    if (restN == 0) {
      if (child->name == expectName) return true;
      else return false;
    } else {
      child = child->nextSibling;
      restN -= 1;
    }
  }
  return false;
}

/* 获取一个节点的第n个子节点（从1开始数），没有第n个则返回NULL */
Node* getCertainChild(Node* node, int n) {
  Node* child = node->child;
  int restN = n - 1;
  while (child != NULL) {
    if (restN == 0) {
      return child;
    } else {
      child = child->nextSibling;
      restN -= 1;
    }
  }
  return NULL;
}

/* 类型复制（浅拷贝） */
Type* typeShallowCopy(Type* type) {
  Type* cp = (Type*)malloc(sizeof(Type));
  cp->kind = type->kind;
  switch (type->kind) {
    case T_STRUCT:
      cp->structure.node = type->structure.node;
      cp->structure.name = type->structure.name;
      cp->structure.isDefined = type->structure.isDefined;
      break;
    case T_ARRAY:
      cp->array.eleType = type->array.eleType;
      cp->array.length = type->array.length;
      break;
    default: cp->isRight = type->isRight;
  }
  return cp;
}

/* 在一个TypeNode*链表前方添加一段链表 */
TypeNode* addToTypeNodeList(TypeNode* preList, TypeNode* addList) {
  if (addList == NULL) { return preList; }
  TypeNode* tmpNode = addList;
  while (tmpNode->next != NULL) { tmpNode = tmpNode->next; }
  tmpNode->next = preList;
  return addList;
}

/* 函数符号表（有序数组） */
SymFuncNode* funcSymList = NULL;
int funcSymListLen = 0;
/* 结构体符号表（有序数组） */
SymStructNode* structSymList = NULL;
int structSymListLen = 0;
/* 全局作用域 */
FieldNode* globalField = NULL;
int globalVarListLen = 0;
/* 当前所在的作用域 */
FieldNode* currentField = NULL;

/* 辅助粗略计算函数/结构体（不是变量）/全局变量个数 */
void calExtDecListVarNum(Node* extDecListNode) {
  globalVarListLen += 1;
  if (childrenMatch(extDecListNode, 2, TN_COMMA)) {
    calExtDecListVarNum(getCertainChild(extDecListNode, 3));
  }
}
void calExtDefSymNum(Node* extDefNode) {
  if (childrenMatch(extDefNode, 2, NTN_FUNDEC)) { // 函数
    funcSymListLen += 1;
  } else {
    if (childrenMatch(getCertainChild(extDefNode, 1), 1, NTN_STRUCTSPECIFIER)) { // 结构体（不是变量）
      structSymListLen += 1;
    }
    if (childrenMatch(extDefNode, 2, NTN_EXTDECLIST)) { // 全局变量
      calExtDecListVarNum(getCertainChild(extDefNode, 2));
    }
  }
}
void calExtDefListSymNum(Node* extDefListNode) {
  if (extDefListNode->child != NULL) {
    calExtDefSymNum(getCertainChild(extDefListNode, 1));
    calExtDefListSymNum(getCertainChild(extDefListNode, 2));
  }
}
/* 粗略计算函数/结构体（不是变量）/全局变量个数（声明/定义都+1） */
void calRoughSymNum() {
  calExtDefListSymNum(getCertainChild(root, 1));
}

/* 从有序函数/结构体符号表中查询，返回下标；没有则返回-1 */
int findInList(char* name, int length, bool isFunc) {
  if (length < 1) return -1;
  int mid = length / 2;
  int result;
  if (isFunc) { // 从有序函数符号表中查询
    result = (funcSymList[mid].isNull) ? (-1) : strcmp(name, funcSymList[mid].name);
  } else { // 从结构体符号表中查询
    result = (structSymList[mid].isNull) ? (-1) : strcmp(name, structSymList[mid].name);
  }
  if (result == 0) {
    return mid;
  } else if (result < 0) { // 查询的值小于mid值
    return findInList(name, mid, isFunc);
  } else { // 查询的值大于mid值
    return findInList(name, length - mid - 1, isFunc);
  }
}

/* Program: 分析全部 */
void handleProgram() {
  // 计算函数/结构体（不是变量）/全局变量个数
  calRoughSymNum();
  // 创建全局函数表
  funcSymList = (SymFuncNode*)malloc(funcSymListLen * sizeof(SymFuncNode));
  for (int i = 0; i < funcSymListLen; i++) { funcSymList[i].isNull = true; }
  // 创建全局结构体表
  structSymList = (SymStructNode*)malloc(structSymListLen * sizeof(SymStructNode));
  for (int i = 0; i < structSymListLen; i++) { structSymList[i].isNull = true; }
  // 创建全局作用域
  globalField = (FieldNode*)malloc(sizeof(FieldNode));
  globalField->type = F_GLOBAL;
  globalField->parent = NULL;
  globalField->func = NULL;
  globalField->var = NULL;
  globalField->symNode = (SymVarNode*)malloc(globalVarListLen * sizeof(SymVarNode));
  currentField = globalField; // 当前处于全局作用域
  // 开始逐层分析
  handleExtDefList(getCertainChild(root, 1));
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
  Type* specType = handleSpecifier(getCertainChild(extDefNode, 1)); // 获取该全局变量的类型
  if (childrenMatch(extDefNode, 2, NTN_EXTDECLIST)) { // 任意类型全局变量(包括结构体变量)声明
    TypeNode* varTypeNodeList = handleExtDecList(getCertainChild(extDefNode, 2), NULL, specType);
    globalField->var = addToTypeNodeList(globalField->var, varTypeNodeList);
  } else if (childrenMatch(extDefNode, 2, TN_SEMI)) { // 结构体定义

  } else if (childrenMatch(extDefNode, 3, NTN_COMPST)) { // 函数定义

  } else { // if (childrenMatch(extDefNode, 3, TN_SEMI)) { // 函数声明

  }
}

/* ExtDecList: 检查零个或多个对一个全局变量的定义VarDec；返回定义后的节点 */
TypeNode* handleExtDecList(Node* extDecListNode, TypeNode* inhTypeNode, Type* inhType) {
  Node* varDecNode = getCertainChild(extDecListNode, 1);
  Type* varDecType = handleVarDec(varDecNode, inhType);
  TypeNode* varTypeNode = (TypeNode*)malloc(sizeof(TypeNode));
  varTypeNode->type = varDecType;
  varTypeNode->name = getVarDecName(varDecNode);
  if (varDecNode->nextSibling == NULL) { // 最后一个VarDec
    varTypeNode->next = inhTypeNode;
    return varTypeNode;
  } else { // 后面还有VarDec
    return handleExtDecList(getCertainChild(extDecListNode, 3), varTypeNode, inhType);
  }
}

/* Specifier: 检查类型定义（基本/结构体） */
Type* handleSpecifier(Node* specNode) {
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
    return handleStructSpecifier(getCertainChild(specNode, 1));
  }
}

/* StructSpecifier: 检查结构体类型定义 */
Type* handleStructSpecifier(Node* structSpecNode) {
  if (childrenMatch(structSpecNode, 2, NTN_OPTTAG)) { // 有结构体具体定义
    Type* structType = (Type*)malloc(sizeof(Type));
    structType->kind = T_STRUCT;
    structType->structure.isDefined = true;
    Node* defListNode = getCertainChild(structSpecNode, 4);
    structType->structure.node = handleDefList(defListNode, NULL, true); // 获取域链表
    Node* optTagNode = getCertainChild(structSpecNode, 2);
    if (optTagNode->child == NULL) { // 为空
      structType->structure.name = "";
    } else { // if (childrenMatch(optTagNode, 1, TN_ID)) { // 获取ID
      Node* idNode = getCertainChild(optTagNode, 1);
      structType->structure.name = getStrncpy(idNode->cval);
    }
    return structType;
  } else { // if (childrenMatch(structSpecNode, 2, NTN_TAG)) { // 结构体仅声明（无内容定义）
    // TODO: ...
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
    return addToTypeNodeList(inhTypeNode, defTypeNode);
  }
}

/* Def: 检查一条局部定义；返回一个TypeNode链表 */
TypeNode* handleDef(Node* defNode, bool inStruct) {
  Node* specNode = getCertainChild(defNode, 1);
  return handleDecList(getCertainChild(defNode, 2), NULL, handleSpecifier(specNode), inStruct);
}

/* DecList: 检查每一组逗号分割的变量名；返回一个TypeNode链表 */
TypeNode* handleDecList(Node* decListNode, TypeNode* inhTypeNode, Type* inhType, bool inStruct) {
  Node* decNode = getCertainChild(decListNode, 1);
  TypeNode* decTypeNode = handleDec(decNode, inhType, inStruct);
  decTypeNode->next = inhTypeNode;
  if (decNode->nextSibling == NULL) { // 最后一个Dec
    decTypeNode->next = inhTypeNode;
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
  if (varDecNode->nextSibling == NULL) { // 无初始化
    return varTypeNode;
  } else { // 有初始化
    if (inStruct) { // 结构体内禁止初始化
      reportError(15, varDecNode->lineno, varTypeNode->name, NULL);
    } else { // 检查初始化是否符合要求
      // TODO: ...
    }
  }
}
