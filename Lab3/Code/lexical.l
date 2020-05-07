/* 读取行号 */
%option yylineno

%{
  /* yylval类型，必须定义在 include "syntax.tab.h" 之前 */
  #define YYSTYPE Node*

  #include "Tree.h"
  #include "syntax.tab.h"

  /* 行号列号信息 */
	int yycolumn = 1;
	#define YY_USER_ACTION \
	yylloc.first_line = yylloc.last_line = yylineno; \
	yylloc.first_column = yycolumn; \
	yylloc.last_column = yycolumn + yyleng - 1; \
	yycolumn += yyleng;

%}

/* Definitions */
ws       [ \t]+
nl       "\n"|"\r\n"|"\r"
digit    [0-9]
letter_  [A-Za-z_]
id       {letter_}({letter_}|{digit})*
uint16   0[Xx][0-9A-Fa-f]+
uint8    0[0-7]+
uint     0|([1-9]{digit}*)
ufloatp  {digit}+\.{digit}+
ufloats  ({ufloatp}|({digit}+\.)|(\.{digit}+)|({digit}+))[eE][+-]?{digit}+
ufloat   {ufloatp}|{ufloats}

/* 识别一些特定错误格式的词元 */
err_uint16   0[Xx][0-9A-Za-z]+
err_uint8    0{digit}+
err_ufloat   ({digit}|\.)+([eE][+-]?)?({digit}|\.)*
err_id       ({letter_}|{digit})+

%% /* Rules */

{ws} {
}

{nl} {
  yycolumn = 1;
}

"//" {
  char c = input();
	while (c != '\n') {
    c = input();
  }
  yycolumn = 1;
}

"/*" { // 寻找 */
  // 0 -- 1(*) -- 2(*/)
  int state = 0;
  int commentLen = 0;
  while (state != 2) {
    char c = input();
    if (!c || c == EOF) { // 文件结束时未读到注释结束符
      // yyless(commentLen); // 回退到缓冲区
      setError();
      fprintf(stderr, "Error type A at Line %d (to %d): Missing Comment End \"*/\"\n", yylloc.first_line, yylineno);
      break;
    }
    commentLen += 1;
    if (state == 0 && c == '*') { state = 1; }
    else if (state == 1) {
      if (c == '/') { state = 2; }
      else if (c == '*') { state = 1; }
      else { state = 0; }
    }
  }
}

";" {
  yylval = createTerminalNode(TN_SEMI, yylloc.first_line, yylloc.first_column, NO_VAL);
  return SEMI;
}

"," {
  yylval = createTerminalNode(TN_COMMA, yylloc.first_line, yylloc.first_column, NO_VAL);
  return COMMA;
}

"==" {
  yylval = createTerminalNode(TN_EQ, yylloc.first_line, yylloc.first_column, NO_VAL);
  return EQ;
}

"!=" {
  yylval = createTerminalNode(TN_NE, yylloc.first_line, yylloc.first_column, NO_VAL);
  return NE;
}

"<=" {
  yylval = createTerminalNode(TN_LE, yylloc.first_line, yylloc.first_column, NO_VAL);
  return LE;
}

">=" {
  yylval = createTerminalNode(TN_GE, yylloc.first_line, yylloc.first_column, NO_VAL);
  return GE;
}

"<" {
  yylval = createTerminalNode(TN_LT, yylloc.first_line, yylloc.first_column, NO_VAL);
  return LT;
}

">" {
  yylval = createTerminalNode(TN_GT, yylloc.first_line, yylloc.first_column, NO_VAL);
  return GT;
}

"=" {
  yylval = createTerminalNode(TN_ASSIGNOP, yylloc.first_line, yylloc.first_column, NO_VAL);
  return ASSIGNOP;
}

"+" {
  yylval = createTerminalNode(TN_PLUS, yylloc.first_line, yylloc.first_column, NO_VAL);
  return PLUS;
}

"-" {
  yylval = createTerminalNode(TN_MINUS, yylloc.first_line, yylloc.first_column, NO_VAL);
  return MINUS;
}

"*" {
  yylval = createTerminalNode(TN_STAR, yylloc.first_line, yylloc.first_column, NO_VAL);
  return STAR;
}

"/" {
  yylval = createTerminalNode(TN_DIV, yylloc.first_line, yylloc.first_column, NO_VAL);
  return DIV;
}

