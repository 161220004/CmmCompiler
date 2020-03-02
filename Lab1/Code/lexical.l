/* 读取行号 */
%option yylineno

%{
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
type     (int)|(float)
id       {letter_}({letter_}|{digit})*
uint16   0[Xx][0-9A-Fa-f]+
uint8    0[0-7]+
uint     0|([1-9]{digit}*)
ufloat   {digit}+(\.{digit}+)?([eE][+-]?{digit}+)?

%% /* Rules */

{ws} {
  if (yyget_debug()) printf("[%s] WS at line %d\n", yytext, yylineno);
}

{nl} {
  if (yyget_debug()) printf("[\\n] NewLine at line %d\n", yylineno);
  yycolumn = 1;
}

"//" {
  if (yyget_debug()) printf("[%s] Comment(One Line) at line %d: \"//", yytext, yylineno);
  char c = input();
	while (c != '\n') {
    if (yyget_debug()) printf("%c", c);
    c = input();
  }
  if (yyget_debug()) printf("\"\n");
}

"/*" { // 寻找 */
  if (yyget_debug()) printf("[%s] Comment(Lines) at line %d: \n\"/*", yytext, yylineno);
  // 0 -- 1(*) -- 2(*/)
  int state = 0;
  int commentLen = 0;
  char c = input();
  while (state != 2) {
    if (!c) { // 文件结束时未读到注释结束符
      // yyless(commentLen); // 回退到缓冲区
      printf("Error type A at Line %d: Absent of Comment End \"*/\"\n", yylineno);
      break;
    }
    commentLen += 1;
    if (state == 0 && c == '*') { state = 1; }
    else if (state == 1) {
      if (c == '/') { state = 2; }
      else if (c == '*') { state = 1; }
      else { state = 0; }
    }
    if (yyget_debug()) printf("%c", c);
    if (state == 2) { if (yyget_debug()) printf("\"\n"); }
    else c = input();
  }
}

";" {
  if (yyget_debug()) printf("[%s] Semi at line %d\n", yytext, yylineno);
  return SEMI;
}

"," {
  if (yyget_debug()) printf("[%s] Comma at line %d\n", yytext, yylineno);
  return COMMA;
}

"==" {
  if (yyget_debug()) printf("[%s] Relop EQ at line %d\n", yytext, yylineno);
  return EQ;
}

"!=" {
  if (yyget_debug()) printf("[%s] Relop NE at line %d\n", yytext, yylineno);
  return NE;
}

"<=" {
  if (yyget_debug()) printf("[%s] Relop LE at line %d\n", yytext, yylineno);
  return LE;
}

">=" {
  if (yyget_debug()) printf("[%s] Relop GE at line %d\n", yytext, yylineno);
  return GE;
}

"<" {
  if (yyget_debug()) printf("[%s] Relop LT at line %d\n", yytext, yylineno);
  return LT;
}

">" {
  if (yyget_debug()) printf("[%s] Relop GT at line %d\n", yytext, yylineno);
  return GT;
}

"=" {
  if (yyget_debug()) printf("[%s] Assign Operation at line %d\n", yytext, yylineno);
  return ASSIGNOP;
}

"+" {
  if (yyget_debug()) printf("[%s] Plus Operation at line %d\n", yytext, yylineno);
  return PLUS;
}

"-" {
  if (yyget_debug()) printf("[%s] Minus Operation at line %d\n", yytext, yylineno);
  return MINUS;
}

"*" {
  if (yyget_debug()) printf("[%s] Star Operation at line %d\n", yytext, yylineno);
  return STAR;
}

"/" {
  if (yyget_debug()) printf("[%s] Div Operation at line %d\n", yytext, yylineno);
  return DIV;
}

"&&" {
  if (yyget_debug()) printf("[%s] And at line %d\n", yytext, yylineno);
  return AND;
}

"||" {
  if (yyget_debug()) printf("[%s] Or at line %d\n", yytext, yylineno);
  return OR;
}

"!" {
  if (yyget_debug()) printf("[%s] Not at line %d\n", yytext, yylineno);
  return NOT;
}

"(" {
  if (yyget_debug()) printf("[%s] LP at line %d\n", yytext, yylineno);
  return LP;
}

")" {
  if (yyget_debug()) printf("[%s] RP at line %d\n", yytext, yylineno);
  return RP;
}

"[" {
  if (yyget_debug()) printf("[%s] LB at line %d\n", yytext, yylineno);
  return LB;
}

"]" {
  if (yyget_debug()) printf("[%s] RB at line %d\n", yytext, yylineno);
  return RB;
}

"{" {
  if (yyget_debug()) printf("[%s] LC at line %d\n", yytext, yylineno);
  return LC;
}

"}" {
  if (yyget_debug()) printf("[%s] RC at line %d\n", yytext, yylineno);
  return RC;
}

{uint16} {
  if (yyget_debug()) printf("[%s] Hex Int %d at line %d\n", yytext, (int)strtol(yytext, NULL, 16), yylineno);
  yylval = (int)strtol(yytext, NULL, 16);
  return INT;
}

{uint8} {
  if (yyget_debug()) printf("[%s] Oct Int %d at line %d\n", yytext, (int)strtol(yytext, NULL, 8), yylineno);
  yylval = (int)strtol(yytext, NULL, 8);
  return INT;
}

{uint} {
  if (yyget_debug()) printf("[%s] Dec Int %d at line %d\n", yytext, (int)strtol(yytext, NULL, 10), yylineno);
  yylval = (int)strtol(yytext, NULL, 10);
  return INT;
}

{ufloat} {
  if (yyget_debug()) printf("[%s] Float %f at line %d\n", yytext, atof(yytext), yylineno);
  yylval = atof(yytext);
  return FLOAT;
}

"." {
  if (yyget_debug()) printf("[%s] Dot at line %d\n", yytext, yylineno);
  return DOT;
}

"struct" {
  if (yyget_debug()) printf("[%s] Struct at line %d\n", yytext, yylineno);
  return STRUCT;
}

"return" {
  if (yyget_debug()) printf("[%s] Return at line %d\n", yytext, yylineno);
  return RETURN;
}

"if" {
  if (yyget_debug()) printf("[%s] If at line %d\n", yytext, yylineno);
  return IF;
}

"else" {
  if (yyget_debug()) printf("[%s] Else at line %d\n", yytext, yylineno);
  return ELSE;
}

"while" {
  if (yyget_debug()) printf("[%s] While at line %d\n", yytext, yylineno);
  return WHILE;
}

{type} {
  if (yyget_debug()) printf("[%s] Type at line %d\n", yytext, yylineno);
  return TYPE;
}

{id} {
  if (yyget_debug()) printf("[%s] ID %s at line %d\n", yytext, yytext, yylineno);
  return ID;
}

. {
	printf("Error type A at Line %d: Mysterious characters \'%s\'\n", yylineno, yytext);
}
%% /* User Subroutines */
