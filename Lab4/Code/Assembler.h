#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "IRUtils.h"

typedef enum { R_ZERO = 0, R_AT, R_V0, R_V1, R_A0, R_A1, R_A2, R_A3,
               R_T0, R_T1, R_T2, R_T3, R_T4, R_T5, R_T6, R_T7,
               R_S0, R_S1, R_S2, R_S3, R_S4, R_S5, R_S6, R_S7, R_T8, R_T9,
               R_K0, R_K1, R_GP, R_SP, R_FP, R_RA } RegName; // 32个

typedef struct Reg Reg;
typedef struct MemElem MemElem;

struct Reg {
  RegName name;
  char* str; // RegName对应的string
	bool isEmpty;
  bool isConst; // 是否是常数
  union {
    char* var; // 寄存器里是变量或临时变量
    int val; // 寄存器里是常数
  };
  int useBit; // 使用位，基本块内下一次使用的位置，用于判定清空哪个寄存器
};

struct MemElem {
  bool isNull;
  char* name;
  int offsetFP;
  bool isArray; // 是否是数组
};

/* 变量+临时变量内存位置表（有序数组） */
extern MemElem* memList;
extern int memListLen;
/* 寄存器 */
extern Reg regs[32];
extern int regNum;

void generateMIPS(char* fileName);
void generateHead();
void generateRead();
void generateWrite();
void initMemory();
void initRegs();
int ensure(Operand* op, InterCode* current);
int allocate(Operand* op, InterCode* current);
int spillFarthestReg(InterCode* current);
void spillFromReg(RegName reg);
void spillAllRegs();
void fillReg(RegName reg, bool isConst, char* var, int val);
bool isBlockHead(InterCode* code);
int IndexInRegs(Operand* op);
int EmptyInRegs();
int addToMemList(char* addVar, int offsetFP);
int findInMemList(char* name, int start, int end);

#endif
