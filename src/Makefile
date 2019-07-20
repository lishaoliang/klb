# 编译命令 : make

SHELL = /bin/bash
PWD = `pwd`


# 引入一些环境参数
-include ../mk_def
MY_LIB_PATH = ../$(MY_LIB_NAME)
MY_GOPATH = $(PWD)/..:$(MY_HOME_GOPATH)


# 从目录检索需要编译的go文件
MY_DIRS := ./main


# 编译选项
#MY_CFLAGS := $(my_flags)
#MY_CFLAGS += -std=c99


# 引用头文件
MY_INCLUDES := -I ../inc -I $(MY_LIB_PATH)


# 引用的静态库
MY_LIB_STATIC := -L $(MY_LIB_PATH) -Bstatic

# 引用的动态库
MY_LIB_DYNAMIC := -L $(MY_LIB_PATH) -Bdynamic


# 编译目标名称
MY_TARGET_NAME := klua
MY_TARGET_EXE := $(MY_LIB_PATH)/$(MY_TARGET_NAME)
MY_TARGET_A := $(MY_LIB_PATH)/libklua_go.a
MY_TARGET_SO := $(MY_LIB_PATH)/libklua_go.so


# 所有编译文件
MY_FIND_FILES_C = $(wildcard $(dir)/*.go)
MY_SOURCES = $(foreach dir, $(MY_DIRS), $(MY_FIND_FILES_C))


MY_LIB_A_OBJS := $(addsuffix .o, $(MY_SOURCES))
MY_A_PARAMS := $(MY_INCLUDES) $(MY_CFLAGS) $(MY_LIB_STATIC) $(MY_LIB_DYNAMIC)

MY_LIB_SO_OBJS := $(addsuffix .oo, $(MY_SOURCES))
MY_SO_PARAMS := -fPIC $(MY_INCLUDES) $(MY_CFLAGS) $(MY_LIB_STATIC) $(MY_LIB_DYNAMIC) 


# 编译静态库时候,使compiler为每个function和data item分配独立的section
MY_LIB_MINI = -ffunction-sections -fdata-sections

# 编译动态库或执行档时,使compiler删除所有未被使用的function和data,即编译之后的文件最小化
MY_LINK_MINI = -Wl,--gc-sections


# 静态链接所有库
MY_EXE_LIBS = -lklua_go -lklb_c -lpthread -ldl -lm


.PHONY: all clean

all: exe

exe: $(MY_TARGET_EXE)
lib: $(MY_TARGET_A)
so: $(MY_TARGET_SO)

%.go.o: %.go
#	$(CC) $(MY_A_PARAMS) $(MY_LIB_MINI) -c -o $@ $<

%.go.oo: %.go
#	$(CC) $(MY_SO_PARAMS) -c -o $@ $<

$(MY_TARGET_EXE):
	$(my_tip)
#	$(CC) -o $@ $(MY_LIB_A_OBJS) $(MY_A_PARAMS) $(MY_LINK_MINI)
	CGO_ENABLED=1 GOOS=$(MY_OS) GOARCH=$(MY_ARCH) GOPATH=$(MY_GOPATH) CC=$(CC) CXX=$(CXX) go build $(MY_GO_STATIC) -a -o $@ main

$(MY_TARGET_A):
	$(my_tip)
#	$(CAR) rs $(MY_TARGET_A) $(MY_LIB_A_OBJS)
#	$(CRANLIB) $(MY_TARGET_A)
	CGO_ENABLED=1 GOOS=$(MY_OS) GOARCH=$(MY_ARCH) GOPATH=$(MY_GOPATH) CC=$(CC) CXX=$(CXX) go build -buildmode=c-archive -a -o $@ main
	$(CC) -o $(MY_TARGET_EXE) ./main.c $(MY_A_PARAMS) $(MY_LIB_MINI) $(MY_EXE_LIBS) $(MY_GCC_STATIC)

$(MY_TARGET_SO):
	$(my_tip)
#	$(CC) -shared -fPIC -o $@ $(MY_LIB_SO_OBJS) $(MY_SO_PARAMS) $(CXX_LINK_MINI)

clean:
	@echo "++++++ make clean ++++++"
	@echo "+ MY_DIRS = $(MY_DIRS)"
	@echo "++ RM = $(RM)"
#	$(RM) $(MY_LIB_A_OBJS)
#	$(RM) $(MY_LIB_SO_OBJS)
	$(RM) $(MY_TARGET_EXE)
	$(RM) $(MY_TARGET_A)
	$(RM) $(MY_TARGET_SO)
	@echo "+++++++++++++++++++++++++"


define my_tip
	@echo "++++++ make tip ++++++"
	@echo "+ CC = $(CC)"
	@echo "+ CXX = $(CXX)"
	@echo "+ MY_SOURCES = $(MY_SOURCES)"
	@echo "+ MY_DIRS = $(MY_DIRS)"
	@echo "+ MY_CFLAGS = $(MY_CFLAGS)"
	@echo "+ MY_TARGET_EXE = $(MY_TARGET_EXE)"
	@echo "+ MY_TARGET_A = $(MY_TARGET_A)"
	@echo "+ MY_TARGET_SO = $(MY_TARGET_SO)"
	@echo "++++++++++++++++++++++"
endef
