# make

SHELL = /bin/bash
PWD = `pwd`

# "linux", "windows", "darwin", "js"
OS ?= linux

# "amd64", "386", "arm", "arm64", "wasm"
ARCH ?= amd64
TAGS ?= "debug normal"

#MY_TOOL_CHAIN ?= arm-linux-gnueabi-
#MY_TOOL_CHAIN ?=

ifeq ($(OS), linux)
	ifeq ($(MY_ARCH), arm)
		MY_TOOL_CHAIN ?= arm-linux-gnueabi-
	endif

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
else ifeq ($(OS), stm32)
	MY_TOOL_CHAIN ?= arm-none-eabi-

	MY_CFLAGS += -D__STM32__
	MY_GO_STATIC  ?= 
	MY_GCC_STATIC ?= -static
else ifeq ($(OS), js)
	MY_TOOL_CHAIN ?= 

	MY_CFLAGS += -D__EMSCRIPTEN__
	MY_GO_STATIC  ?= 
	MY_GCC_STATIC ?= -static
else
	MY_GO_STATIC  ?= -ldflags -extldflags=-static
	MY_GCC_STATIC ?= -static
endif

INSTALL_PATH_VERSION = $(INSTALL_LIB)/klb1.0

# 传递给子makefile的参数
MK_PARAMS := OS=$(OS) ARCH=$(ARCH) TAGS=$(TAGS) MY_TOOL_CHAIN=$(MY_TOOL_CHAIN)


.PHONY: all clean strip upx doc install uninstall


all:
	if [ -d ./src_c ]; then $(MAKE) $(MK_PARAMS) -C ./src_c; fi
	if [ -d ./src ]; then $(MAKE) $(MK_PARAMS) -C ./src; fi


clean:
	if [ -d ./src_c ]; then $(MAKE) $(MK_PARAMS) -C ./src_c clean; fi
	if [ -d ./src ]; then $(MAKE) $(MK_PARAMS) -C ./src clean; fi


strip:
	if [ -d ./src ]; then $(MAKE) $(MK_PARAMS) -C ./src strip; fi

upx:
	if [ -d ./src ]; then $(MAKE) $(MK_PARAMS) -C ./src upx; fi

doc:
	doxygen


install:
	mkdir -p $(INSTALL_PATH_VERSION)/
	$(CP_F)  $(MY_LIB_PATH)/klb $(INSTALL_PATH_VERSION)/
	$(CP_RF) $(MY_LIB_PATH)/base/ $(INSTALL_PATH_VERSION)/
	$(CP_RF) $(MY_LIB_PATH)/html/ $(INSTALL_PATH_VERSION)/
	$(CP_RF) $(MY_LIB_PATH)/http/ $(INSTALL_PATH_VERSION)/
	$(CP_RF) $(MY_LIB_PATH)/tls/ $(INSTALL_PATH_VERSION)/
	$(CP_RF) $(MY_LIB_PATH)/rtsp/ $(INSTALL_PATH_VERSION)/
	$(CP_RF) $(MY_LIB_PATH)/server/ $(INSTALL_PATH_VERSION)/
	$(CP_RF) $(MY_LIB_PATH)/util/ $(INSTALL_PATH_VERSION)/
	$(CP_F)  $(MY_LIB_PATH)/*.lua $(INSTALL_PATH_VERSION)/
	ln -s -f $(INSTALL_PATH_VERSION)/klb $(INSTALL_BIN)/klb
	ln -s -f $(INSTALL_PATH_VERSION)/ $(INSTALL_LIB)/klb1

uninstall:
	$(RM_RF) $(INSTALL_PATH_VERSION)/
	$(RM_F)  $(INSTALL_BIN)/klb
	$(RM_F)  $(INSTALL_LIB)/klb1
