# GNU make手册：http://www.gnu.org/software/make/manual/make.html
# ************ 遇到不明白的地方请google以及阅读手册 *************

# 编译器设定和编译选项
CC = gcc
FLEX = flex
BISON = bison
CFLAGS = -std=c99

# 编译目标：src目录下的所有.c文件
CFILES = $(shell find ./ -name "*.c")
OBJS = $(CFILES:.c=.o)
LFILE = $(shell find ./ -name "*.l")
YFILE = $(shell find ./ -name "*.y")
LFC = $(shell find ./ -name "*.l" | sed s/[^/]*\\.l/lex.yy.c/)
YFC = $(shell find ./ -name "*.y" | sed s/[^/]*\\.y/syntax.tab.c/)
LFO = $(LFC:.c=.o)
YFO = $(YFC:.c=.o)

parser: syntax $(filter-out $(LFO),$(OBJS))
	$(CC) -o parser $(filter-out $(LFO),$(OBJS)) -lfl -ly

syntax: lexical syntax-c
	$(CC) -c $(YFC) -o $(YFO)

lexical: $(LFILE)
	$(FLEX) -o $(LFC) $(LFILE)

syntax-c: $(YFILE)
	$(BISON) -o $(YFC) -d -v $(YFILE)

-include $(patsubst %.o, %.d, $(OBJS))

# 定义的一些伪目标
.PHONY: clean test
test:
	./parser ../Test/test.cmm ../Result/test.ir
testall:
	./parser ../Test/test1.cmm ../Result/test1.ir
	./parser ../Test/test2.cmm ../Result/test2.ir
	./parser ../Test/test3.cmm ../Result/test3.ir
	./parser ../Test/test4.cmm ../Result/test4.ir
	./parser ../Test/test5.cmm ../Result/test5.ir
	./parser ../Test/test6.cmm ../Result/test6.ir
	./parser ../Test/test7.cmm ../Result/test7.ir
	./parser ../Test/test8.cmm ../Result/test8.ir
	./parser ../Test/test9.cmm ../Result/test9.ir
	./parser ../Test/test10.cmm ../Result/test10.ir
	./parser ../Test/test11.cmm ../Result/test11.ir
	./parser ../Test/test12.cmm ../Result/test12.ir
	./parser ../Test/test13.cmm ../Result/test13.ir
	./parser ../Test/test14.cmm ../Result/test14.ir
	./parser ../Test/test15.cmm ../Result/test15.ir
	./parser ../Test/test16.cmm ../Result/test16.ir
	./parser ../Test/test17.cmm ../Result/test17.ir
	./parser ../Test/test18.cmm ../Result/test18.ir
clean:
	rm -f parser lex.yy.c syntax.tab.c syntax.tab.h syntax.output
	rm -f $(OBJS) $(OBJS:.o=.d)
	rm -f $(LFC) $(YFC) $(YFC:.c=.h)
	rm -f *~
