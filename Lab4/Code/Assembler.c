#include "Assembler.h"

/* 保存的文件 */
FILE* MIPSFile = NULL;

/* 变量+临时变量内存位置表（有序数组） */
MemElem* memList = NULL;
int memListLen = 0;

/* 寄存器 */
Reg regs[32];
int regNum = 32;
char* regStr[32] = {"$0", "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3",
                    "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",
                    "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7", "$t8", "$t9",
                    "$k0", "$k1", "$gp", "$sp", "$fp", "$ra"};
/* 指针位置 */
int esp = 0;
int ebp = 0;
int ebpLast = 0;

/* 生成MIPS32目标代码 */
void generateMIPS(char* fileName) {
  if (isError() || !isLab(4)) return;
  if (IRList == NULL) return;
  if (fileName == NULL) {
    MIPSFile = stdout;
  } else {
    MIPSFile = fopen(fileName, "w");
    if (MIPSFile == NULL) {
      fprintf(stderr, "Failed to Open File %s\n", fileName);
      return;
    }
  }
  initMemory();
  initRegs();
  generateHead();
  if (hasRead) generateRead();
  if (hasWrite) generateWrite();
  InterCode* IRCode = IRList;
  while (IRCode != NULL) {
    if (IRCode->kind == IR_LABEL) {
      spillAllRegs();
      if (yyget_debug()) fprintf(MIPSFile, "    # spill all after LABEL (start of Block)\n");
      fprintf(MIPSFile, "%s:\n", IRCode->name);
    } else if (IRCode->kind == IR_GOTO) {
      spillAllRegs();
      if (yyget_debug()) fprintf(MIPSFile, "    # spill all before GOTO (end of block)\n");
      fprintf(MIPSFile, "  j %s\n", IRCode->name);
    } else if (IRCode->kind == IR_READ) {
      if (yyget_debug()) fprintf(MIPSFile, "    # read %s start\n", IRCode->one.op->name);
			fprintf(MIPSFile, "  addi $sp, $sp, -4\n");
			fprintf(MIPSFile, "  sw $ra, 0($sp)\n");
			fprintf(MIPSFile, "  jal read\n");
			fprintf(MIPSFile, "  lw $ra, 0($sp)\n");
			fprintf(MIPSFile, "  addi $sp, $sp, 4\n");
      int readIndex = ensure(IRCode->one.op, IRCode);
			fprintf(MIPSFile, "  move %s, $v0\n", regs[readIndex].str);
      if (yyget_debug()) fprintf(MIPSFile, "    # read %s to Reg[%s] finished\n", IRCode->one.op->name, regs[readIndex].str);
    } else if (IRCode->kind == IR_WRITE) {
      if (yyget_debug()) fprintf(MIPSFile, "    # write %s start\n", IRCode->one.op->name);
      int writeIndex = IndexInRegs(IRCode->one.op);
      fprintf(MIPSFile, "  move $a0, %s\n", regs[writeIndex].str);
			fprintf(MIPSFile, "  addi $sp, $sp, -4\n");
			fprintf(MIPSFile, "  sw $ra, 0($sp)\n");
			fprintf(MIPSFile, "  jal write\n");
			fprintf(MIPSFile, "  lw $ra, 0($sp)\n");
			fprintf(MIPSFile, "  addi $sp, $sp, 4\n");
      if (yyget_debug()) fprintf(MIPSFile, "    # write %s from Reg[%s] finished\n", IRCode->one.op->name, regs[writeIndex].str);
    } else if (IRCode->kind == IR_FUNCTION) {
      fprintf(MIPSFile, "%s:\n", IRCode->name);
      if (strcmp(IRCode->name, "main") == 0) { // main函数
      }
      esp -= 4;
      fprintf(MIPSFile, "  addi $sp, $sp, -4\n");
      ebpLast = ebp;
			fprintf(MIPSFile, "  sw $fp, 0($sp)\n");
      ebp = esp;
			fprintf(MIPSFile, "  move $fp, $sp\n");
      if (yyget_debug()) fprintf(MIPSFile, "    # function %s inited\n", IRCode->name);
    } else if (IRCode->kind == IR_RETURN) {
      int returnIndex = ensure(IRCode->one.op, IRCode);
      fprintf(MIPSFile, "  move $v0, %s\n", regs[returnIndex].str);
      esp = ebp + 4;
			fprintf(MIPSFile, "  addi $sp, $fp, 4\n");
      ebp = ebpLast;
			fprintf(MIPSFile, "  lw $fp, 0($fp)\n");
			fprintf(MIPSFile, "  jr $ra\n");
      if (yyget_debug()) {
        if (IRCode->one.op->kind == OP_CONST) fprintf(MIPSFile, "    # return %d finished\n", IRCode->one.op->val);
        else fprintf(MIPSFile, "    # return %s finished\n", IRCode->one.op->name);
      }
    } else if (IRCode->kind == IR_PARAM) {
      int paramNum = 0;
      while (IRCode != NULL && IRCode->kind == IR_PARAM) {
        // 把之前保存的变量加载到寄存器
        int paramIndex = ensure(IRCode->one.op, IRCode);
        if (paramNum < 4) { // 从寄存器$a0等中加载
          fprintf(MIPSFile, "  move %s, $a%d\n", regs[paramIndex].str, paramNum);
        } else { // 从之前的栈中加载
          fprintf(MIPSFile, "  lw %s, %d($fp)\n", regs[paramIndex].str, 4/*ra*/ + 4 * (paramNum - 3));
        }
        paramNum += 1;
        IRCode = IRCode->next;
      }
      IRCode = IRCode->prev; // 保证当前是最后一个PARAM
    } else if (IRCode->kind == IR_CALL) {
      // 先加载实参值到寄存器 $a_
      int argNum = 0;
      InterCode* argCode = IRCode->prev;
      while (argCode != NULL && argCode->kind == IR_ARG) {
        Operand* argOp = argCode->one.op;
        int argIndex = ensure(argCode->one.op, argCode);
        if (argNum < 4) {
          fillReg(R_A0 + argNum, (argOp->kind == OP_CONST), argOp->name, argOp->val);
          fprintf(MIPSFile, "  move $a%d, %s\n", argNum, regs[argIndex].str);
        } else { // 先读到寄存器，再放入栈
          fprintf(MIPSFile, "  addi $sp, $sp, -4\n");
          fprintf(MIPSFile, "  sw %s, 0($sp)\n", regs[argIndex].str);
        }
        argNum += 1;
        argCode = argCode->prev;
      }
      // 再保存所有寄存器的值
      spillAllRegs();
      if (yyget_debug()) fprintf(MIPSFile, "    # spill all before CALL\n");
      // 开始调用函数
      fprintf(MIPSFile, "  addi $sp, $sp, -4\n");
			fprintf(MIPSFile, "  sw $ra, 0($sp)\n");
			fprintf(MIPSFile, "  jal %s\n", IRCode->call.funcName);
			fprintf(MIPSFile, "  lw $ra, 0($sp)\n");
			fprintf(MIPSFile, "  addi $sp, $sp, 4\n");
      if (argNum > 4) {
        fprintf(MIPSFile, "  addi $sp, $sp, %d\n", 4 * (argNum - 4)); // 至此，esp变化量为0
      }
      int returnIndex = ensure(IRCode->call.op, IRCode);
			fprintf(MIPSFile, "  move %s, $v0\n", regs[returnIndex].str);
    } else if (IRCode->kind == IR_IF) {
      spillFarthestReg(IRCode);
      if (yyget_debug()) fprintf(MIPSFile, "    # spill all before GOTO except IF relop\n");
      int leftIndex = ensure(IRCode->ifcode.op1, IRCode);
      int rightIndex = ensure(IRCode->ifcode.op2, IRCode);
      switch (IRCode->ifcode.relop) {
  			case EQ: fprintf(MIPSFile, "  beq "); break;
  			case NE: fprintf(MIPSFile, "  bne "); break;
  			case GE: fprintf(MIPSFile, "  bge "); break;
  			case GT: fprintf(MIPSFile, "  bgt "); break;
  			case LE: fprintf(MIPSFile, "  ble "); break;
  			case LT: fprintf(MIPSFile, "  blt "); break;
			}
      fprintf(MIPSFile, "%s, %s, %s\n", regs[leftIndex].str, regs[rightIndex].str, IRCode->ifcode.label);
    } else if (IRCode->kind == IR_DEC) {
      int addrIndex = ensure(IRCode->two.op1, IRCode); // 这里寄存器中是数组首地址
    } else if (IRCode->kind == IR_ASSIGN) {
      Operand* leftOp = IRCode->two.op1;
      int leftIndex = ensure(leftOp, IRCode);
      Operand* rightOp = IRCode->two.op2;
      int rightIndex = ensure(rightOp, IRCode);
      if (leftOp->kind == OP_GETCONT) { // 数组赋值，则左侧寄存器中是数组某个位置的地址
        fprintf(MIPSFile, "  sw %s, 0(%s)\n", regs[rightIndex].str, regs[leftIndex].str);
      } else { // 变量赋值
        fprintf(MIPSFile, "  move %s, %s\n", regs[leftIndex].str, regs[rightIndex].str);
      }
      if (yyget_debug()) {
        if (rightOp->kind == OP_CONST) fprintf(MIPSFile, "    # assign %s(Reg[%s]) <- %d(Reg[%s]) above\n", regs[leftIndex].var, regs[leftIndex].str, regs[rightIndex].val, regs[rightIndex].str);
        else fprintf(MIPSFile, "    # assign %s(Reg[%s]) <- %s(Reg[%s]) above\n", regs[leftIndex].var, regs[leftIndex].str, regs[rightIndex].var, regs[rightIndex].str);
      }
    } else if (IRCode->kind == IR_ADD || IRCode->kind == IR_SUB || IRCode->kind == IR_MUL || IRCode->kind == IR_DIV) {
      Operand* resultOp = IRCode->three.op1;
      int resultIndex = ensure(resultOp, IRCode);
      Operand* leftOp = IRCode->three.op2;
      int leftIndex = ensure(leftOp, IRCode);
      Operand* rightOp = IRCode->three.op3;
      int rightIndex = ensure(rightOp, IRCode);
      switch (IRCode->kind) {
        case IR_ADD: fprintf(MIPSFile, "  add %s, %s, %s\n", regs[resultIndex].str, regs[leftIndex].str, regs[rightIndex].str); break;
        case IR_SUB: fprintf(MIPSFile, "  sub %s, %s, %s\n", regs[resultIndex].str, regs[leftIndex].str, regs[rightIndex].str); break;
        case IR_MUL: fprintf(MIPSFile, "  mul %s, %s, %s\n", regs[resultIndex].str, regs[leftIndex].str, regs[rightIndex].str); break;
        case IR_DIV:
          fprintf(MIPSFile, "  div %s, %s\n", regs[leftIndex].str, regs[rightIndex].str);
				  fprintf(MIPSFile, "  mflo %s\n", regs[resultIndex].str);
          break;
        default: break;
      }
    }
    IRCode = IRCode->next;
  }
}

