# make

SHELL = /bin/bash
PWD = `pwd`

# "linux", "windows", "darwin", "js"
OS ?= linux

# "amd64", "386", "arm", "arm64", "wasm"
ARCH ?= amd64
TAGS ?= "normal"


ifeq ($(OS), linux)
	MY_TOOL_CHAIN ?=
	MY_CFLAGS +=
	MY_GO_STATIC  ?= 
	MY_GCC_STATIC ?= 
else ifeq ($(OS), windows)
	ifeq ($(MY_ARCH), 386)
		MY_TOOL_CHAIN ?= i686-w64-mingw32-
	else
		MY_TOOL_CHAIN ?= x86_64-w64-mingw32-
	endif

	MY_CFLAGS += -D__WINDOWS__
	MY_GO_STATIC  ?= -ldflags -extldflags=-static
	MY_GCC_STATIC ?= -static
else ifeq ($(OS), darwin)
	CC ?= clang
	CXX ?= clang
	MY_GO_STATIC ?=

	MY_CFLAGS +=
	MY_GO_STATIC  ?= 
	MY_GCC_STATIC ?= 
else ifeq ($(OS), js)
	MY_TOOL_CHAIN ?= 

	MY_CFLAGS += -D__EMSCRIPTEN__
	MY_GO_STATIC  ?= 
	MY_GCC_STATIC ?= -static
else
	MY_GO_STATIC  ?= -ldflags -extldflags=-static
	MY_GCC_STATIC ?= -static
endif


# gcc编译工具链
CC		:= $(MY_TOOL_CHAIN)gcc
CXX		:= $(MY_TOOL_CHAIN)g++
CAR		:= $(MY_TOOL_CHAIN)ar
CRANLIB	:= $(MY_TOOL_CHAIN)ranlib
CSTRIP	:= $(MY_TOOL_CHAIN)strip
CREADELF:= $(MY_TOOL_CHAIN)readelf
MAKE	:= make
RM		:= -rm
RM_F	:= -rm -f
RM_RF	:= -rm -rf
CP		:= -cp
CP_F	:= -cp -f
CP_RF	:= -cp -rf


# 传递给子makefile的参数
MK_PARAMS := OS=$(OS) ARCH=$(ARCH) TAGS=$(TAGS) MY_TOOL_CHAIN=$(MY_TOOL_CHAIN)


.PHONY: all clean strip upx readelf install uninstall

define copy_to_bin
	$(CP) ../../lib/klua ../../bin/
endef

all:
	if [ -f ../../Makefile ]; then $(MAKE) $(MK_PARAMS) -C ../../; fi
	if [ -f ../../klua/Makefile ]; then $(MAKE) $(MK_PARAMS) -C ../../klua/; fi

	$(copy_to_bin)

clean:
	if [ -f ../../Makefile ]; then $(MAKE) $(MK_PARAMS) -C ../../ clean; fi
	if [ -f ../../klua/Makefile ]; then $(MAKE) $(MK_PARAMS) -C ../../klua/ clean; fi

strip:
	if [ -f ../../bin/klua ]; then $(CSTRIP) ../../bin/klua; fi

upx:
	if [ -f ../upx ]; then ../upx ../../bin/klua; fi

readelf:
	if [ -f ../../bin/klua ]; then $(CREADELF) -d ../../bin/klua; fi

# install / uninstall
INSTALL_PATH = /usr/local/klb1x

install:
	if [ -f /usr/local/bin/klua ]; then rm -f /usr/local/bin/klua; fi
	if [ -d $(INSTALL_PATH)/ ]; then rm -rf $(INSTALL_PATH)/; fi

	mkdir -p $(INSTALL_PATH)/
	$(CP_RF) ../../bin/klbcore $(INSTALL_PATH)/
	$(CP_RF) ../../bin/res $(INSTALL_PATH)/
	$(CP_RF) ../../bin/sample1 $(INSTALL_PATH)/
	$(CP_RF) ../../bin/sample2 $(INSTALL_PATH)/
	$(CP_F)	../../bin/klua $(INSTALL_PATH)/
				
	ln -s $(INSTALL_PATH)/klua /usr/local/bin/klua

uninstall:
	$(RM_RF) $(INSTALL_PATH)/
	$(RM_F)  /usr/local/bin/klua
