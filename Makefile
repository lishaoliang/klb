# 编译命令 : make
# make MY_VERSION=release MY_TOOL_CHAIN=arm-himix200-linux- MY_CFLAGS_EX="-D__XXXX_XX__ -D__XXXX_YYY__"
# 裁剪参数 MY_CLIP="no-all no-lpeg no-sqlite no-packages"
#

SHELL = /bin/bash
PWD = `pwd`

# 编译工具, arm-linux-gnueabi-, arm-himix200-linux-
#MY_TOOL_CHAIN ?= arm-himix200-linux-
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


# 从目录检索需要编译的c文件
MY_DIRS := ./src_c/klbplatform ./src_c/klbmem ./src_c/klbutil ./src_c/klbbase

# klbnet
MY_DIRS += ./src_c/klbnet ./src_c/klbnet/klb_ncm_ops

# klbformat
MY_DIRS += ./src_c/klbformat

# klbgui
MY_DIRS += ./src_c/klbgui ./src_c/klbgui/extensions ./src_c/klbgui/widgets ./src_c/klbgui/subviews ./src_c/klbgui/wnd
MY_DIRS += ./src_c/klbgui/shwnd

# klua
MY_DIRS += ./src_c/klua ./src_c/klua/extension ./src_c/klua/klua_platform ./src_c/klua/klua_util ./src_c/klua/klua_base
MY_DIRS += ./src_c/klua/klua_multithread ./src_c/klua/klua_net ./src_c/klua/klua_format
MY_DIRS += ./src_c/klua/lua-5.4.6/src ./src_c/klua/lua-cjson-2.1.0 ./src_c/klua/LuaXML_130610 ./src_c/klua/luafilesystem-2.0/src

# libavutil
MY_DIRS += ./src_c/compat ./src_c/libavutil

# 第三方库
MY_DIRS += ./src_c/klbthird ./src_c/klbthird/sds

# cpp / src_cpp
MY_DIRS += ./src_cpp/klbmem ./src_cpp/klbutil ./src_cpp/klbnet
MY_DIRS += ./src_cpp/klbplatform ./src_cpp/klua 

################################################
# 裁剪代码
MY_CLIP_TAG := $(MY_CLIP)
MY_CLIP_FLAGS :=

# 默认所有可裁剪参数 MY_CLIP = no-all
ifeq ($(filter no-all, $(MY_CLIP_TAG)), )
	MY_CLIP_TAG += 
else
	MY_CLIP_TAG += no-lpeg no-sqlite no-packages
endif

# 可裁剪参数: MY_CLIP = no-lpeg
ifeq ($(filter no-lpeg, $(MY_CLIP_TAG)), )
	MY_DIRS += ./src_c/klua/lpeg-1.0.2
else
	MY_CLIP_FLAGS += -D__KLB_NO_LPEG__
endif

# 可裁剪参数: MY_CLIP = no-sqlite
ifeq ($(filter no-sqlite, $(MY_CLIP_TAG)), )
	MY_DIRS += ./src_c/klua/lsqlite3
else
	MY_CLIP_FLAGS += -D__KLB_NO_SQLITE__
endif

# 可裁剪参数: MY_CLIP = no-packages
ifeq ($(filter no-packages, $(MY_CLIP_TAG)), )
	MY_DIRS += ./src_packages/kpa_flv
	MY_DIRS += ./src_packages/kpa_http
	MY_DIRS += ./src_packages/kpa_mgui
	MY_DIRS += ./src_packages/kpa_mnp
	MY_DIRS += ./src_packages/kpa_rtsp
	MY_DIRS += ./src_packages/kpa_sip
	MY_DIRS += ./src_packages/kpa_ws
else
	MY_CLIP_FLAGS += -D__KLB_NO_PACKAGES__
endif

###########################################################

# 编译选项 -D__XXX_XXX__
MY_CFLAGS := $(MY_CLIP_FLAGS) $(MY_CFLAGS_EX) -D_GNU_SOURCE 

# lua的宏
MY_CFLAGS += -DLUA_USE_LINUX

# pcre2的宏
MY_CFLAGS += -DHAVE_CONFIG_H