/* 返回变量/常数所在的寄存器 */
int ensure(Operand* op, InterCode* current) {
  int index = IndexInRegs(op);
  if (index > -1) { // 已经在寄存器中
    return index;
  } else { // 分配一个寄存器newIndex
    int newIndex = allocate(op, current);
    if (op->kind == OP_CONST) { // 常数
      fillReg(newIndex, true, NULL, op->val);
      if (yyget_debug()) fprintf(MIPSFile, "    # ensure: load %d to Reg[%s] below\n", op->val, regs[newIndex].str);
      fprintf(MIPSFile, "  li %s, %d\n", regs[newIndex].str, op->val);
    } else { // 变量
      int memIndex = findInMemList(op->name, 0, memListLen);
      if (memIndex == -1) { // 不在变量内存表内，则添加到变量内存表，并首次进入寄存器，不写入内存
        if (current->kind == IR_DEC || op->kind == OP_GETADDR) { // 是数组
          int size = current->two.op2->val; // 数组大小
          esp -= size;
          fprintf(MIPSFile, "  addi $sp, $sp, -%d\n", size); // 在栈中保留数组大小的空位
          memIndex = addToMemList(op->name, esp - ebp);
          memList[memIndex].isArray = true;
          if (yyget_debug()) fprintf(MIPSFile, "    # ensure: keep space at (%d)fp for array %s above\n", esp - ebp, op->name);
        } else { // 普通变量
          esp -= 4;
          fprintf(MIPSFile, "  addi $sp, $sp, -4\n");
          memIndex = addToMemList(op->name, esp - ebp);
          if (yyget_debug()) fprintf(MIPSFile, "    # ensure: keep space at (%d)fp for var %s above\n", esp - ebp, op->name);
        }
        // 写入空值到寄存器（仅占位），不必从内存加载
        fillReg(newIndex, false, memList[memIndex].name, 0);
      } else { // 已经在变量内存表内，内存中一定有（若内存中没有则一定在寄存器中，即index > -1）
        fillReg(newIndex, false, memList[memIndex].name, 0);
        // 从内存中加载进寄存器
        if (memList[memIndex].isArray) { // 数组，寄存器得到首地址
          fprintf(MIPSFile, "  addi %s, $fp, %d\n", regs[newIndex].str, memList[memIndex].offsetFP);
          if (yyget_debug()) fprintf(MIPSFile, "    # ensure: load array %s addr to Reg[%s] above\n", memList[memIndex].name, regs[newIndex].str);
        } else { // 变量，寄存器得到值
          fprintf(MIPSFile, "  lw %s, %d($fp)\n", regs[newIndex].str, memList[memIndex].offsetFP);
          if (yyget_debug()) fprintf(MIPSFile, "    # ensure: load var %s to Reg[%s] above\n", memList[memIndex].name, regs[newIndex].str);
        }
      }
    }
    return newIndex;
  }
}

