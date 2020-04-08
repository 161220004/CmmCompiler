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
