%locations

%{
	#include "lex.yy.c"
	extern int yyget_debug(void);
	void yyerror(const char* msg);
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

/* 处理IF-ELSE冲突 */
%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%% /* Rules */

/* Program是初始语法单元， 表示整个程序 */
Program : ExtDefList {
		$$ = createNonTerminalNode(NTN_PROGRAM, @$.first_line, @$.first_column, 1, $1);
		root = $$;
	}
	;
/* ExtDefList表示零个或多个ExtDef */
ExtDefList : /* empty */ {
		$$ = createNonTerminalNode(NTN_EXTDEFLIST, @$.first_line, @$.first_column, 0);
	}
	| ExtDef ExtDefList {
		$$ = createNonTerminalNode(NTN_EXTDEFLIST, @$.first_line, @$.first_column, 2, $1, $2);
	}
	;
/* ExtDef表示一个全局变量、结构体或函数的定义 */
ExtDef : Specifier ExtDecList SEMI { /* 任意类型全局变量(包括结构体)，禁止初始化 */
		$$ = createNonTerminalNode(NTN_EXTDEF, @$.first_line, @$.first_column, 3, $1, $2, $3);
	}
	| Specifier SEMI { /* 结构体 */
		$$ = createNonTerminalNode(NTN_EXTDEF, @$.first_line, @$.first_column, 2, $1, $2);
	}
	| Specifier FunDec CompSt { /* 函数定义 = 返回类型 + 函数头 + {...} */
		$$ = createNonTerminalNode(NTN_EXTDEF, @$.first_line, @$.first_column, 3, $1, $2, $3);
	}
	| Specifier FunDec SEMI { /* 函数声明 = 返回类型 + 函数头 + ; */
		$$ = createNonTerminalNode(NTN_EXTDEF, @$.first_line, @$.first_column, 3, $1, $2, $3);
	}
	/* 错误恢复 */
	| Specifier ExtDecList error SEMI {}
	| Specifier error SEMI {}
	| Specifier FunDec error SEMI {}
	;
/* ExtDecList表示零个或多个对一个全局变量的定义VarDec（禁止初始化） */
ExtDecList : VarDec {
		$$ = createNonTerminalNode(NTN_EXTDECLIST, @$.first_line, @$.first_column, 1, $1);
	}
	| VarDec COMMA ExtDecList {
		$$ = createNonTerminalNode(NTN_EXTDECLIST, @$.first_line, @$.first_column, 3, $1, $2, $3);
	}
	;
/* Specifier是类型描述符，基本类型TYPE，或结构体类型StructSpecifier */
Specifier : TYPE {
		$$ = createNonTerminalNode(NTN_SPECIFIER, @$.first_line, @$.first_column, 1, $1);
	}
	| StructSpecifier {
		$$ = createNonTerminalNode(NTN_SPECIFIER, @$.first_line, @$.first_column, 1, $1);
	}
	;
/* StructSpecifier表示结构体类型，结构体名OptTag可为空或为ID */
StructSpecifier : STRUCT OptTag LC DefList RC {
		$$ = createNonTerminalNode(NTN_STRUCTSPECIFIER, @$.first_line, @$.first_column, 5, $1, $2, $3, $4, $5);
	}
	| STRUCT Tag {
		$$ = createNonTerminalNode(NTN_STRUCTSPECIFIER, @$.first_line, @$.first_column, 2, $1, $2);
	}
	/* 错误恢复 */
	| STRUCT OptTag LC DefList error RC {}
	;
OptTag : /* empty */ {
		$$ = createNonTerminalNode(NTN_OPTTAG, @$.first_line, @$.first_column, 0);
	}
	| ID {
		$$ = createNonTerminalNode(NTN_OPTTAG, @$.first_line, @$.first_column, 1, $1);
	}
	;
Tag : ID {
		$$ = createNonTerminalNode(NTN_TAG, @$.first_line, @$.first_column, 1, $1);
	}
	;
/* VarDec表示对一个变量的定义（禁止初始化） */
VarDec : ID {
		$$ = createNonTerminalNode(NTN_VARDEC, @$.first_line, @$.first_column, 1, $1);
	}
	| VarDec LB INT RB {/* 数组变量 */
		$$ = createNonTerminalNode(NTN_VARDEC, @$.first_line, @$.first_column, 4, $1, $2, $3, $4);
	}
	/* 错误恢复 */
	| VarDec LB INT error RB {}
	;
/* FunDec表示对一个函数头的定义 */
FunDec : ID LP VarList RP {
		$$ = createNonTerminalNode(NTN_FUNDEC, @$.first_line, @$.first_column, 4, $1, $2, $3, $4);
	}
	| ID LP RP {
		$$ = createNonTerminalNode(NTN_FUNDEC, @$.first_line, @$.first_column, 3, $1, $2, $3);
	}
	/* 错误恢复 */
	| ID LP VarList error RP {}
	| ID LP error RP {}
	;
/* VarList包括一个或多个形参ParamDec */
VarList : ParamDec COMMA VarList {
		$$ = createNonTerminalNode(NTN_VARLIST, @$.first_line, @$.first_column, 3, $1, $2, $3);
	}
	| ParamDec {
		$$ = createNonTerminalNode(NTN_VARLIST, @$.first_line, @$.first_column, 1, $1);
	}
	;
/* ParamDec是对一个形参的定义，为类型描述符+变量名 */
ParamDec : Specifier VarDec {
		$$ = createNonTerminalNode(NTN_PARAMDEC, @$.first_line, @$.first_column, 2, $1, $2);
	}
	;
/* CompSt表示一个由一对花括号括起来的语句块，其中全部局部变量的定义必须在全部语句之前 */
CompSt : LC DefList StmtList RC {
		$$ = createNonTerminalNode(NTN_COMPST, @$.first_line, @$.first_column, 4, $1, $2, $3, $4);
	}
	/* 错误恢复 */
	| LC DefList StmtList error RC {}
	;
/* StmtList表示零个或多个Stmt的组合 */
StmtList : /* empty */ {
		$$ = createNonTerminalNode(NTN_STMTLIST, @$.first_line, @$.first_column, 0);
	}
	| Stmt StmtList {
		$$ = createNonTerminalNode(NTN_STMTLIST, @$.first_line, @$.first_column, 2, $1, $2);
	}
	;
/* Stmt表示一条语句 */
Stmt : Exp SEMI {
		$$ = createNonTerminalNode(NTN_STMT, @$.first_line, @$.first_column, 2, $1, $2);
	}
	| CompSt {
		$$ = createNonTerminalNode(NTN_STMT, @$.first_line, @$.first_column, 1, $1);
	}
	| RETURN Exp SEMI {
		$$ = createNonTerminalNode(NTN_STMT, @$.first_line, @$.first_column, 3, $1, $2, $3);
	}
	| IF LP Exp RP Stmt %prec LOWER_THAN_ELSE {
		$$ = createNonTerminalNode(NTN_STMT, @$.first_line, @$.first_column, 5, $1, $2, $3, $4, $5);
	}
	| IF LP Exp RP Stmt ELSE Stmt {
		$$ = createNonTerminalNode(NTN_STMT, @$.first_line, @$.first_column, 7, $1, $2, $3, $4, $5, $6, $7);
	}
	| WHILE LP Exp RP Stmt {
		$$ = createNonTerminalNode(NTN_STMT, @$.first_line, @$.first_column, 5, $1, $2, $3, $4, $5);
	}
	/* 错误恢复 */
	| Exp error SEMI {}
	| RETURN Exp error SEMI {}
	| IF LP Exp error RP Stmt %prec LOWER_THAN_ELSE {}
	| IF LP Exp error RP Stmt ELSE Stmt {}
	| WHILE LP Exp error RP Stmt {}
	;
/* DefList表示零个或多个局部变量的定义Def */
DefList : /* empty */ {
		$$ = createNonTerminalNode(NTN_DEFLIST, @$.first_line, @$.first_column, 0);
	}
	| Def DefList {
		$$ = createNonTerminalNode(NTN_DEFLIST, @$.first_line, @$.first_column, 2, $1, $2);
	}
	;
/* Def表示一条局部变量定义，允许在定义时进行初始化 */
Def : Specifier DecList SEMI {
		$$ = createNonTerminalNode(NTN_DEF, @$.first_line, @$.first_column, 3, $1, $2, $3);
	}
	/* 错误恢复 */
	| Specifier DecList error SEMI {}
	;
DecList : Dec {
		$$ = createNonTerminalNode(NTN_DECLIST, @$.first_line, @$.first_column, 1, $1);
	}
	| Dec COMMA DecList {
		$$ = createNonTerminalNode(NTN_DECLIST, @$.first_line, @$.first_column, 3, $1, $2, $3);
	}
	;
Dec : VarDec {
		$$ = createNonTerminalNode(NTN_DEC, @$.first_line, @$.first_column, 1, $1);
	}
	| VarDec ASSIGNOP Exp {
		$$ = createNonTerminalNode(NTN_DEC, @$.first_line, @$.first_column, 3, $1, $2, $3);
	}
	;
/* Exp表示表达式，包括：
 * (1) 二元运算表达式：赋值、逻辑与或、关系表达式(RELOP)、四则运算；
 * (2) 一元运算表达式：括号表达式、取负、逻辑非；
 * (3) 不含运算符的特殊表达式：函数调用、数组访问、结构体访问；
 * (4) 最基本的表达式：整型常数、浮点型常数、普通变量 */
Exp : Exp ASSIGNOP Exp {
		$$ = createNonTerminalNode(NTN_EXP, @$.first_line, @$.first_column, 3, $1, $2, $3);
	}
	| Exp AND Exp {
		$$ = createNonTerminalNode(NTN_EXP, @$.first_line, @$.first_column, 3, $1, $2, $3);
	}
	| Exp OR Exp {
		$$ = createNonTerminalNode(NTN_EXP, @$.first_line, @$.first_column, 3, $1, $2, $3);
	}
	| Exp EQ/*RELOP*/ Exp {
		$$ = createNonTerminalNode(NTN_EXP, @$.first_line, @$.first_column, 3, $1, $2, $3);
	}
	| Exp NE/*RELOP*/ Exp {
		$$ = createNonTerminalNode(NTN_EXP, @$.first_line, @$.first_column, 3, $1, $2, $3);
	}
	| Exp LE/*RELOP*/ Exp {
		$$ = createNonTerminalNode(NTN_EXP, @$.first_line, @$.first_column, 3, $1, $2, $3);
	}
	| Exp GE/*RELOP*/ Exp {
		$$ = createNonTerminalNode(NTN_EXP, @$.first_line, @$.first_column, 3, $1, $2, $3);
	}
	| Exp LT/*RELOP*/ Exp {
		$$ = createNonTerminalNode(NTN_EXP, @$.first_line, @$.first_column, 3, $1, $2, $3);
	}
	| Exp GT/*RELOP*/ Exp {
		$$ = createNonTerminalNode(NTN_EXP, @$.first_line, @$.first_column, 3, $1, $2, $3);
	}
	| Exp PLUS Exp {
		$$ = createNonTerminalNode(NTN_EXP, @$.first_line, @$.first_column, 3, $1, $2, $3);
	}
	| Exp MINUS Exp {
		$$ = createNonTerminalNode(NTN_EXP, @$.first_line, @$.first_column, 3, $1, $2, $3);
	}
	| Exp STAR Exp {
		$$ = createNonTerminalNode(NTN_EXP, @$.first_line, @$.first_column, 3, $1, $2, $3);
	}
	| Exp DIV Exp {
		$$ = createNonTerminalNode(NTN_EXP, @$.first_line, @$.first_column, 3, $1, $2, $3);
	}
	| LP Exp RP {
		$$ = createNonTerminalNode(NTN_EXP, @$.first_line, @$.first_column, 3, $1, $2, $3);
	}
	| MINUS Exp {
		$$ = createNonTerminalNode(NTN_EXP, @$.first_line, @$.first_column, 2, $1, $2);
	}
	| NOT Exp {
		$$ = createNonTerminalNode(NTN_EXP, @$.first_line, @$.first_column, 2, $1, $2);
	}
	| ID LP Args RP {/* 函数调用 */
		$$ = createNonTerminalNode(NTN_EXP, @$.first_line, @$.first_column, 4, $1, $2, $3, $4);
	}
	| ID LP RP {/* 函数调用 */
		$$ = createNonTerminalNode(NTN_EXP, @$.first_line, @$.first_column, 3, $1, $2, $3);
	}
	| Exp LB Exp RB {/* 数组访问 */
		$$ = createNonTerminalNode(NTN_EXP, @$.first_line, @$.first_column, 4, $1, $2, $3, $4);
	}
	| Exp DOT ID {/* 结构体访问 */
		$$ = createNonTerminalNode(NTN_EXP, @$.first_line, @$.first_column, 3, $1, $2, $3);
	}
	| ID {
		$$ = createNonTerminalNode(NTN_EXP, @$.first_line, @$.first_column, 1, $1);
	}
	| INT {
		$$ = createNonTerminalNode(NTN_EXP, @$.first_line, @$.first_column, 1, $1);
	}
	| FLOAT {
		$$ = createNonTerminalNode(NTN_EXP, @$.first_line, @$.first_column, 1, $1);
	}
	/* 错误恢复 */
	| LP Exp error RP {}
	| ID LP Args error RP {}
	| ID LP error RP {}
	| Exp LB Exp error RB {}
	;
/* Args表示实参列表，用于函数调用表达式，每个实参都可以变成一个表达式Exp */
Args : Exp COMMA Args {
		$$ = createNonTerminalNode(NTN_ARGS, @$.first_line, @$.first_column, 3, $1, $2, $3);
	}
	| Exp {
		$$ = createNonTerminalNode(NTN_ARGS, @$.first_line, @$.first_column, 1, $1);
	}
	;

%%

void yyerror(const char* msg) {
	setError();
  fprintf(stderr, "Error type B at Line %d: %s. ", yylloc.first_line, msg);
}