/* 为一个变量/常数新分配一个寄存器，返回新的寄存器 */
int allocate(Operand* op, InterCode* current) {
  int emptyIndex = EmptyInRegs();
  if (emptyIndex > -1) { // 找到空寄存器
    return emptyIndex;
  } else { // 全满，需要清空一个
    // 先看看是否有常数的寄存器，直接清空
    int constIndex = -1;
    for (int i = R_T0; i < R_S0; i++) {
      if (regs[i].isConst) {
        spillFromReg(i);
        constIndex = i;
      }
    }
    if (constIndex > -1) return constIndex;
    // 没有常数的寄存器，则溢出最近不使用的变量寄存器
    return spillFarthestReg(current);
  }
}

/* 把所有需要被溢出到内存的寄存器溢出到内存，返回其中的一个 */
int spillFarthestReg(InterCode* current) {
  // 清空使用位
  for (int i = R_T0; i < R_S0; i++) { regs[i].useBit = 0; }
  int useNo = 0; // 使用顺序
  for (int i = R_T0; i < R_S0; i++) {
    if (regs[i].isEmpty) continue;
    else if (regs[i].isConst) continue;
    InterCode* code = current;
    while (code != NULL && !isBlockHead(code) && code->kind != IR_FUNCTION) { // 还在基本块内（没进入下一个基本块）
      if (code->kind == IR_READ || code->kind == IR_WRITE || code->kind == IR_ARG || code->kind == IR_RETURN) {
        Operand* op = code->one.op;
        if (op->kind != OP_CONST && strcmp(op->name, regs[i].var) == 0) {
          useNo += 1;
          regs[i].useBit = useNo;
        }
      } else if (code->kind == IR_ASSIGN) {
        Operand* op1 = code->two.op1;
        Operand* op2 = code->two.op2;
        if ((op1->kind != OP_CONST && strcmp(op1->name, regs[i].var) == 0) ||
            (op2->kind != OP_CONST && strcmp(op2->name, regs[i].var) == 0)) {
          useNo += 1;
          regs[i].useBit = useNo;
        }
      } else if (code->kind == IR_DEC) {
        Operand* op = code->two.op1;
        if (op->kind != OP_CONST && strcmp(op->name, regs[i].var) == 0) {
          useNo += 1;
          regs[i].useBit = useNo;
        }
      } else if (code->kind == IR_ADD || code->kind == IR_SUB || code->kind == IR_MUL || code->kind == IR_DIV) {
        Operand* op1 = code->three.op1;
        Operand* op2 = code->three.op2;
        Operand* op3 = code->three.op3;
        if ((op1->kind != OP_CONST && strcmp(op1->name, regs[i].var) == 0) ||
            (op2->kind != OP_CONST && strcmp(op2->name, regs[i].var) == 0) ||
            (op3->kind != OP_CONST && strcmp(op3->name, regs[i].var) == 0)) {
          useNo += 1;
          regs[i].useBit = useNo;
        }
      } else if (code->kind == IR_IF) {
        Operand* op1 = code->ifcode.op1;
        Operand* op2 = code->ifcode.op2;
        if ((op1->kind != OP_CONST && strcmp(op1->name, regs[i].var) == 0) ||
            (op2->kind != OP_CONST && strcmp(op2->name, regs[i].var) == 0)) {
          useNo += 1;
          regs[i].useBit = useNo;
        }
      }
      code = code->next;
    }
  }
  if (yyget_debug()) {
    for (int i = R_T0; i < R_S0; i++) {
      if (regs[i].isEmpty) fprintf(MIPSFile, "    # Reg[%s]: -, use = %d\n", regs[i].str, regs[i].useBit);
      else {
        if (regs[i].isConst) fprintf(MIPSFile, "    # Reg[%s]: #%d, use = %d\n", regs[i].str, regs[i].val, regs[i].useBit);
        else fprintf(MIPSFile, "    # Reg[%s]: %s, use = %d\n", regs[i].str, regs[i].var, regs[i].useBit);
      }
    }
  }
  int farIndex = -1;
  // 溢出基本块内不再使用的变量（useBit = 0）
  for (int i = R_T0; i < R_S0; i++) {
    if (regs[i].useBit == 0) {
      spillFromReg(i);
      farIndex = i;
    }
  }
  if (farIndex > -1) return farIndex;
  // 没有则溢出最晚使用的变量（useBit = useNo）
  for (int i = R_T0; i < R_S0; i++) {
    if (regs[i].useBit == useNo) {
      spillFromReg(i);
      return i;
    }
  }
  // 没找到最晚使用的变量，不应出现此情况
  spillFromReg(R_T0);
  return R_T0;
}

