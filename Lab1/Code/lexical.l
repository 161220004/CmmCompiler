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
afterp   \.{digit}+
scientf  [eE][+-]?{digit}+
ufloat   {uint}({afterp}|{scientf}|({afterp}{scientf}))

/* 识别一些特定错误格式的词元 */
err_uint16   0[Xx][0-9A-Za-z]+
err_uint8    0{digit}+
err_ufloat   ({digit}|\.)+([eE][+-]?)?({digit}|\.)*
err_id       ({letter_}|{digit})+

%% /* Rules */

{ws} {
  if (yyget_debug()) printf("\t[%s] WS at line %d (%d)\n", yytext, yylloc.first_line, yylloc.first_column);
}

{nl} {
  if (yyget_debug()) printf("\t[\\n] NewLine at line %d\n", yylloc.first_line - 1);
  yycolumn = 1;
}

"//" {
  if (yyget_debug()) printf("\t[%s] Comment(One Line) at line %d: \"//", yytext, yylloc.first_line);
  char c = input();
	while (c != '\n') {
    if (yyget_debug()) printf("%c", c);
    c = input();
  }
  if (yyget_debug()) printf("\"\n");
}

"/*" { // 寻找 */
  if (yyget_debug()) printf("\t[%s] Comment(Lines) at line %d: \n\"/*", yytext, yylloc.first_line);
  // 0 -- 1(*) -- 2(*/)
  int state = 0;
  int commentLen = 0;
  char c = input();
  while (state != 2) {
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
    if (yyget_debug()) printf("%c", c);
    if (state == 2) { if (yyget_debug()) printf("\"\n"); }
    else c = input();
  }
}

";" {
  if (yyget_debug()) printf("\t[%s] Semi at line %d (%d)\n", yytext, yylloc.first_line, yylloc.first_column);
  yylval = createTerminalNode("SEMI", yylloc.first_line, yylloc.first_column, NO_VAL);
  return SEMI;
}

"," {
  if (yyget_debug()) printf("\t[%s] Comma at line %d (%d)\n", yytext, yylloc.first_line, yylloc.first_column);
  yylval = createTerminalNode("COMMA", yylloc.first_line, yylloc.first_column, NO_VAL);
  return COMMA;
}

"==" {
  if (yyget_debug()) printf("\t[%s] Relop EQ at line %d (%d)\n", yytext, yylloc.first_line, yylloc.first_column);
  yylval = createTerminalNode("EQ", yylloc.first_line, yylloc.first_column, NO_VAL);
  return EQ;
}

"!=" {
  if (yyget_debug()) printf("\t[%s] Relop NE at line %d (%d)\n", yytext, yylloc.first_line, yylloc.first_column);
  yylval = createTerminalNode("NE", yylloc.first_line, yylloc.first_column, NO_VAL);
  return NE;
}

"<=" {
  if (yyget_debug()) printf("\t[%s] Relop LE at line %d (%d)\n", yytext, yylloc.first_line, yylloc.first_column);
  yylval = createTerminalNode("LE", yylloc.first_line, yylloc.first_column, NO_VAL);
  return LE;
}

">=" {
  if (yyget_debug()) printf("\t[%s] Relop GE at line %d (%d)\n", yytext, yylloc.first_line, yylloc.first_column);
  yylval = createTerminalNode("GE", yylloc.first_line, yylloc.first_column, NO_VAL);
  return GE;
}

"<" {
  if (yyget_debug()) printf("\t[%s] Relop LT at line %d (%d)\n", yytext, yylloc.first_line, yylloc.first_column);
  yylval = createTerminalNode("LT", yylloc.first_line, yylloc.first_column, NO_VAL);
  return LT;
}

">" {
  if (yyget_debug()) printf("\t[%s] Relop GT at line %d (%d)\n", yytext, yylloc.first_line, yylloc.first_column);
  yylval = createTerminalNode("GT", yylloc.first_line, yylloc.first_column, NO_VAL);
  return GT;
}

