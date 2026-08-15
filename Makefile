# Doc-Encode UTF8, Unix(LF)
# 编译命令 : make
# make MY_VERSION=release MY_TOOL_CHAIN=arm-linux-gnueabi- MY_CFLAGS_EX="-D__XXXX_XX__ -D__XXXX_YYY__"
# 裁剪参数 MY_CLIP="no-pcre2 no-lpeg no-sqlite no-zlib no-packages"
# 裁剪参数 MY_CLIP="no-all"

SHELL = /bin/bash
PWD = `pwd`

###########################################################
# 编译工具, arm-linux-gnueabi-
#MY_TOOL_CHAIN ?= arm-linux-gnueabi-
MY_TOOL_CHAIN ?= 
MY_CFLAGS_EX ?= 
MY_VERSION ?= debug
MY_CLIP ?= 

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


###########################################################
# 基础C/C++文件

# 从目录检索需要编译的c文件
MY_DIRS := ./src_c/klbplatform ./src_c/klbmem ./src_c/klbutil ./src_c/klbbase

# klbnet - flv, http, mnp, rtsp, sip, smp, webrtc, ws
MY_DIRS += ./src_c/klbnet ./src_c/klbnet/klbiopoll ./src_c/klbnet/klblisten
MY_DIRS += ./src_c/klbnet/klbflv ./src_c/klbnet/klbflvclient ./src_c/klbnet/klbflvserve
MY_DIRS += ./src_c/klbnet/klbhttp ./src_c/klbnet/klbhttpclient ./src_c/klbnet/klbhttpserve
MY_DIRS += ./src_c/klbnet/klbmnp ./src_c/klbnet/klbmnpclient ./src_c/klbnet/klbmnpserve
MY_DIRS += ./src_c/klbnet/klbrtp ./src_c/klbnet/klbrtsp ./src_c/klbnet/klbrtspclient ./src_c/klbnet/klbrtspserve
MY_DIRS += ./src_c/klbnet/klbsip ./src_c/klbnet/klbsipclient ./src_c/klbnet/klbsipserve
MY_DIRS += ./src_c/klbnet/klbsmp ./src_c/klbnet/klbsmpclient ./src_c/klbnet/klbsmpserve
MY_DIRS += ./src_c/klbnet/klbwebrtc ./src_c/klbnet/klbwebrtcclient ./src_c/klbnet/klbwebrtcserve
MY_DIRS += ./src_c/klbnet/klbws ./src_c/klbnet/klbwsclient ./src_c/klbnet/klbwsserve

# klbformat
MY_DIRS += ./src_c/klbformat

# klbgui
MY_DIRS += ./src_c/klbgui ./src_c/klbgui/extensions ./src_c/klbgui/widgets ./src_c/klbgui/subviews ./src_c/klbgui/wnd
MY_DIRS += ./src_c/klbgui/shwnd

# klua
MY_DIRS += ./src_c/klua ./src_c/klua/extension ./src_c/klua/klua_platform ./src_c/klua/klua_util ./src_c/klua/klua_base
MY_DIRS += ./src_c/klua/klua_multithread ./src_c/klua/klua_net ./src_c/klua/klua_format
MY_DIRS += ./src_c/klua/lua-5.4.6/src ./src_c/klua/lua-cjson-2.1.0 ./src_c/klua/LuaXML_130610 ./src_c/klua/luafilesystem-2.0/src

# klbapp
MY_DIRS += ./src_c/klbapp

# libavutil
MY_DIRS += ./src_c/compat ./src_c/libavutil

# 第三方库
MY_DIRS += ./src_c/klbthird ./src_c/klbthird/sds
MY_DIRS += ./src_c/qrencode-4.1.1