# 引用头文件
MY_INCLUDES := -I ./src_c -I ./inc -I ./src_c/compat
MY_INCLUDES += -I ./src_c/klua/lsqlite3
MY_INCLUDES += -I ./src_c/klbthird/sds
MY_INCLUDES += -I ./src_c/klbthird/pcre2/src
MY_INCLUDES += -I ./inc/klbthird

# 第三方库头文件
MY_INCLUDES += -I ./src_c/klua/lua-5.4.6/src

# cpp
MY_INCLUDES += -I ./inc_hpp

# src_packages
MY_INCLUDES += -I ./src_packages


# 引用的静态库
MY_LIB_STATIC := -L ./lib -Bstatic

# 引用的动态库
MY_LIB_DYNAMIC := -L ./lib -Bdynamic
MY_LIB_DYNAMIC += -lstdc++ -lpthread -lrt -ldl -lm


# openssl
#MY_CFLAGS += -D__KLB_OPENSSL__
#MY_LIB_STATIC += -lssl -lcrypto

# debug/release
ifneq ($(MY_VERSION),release)
	MY_CFLAGS += -g
endif

# 防止未定义函数
MY_CFLAGS += -Wl,--no-undefined

# 优化
MY_CFLAGS += -Os

# 编译目标名称
MY_TARGET_NAME := klb
MY_TARGET_A := ./lib/lib$(MY_TARGET_NAME).a
MY_TARGET_SO := ./lib/lib$(MY_TARGET_NAME).so


# 所有编译文件 C/C++
MY_FIND_FILES_C = $(wildcard $(dir)/*.c)
MY_FIND_FILES_CPP = $(wildcard $(dir)/*.cpp)
MY_SOURCES = $(foreach dir, $(MY_DIRS), $(MY_FIND_FILES_C))
MY_SOURCES += $(foreach dir, $(MY_DIRS), $(MY_FIND_FILES_CPP))


MY_LIB_A_OBJS := $(addsuffix .o, $(MY_SOURCES))
MY_A_PARAMS := $(MY_INCLUDES) $(MY_CFLAGS) $(MY_LIB_STATIC) $(MY_LIB_DYNAMIC)


# 编译静态库时候,使compiler为每个function和data item分配独立的section
MY_LIB_MINI = -ffunction-sections -fdata-sections

# 编译动态库或执行档时,使compiler删除所有未被使用的function和data,即编译之后的文件最小化
MY_LINK_MINI = -Wl,--gc-sections

MY_SO_PARAMS := -fPIC
MY_STD_C99 := -std=c99

.PHONY: all clean

all: lib so

lib: $(MY_TARGET_A)
so: $(MY_TARGET_SO)

%.c.o: %.c
	$(CC) $(MY_STD_C99) $(MY_SO_PARAMS) $(MY_A_PARAMS) $(MY_LIB_MINI) -c -o $@ $<

%.cpp.o: %.cpp
	$(CXX) $(MY_SO_PARAMS) $(MY_A_PARAMS) $(MY_LIB_MINI) -c -o $@ $<

$(MY_TARGET_A): $(MY_LIB_A_OBJS)
	$(my_tip)
	$(CAR) rs $(MY_TARGET_A) $(MY_LIB_A_OBJS)

$(MY_TARGET_SO): $(MY_LIB_A_OBJS)
	$(my_tip)
	$(CXX) -shared -fPIC $(MY_LIB_A_OBJS) $(MY_A_PARAMS) $(MY_LINK_MINI) -o $@

strip:
#	$(CSTRIP) $(MY_TARGET_A)
	$(CSTRIP) $(MY_TARGET_SO)

clean:
	@echo "++++++ make clean ++++++"
	@echo "+ MY_DIRS = $(MY_DIRS)"
	@echo "++ RM_F = $(RM_F)"
	$(RM_F) $(MY_LIB_A_OBJS)
	$(RM_F) $(MY_TARGET_A)
	$(RM_F) $(MY_TARGET_SO)
	@echo "+++++++++++++++++++++++++"

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
	@echo "+ MY_CLIP_FLAGS = $(MY_CLIP_FLAGS)"
	@echo "+ MY_SOURCES = $(MY_SOURCES)"
	@echo "+ MY_DIRS = $(MY_DIRS)"
	@echo "+ MY_TARGET_A = $(MY_TARGET_A)"
	@echo "++++++++++++++++++++++"
endef