"=" {
  if (yyget_debug()) printf("\t[%s] Assign Operation at line %d (%d)\n", yytext, yylloc.first_line, yylloc.first_column);
  yylval = createTerminalNode("ASSIGNOP", yylloc.first_line, yylloc.first_column, NO_VAL);
  return ASSIGNOP;
}

"+" {
  if (yyget_debug()) printf("\t[%s] Plus Operation at line %d (%d)\n", yytext, yylloc.first_line, yylloc.first_column);
  yylval = createTerminalNode("PLUS", yylloc.first_line, yylloc.first_column, NO_VAL);
  return PLUS;
}

"-" {
  if (yyget_debug()) printf("\t[%s] Minus Operation at line %d (%d)\n", yytext, yylloc.first_line, yylloc.first_column);
  yylval = createTerminalNode("MINUS", yylloc.first_line, yylloc.first_column, NO_VAL);
  return MINUS;
}

"*" {
  if (yyget_debug()) printf("\t[%s] Star Operation at line %d (%d)\n", yytext, yylloc.first_line, yylloc.first_column);
  yylval = createTerminalNode("STAR", yylloc.first_line, yylloc.first_column, NO_VAL);
  return STAR;
}

"/" {
  if (yyget_debug()) printf("\t[%s] Div Operation at line %d (%d)\n", yytext, yylloc.first_line, yylloc.first_column);
  yylval = createTerminalNode("DIV", yylloc.first_line, yylloc.first_column, NO_VAL);
  return DIV;
}

"&&" {
  if (yyget_debug()) printf("\t[%s] And at line %d (%d)\n", yytext, yylloc.first_line, yylloc.first_column);
  yylval = createTerminalNode("AND", yylloc.first_line, yylloc.first_column, NO_VAL);
  return AND;
}

"||" {
  if (yyget_debug()) printf("\t[%s] Or at line %d (%d)\n", yytext, yylloc.first_line, yylloc.first_column);
  yylval = createTerminalNode("OR", yylloc.first_line, yylloc.first_column, NO_VAL);
  return OR;
}

"!" {
  if (yyget_debug()) printf("\t[%s] Not at line %d (%d)\n", yytext, yylloc.first_line, yylloc.first_column);
  yylval = createTerminalNode("NOT", yylloc.first_line, yylloc.first_column, NO_VAL);
  return NOT;
}

"(" {
  if (yyget_debug()) printf("\t[%s] LP at line %d (%d)\n", yytext, yylloc.first_line, yylloc.first_column);
  yylval = createTerminalNode("LP", yylloc.first_line, yylloc.first_column, NO_VAL);
  return LP;
}

")" {
  if (yyget_debug()) printf("\t[%s] RP at line %d (%d)\n", yytext, yylloc.first_line, yylloc.first_column);
  yylval = createTerminalNode("RP", yylloc.first_line, yylloc.first_column, NO_VAL);
  return RP;
}

"[" {
  if (yyget_debug()) printf("\t[%s] LB at line %d (%d)\n", yytext, yylloc.first_line, yylloc.first_column);
  yylval = createTerminalNode("LB", yylloc.first_line, yylloc.first_column, NO_VAL);
  return LB;
}

"]" {
  if (yyget_debug()) printf("\t[%s] RB at line %d (%d)\n", yytext, yylloc.first_line, yylloc.first_column);
  yylval = createTerminalNode("RB", yylloc.first_line, yylloc.first_column, NO_VAL);
  return RB;
}

"{" {
  if (yyget_debug()) printf("\t[%s] LC at line %d (%d)\n", yytext, yylloc.first_line, yylloc.first_column);
  yylval = createTerminalNode("LC", yylloc.first_line, yylloc.first_column, NO_VAL);
  return LC;
}

"}" {
  if (yyget_debug()) printf("\t[%s] RC at line %d (%d)\n", yytext, yylloc.first_line, yylloc.first_column);
  yylval = createTerminalNode("RC", yylloc.first_line, yylloc.first_column, NO_VAL);
  return RC;
}