# cpp / src_cpp
MY_DIRS += ./src_cpp/klbplatform ./src_cpp/klbmem ./src_cpp/klbutil ./src_cpp/klbbase
MY_DIRS += ./src_cpp/klbnet
MY_DIRS += ./src_cpp/klbgui ./src_cpp/klbgui/wnd ./src_cpp/klbgui/widgets
MY_DIRS += ./src_cpp/klua ./src_cpp/klua/extension
MY_DIRS += ./src_cpp/klbapp


###########################################################
# 头文件目录

# 引用头文件
MY_INCLUDES := -I ./src_c -I ./inc -I ./src_c/compat
MY_INCLUDES += -I ./src_c/klbthird/sds
MY_INCLUDES += -I ./inc/klbthird

# 第三方库头文件
MY_INCLUDES += -I ./src_c/klua/lua-5.4.6/src
MY_INCLUDES += -I ./src_c/qrencode-4.1.1

# cpp
MY_INCLUDES += -I ./src_cpp -I ./inc_hpp


###########################################################
# 裁剪代码

# 裁剪步骤1. 入参
# @param [in]	$(MY_CLIP)	裁剪参数: eg. "no-all"
export MY_CLIP

# 裁剪步骤2. 引入裁剪
include ./clip.mk

# 裁剪步骤3. 引入裁剪
# @param [out]		$(MY_CLIP_FLAGS)	处理裁剪参数之后的 宏定义等
# @param [out]		$(MY_CLIP_DIRS)		处理裁剪之后, 需要加入编译的目录
# @param [out]		$(MY_CLIP_SOURCES)	处理裁剪之后, 需要加入编译的源文件(非目录)
# @param [out]		$(MY_CLIP_INC)		处理裁剪之后, 需要引用的头文件目录
MY_CFLAGS := $(MY_CLIP_FLAGS)
MY_DIRS += $(MY_CLIP_DIRS)
MY_INCLUDES += $(MY_CLIP_INC)


###########################################################

# 编译选项 -D__XXX_XXX__
MY_CFLAGS += $(MY_CFLAGS_EX) -D_GNU_SOURCE 

# lua的宏
MY_CFLAGS += -DLUA_USE_LINUX

# 引用的静态库
MY_LIB_STATIC := -L ./lib -Bstatic

# 引用的动态库
MY_LIB_DYNAMIC := -L ./lib -Bdynamic
MY_LIB_DYNAMIC += -lstdc++ -lpthread -lrt -ldl -lm

# 链接选项
MY_LDFLAGS := -Wl,--no-undefined

# debug/release
ifeq ($(MY_VERSION),release)
	MY_CFLAGS += -Os
else
	MY_CFLAGS += -g -fno-omit-frame-pointer -Og
	MY_LDFLAGS += -rdynamic
endif

# 防止返回值格式错误, 警告变错误
MY_CFLAGS += -Werror=return-type

# 默认隐藏 所有符号; 防止符号污染(仅动态库生效)
MY_CFLAGS += -D__KLB_SYMBOL_HIDING__ -fvisibility=hidden


