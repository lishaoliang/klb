# Doc-Encode UTF8, Unix(LF)
# 编译命令 : make / make linux
# make MY_VERSION=release MY_TOOL_CHAIN=arm-linux-gnueabi- MY_CFLAGS_EX="-D__XXXX_XX__ -D__XXXX_YYY__"
# 裁剪参数 MY_CLIP="min-core"
# 裁剪参数 MY_CLIP="--disable-gui --disable-zlib"   (同 no-gui no-zlib)
# 裁剪参数 MY_CLIP="min-core use-zlib" / "use-wui-embed"  (clip.mk 归一为 no-*)
# mingw-w64 (非默认, 见 mingw.mk): make mingw
# mingw-w64: make MY_HOST=mingw  /  make MY_TOOL_CHAIN=x86_64-w64-mingw32-

SHELL = /bin/bash
PWD = `pwd`

###########################################################
# 编译工具, arm-linux-gnueabi-
#MY_TOOL_CHAIN ?= arm-linux-gnueabi-
MY_TOOL_CHAIN ?= 
MY_CFLAGS_EX ?= 
MY_VERSION ?= debug
MY_CLIP ?= 
MY_HOST ?= 

# Linux 默认产物目录; mingw 覆盖见 mingw.mk
MY_OUT_DIR := ./lib
MY_TMP_DIR := ./tmp

include ./mingw.mk

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

# 硬核心 + 运行最小集 (klbapp / klua / socket+iopoll; 可选模块见 clip.mk)
MY_DIRS := ./src_c/klbplatform ./src_c/klbmem ./src_c/klbutil ./src_c/klbbase

# klbnet — socket / iopoll / netconn / netmulti (协议包见 clip.mk no-smp/no-rtsp/no-mnp/no-http/no-ws)
MY_DIRS += ./src_c/klbnet ./src_c/klbnet/klbiopoll ./src_c/klbnet/klblisten

# klua — 框架 + 最小 bundled (cjson / LuaXML / lfs; 协议/format 绑定见 clip.mk)
MY_DIRS += ./src_c/klua ./src_c/klua/extension ./src_c/klua/klua_platform ./src_c/klua/klua_util ./src_c/klua/klua_base
MY_DIRS += ./src_c/klua/klua_multithread
MY_DIRS += ./src_c/klua/lua-5.4.6/src ./src_c/klua/lua-cjson-2.1.0 ./src_c/klua/LuaXML_130610 ./src_c/klua/luafilesystem-2.0/src

# klbapp — 主应用程序框架
MY_DIRS += ./src_c/klbapp

# compat / libavutil / klbthird
MY_DIRS += ./src_c/compat ./src_c/libavutil
MY_DIRS += ./src_c/klbthird ./src_c/klbthird/sds

# src_vendor: 第三方开源可裁剪库 (clip.mk; mbedtls-3.6.7 / no-ssl)

###########################################################
# 头文件目录

# 引用头文件
MY_INCLUDES := -I ./src_c -I ./inc -I ./src_c/compat
MY_INCLUDES += -I ./src_c/klbthird/sds
MY_INCLUDES += -I ./inc/klbthird

# klua bundled 头文件 (qrencode 等可选见 clip.mk)
MY_INCLUDES += -I ./src_c/klua/lua-5.4.6/src

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

MY_LIB_DYNAMIC := -L $(MY_OUT_DIR) -Bdynamic
MY_LIB_DYNAMIC += -lpthread -lrt -ldl -lm
MY_LIB_STATIC := -L $(MY_OUT_DIR) -Bstatic


###########################################################

# 编译选项 -D__XXX_XXX__
MY_CFLAGS += $(MY_CFLAGS_EX) -D_GNU_SOURCE 
MY_CFLAGS += -DLUA_USE_LINUX

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


MY_SO_PARAMS := -fPIC
MY_STD_C99 := -std=c99


# 传递给子makefile的参数
MK_PARAMS := OS=$(OS) ARCH=$(ARCH) MY_TOOL_CHAIN=$(MY_TOOL_CHAIN) MY_HOST=$(MY_HOST) MY_VERSION=$(MY_VERSION)