"&&" {
  yylval = createTerminalNode(TN_AND, yylloc.first_line, yylloc.first_column, NO_VAL);
  return AND;
}

"||" {
  yylval = createTerminalNode(TN_OR, yylloc.first_line, yylloc.first_column, NO_VAL);
  return OR;
}

"!" {
  yylval = createTerminalNode(TN_NOT, yylloc.first_line, yylloc.first_column, NO_VAL);
  return NOT;
}

"(" {
  yylval = createTerminalNode(TN_LP, yylloc.first_line, yylloc.first_column, NO_VAL);
  return LP;
}

")" {
  yylval = createTerminalNode(TN_RP, yylloc.first_line, yylloc.first_column, NO_VAL);
  return RP;
}

"[" {
  yylval = createTerminalNode(TN_LB, yylloc.first_line, yylloc.first_column, NO_VAL);
  return LB;
}

"]" {
  yylval = createTerminalNode(TN_RB, yylloc.first_line, yylloc.first_column, NO_VAL);
  return RB;
}

"{" {
  yylval = createTerminalNode(TN_LC, yylloc.first_line, yylloc.first_column, NO_VAL);
  return LC;
}

"}" {
  yylval = createTerminalNode(TN_RC, yylloc.first_line, yylloc.first_column, NO_VAL);
  return RC;
}

{uint16} {
  yylval = createTerminalNode(TN_INT, yylloc.first_line, yylloc.first_column, INT_VAL, (int)strtol(yytext, NULL, 16), yytext);
  return INT;
}

{uint8} {
  yylval = createTerminalNode(TN_INT, yylloc.first_line, yylloc.first_column, INT_VAL, (int)strtol(yytext, NULL, 8), yytext);
  return INT;
}

{uint} {
  yylval = createTerminalNode(TN_INT, yylloc.first_line, yylloc.first_column, INT_VAL, (int)strtol(yytext, NULL, 10), yytext);
  return INT;
}

{ufloat} {
  yylval = createTerminalNode(TN_FLOAT, yylloc.first_line, yylloc.first_column, FLOAT_VAL, atof(yytext), yytext);
  return FLOAT;
}

"." {
  yylval = createTerminalNode(TN_DOT, yylloc.first_line, yylloc.first_column, NO_VAL);
  return DOT;
}

"struct" {
  yylval = createTerminalNode(TN_STRUCT, yylloc.first_line, yylloc.first_column, NO_VAL);
  return STRUCT;
}

"return" {
  yylval = createTerminalNode(TN_RETURN, yylloc.first_line, yylloc.first_column, NO_VAL);
  return RETURN;
}

"if" {
  yylval = createTerminalNode(TN_IF, yylloc.first_line, yylloc.first_column, NO_VAL);
  return IF;
}

"else" {
  yylval = createTerminalNode(TN_ELSE, yylloc.first_line, yylloc.first_column, NO_VAL);
  return ELSE;
}

"while" {
  yylval = createTerminalNode(TN_WHILE, yylloc.first_line, yylloc.first_column, NO_VAL);
  return WHILE;
}

"int"|"float" {
  yylval = createTerminalNode(TN_TYPE, yylloc.first_line, yylloc.first_column, TYPE_VAL, yytext);
  return TYPE;
}

{id} {
  yylval = createTerminalNode(TN_ID, yylloc.first_line, yylloc.first_column, ID_VAL, yytext);
  return ID;
}

{err_uint16} {
  setError();
	fprintf(stderr, "Error type A at Line %d: Illegal hexadecimal number \'%s\'\n", yylloc.first_line, yytext);
  return INT;
}

{err_uint8} {
  setError();
	fprintf(stderr, "Error type A at Line %d: Illegal octal number \'%s\'\n", yylloc.first_line, yytext);
  return INT;
}

{err_ufloat} {
  setError();
	fprintf(stderr, "Error type A at Line %d: Illegal floating point number \'%s\'\n", yylloc.first_line, yytext);
  return FLOAT;
}

{err_id} {
  setError();
	fprintf(stderr, "Error type A at Line %d: Illegal id \'%s\'\n", yylloc.first_line, yytext);
  return ID;
}

. {
  setError();
	fprintf(stderr, "Error type A at Line %d: Mysterious characters \'%s\'\n", yylloc.first_line, yytext);
}
%% /* User Subroutines */
