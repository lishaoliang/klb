# make

SHELL = /bin/bash
PWD = `pwd`


OS ?= linux
ARCH ?= amd64
TAGS ?= debug normal
MY_TOOL_CHAIN ?= 


# "linux", "windows", "darwin", "stm32", "js"
MY_OS := $(OS)

# "amd64", "386", "arm", "arm64", "wasm"
MY_ARCH := $(ARCH)

# go编译标签 -tags="debug normal"
MY_GO_TAGS := -tags="$(TAGS)"

# gcc/cgo静态链接项
MY_GCC_STATIC := -static
MY_CGO_STATIC := -ldflags -extldflags=-static

# go main 目录
MY_GOPATH := $(PWD)/../../../../../
MY_GO_MAIN := github.com/lishaoliang/klb/src/main


# gcc编译工具链
CC		:= $(MY_TOOL_CHAIN)gcc
CXX		:= $(MY_TOOL_CHAIN)g++
CAR		:= $(MY_TOOL_CHAIN)ar
CRANLIB	:= $(MY_TOOL_CHAIN)ranlib
CSTRIP	:= $(MY_TOOL_CHAIN)strip
MAKE	:= make
RM		:= -rm
RM_F	:= -rm -f
RM_RF	:= -rm -rf
CP		:= -cp
CP_F	:= -cp -f
CP_RF	:= -cp -rf


# 编译目标
MY_TARGET_NAME := klb
MY_TARGET_EXE := ../lib/$(MY_TARGET_NAME)


# cgo 部分引用的C头文件目录
MY_CGO_CFLAGS := "
MY_CGO_CFLAGS += -I $(PWD)/../inc
MY_CGO_CFLAGS += "

# cgo 部分需要链接的库
MY_CGO_LDFLAGS := "
MY_CGO_LDFLAGS += -L $(PWD)/../lib -lklb_c
ifeq ($(OS), windows)
MY_CGO_LDFLAGS += -lpthread -lm
else
MY_CGO_LDFLAGS += -lpthread -lrt -ldl -lm
endif
MY_CGO_LDFLAGS += "

# cgo 前置环境
MY_CGO := CGO_ENABLED=1 CGO_CFLAGS=${MY_CGO_CFLAGS} CGO_LDFLAGS=${MY_CGO_LDFLAGS}
MY_CGO += GOOS=$(MY_OS) GOARCH=$(MY_ARCH) GOPATH=$(MY_GOPATH)
MY_CGO += CC=$(CC) CXX=$(CXX)


.PHONY: all clean strip upx

all: exe

exe: $(MY_TARGET_EXE)

$(MY_TARGET_EXE):
	$(my_tip)
	${MY_CGO} go build $(MY_GO_TAGS) $(MY_CGO_STATIC) -o $@ $(MY_GO_MAIN)

clean:
	@echo "++++++ make clean ++++++"
	@echo "+ PWD = $(PWD)"
	@echo "++ RM_F = $(RM_F)"
	$(RM_F) $(MY_TARGET_EXE)
	@echo "+++++++++++++++++++++++++"

strip:
	$(CSTRIP) $(MY_TARGET_EXE)

upx:
	../upx $(MY_TARGET_EXE)

define my_tip
	@echo "++++++ make tip ++++++"
	@echo "+ PWD = $(PWD)"
	@echo "+ MY_TOOL_CHAIN = $(MY_TOOL_CHAIN)"
	@echo "+ CC = $(CC)"
	@echo "+ CXX = $(CXX)"
	@echo "+ MY_GO_MAIN = $(MY_GO_MAIN)"
	@echo "+ MY_TARGET_EXE = $(MY_TARGET_EXE)"
	@echo "++++++++++++++++++++++"
endef
