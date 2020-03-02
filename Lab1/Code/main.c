#include <stdio.h>
#include <stdlib.h>

// 声明外部变量
extern FILE* yyin;
extern int yylineno;
extern int yylex(void);
extern void yyrestart (FILE *input_file);
extern int yyparse(void);
extern void yyset_debug (int);

int main(int argc, char** argv)
{
  if (argc <= 1) return 1;
  // 是否DEBUG环境
  yyset_debug(1);
  // 默认只分析一个文件
  FILE* f = fopen(argv[1], "r");
  if (!f) {
    perror(argv[1]);
    return 1;
  }
  yyrestart(f);
  yyparse();
  return 0;
}
