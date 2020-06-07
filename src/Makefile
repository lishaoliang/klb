# make

SHELL = /bin/bash
PWD = `pwd`


# include make param
-include ../make_define
MY_LIB_PATH = ../$(MY_LIB_NAME)
MY_BIN_PATH = ../$(MY_LIB_NAME)
MY_GOPATH = $(PWD)/../../../../../
MY_MAIN = github.com/lishaoliang/klb/src/main


# go main


# target
MY_TARGET_NAME := klb
MY_TARGET_EXE := $(MY_BIN_PATH)/$(MY_TARGET_NAME)
MY_TARGET_A := $(MY_LIB_PATH)/lib$(MY_TARGET_NAME).a
MY_TARGET_SO := $(MY_LIB_PATH)/lib$(MY_TARGET_NAME).so

ifeq ($(MY_OS), windows)
MY_TARGET_NAME := klb
MY_TARGET_EXE := $(MY_BIN_PATH)/$(MY_TARGET_NAME).exe
MY_TARGET_A := $(MY_LIB_PATH)/lib$(MY_TARGET_NAME).a
MY_TARGET_SO := $(MY_LIB_PATH)/lib$(MY_TARGET_NAME).dll
endif

# static libs
MY_EXE_LIBS = -lklua_go -lklb_c -lpthread -ldl -lm

#MY_GO_TAGS = -tags="debug"
#MY_GO_TAGS = 

.PHONY: all clean

all: exe

exe: $(MY_TARGET_EXE)
lib: $(MY_TARGET_A)
so: $(MY_TARGET_SO)

$(MY_TARGET_EXE):
	$(my_tip)
#	$(CC) -o $@ $(MY_LIB_A_OBJS) $(MY_A_PARAMS) $(MY_LINK_MINI)
	CGO_ENABLED=1 GOOS=$(MY_OS) GOARCH=$(MY_ARCH) GOPATH=$(MY_GOPATH) CC=$(CC) CXX=$(CXX) go build $(MY_GO_TAGS) $(MY_GO_STATIC) -o $@ $(MY_MAIN)

$(MY_TARGET_A):
	$(my_tip)
#	$(CAR) rs $(MY_TARGET_A) $(MY_LIB_A_OBJS)
#	$(CRANLIB) $(MY_TARGET_A)
	CGO_ENABLED=1 GOOS=$(MY_OS) GOARCH=$(MY_ARCH) GOPATH=$(MY_GOPATH) CC=$(CC) CXX=$(CXX) go build $(MY_GO_TAGS) -buildmode=c-archive -o $@ main
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
	@echo "+ MY_MAIN = $(MY_MAIN)"
	@echo "+ MY_TARGET_EXE = $(MY_TARGET_EXE)"
	@echo "+ MY_TARGET_A = $(MY_TARGET_A)"
	@echo "+ MY_TARGET_SO = $(MY_TARGET_SO)"
	@echo "++++++++++++++++++++++"
endef