# 所有编译文件 C/C++
MY_FIND_FILES_C = $(wildcard $(dir)/*.c)
MY_FIND_FILES_CPP = $(wildcard $(dir)/*.cpp)
MY_SOURCES = $(foreach dir, $(MY_DIRS), $(MY_FIND_FILES_C))
MY_SOURCES += $(foreach dir, $(MY_DIRS), $(MY_FIND_FILES_CPP))
MY_SOURCES += $(MY_CLIP_SOURCES)

# 编译中间文件统一放到 tmp/ 下, 目录结构与源码镜像
MY_TMP_DIR := ./tmp
MY_LIB_A_OBJS := $(addprefix $(MY_TMP_DIR)/,$(addsuffix .o,$(patsubst ./%,%,$(MY_SOURCES))))
MY_COMPILE_PARAMS := $(MY_INCLUDES) $(MY_CFLAGS)
MY_LINK_PARAMS := $(MY_LIB_STATIC) $(MY_LIB_DYNAMIC) $(MY_LDFLAGS)


# 编译静态库时候,使compiler为每个function和data item分配独立的section
MY_LIB_MINI = -ffunction-sections -fdata-sections

# 编译动态库或执行档时,使compiler删除所有未被使用的function和data,即编译之后的文件最小化
MY_LINK_MINI = -Wl,--gc-sections

MY_SO_PARAMS := -fPIC
MY_STD_C99 := -std=c99


# 传递给子makefile的参数
MK_PARAMS := OS=$(OS) ARCH=$(ARCH) MY_TOOL_CHAIN=$(MY_TOOL_CHAIN)


# 编译目标名称
MY_TARGET_NAME := klb
MY_TARGET_A := ./lib/lib$(MY_TARGET_NAME).a
MY_TARGET_SO := ./lib/lib$(MY_TARGET_NAME).so

###########################################################
# install

# install 相关
include ./install.mk


###########################################################
# .PHONY

.PHONY: all clean strip klua install

all: lib so

lib: $(MY_TARGET_A)
so: $(MY_TARGET_SO)

$(MY_TMP_DIR)/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(MY_STD_C99) $(MY_SO_PARAMS) $(MY_COMPILE_PARAMS) $(MY_LIB_MINI) -c -o $@ $<

$(MY_TMP_DIR)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(MY_SO_PARAMS) $(MY_COMPILE_PARAMS) $(MY_LIB_MINI) -c -o $@ $<

$(MY_TARGET_A): $(MY_LIB_A_OBJS)
	$(my_tip)
	$(CAR) rs $(MY_TARGET_A) $(MY_LIB_A_OBJS)

$(MY_TARGET_SO): $(MY_LIB_A_OBJS)
	$(my_tip)
	$(CXX) -shared -fPIC $(MY_LIB_A_OBJS) $(MY_LINK_PARAMS) $(MY_LINK_MINI) -o $@

clean:
	@echo "++++++ make clean ++++++"
	@echo "+ MY_DIRS = $(MY_DIRS)"
	@echo "++ RM_F = $(RM_F)"
	$(RM_RF) $(MY_TMP_DIR)
	$(RM_F) $(MY_TARGET_A)
	$(RM_F) $(MY_TARGET_SO)
	@echo "+++++++++++++++++++++++++"

	if [ -f ./klua/Makefile ]; then $(MAKE) $(MK_PARAMS) -C ./klua/ clean; fi

strip:
	#$(CSTRIP) $(MY_TARGET_A)
	$(CSTRIP) $(MY_TARGET_SO)

klua: all
	if [ -f ./klua/Makefile ]; then $(MAKE) $(MK_PARAMS) -C ./klua/; fi

install: klua
	#$(CSTRIP) $(MY_TARGET_A)
	$(CSTRIP) $(MY_TARGET_SO)

	$(call DO_install_by_path, /usr/local)

local:
	#$(CSTRIP) $(MY_TARGET_A)
	$(CSTRIP) $(MY_TARGET_SO)

	$(call DO_install_by_path, ./install)

info:
	$(my_tip)

define my_tip
	@echo "++++++ make tip ++++++"
	@echo "+ MY_TOOL_CHAIN = $(MY_TOOL_CHAIN)"
	@echo "+ CC = $(CC)"
	@echo "+ CXX = $(CXX)"	
	@echo "+ MY_CLIP = $(MY_CLIP)"
	@echo "+ MY_CLIP_TAG = $(MY_CLIP_TAG)"
	@echo "+ MY_CFLAGS = $(MY_CFLAGS)"
	@echo "+ MY_LDFLAGS = $(MY_LDFLAGS)"
	@echo "+ MY_CLIP_FLAGS = $(MY_CLIP_FLAGS)"
	@echo "+ MY_SOURCES = $(MY_SOURCES)"
	@echo "+ MY_DIRS = $(MY_DIRS)"
	@echo "+ MY_TARGET_A = $(MY_TARGET_A)"
	@echo "++++++++++++++++++++++"
endef