# 编译目标名称
MY_TARGET_NAME := klb
MY_TARGET_A := $(MY_OUT_DIR)/lib$(MY_TARGET_NAME).a
MY_TARGET_SO := $(MY_OUT_DIR)/lib$(MY_TARGET_NAME).so

# Linux 动态库链接; mingw.mk 可覆盖
MY_SO_LINK = $(CXX) -shared -fPIC $(MY_LIB_A_OBJS) $(MY_LINK_PARAMS) $(MY_LINK_MINI) -o $@

define DO_host_install
	#$(CSTRIP) $(MY_TARGET_A)
	$(CSTRIP) $(MY_TARGET_SO)
	$(call DO_install_by_path, /usr/local)
endef

define DO_host_local
	#$(CSTRIP) $(MY_TARGET_A)
	$(CSTRIP) $(MY_TARGET_SO)
	$(call DO_install_by_path, ./install)
endef

# mingw 覆盖编译/链接/产物 (Linux 时无操作)
include ./mingw.mk


# 所有编译文件 C/C++
MY_FIND_FILES_C = $(wildcard $(dir)/*.c)
MY_FIND_FILES_CPP = $(wildcard $(dir)/*.cpp)
MY_SOURCES = $(foreach dir, $(MY_DIRS), $(MY_FIND_FILES_C))
MY_SOURCES += $(foreach dir, $(MY_DIRS), $(MY_FIND_FILES_CPP))
MY_SOURCES += $(MY_CLIP_SOURCES)
MY_SOURCES := $(filter-out $(MY_CLIP_SOURCES_EXCLUDE),$(MY_SOURCES))

# 编译中间文件统一放到 tmp/ 下, 目录结构与源码镜像
MY_LIB_A_OBJS := $(addprefix $(MY_TMP_DIR)/,$(addsuffix .o,$(patsubst ./%,%,$(MY_SOURCES))))
MY_COMPILE_PARAMS := $(MY_INCLUDES) $(MY_CFLAGS)
MY_LINK_PARAMS := $(MY_LIB_STATIC) $(MY_LIB_DYNAMIC) $(MY_LDFLAGS)


# 编译静态库时候,使compiler为每个function和data item分配独立的section
MY_LIB_MINI = -ffunction-sections -fdata-sections

# 编译动态库或执行档时,使compiler删除所有未被使用的function和data,即编译之后的文件最小化
MY_LINK_MINI = -Wl,--gc-sections

###########################################################
# install

# install 相关
include ./install.mk


###########################################################
# .PHONY

.PHONY: all linux clean strip klua install mingw

all: lib so
linux: all

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
	@mkdir -p $(dir $@)
	$(CAR) rs $(MY_TARGET_A) $(MY_LIB_A_OBJS)

$(MY_TARGET_SO): $(MY_LIB_A_OBJS)
	$(my_tip)
	@mkdir -p $(dir $@)
	$(MY_SO_LINK)

clean:
	@echo "++++++ make clean ++++++"
	@echo "+ MY_DIRS = $(MY_DIRS)"
	@echo "++ RM_F = $(RM_F)"
	$(RM_RF) $(MY_TMP_DIR)
	$(RM_F) $(MY_TARGET_A)
	$(RM_F) $(MY_TARGET_SO)
	$(RM_F) $(MY_TARGET_IMPLIB)
	@echo "+++++++++++++++++++++++++"

	if [ -f ./proj/klua/Makefile ]; then $(MAKE) $(MK_PARAMS) -C ./proj/klua/ clean; fi

strip:
	#$(CSTRIP) $(MY_TARGET_A)
	$(CSTRIP) $(MY_TARGET_SO)

klua: all
	if [ -f ./proj/klua/Makefile ]; then $(MAKE) $(MK_PARAMS) -C ./proj/klua/; fi

install: klua
	$(DO_host_install)

local:
	$(DO_host_local)

info:
	$(my_tip)

define my_tip
	@echo "++++++ make tip ++++++"
	@echo "+ MY_HOST = $(MY_HOST)"
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
	@echo "+ MY_TMP_DIR = $(MY_TMP_DIR)"
	@echo "+ MY_TARGET_A = $(MY_TARGET_A)"
	@echo "+ MY_TARGET_SO = $(MY_TARGET_SO)"
	@echo "++++++++++++++++++++++"
endef
