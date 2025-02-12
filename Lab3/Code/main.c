#include <stdio.h>

// 声明外部变量
extern void yyrestart (FILE *input_file);
extern int yyparse(void);
extern void yyset_debug(int);
extern void showDetail();
extern void setLabFlag(int n);
extern void setProcessFlag(int n);
extern void printTree();
extern void semanticAnalysis();
extern void generateIR(char* fileName);

int main(int argc, char** argv)
{
  if (argc <= 1) return 1;
  // 是否DEBUG环境
  // yyset_debug(1);
  // 第几次实验
  setLabFlag(3);
  // 默认只分析一个文件
  FILE* f = fopen(argv[1], "r");
  if (!f) {
    perror(argv[1]);
    return 1;
  }
  yyrestart(f);
  yyparse();
  setProcessFlag(1);
  printTree();
  setProcessFlag(2);
  semanticAnalysis();
  setProcessFlag(3);
  if (argc > 2) {
    generateIR(argv[2]);
  } else {
    generateIR(NULL);
  }
  setProcessFlag(4);
  return 0;
}