/* 清空一个寄存器，溢出到内存 */
void spillFromReg(RegName reg) {
  if (regs[reg].isEmpty) return;
  if (regs[reg].isConst) { // 常数，直接清空
    regs[reg].isEmpty = true;
    if (yyget_debug()) fprintf(MIPSFile, "    # spill: const %d from Reg[%s]\n", regs[reg].val, regs[reg].str);
  } else { // 变量，溢出到内存
    int memIndex = findInMemList(regs[reg].var, 0, memListLen);
    if (memIndex == -1) { // 不应出现此情况
      fprintf(stderr, "Failed to Spill to %s\n", regs[reg].var);
      return;
    }
    regs[reg].isEmpty = true;
    if (memList[memIndex].isArray) { // 数组则不存到内存
      if (yyget_debug()) fprintf(MIPSFile, "    # spill: array %d from Reg[%s]\n", regs[reg].val, regs[reg].str);
    } else { // 普通变量
      fprintf(MIPSFile, "  sw %s, %d($fp)\n", regs[reg].str, memList[memIndex].offsetFP);
      if (yyget_debug()) fprintf(MIPSFile, "    # spill: var %s from Reg[%s] above\n", regs[reg].var, regs[reg].str);
    }
  }
}

/* 清空全部寄存器，溢出到内存 */
void spillAllRegs() {
  if (yyget_debug()) {
    for (int i = R_T0; i < R_S0; i++) {
      if (regs[i].isEmpty) fprintf(MIPSFile, "    # Reg[%s]: -\n", regs[i].str);
      else {
        if (regs[i].isConst) fprintf(MIPSFile, "    # Reg[%s]: #%d\n", regs[i].str, regs[i].val);
        else fprintf(MIPSFile, "    # Reg[%s]: %s\n", regs[i].str, regs[i].var);
      }
    }
  }
  for (int i = R_T0; i < R_S0; i++) {
    regs[i].useBit = 0;
    spillFromReg(i);
  }
}