{uint16} {
  if (yyget_debug()) printf("\t[%s] Hex Int %d at line %d (%d)\n", yytext, (int)strtol(yytext, NULL, 16), yylloc.first_line, yylloc.first_column);
  yylval = createTerminalNode("INT", yylloc.first_line, yylloc.first_column, INT_VAL, (int)strtol(yytext, NULL, 16));
  return INT;
}

{uint8} {
  if (yyget_debug()) printf("\t[%s] Oct Int %d at line %d (%d)\n", yytext, (int)strtol(yytext, NULL, 8), yylloc.first_line, yylloc.first_column);
  yylval = createTerminalNode("INT", yylloc.first_line, yylloc.first_column, INT_VAL, (int)strtol(yytext, NULL, 8));
  return INT;
}

{uint} {
  if (yyget_debug()) printf("\t[%s] Dec Int %d at line %d (%d)\n", yytext, (int)strtol(yytext, NULL, 10), yylloc.first_line, yylloc.first_column);
  yylval = createTerminalNode("INT", yylloc.first_line, yylloc.first_column, INT_VAL, (int)strtol(yytext, NULL, 10));
  return INT;
}

{ufloat} {
  if (yyget_debug()) printf("\t[%s] Float %f at line %d (%d)\n", yytext, atof(yytext), yylloc.first_line, yylloc.first_column);
  yylval = createTerminalNode("FLOAT", yylloc.first_line, yylloc.first_column, FLOAT_VAL, atof(yytext));
  return FLOAT;
}

"." {
  if (yyget_debug()) printf("\t[%s] Dot at line %d (%d)\n", yytext, yylloc.first_line, yylloc.first_column);
  yylval = createTerminalNode("DOT", yylloc.first_line, yylloc.first_column, NO_VAL);
  return DOT;
}

"struct" {
  if (yyget_debug()) printf("\t[%s] Struct at line %d (%d)\n", yytext, yylloc.first_line, yylloc.first_column);
  yylval = createTerminalNode("STRUCT", yylloc.first_line, yylloc.first_column, NO_VAL);
  return STRUCT;
}

"return" {
  if (yyget_debug()) printf("\t[%s] Return at line %d (%d)\n", yytext, yylloc.first_line, yylloc.first_column);
  yylval = createTerminalNode("RETURN", yylloc.first_line, yylloc.first_column, NO_VAL);
  return RETURN;
}

"if" {
  if (yyget_debug()) printf("\t[%s] If at line %d (%d)\n", yytext, yylloc.first_line, yylloc.first_column);
  yylval = createTerminalNode("IF", yylloc.first_line, yylloc.first_column, NO_VAL);
  return IF;
}

"else" {
  if (yyget_debug()) printf("\t[%s] Else at line %d (%d)\n", yytext, yylloc.first_line, yylloc.first_column);
  yylval = createTerminalNode("ELSE", yylloc.first_line, yylloc.first_column, NO_VAL);
  return ELSE;
}

"while" {
  if (yyget_debug()) printf("\t[%s] While at line %d (%d)\n", yytext, yylloc.first_line, yylloc.first_column);
  yylval = createTerminalNode("WHILE", yylloc.first_line, yylloc.first_column, NO_VAL);
  return WHILE;
}

"int"|"float" {
  if (yyget_debug()) printf("\t[%s] Type at line %d (%d)\n", yytext, yylloc.first_line, yylloc.first_column);
  yylval = createTerminalNode("TYPE", yylloc.first_line, yylloc.first_column, TYPE_VAL, yytext);
  return TYPE;
}

{id} {
  if (yyget_debug()) printf("\t[%s] ID %s at line %d (%d)\n", yytext, yytext, yylloc.first_line, yylloc.first_column);
  yylval = createTerminalNode("ID", yylloc.first_line, yylloc.first_column, ID_VAL, yytext);
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
