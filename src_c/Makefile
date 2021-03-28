# 编译命令 : make

SHELL = /bin/bash
PWD = `pwd`

# 编译工具, arm-linux-gnueabi-, arm-himix200-linux-
#MY_TOOL_CHAIN ?= arm-himix200-linux-
MY_TOOL_CHAIN ?= 

MY_CFLAGS_EX ?= 

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
MY_DIRS := ./klbplatform ./klbmem ./klbutil ./klbbase

# klbnet
MY_DIRS := ./klbnet ./klbnet/klbncm_parser

# klbgui
MY_DIRS := ./klbgui

# klua
MY_DIRS += ./klua ./klua/extension ./klua/extension_single ./klua/klua_platform ./klua/klua_util ./klua/klua_base
MY_DIRS += ./klua/klua_multithread ./klua/klua_net
MY_DIRS += ./klua/lua-5.4.1/src ./klua/lua-cjson-2.1.0 ./klua/lpeg-1.0.2 ./klua/luafilesystem-2.0/src

# libavutil
MY_DIRS += ./compat ./libavutil

# 第三方库
MY_DIRS += ./klbthird ./klbthird/sds ./klbthird/pcre2/src ./klbthird/sqlite3
MY_DIRS += ./zlib-1.2.11


# 编译选项 -D__KLB_USE_KMNP_DEV_STATIC__
MY_CFLAGS := -g -std=c99 -D_GNU_SOURCE $(MY_CFLAGS_EX)

# pcre2的宏
MY_CFLAGS += -DHAVE_CONFIG_H

# quickjs的宏
MY_CFLAGS += -DCONFIG_BIGNUM -DCONFIG_VERSION=\"2019-10-27\"


# 引用头文件
MY_INCLUDES := -I ../inc -I .
MY_INCLUDES += -I ./compat
MY_INCLUDES += -I ./klbthird/sds
MY_INCLUDES += -I ./klbthird/pcre2/src
MY_INCLUDES += -I ./klbthird/sqlite3


# 第三方库头文件
MY_INCLUDES += -I ./klua/lua-5.4.1/src


# 引用的静态库
MY_LIB_STATIC := -L ../lib -Bstatic

# 引用的动态库
MY_LIB_DYNAMIC := -L ../lib -Bdynamic


# 编译目标名称
MY_TARGET_NAME := klb_c
MY_TARGET_A := ../lib/lib$(MY_TARGET_NAME).a


# 所有编译文件
MY_FIND_FILES_C = $(wildcard $(dir)/*.c)
MY_SOURCES = $(foreach dir, $(MY_DIRS), $(MY_FIND_FILES_C))


MY_LIB_A_OBJS := $(addsuffix .o, $(MY_SOURCES))
MY_A_PARAMS := $(MY_INCLUDES) $(MY_CFLAGS) $(MY_LIB_STATIC) $(MY_LIB_DYNAMIC)


# 编译静态库时候,使compiler为每个function和data item分配独立的section
MY_LIB_MINI = -ffunction-sections -fdata-sections

# 编译动态库或执行档时,使compiler删除所有未被使用的function和data,即编译之后的文件最小化
MY_LINK_MINI = -Wl,--gc-sections


.PHONY: all clean

all: lib

lib: $(MY_TARGET_A)

%.c.o: %.c
	$(CC) $(MY_A_PARAMS) $(MY_LIB_MINI) -c -o $@ $<

$(MY_TARGET_A): $(MY_LIB_A_OBJS)
	$(my_tip)
	$(CAR) rs $(MY_TARGET_A) $(MY_LIB_A_OBJS)

clean:
	@echo "++++++ make clean ++++++"
	@echo "+ MY_DIRS = $(MY_DIRS)"
	@echo "++ RM_F = $(RM_F)"
	$(RM_F) $(MY_LIB_A_OBJS)
	$(RM_F) $(MY_TARGET_A)
	@echo "+++++++++++++++++++++++++"


define my_tip
	@echo "++++++ make tip ++++++"
	@echo "+ MY_TOOL_CHAIN = $(MY_TOOL_CHAIN)"
	@echo "+ CC = $(CC)"
	@echo "+ CXX = $(CXX)"
	@echo "+ MY_SOURCES = $(MY_SOURCES)"
	@echo "+ MY_DIRS = $(MY_DIRS)"
	@echo "+ MY_CFLAGS = $(MY_CFLAGS)"
	@echo "+ MY_TARGET_A = $(MY_TARGET_A)"
	@echo "++++++++++++++++++++++"
endef
