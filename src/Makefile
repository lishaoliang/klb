# 编译命令 : make

SHELL = /bin/bash
PWD = `pwd`


# 引入一些环境参数
-include ../mk_def
MY_LIB_PATH = ../$(MY_LIB_NAME)
MY_GOPATH = $(PWD)/../../../../../:$(MY_WORK_GOPATH):$(MY_HOME_GOPATH)
MY_MAIN = github.com/lishaoliang/klb/src/main


# 编译目标名称
MY_TARGET_NAME := klua
MY_TARGET_EXE := $(MY_LIB_PATH)/$(MY_TARGET_NAME)
MY_TARGET_A := $(MY_LIB_PATH)/libklua_go.a
MY_TARGET_SO := $(MY_LIB_PATH)/libklua_go.so

# 静态链接所有库
MY_EXE_LIBS = -lklua_go -lklb_c -lpthread -ldl -lm

.PHONY: all clean

all: exe

exe: $(MY_TARGET_EXE)
lib: $(MY_TARGET_A)
so: $(MY_TARGET_SO)

$(MY_TARGET_EXE):
	$(my_tip)
#	$(CC) -o $@ $(MY_LIB_A_OBJS) $(MY_A_PARAMS) $(MY_LINK_MINI)
	CGO_ENABLED=1 GOOS=$(MY_OS) GOARCH=$(MY_ARCH) GOPATH=$(MY_GOPATH) CC=$(CC) CXX=$(CXX) go build $(MY_GO_STATIC) -a -o $@ $(MY_MAIN)

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
	$(RM) $(MY_TARGET_EXE)
	$(RM) $(MY_TARGET_A)
	$(RM) $(MY_TARGET_SO)
	@echo "+++++++++++++++++++++++++"


define my_tip
	@echo "++++++ make tip ++++++"
	@echo "+ CC = $(CC)"
	@echo "+ CXX = $(CXX)"
	@echo "+ MY_TARGET_EXE = $(MY_TARGET_EXE)"
	@echo "+ MY_TARGET_A = $(MY_TARGET_A)"
	@echo "+ MY_TARGET_SO = $(MY_TARGET_SO)"
	@echo "++++++++++++++++++++++"
endef
