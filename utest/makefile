CXX = g++
AR = ar

INCLUDES = -I../ -I./
TSTDIR = ../
TARGETS = AmosUnitTest
TSTOBJS = $(TSTDIR)timer_q.o $(TSTDIR)basedef.o $(TSTDIR)reactor_epoll.o $(TSTDIR)reactor_poll.o 
OBJS = TimerTest.o TimerQTest.o demultiplex_stub.o ReactorImplTest.o main.o
LIBS = gtest.a
LDFLAGS = -lrt

CXXFLAGS = -g -Wall $(INCLUDES)
#编译可执行程序
EXETARGETS = $(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS) $(LDFLAGS)
#编译静态库文件
LIBTARGETS = $(AR) -rv $@ $^
#编译动态库文件，动态库文件名必须为libxxx.so，后使用-lxxx包含库文件
SHAREDTARGETS = $(CXX) $(CXXFLAGS) -shared -fPIC -o $@ $(SOURCES)
#编译规则，默认编译可执行文件
MAKETARGETS = $(EXETARGETS)

#防止all clean文件时，make all或者make clean执行失败
.PHONY: all clean $(TARGETS)

all : $(TARGETS)

$(TARGETS) : $(TSTOBJS) $(OBJS)
	$(MAKETARGETS)

#生成依赖文件，头文件修改时重新编译依赖源文件
%.d:%.c
	@set -e; rm -f $@; \
	$(CC) -M $(CXXFLAGS) $< > $@.$$$$;\
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@;\
	rm -f $@.$$$$

%.d:%.cpp
	@set -e; rm -f $@; \
	$(CC) -M $(CXXFLAGS) $< > $@.$$$$;\
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@;\
	rm -f $@.$$$$

#将依赖文件加入makefile中
sinclude $(OBJS:.o=.d)

%.o:%.c
	$(CXX) $(CXXFLAGS) -c $< -o $@
%o:%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJS) $(TARGETS) *.d* *.o $(TSTDIR)*.o

