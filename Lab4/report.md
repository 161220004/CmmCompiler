### 实验报告（四）

- 曹洋笛  161220004  2904428882@qq.com

#### 一、实现功能

- 将实验三中得到的中间代码经过指令选择、寄存器选择以及栈管理之后，转换为MIPS32汇编代码



#### 二、亮点

1. 良好的代码结构：

   封装了多个实用函数，包括寻找可用寄存器的allocate函数、把变量从内存加载到寄存器的ensure函数、把变量从寄存器溢出到内存的spillFromReg函数等等。

   

2. 使用帧指针 \$fp (ebp)：

   每个函数在栈上占据一块空间（活动记录），使用 \$fp 保存这个活动记录的底部，如图：

   | 函数调用时的栈空间 | 指针   |
   | :----------------: | ------ |
   |     Argument 5     |        |
   |     Argument 6     |        |
   |    ebp Address     | = \$fp |
   |    Local Var 1     |        |
   |        ...         |        |
   |    Local Var n     | = \$sp |

   这样就可以使用 \$fp 来描述变量的位置，比如上表中的 Local Var 1 的位置就是 “**-4(\$fp)**”。

   

3. 内存变量对应表：

   该表的每个元素的结构请见下一节数据结构介绍的 “struct MemElem”。

   它记录了中间代码中出现的每个变量和临时变量所对应的在栈中应存储的位置（记录为属性值 offsetFP，即相对于帧指针 ebp(\$fp) 的偏移量），在每个变量第一次出现时，栈都为其预留一个空位（即 “addi \$sp, \$sp, -4”），并把此时的偏移量（\$sp - \$fp）保存到 offsetFP，这个位置不会改变（一个萝卜一个坑），这样，在寄存器溢出时就能根据这个属性直接找到该寄存器内的变量应该保存到的位置。

   

4. 数组在栈上存储：

   当使用 DEC 为数组 array 声明一个长为 size 的空间时，在栈上为其预留出长 size 的空间来保存数组的值，假设当前栈指针 esp、帧指针 ebp，且 esp - ebp = -x，则声明数组空间就是令当前栈指针 esp = esp - size，数组地址范围是 [ebp - x - size, ebp - x) 的区间，对于内存变量对应表中的元素 array，它的属性 offsetFP 的值就是数组首地址相对于帧指针 \$fp 的偏移。

   在加载数组array到寄存器时（即中间代码的 **&array**），直接加载首地址而不是数组元素，例如加载 &array 到寄存器 \$t0，读出 array 的属性值 offsetFP，则代码为：**addi \$t0, \$fp, offsetFP** ，这样寄存器里就是数组首地址，可以直接通过加一个数来得到数组某一个下标的地址。

   在从数组的某个元素位置 addr 取值时（即中间代码的 **\*addr**），使用 “sw”，例如当前寄存器 \$t0 中是变量 m，寄存器 \$t1 中是数组的某个元素的地址的临时变量 t9，则对于中间代码 “m = \*t9”，应该翻译成：**sw \$t0, 0(\$t1)** ，而对于中间代码 “\*t9 = m”，应该翻译成：**sw \$t0, 0(\$t1)** 。



#### 三、实现

1. 数据结构

- InterCode：表示一行中间代码

  ```c
  typedef enum { IR_ASSIGN, IR_ADD, IR_SUB, IR_MUL, IR_DIV, IR_LABEL, IR_FUNCTION, IR_RETURN, IR_PARAM, IR_ARG, IR_CALL, IR_READ, IR_WRITE, IR_IF, IR_GOTO, IR_DEC } IRKind; // 操作类型，即多个操作数的连接方式
  
  struct InterCode {
    IRKind kind;
  	union {
  		struct { Operand* op; } one; // 包括：RETURN, PARAM, ARG, READ, WRITE
  		struct { Operand* op1; Operand* op2; } two; // 包括：ASSIGN, DEC
  		struct { Operand* op1; Operand* op2; Operand* op3; } three; // 包括：ADD, SUB, MUL, DIV
  		struct { Operand* op; char* funcName; } call; // 包括：CALL
  		struct { Operand* op1; Relop relop; Operand* op2; char* label; } ifcode; // 包括：IF
  		char* name; // 包括：LABEL, FUNCTION, GOTO
  	};
    InterCode* prev;
    InterCode* next;
  };
  ```

  

- Operand：某些中间代码的组成部分

  ```c
  typedef enum { OP_VAR, OP_CONST, OP_TEMP, OP_ADDR, OP_GETADDR, OP_GETCONT } OpKind;
  
  struct Operand {
  	OpKind kind;
  	union {
  		char* name; // 变量名
  		int val; // Lab3只考虑整数
  	};
  };
  ```



- 实现思路：

  首先定义并在语义分析过程中调用一个函数负责把“read()”和“write()”函数写入函数符号表。

  生成中间代码时，遍历一遍语法树，每遇到一个节点Node就调用其对应的“translateNode()”函数，被调用的函数会返回该子节点下的全部中间代码的双向链表，而父节点也就是调用者负责把它所有子节点返回的几段中间代码连接起来，直到root节点获得了全部的中间代码。

  对于数组和结构体，添加了函数“translateStructAddr(Node* expNode, ..., Operand* place)”以及“translateArrayAddr(Node* expNode, ..., Operand* place)”。当一个Exp节点发现它是一个数组访问，则调用translateArrayAddr，结构体则调用translateStructAddr，这两个函数都是能够计算出访问的地址，并把这个地址填写进place中。

  对于实参，由于数组和结构体传递的是**地址**，将传递的Operand的kind置为**OP_ADDR**；当且仅当打印PARAM时，OP_ADDR将打印出“**&**”，且对于函数之中类型为数组或结构体的参数，在该PARAM相关的translateExp中，不使用**取址(&)**符号。



#### 四、编译

使用助教的Makefile可以成功编译。

运行时，可以选择：

- `./parser xxx.cmm` ：翻译成中间代码并打印在控制台
- `./parser xxx.cmm out.ir` ：翻译成中间代码并保存在.ir 文件中



#### 五、注意事项（一些私设）

- 如果出现语法/词法错误，将不进行中间代码分析
- 如果出现作用域导致的语义错误（因为本人实验二实现了作用域），不报错，且不影响中间代码的生成（因为中间代码都是全局）

