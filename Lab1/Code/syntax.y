%locations

%{
	#include "lex.yy.c"
	extern int yyget_debug(void);
%}

/* Definitions */
%token SEMI                  /* ; */
%token COMMA                 /* , */
%token EQ NE LE GE LT GT     /* Relop */
%token ASSIGNOP              /* = */
%token PLUS MINUS STAR DIV   /* Operations */
%token AND OR NOT            /* Logic */
%token LP RP LB RB LC RC     /* ()[]{} */
%token INT
%token FLOAT
%token DOT
%token STRUCT
%token RETURN
%token IF ELSE
%token WHILE
%token TYPE
%token ID

/* 结合性，优先级（后 > 前） */
%right ASSIGNOP
%left OR
%left AND
%left EQ NE LE GE LT GT
%left PLUS MINUS
%left STAR DIV
%right NOT
%left LP RP LB RB DOT

%% /* Rules */

/* Program是初始语法单元， 表示整个程序 */
Program : ExtDefList { if (yyget_debug()) printf("-- Analyzed Program\n"); }
	;
/* ExtDefList表示零个或多个ExtDef */
ExtDefList : /* empty */ {}
	| ExtDef ExtDefList {}
	;
/* ExtDef表示一个全局变量、结构体或函数的定义 */
ExtDef : Specifier ExtDecList SEMI { if (yyget_debug()) printf("-- Analyzed Global Var\n"); }
	| Specifier SEMI { if (yyget_debug()) printf("-- Analyzed Struct\n"); }
	| Specifier FunDec CompSt { if (yyget_debug()) printf("-- Analyzed Function\n"); }
	;
/* ExtDecList表示零个或多个对一个变量的定义VarDec */
ExtDecList : VarDec {}
	| VarDec COMMA ExtDecList {}
	;
/* Specifier是类型描述符，基本类型TYPE，或结构体类型StructSpecifier */
Specifier : TYPE {}
	| StructSpecifier {}
	;
/* StructSpecifier表示结构体类型，结构体名OptTag可为空或为ID */
StructSpecifier : STRUCT OptTag LC DefList RC {}
	| STRUCT Tag {}
	;
OptTag : /* empty */ {}
	| ID {}
	;
Tag : ID {}
	;

/* VarDec表示对一个变量的定义 */
VarDec : ID {}
	| VarDec LB INT RB {/* 数组变量 */}
	;

/* FunDec表示对一个函数头的定义 */
FunDec : ID LP VarList RP {}
	| ID LP RP {}
	;
/* VarList包括一个或多个形参ParamDec */
VarList : ParamDec COMMA VarList {}
	| ParamDec {}
	;
/* ParamDec是对一个形参的定义，为类型描述符+变量名 */
ParamDec : Specifier VarDec {}
	;

/* CompSt表示一个由一对花括号括起来的语句块 */
CompSt : LC DefList StmtList RC { if (yyget_debug()) printf("-- Analyzed Block\n"); }
	;
/* StmtList表示零个或多个Stmt的组合 */
StmtList : /* empty */ {}
	| Stmt StmtList {}
	;
/* Stmt表示一条语句 */
Stmt : Exp SEMI { if (yyget_debug()) printf("-- Analyzed Exp\n"); }
	| CompSt {}
	| RETURN Exp SEMI {}
	| IF LP Exp RP Stmt { if (yyget_debug()) printf("-- Analyzed If\n"); }
	| IF LP Exp RP Stmt ELSE Stmt { if (yyget_debug()) printf("-- Analyzed If Else\n"); }
	| WHILE LP Exp RP Stmt { if (yyget_debug()) printf("-- Analyzed While\n"); }
	;

/* DefList表示零个或多个局部变量的定义Def */
DefList : /* empty */ {}
	| Def DefList {}
	;
/* Def表示一条局部变量定义，允许在定义时进行初始化 */
Def : Specifier DecList SEMI { if (yyget_debug()) printf("-- Analyzed Local Var\n"); }
	;
DecList : Dec {}
	| Dec COMMA DecList {}
	;
Dec : VarDec {}
	| VarDec ASSIGNOP Exp {}
	;

/* Exp表示表达式，包括：
 * (1) 二元运算表达式：赋值、逻辑与或、关系表达式(RELOP)、四则运算；
 * (2) 一元运算表达式：括号表达式、取负、逻辑非；
 * (3) 不含运算符的特殊表达式：函数调用、数组访问、结构体访问；
 * (4) 最基本的表达式：整型常数、浮点型常数、普通变量 */
Exp : Exp ASSIGNOP Exp { $1 = $3; }
	| Exp AND Exp { $$ = $1 && $3; }
	| Exp OR Exp { $$ = $1 || $3; }
	| Exp EQ/*RELOP*/ Exp { $$ = ($1 == $3); }
	| Exp NE/*RELOP*/ Exp { $$ = ($1 != $3); }
	| Exp LE/*RELOP*/ Exp { $$ = ($1 <= $3); }
	| Exp GE/*RELOP*/ Exp { $$ = ($1 >= $3); }
	| Exp LT/*RELOP*/ Exp { $$ = ($1 < $3); }
	| Exp GT/*RELOP*/ Exp { $$ = ($1 > $3); }
	| Exp PLUS Exp { $$ = $1 + $3; }
	| Exp MINUS Exp { $$ = $1 - $3; }
	| Exp STAR Exp { $$ = $1 * $3; }
	| Exp DIV Exp { $$ = $1 / $3; }
	| LP Exp RP {}
	| MINUS Exp {}
	| NOT Exp {}
	| ID LP Args RP {/* 函数调用 */}
	| ID LP RP {/* 函数调用 */}
	| Exp LB Exp RB {/* 数组访问 */}
	| Exp DOT ID {/* 结构体访问 */}
	| ID {}
	| INT {}
	| FLOAT {}
	;
/* Args表示实参列表，用于函数调用表达式，每个实参都可以变成一个表达式Exp */
Args : Exp COMMA Args {}
	| Exp {}
	;

%%

yyerror(char* msg) {
  fprintf(stderr, "Error: %s\n", msg);
}