/* 填充一个临时变量寄存器 */
void fillReg(RegName reg, bool isConst, char* var, int val) {
  regs[reg].isEmpty = false;
  regs[reg].isConst = isConst;
  if (isConst) {
    regs[reg].val = val;
  } else {
    regs[reg].var = var;
  }
}

/* 是否是一个块的首句 */
bool isBlockHead(InterCode* code) {
  if (code == NULL) return true;
  if (code->kind == IR_LABEL) return true; // 跳转指令的目标指令
  if (code->prev == NULL) return true; // 第一条指令
  else if (code->prev->kind == IR_GOTO || code->prev->kind == IR_IF) return true; // 跳转指令的下一条指令
  return false;
}

/* 判断寄存器里是否已经有某变量/常数，若有则返回寄存器，否则返回-1 */
int IndexInRegs(Operand* op) {
  for (int i = R_T0; i < R_S0; i++) {
    if (!regs[i].isEmpty) {
      if (op->kind == OP_CONST) { // 常数
        if (regs[i].isConst && op->val == regs[i].val) return i;
      } else {
        if (!regs[i].isConst && strcmp(op->name, regs[i].var) == 0) return i;
      }
    }
  }
  return -1;
}

/* 判断是否有空的寄存器，若有则返回寄存器，否则返回-1 */
int EmptyInRegs() {
  for (int i = R_T0; i < R_S0; i++) {
    if (regs[i].isEmpty) return i;
  }
  return -1;
}

