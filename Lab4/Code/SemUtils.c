#include "SemUtils.h"

/* 函数符号表（有序数组） */
SymElem* funcSymList = NULL;
int funcSymListLen = 0;
/* 结构体符号表（有序数组） */
SymElem* structSymList = NULL;
int structSymListLen = 0;
/* 当前所在的作用域 */
FieldNode* currentField = NULL;

/** Lab3 唯一作用域 */
FieldNode* IRField = NULL;

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
  if (isLab(3)) return;
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
  fprintf(stderr, "\n");
}

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

/* 将一个int(>=0)转换为string */
char* itoa(int num) {
  if (num == 0) return "0";
  char tmp[12];
  char* str = (char*)malloc(12 * sizeof(char));
  int n = 0;
  while (num > 0) {
    tmp[n] = '0' + (num % 10);
    num /= 10;
    n += 1;
  }
  tmp[n] = '\0';
  for (int i = 0; i < n; i++) {
    str[i] = tmp[n - 1 - i];
  }
  str[n] = '\0';
  return str;
}

/* 类型复制（浅拷贝） */
Type* typeShallowCopy(Type* type) {
  Type* cp = (Type*)malloc(sizeof(Type));
  cp->kind = type->kind;
  switch (type->kind) {
    case T_STRUCT:
      cp->structure.node = type->structure.node;
      cp->structure.name = type->structure.name;
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
TypeNode* linkTypeNodeList(TypeNode* preList, TypeNode* addList) {
  if (addList == NULL) { return preList; }
  TypeNode* tmpNode = addList;
  while (tmpNode->next != NULL) { tmpNode = tmpNode->next; }
  tmpNode->next = preList;
  return addList;
}

/* 计算一个节点下方的特殊节点（StructSpecifier/FuncDec/VarDec）个数，来推测结构体/函数/变量个数 */
int getCertainNum(Node* node, NodeName certainName) {
  if (node == NULL) return 0;
  else if (node->name == certainName) return 1;
  else {
    int sum = 0;
    Node* child = node->child;
    while (child != NULL) {
      sum += getCertainNum(child, certainName);
      child = child->nextSibling;
    }
    return sum;
  }
}

/* 粗略计算结构体个数 */
int getRoughStructNum(Node* extDefListNode) {
  if (extDefListNode->child == NULL) return 0;
  else {
    return (getCertainNum(getCertainChild(getCertainChild(extDefListNode, 1), 1), NTN_STRUCTSPECIFIER)
          + getRoughStructNum(getCertainChild(extDefListNode, 2)));
  }
}

/* 粗略计算函数个数 */
int getRoughFuncNum(Node* extDefListNode) {
  if (extDefListNode->child == NULL) return 0;
  else {
    return (getCertainNum(getCertainChild(extDefListNode, 1), NTN_FUNDEC)
          + getRoughFuncNum(getCertainChild(extDefListNode, 2)));
  }
}

/* 粗略计算全局变量个数 */
int getRoughGloVarNum(Node* extDefListNode) {
  if (extDefListNode->child == NULL) return 0;
  else {
    Node* extDefNode = getCertainChild(extDefListNode, 1);
    if (childrenMatch(extDefNode, 2, NTN_EXTDECLIST)) {
      return (getCertainNum(getCertainChild(extDefNode, 2), NTN_VARDEC)
            + getRoughGloVarNum(getCertainChild(extDefListNode, 2)));
    } else {
      return getRoughGloVarNum(getCertainChild(extDefListNode, 2));
    }
  }
}

/* 粗略计算局部变量个数 */
int getRoughLocVarNum(Node* defListNode) {
  if (defListNode->child == NULL) return 0;
  else {
    Node* defNode = getCertainChild(defListNode, 1);
    return (getCertainNum(getCertainChild(defNode, 2), NTN_VARDEC)
          + getRoughLocVarNum(getCertainChild(defListNode, 2)));
  }
}

/* 添加一个元素到函数符号表有序数组 */
void addToFuncList(Function* func) {
  for (int i = 0; i < funcSymListLen; i++) {
    if (funcSymList[i].isNull) { // 发现空项
      funcSymList[i].isNull = false;
      funcSymList[i].name = func->name;
      funcSymList[i].func = func;
      break;
    } else { // 比较大小，保持有序
      int result = strcmp(func->name, funcSymList[i].name);
      if (result < 0) { // 新函数名小于i处函数名，则i之后全部后移出空位给i
        for (int j = funcSymListLen - 2; j >= i; j--) {
          if (!funcSymList[j].isNull) {
            funcSymList[j + 1].isNull = funcSymList[j].isNull;
            funcSymList[j + 1].name = funcSymList[j].name;
            funcSymList[j + 1].func = funcSymList[j].func;
          }
        }
        funcSymList[i].isNull = false;
        funcSymList[i].name = func->name;
        funcSymList[i].func = func;
        break;
      }
    }
  }
  if (yyget_debug() && isLab(2)) {
    printf("addToFuncList: [\"%s\"]\n  ", func->name);
    printSymList(funcSymListLen, funcSymList, true);
  }
}

/* 添加一个元素到结构体符号表有序数组 */
void addToStructList(Type* type) {
  for (int i = 0; i < structSymListLen; i++) {
    if (structSymList[i].isNull) { // 发现空项
      structSymList[i].isNull = false;
      structSymList[i].name = type->structure.name;
      structSymList[i].type = type;
      break;
    } else { // 比较大小，保持有序
      int result = strcmp(type->structure.name, structSymList[i].name);
      if (result < 0) { // 新函数名小于i处函数名，则i之后全部后移出空位给i
        for (int j = structSymListLen - 2; j >= i; j--) {
          if (!structSymList[j].isNull) {
            structSymList[j + 1].isNull = structSymList[j].isNull;
            structSymList[j + 1].name = structSymList[j].name;
            structSymList[j + 1].type = structSymList[j].type;
          }
        }
        structSymList[i].isNull = false;
        structSymList[i].name = type->structure.name;
        structSymList[i].type = type;
        break;
      }
    }
  }
  if (yyget_debug() && isLab(2)) {
    printf("addToStructList: [\"%s\"]\n  ", type->structure.name);
    printSymList(structSymListLen, structSymList, true);
  }
}

/* 添加一个元素到局部/全局变量符号表有序数组 */
void addToVarList(TypeNode* addVar, SymElem* varList, int varListLen) {
  for (int i = 0; i < varListLen; i++) {
    if (varList[i].isNull) { // 发现空项（只能是最后一项）
      varList[i].isNull = false;
      varList[i].name = addVar->name;
      varList[i].type = addVar->type;
      varList[i].isParam = false;
      break; // 结束
    } else { // 比较大小，保持有序
      int result = strcmp(addVar->name, varList[i].name);
      if (result < 0) { // 新函数名小于i处函数名，则i之后全部后移出空位给i
        for (int j = varListLen - 2; j >= i; j--) {
          if (!varList[j].isNull) {
            varList[j + 1].isNull = varList[j].isNull;
            varList[j + 1].name = varList[j].name;
            varList[j + 1].type = varList[j].type;
            varList[j + 1].isParam = varList[j].isParam;
          }
        }
        varList[i].isNull = false;
        varList[i].name = addVar->name;
        varList[i].type = addVar->type;
        varList[i].isParam = false;
        break; // 结束
      }
    }
  }
  if (yyget_debug() && (isLab(2) || isLab(3))) {
    printf("addToVarList: [\"%s\"(%d): ", addVar->name, addVar->lineno);
    printType(addVar->type, false);
    printf("]\n  ");
    printSymList(varListLen, varList, true);
  }
}

/* 从TypeNode链表中查询，返回查询的节点，没有则返回NULL */
TypeNode* findInTypeNode(char* name, TypeNode* typeNode) {
  while (typeNode != NULL) {
    if (strcmp(name, typeNode->name) == 0) return typeNode;
    else typeNode = typeNode->next;
  }
  return NULL;
}

/* 从有序函数/结构体符号表/当前变量符号表中查询，返回下标；没有则返回-1 */
int findInSymList(char* name, int start, int end, SymElem* symList) { // end是最后一个下标+1
  if (start >= end) return -1;
  if (symList == NULL) return -1;
  int mid = (start + end) / 2;
  int result = (symList[mid].isNull) ? (-1) : strcmp(name, symList[mid].name);
  if (result == 0) {
    return mid;
  } else if (result < 0) { // 查询的值小于mid值
    return findInSymList(name, start, mid, symList);
  } else { // 查询的值大于mid值
    return findInSymList(name, mid + 1, end, symList);
  }
}

/* 从当前以及其全部父作用域中查询，若有返回最近那个的类型，没有则返回NULL */
Type* findTypeInAllVarList(char* name, FieldNode* field) {
  while (field != NULL) {
    int index = findInSymList(name, 0, field->varListLen, field->varSymList);
    if (index < 0) { // 当前没找到
      field = field->parent;
    } else { // 找到了
      return field->varSymList[index].type;
    }
  }
  return NULL;
}

/* 新建未定义类型 */
Type* createUndefinedType(bool isRight) {
  Type* undefinedType = (Type*)malloc(sizeof(Type));
  undefinedType->kind = T_UNDEFINED;
  undefinedType->isRight = isRight;
  return undefinedType;
}

/* 新建右值基本类型（int/float） */
Type* createRightType(Kind kind) {
  Type* rightType = (Type*)malloc(sizeof(Type));
  rightType->isRight = true;
  rightType->kind = kind;
  return rightType;
}

/* 新建基本类型（int/float） */
Type* createBasicType(Kind kind) {
  Type* basicType = (Type*)malloc(sizeof(Type));
  basicType->isRight = false;
  basicType->kind = kind;
  return basicType;
}

/* 新建数组类型 */
Type* createArrayType(int length, Type* eleType) {
  Type* arrayType = (Type*)malloc(sizeof(Type));
  arrayType->kind = T_ARRAY;
  arrayType->array.length = length;
  arrayType->array.eleType = eleType;
  return arrayType;
}

/* 新建空的结构体类型 */
Type* createStructType(char* name, TypeNode* structNode) {
  Type* structType = (Type*)malloc(sizeof(Type));
  structType->kind = T_STRUCT;
  structType->structure.node = structNode;
  structType->structure.name = name;
  return structType;
}

/* 新建TypeNode类型 */
TypeNode* createTypeNode(Type* type, char* name, int lineno, TypeNode* next) {
  TypeNode* typeNode = (TypeNode*)malloc(sizeof(TypeNode));
  typeNode->type = type;
  typeNode->name = name;
  typeNode->lineno = lineno;
  typeNode->next = next;
  return typeNode;
}

/* 新建函数 */
Function* createFunction(char* name, int lineno, bool isDefined, Type* returnType, TypeNode* paramNode) {
  Function* func = (Function*)malloc(sizeof(Function));
  func->name = name;
  func->lineno = lineno;
  func->isDefined = isDefined;
  func->returnType = returnType;
  func->paramNode = paramNode;
  return func;
}

/* 新建currentField的子作用域 */
FieldNode* createChildField(FieldType type, int varListLen, Function* func) {
  FieldNode* field = (FieldNode*)malloc(sizeof(FieldNode));
  field->type = type;
  field->parent = currentField;
  field->func = func;
  int addLen = 0; // 如果是函数作用域，将参数表加入局部变量表，因而长度要加上参数个数
  if (type == F_FUNCTION) { // 计算参数长度
    TypeNode* paramNode = func->paramNode;
    while (paramNode != NULL) {
      addLen += 1;
      paramNode = paramNode->next;
    }
  }
  field->varListLen = varListLen + addLen;
  if (field->varListLen > 0) {
    field->varSymList = (SymElem*)malloc(field->varListLen * sizeof(SymElem));
  } else {
    field->varSymList = NULL;
  }
  for (int i = 0; i < field->varListLen; i++) { field->varSymList[i].isNull = true; }
  if (type == F_FUNCTION) { // 如果是函数作用域，将参数表加入局部变量表
    TypeNode* paramNode = func->paramNode;
    while (paramNode != NULL) {
      addToVarList(paramNode, field->varSymList, field->varListLen);
      paramNode = paramNode->next;
    }
  }
  return field;
}

/* 获取函数参数表字符串 */
char* getArgsString(TypeNode* paramNode, char* funcName) {
  char* result = (char*)malloc(64 * sizeof(char));
  result[0] = '\0';
  strcat(result, funcName);
  strcat(result, "(");
  while (paramNode != NULL) {
    switch (paramNode->type->kind) {
      case T_INT: strcat(result, "int"); break;
      case T_FLOAT: strcat(result, "float"); break;
      case T_STRUCT: strcat(result, "struct "); strcat(result, paramNode->type->structure.name); break;
      case T_ARRAY: strcat(result, "array"); break;
      default: strcat(result, "undefined");
    }
    paramNode = paramNode->next;
    if (paramNode != NULL) strcat(result, ", ");
  }
  strcat(result, ")");
  return result;
}

/* 辅助获取Exp字符串 */
char* getTmpExpString(Node* expNode, char* inhStr) {
  Node* child = expNode->child;
  while (child != NULL) {
    if (child->name == NTN_EXP) inhStr = getTmpExpString(child, inhStr);
    else if (child->name == NTN_ARGS) inhStr = strcat(inhStr, "...");
    else inhStr = strcat(inhStr, child->cval);
    child = child->nextSibling;
  }
  return inhStr;
}

/* 获取Exp字符串 */
char* getExpString(Node* expNode) {
  char* result = (char*)malloc(64 * sizeof(char));
  return getTmpExpString(expNode, result);
}

/* 检查是否是基本类型 */
bool isBasicType(Type* type) {
  if (type->kind == T_INT || type->kind == T_FLOAT) return true;
  else return false;
}

/* 检查两个类型是否一致 */
bool typeEquals(Type* type1, Type* type2) {
  if (type1->kind != type2->kind) return false;
  switch (type1->kind) {
    case T_ARRAY: // 只看基类型和维数，不管数组长度
      return typeEquals(type1->array.eleType, type2->array.eleType);
    case T_STRUCT: // 域的顺序和类型都相同
      return paramEquals(type1->structure.node, type2->structure.node);
    case T_UNDEFINED: return false; // 两个未定义类型不等
    default: return true; // 这里基本类型不考虑右值
  }
}

/* 检查两个函数参数链表类型/结构体域链表类型是否一致（名字不同没关系） */
bool paramEquals(TypeNode* param1, TypeNode* param2) {
  while (param1 != NULL && param2 != NULL) {
    if (typeEquals(param1->type, param2->type)) { // 判断下一个
      param1 = param1->next;
      param2 = param2->next;
    } else return false; // 任何一个域/参数不同就判定结构体/参数表不同
  }
  if (param1 == NULL && param2 == NULL) return true; // 同时结束
  else return false;
}

/* DEBUG: 打印类型 */
void printType(Type* type, bool toNewLine) {
  switch (type->kind) {
    case T_INT: printf("Int");  break;
    case T_FLOAT: printf("Float"); break;
    case T_ARRAY:
      printf("Array(");
      printType(type->array.eleType, false);
      printf(", %d)", type->array.length);
      break;
    case T_STRUCT: printf("Struct %s", type->structure.name); break;
    default: printf("Undefined");
  }
  if (toNewLine) printf("\n");
}

/* DEBUG: 打印Struct类型详细信息 */
void printStruct(Type* type) {
  if (type->kind == T_STRUCT) {
    printf("Struct %s {", type->structure.name);
    TypeNode* structNode = type->structure.node;
    while (structNode != NULL) {
      printf("\"%s\": ", structNode->name);
      printType(structNode->type, false);
      structNode = structNode->next;
      if (structNode != NULL) printf(", ");
    }
    printf("}\n");
  }
}

/* DEBUG: 打印TypeNode */
void printTypeNode(TypeNode* typeNode, bool toNewLine) {
  while (typeNode != NULL) {
    printf("[\"%s\"(%d): ", typeNode->name, typeNode->lineno);
    printType(typeNode->type, false);
    printf("] -> ");
    typeNode = typeNode->next;
  }
  if (toNewLine) printf("NULL\n");
  else printf("NULL");
}

/* DEBUG: 打印函数 */
void printFunction(Function* func, bool toNewLine) {
  printf("Function(\"%s\"(%d), return: ", func->name, func->lineno);
  printType(func->returnType, false);
  printf(", params: ");
  printTypeNode(func->paramNode, false);
  if (toNewLine) printf(")\n");
  else printf(")");
}

/* DEBUG: 打印符号表 */
void printSymList(int symListLen, SymElem* symList, bool toNewLine) {
  for (int i = 0; i < symListLen; i++) {
    if (!symList[i].isNull) printf("[\"%s\"] -> ", symList[i].name);
    else break;
  }
  if (toNewLine) printf("NULL\n");
  else printf("NULL");
}

char* fieldTypeStr[4] = {"Global", "Func", "Cond/Loop", "Anony"};
/* DEBUG: 打印一个作用域 */
void printFieldNode(FieldNode* field) {
  printf("Field %s {SymList(len = %d): ", fieldTypeStr[field->type], field->varListLen);
  printSymList(field->varListLen, field->varSymList, false);
  if (field->type == F_FUNCTION) {
    printf(", ");
    printFunction(field->func, false);
  }
  printf("}\n");
}

/** 用于实验三/四，预添加两个函数 int read() 和 int write(int) */
void preAddFunctions() {
  Type* intType = createBasicType(T_INT);
  Function* readFunc = createFunction("read", 0, true, typeShallowCopy(intType), NULL);
  TypeNode* intTypeNode = createTypeNode(intType, "n", 0, NULL);
  Function* writeFunc = createFunction("write", 0, true, typeShallowCopy(intType), intTypeNode);
  addToFuncList(readFunc);
  addToFuncList(writeFunc);
}

/** 用于实验三/四，新建全局作用域，并置为IRField */
void createGlobalField(int varListLen, SymElem* varSymList) {
  FieldNode* field = (FieldNode*)malloc(sizeof(FieldNode));
  field->type = F_GLOBAL;
  field->parent = NULL;
  field->func = NULL;
  field->varListLen = varListLen;
  field->varSymList = varSymList;
  IRField = field;
}
