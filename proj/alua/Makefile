# Doc-Encode UTF8, Unix(LF)
# 编译命令 : make
# make MY_VERSION=release MY_TOOL_CHAIN=arm-himix200-linux- MY_CFLAGS_EX="-D__XXXX_XX__ -D__XXXX_YYY__"
# mingw-w64 (非默认, 见 mingw.mk): 由主编译 make mingw alua 传入 MY_HOST=mingw
# mingw-w64: make MY_HOST=mingw  /  make MY_TOOL_CHAIN=x86_64-w64-mingw32-
# alua: AI agent 辅助可执行程序; 仅读取 klb/bin/klbagent

SHELL = /bin/bash
PWD = `pwd`

# 编译工具, arm-linux-gnueabi-, arm-himix200-linux-
#MY_TOOL_CHAIN ?= arm-himix200-linux-
MY_TOOL_CHAIN ?= 
MY_CFLAGS_EX ?= 
MY_VERSION ?= debug
MY_HOST ?= 

# Linux 默认产物目录; mingw 覆盖见 mingw.mk
MY_OUT_DIR := ../../lib
MY_TMP_DIR := ../../tmp/alua

# 裸 make → all (勿让 mingw.mk 空目标抢默认)
.DEFAULT_GOAL := all

include ../../mingw.mk

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


# 从目录检索需要编译的c文件
MY_DIRS := .
# src_apps/alua: 应用支撑包 (仅编入 alua, 不进 libklb)
# ALUA_WITH_KSVNRA=0 时裁掉 (嵌入式 INSTALL_ALUA_EMBEDDED=1 构建用)
ALUA_WITH_KSVNRA ?= 1
ifeq ($(ALUA_WITH_KSVNRA),0)
else
MY_DIRS += ../../src_apps/alua/svn_ra/core
MY_DIRS += ../../src_apps/alua/svn_ra/svn_ra_klua
endif

# 编译选项 -D__XXX_XXX__
MY_CFLAGS := $(MY_CFLAGS_EX) -D_GNU_SOURCE 
ifeq ($(ALUA_WITH_KSVNRA),0)
MY_CFLAGS += -DALUA_NO_KSVNRA=1
endif
MY_LIB_DYNAMIC := -L $(MY_OUT_DIR) -Bdynamic -lklb
MY_LIB_DYNAMIC += -lpthread -lrt -ldl -lm
MY_LIB_STATIC := -L $(MY_OUT_DIR) -Bstatic

# 引用头文件
MY_INCLUDES := -I . -I ../../inc
MY_INCLUDES += -I ../../src_apps


# 第三方库头文件
MY_INCLUDES += -I ../../src_c/klua/lua-5.4.6/src
MY_INCLUDES += -I ../../inc/klbthird
MY_INCLUDES += -I ../../src_3rdparty/zlib-1.3.2


# openssl
#MY_LIB_DYNAMIC += -lssl -lcrypto

# debug/release
ifneq ($(MY_VERSION),release)
	MY_CFLAGS += -g
endif


# 编译目标名称
MY_TARGET_NAME := alua
MY_TARGET_A := $(MY_OUT_DIR)/lib$(MY_TARGET_NAME).a
MY_TARGET_SO := $(MY_OUT_DIR)/lib$(MY_TARGET_NAME).so
MY_TARGET_EXE := $(MY_OUT_DIR)/$(MY_TARGET_NAME)

# mingw 覆盖编译/链接/产物 (Linux 时无操作)
include ../../mingw.mk

# 运行时库搜索路径 (Linux): 产物同级 + 安装树 bin 的 ../lib; 免 ldconfig 刷新
ifeq ($(MY_HOST),mingw)
MY_RPATH :=
else
MY_RPATH := -Wl,-rpath,'$$ORIGIN' -Wl,-rpath,'$$ORIGIN/../lib'
endif