/* 添加一个元素到内存变量表（无重复），返回添加的位置 */
int addToMemList(char* addVar, int offsetFP) {
  for (int i = 0; i < memListLen; i++) {
    if (memList[i].isNull) { // 发现空项（只能是最后一项）
      memList[i].isNull = false;
      memList[i].name = addVar;
      memList[i].offsetFP = offsetFP;
      return i; // 结束
    } else { // 比较大小，保持有序
      int result = strcmp(addVar, memList[i].name);
      if (result < 0) { // 新函数名小于i处函数名，则i之后全部后移出空位给i
        for (int j = memListLen - 2; j >= i; j--) {
          if (!memList[j].isNull) {
            memList[j + 1].isNull = memList[j].isNull;
            memList[j + 1].name = memList[j].name;
            memList[j + 1].offsetFP = memList[j].offsetFP;
          }
        }
        memList[i].isNull = false;
        memList[i].name = addVar;
        memList[i].offsetFP = offsetFP;
        return i; // 结束
      }
    }
  }
  return -1; // 不应出现此情况
}

/* 获取变量在内存变量表中的下标；没有则返回-1 */
int findInMemList(char* name, int start, int end) { // end是最后一个下标+1
  if (start >= end) return -1;
  if (memList == NULL) return -1;
  int mid = (start + end) / 2;
  int result = (memList[mid].isNull) ? (-1) : strcmp(name, memList[mid].name);
  if (result == 0) {
    return mid;
  } else if (result < 0) { // 查询的值小于mid值
    return findInMemList(name, start, mid);
  } else { // 查询的值大于mid值
    return findInMemList(name, mid + 1, end);
  }
}

/* 初始化内存位置记录 */
void initMemory() {
  memListLen = interVarNum + tempCount;
  memList = (MemElem*)malloc(memListLen * sizeof(MemElem));
  for (int i = 0; i < memListLen; i++) {
    memList[i].isNull = true;
    memList[i].isArray = false;
  }
}

/* 初始化寄存器 */
void initRegs() {
  for (int i = 0; i < regNum; i++) {
    regs[i].name = i;
    regs[i].str = regStr[i];
    regs[i].isEmpty = true;
  }
}

/* 生成头部代码 */
void generateHead() {
  fprintf(MIPSFile, ".data\n");
  fprintf(MIPSFile, "_prompt: .asciiz \"Enter an integer:\"\n");
  fprintf(MIPSFile, "_ret: .asciiz \"\\n\"\n");
  fprintf(MIPSFile, ".globl main\n.text\n");
}

/* 生成read代码 */
void generateRead() {
  fprintf(MIPSFile, "read:\n");
  fprintf(MIPSFile, "  li $v0, 4\n");
  fprintf(MIPSFile, "  la $a0, _prompt\n");
  fprintf(MIPSFile, "  syscall\n");
  fprintf(MIPSFile, "  li $v0, 5\n");
  fprintf(MIPSFile, "  syscall\n");
  fprintf(MIPSFile, "  jr $ra\n");
}

/* 生成write代码 */
void generateWrite() {
  fprintf(MIPSFile, "write:\n");
  fprintf(MIPSFile, "  li $v0, 1\n");
  fprintf(MIPSFile, "  syscall\n");
  fprintf(MIPSFile, "  li $v0, 4\n");
  fprintf(MIPSFile, "  la $a0, _ret\n");
  fprintf(MIPSFile, "  syscall\n");
  fprintf(MIPSFile, "  move $v0, $s0\n");
  fprintf(MIPSFile, "  jr $ra\n");
}
