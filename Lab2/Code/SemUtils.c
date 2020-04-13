#include "SemUtils.h"

/* 函数符号表（有序数组） */
SymElem* funcSymList = NULL;
int funcSymListLen = 0;
/* 结构体符号表（有序数组） */
SymElem* structSymList = NULL;
int structSymListLen = 0;
/* 全局作用域 */
FieldNode* globalField = NULL;
/* 当前所在的作用域 */
FieldNode* currentField = NULL;

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

/* 将一个int(>=1)转换为string */
char* itoa(int num) {
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
    } else return 0;
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
      }
    }
  }
}

/* 添加一个元素到结构体符号表有序数组 */
void addToStructList(Type* type) {
  for (int i = 0; i < structSymListLen; i++) {
    if (structSymList[i].isNull) { // 发现空项
      structSymList[i].isNull = false;
      structSymList[i].name = type->structure.name;
      structSymList[i].type = type;
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
      }
    }
  }
}

/* 添加一个元素到局部/全局变量符号表有序数组 */
void addToVarList(TypeNode* addVar, SymElem* varList, int varListLen) {
  for (int i = 0; i < varListLen; i++) {
    if (varList[i].isNull) { // 发现空项（只能是最后一项）
      varList[i].isNull = false;
      varList[i].name = addVar->name;
      varList[i].type = addVar->type;
    } else { // 比较大小，保持有序
      int result = strcmp(addVar->name, varList[i].name);
      if (result < 0) { // 新函数名小于i处函数名，则i之后全部后移出空位给i
        for (int j = varListLen - 2; j >= i; j--) {
          if (!varList[j].isNull) {
            varList[j + 1].isNull = varList[j].isNull;
            varList[j + 1].name = varList[j].name;
            varList[j + 1].type = varList[j].type;
          }
        }
        varList[i].isNull = false;
        varList[i].name = addVar->name;
        varList[i].type = addVar->type;
      }
    }
  }
}

/* 从有序函数/结构体符号表中查询，返回下标；没有则返回-1 */
int findInSymList(char* name, int start, int end, bool isFunc) { // end是最后一个下标+1
  if (start >= end) return -1;
  int mid = (end - start) / 2;
  int result;
  if (isFunc) { // 从有序函数符号表中查询
    result = (funcSymList[mid].isNull) ? (-1) : strcmp(name, funcSymList[mid].name);
  } else { // 从结构体符号表中查询
    result = (structSymList[mid].isNull) ? (-1) : strcmp(name, structSymList[mid].name);
  }
  if (result == 0) {
    return mid;
  } else if (result < 0) { // 查询的值小于mid值
    return findInSymList(name, start, mid, isFunc);
  } else { // 查询的值大于mid值
    return findInSymList(name, mid + 1, end, isFunc);
  }
}

/* 从当前变量符号表中查询，返回下标；没有则返回-1 */
int findInVarList(char* name, int start, int end, SymElem* varList) {
  if (start >= end) return -1;
  int mid = (end - start) / 2;
  int result = (varList[mid].isNull) ? (-1) : strcmp(name, varList[mid].name);
  if (result == 0) {
    return mid;
  } else if (result < 0) { // 查询的值小于mid值
    return findInVarList(name, start, mid, varList);
  } else { // 查询的值大于mid值
    return findInVarList(name, mid + 1, end, varList);
  }
}

/* 从当前以及其全部父作用域中查询 */
bool isInVarList(char* name, FieldNode* field) {
  while (field != NULL) {
    int varListLen = field->varListLen;
    SymElem* varList = field->varSymList;
    if (findInVarList(name, 0, varListLen, varList) < 0) { // 当前没找到
      field = field->parent;
    } else { // 找到了
      return true;
    }
  }
  return false;
}

/* 新建基本类型（int/float） */
Type* createBasicType(char* typeStr) {
  Type* basicType = (Type*)malloc(sizeof(Type));
  basicType->isRight = false;
  if (strcmp("int", typeStr) == 0) { // int
    basicType->kind = T_INT;
  } else if (strcmp("float", typeStr) == 0) { // float
    basicType->kind = T_FLOAT;
  }
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

/* 新建结构体类型 */
Type* createStructType(char* name, TypeNode* typeNode) {
  Type* structType = (Type*)malloc(sizeof(Type));
  structType->kind = T_STRUCT;
  structType->structure.node = typeNode;
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
Function* createFunction(char* name, bool isDefined, Type* returnType, TypeNode* paramNode) {
  Function* func = (Function*)malloc(sizeof(Function));
  func->name = name;
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
  field->varSymList = (SymElem*)malloc(field->varListLen * sizeof(SymElem));
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

/* 检查两个类型是否一致 */
bool typeEquals(Type* type1, Type* type2) {

}

/* 检查两个函数参数类型是否一致 */
bool paramEquals(TypeNode* param1, TypeNode* param2) {

}