# 所有编译文件
MY_FIND_FILES_C = $(wildcard $(dir)/*.c)
MY_FIND_FILES_CPP = $(wildcard $(dir)/*.cpp)
MY_SOURCES = $(foreach dir, $(MY_DIRS), $(MY_FIND_FILES_C))
MY_SOURCES += $(foreach dir, $(MY_DIRS), $(MY_FIND_FILES_CPP))
ifneq ($(ALUA_WITH_KSVNRA),0)
MY_ZLIB_DIR := ../../src_3rdparty/zlib-1.3.2
MY_SOURCES += $(MY_ZLIB_DIR)/adler32.c $(MY_ZLIB_DIR)/crc32.c $(MY_ZLIB_DIR)/inffast.c
MY_SOURCES += $(MY_ZLIB_DIR)/inflate.c $(MY_ZLIB_DIR)/inftrees.c $(MY_ZLIB_DIR)/zutil.c
endif

# 编译中间文件统一放到 MY_TMP_DIR 下
# 剥掉 ./ 与 ../../, 避免 ../../tmp/alua/../../src_apps 抵消后写回源码树
MY_SOURCES_LOCAL := $(filter-out ../../%,$(MY_SOURCES))
MY_SOURCES_EXT := $(filter ../../%,$(MY_SOURCES))
MY_OBJS_LOCAL := $(addprefix $(MY_TMP_DIR)/,$(addsuffix .o,$(patsubst ./%,%,$(MY_SOURCES_LOCAL))))
MY_OBJS_EXT := $(addprefix $(MY_TMP_DIR)/,$(addsuffix .o,$(patsubst ../../%,%,$(MY_SOURCES_EXT))))
MY_LIB_A_OBJS := $(MY_OBJS_LOCAL) $(MY_OBJS_EXT)
MY_A_PARAMS := $(MY_INCLUDES) $(MY_CFLAGS) $(MY_LIB_STATIC) $(MY_LIB_DYNAMIC)


# 编译静态库时候,使compiler为每个function和data item分配独立的section
MY_LIB_MINI = -ffunction-sections -fdata-sections

# 编译动态库或执行档时,使compiler删除所有未被使用的function和data,即编译之后的文件最小化
MY_LINK_MINI = -Wl,--gc-sections


.PHONY: all strip clean mingw

all: exe

lib: $(MY_TARGET_A)
so: $(MY_TARGET_SO)
exe: $(MY_TARGET_EXE)

# 本地源 (proj/alua/*.c)
$(filter %.c.o,$(MY_OBJS_LOCAL)): $(MY_TMP_DIR)/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(MY_A_PARAMS) $(MY_LIB_MINI) -c -o $@ $<

$(filter %.cpp.o,$(MY_OBJS_LOCAL)): $(MY_TMP_DIR)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(MY_A_PARAMS) $(MY_LIB_MINI) -c -o $@ $<

# 跨目录源 (../../src_apps/...); stem 为剥掉 ../../ 后的相对路径
$(filter %.c.o,$(MY_OBJS_EXT)): $(MY_TMP_DIR)/%.c.o: ../../%.c
	@mkdir -p $(dir $@)
	$(CC) $(MY_A_PARAMS) $(MY_LIB_MINI) -c -o $@ $<

$(filter %.cpp.o,$(MY_OBJS_EXT)): $(MY_TMP_DIR)/%.cpp.o: ../../%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(MY_A_PARAMS) $(MY_LIB_MINI) -c -o $@ $<

$(MY_TARGET_A): $(MY_LIB_A_OBJS)
	$(my_tip)
	$(CAR) rs $(MY_TARGET_A) $(MY_LIB_A_OBJS)

$(MY_TARGET_SO): $(MY_LIB_A_OBJS)
	$(my_tip)
	$(CXX) -shared -fPIC -o $@ $(MY_LIB_A_OBJS) $(MY_A_PARAMS) $(MY_LINK_MINI)

$(MY_TARGET_EXE): $(MY_LIB_A_OBJS)
	$(my_tip)
	@mkdir -p $(dir $@)
	$(CXX) -o $@ $(MY_LIB_A_OBJS) $(MY_A_PARAMS) $(MY_LINK_MINI) $(MY_RPATH)

strip:
	$(CSTRIP) $(MY_TARGET_EXE)

clean:
	@echo "++++++ make clean ++++++"
	@echo "+ MY_DIRS = $(MY_DIRS)"
	@echo "++ RM_F = $(RM_F)"
	$(RM_RF) $(MY_TMP_DIR)
	$(RM_F) $(MY_TARGET_EXE)
	@echo "+++++++++++++++++++++++++"


define my_tip
	@echo "++++++ make tip ++++++"
	@echo "+ MY_HOST = $(MY_HOST)"
	@echo "+ MY_TOOL_CHAIN = $(MY_TOOL_CHAIN)"
	@echo "+ CC = $(CC)"
	@echo "+ CXX = $(CXX)"
	@echo "+ MY_SOURCES = $(MY_SOURCES)"
	@echo "+ MY_DIRS = $(MY_DIRS)"
	@echo "+ MY_CFLAGS = $(MY_CFLAGS)"
	@echo "+ MY_TARGET_EXE = $(MY_TARGET_EXE)"
	@echo "++++++++++++++++++++++"
endef
