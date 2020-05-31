# 编译命令 : make

SHELL = /bin/bash


# 引入一些环境参数
-include ../../mk_def
MY_LIB_PATH = ../../$(MY_LIB_NAME)


# 从目录检索需要编译的c文件
MY_DIRS := .


# 编译选项
#MY_CFLAGS := $(my_flags)
#MY_CFLAGS += -std=c99

# quickjs的宏
#MY_CFLAGS += -DCONFIG_CHECK_JSVALUE -DCONFIG_BIGNUM -DCONFIG_VERSION=\"2019-10-27\"
MY_CFLAGS += -DCONFIG_VERSION=\"2019-10-27\"

# 引用头文件
MY_INCLUDES := -I .


# 引用的静态库
MY_LIB_STATIC := -L $(MY_LIB_PATH) -Bstatic


# 引用的动态库
MY_LIB_DYNAMIC := -L $(MY_LIB_PATH) -Bdynamic


# 编译目标名称
MY_TARGET_NAME := quickjs
MY_TARGET_EXE := $(MY_LIB_PATH)/$(MY_TARGET_NAME)
MY_TARGET_A := $(MY_LIB_PATH)/lib$(MY_TARGET_NAME).a
MY_TARGET_SO := $(MY_LIB_PATH)/lib$(MY_TARGET_NAME).so
MY_TARGET_DLL := $(MY_LIB_PATH)/$(MY_TARGET_NAME).dll


# 所有编译文件
MY_FIND_FILES_C = $(wildcard $(dir)/*.c)
MY_SOURCES = $(foreach dir, $(MY_DIRS), $(MY_FIND_FILES_C))


MY_LIB_A_OBJS := $(addsuffix .o, $(MY_SOURCES))
MY_A_PARAMS := $(MY_INCLUDES) $(MY_CFLAGS) $(MY_LIB_STATIC) $(MY_LIB_DYNAMIC)

MY_LIB_SO_OBJS := $(addsuffix .oo, $(MY_SOURCES))
MY_SO_PARAMS := -fPIC $(MY_INCLUDES) $(MY_CFLAGS) $(MY_LIB_STATIC) $(MY_LIB_DYNAMIC) 


# 编译静态库时候,使compiler为每个function和data item分配独立的section
MY_LIB_MINI = -ffunction-sections -fdata-sections

# 编译动态库或执行档时,使compiler删除所有未被使用的function和data,即编译之后的文件最小化
MY_LINK_MINI = -Wl,--gc-sections


ifeq ($(MY_OS), windows)

MY_MINGW_LIB = -static -lws2_32 -lpthread -lm
MY_MINGW_DEF = -Wl,--output-def,$(MY_LIB_PATH)/lib$(MY_TARGET_NAME).def,--out-implib,$(MY_LIB_PATH)/lib$(MY_TARGET_NAME).a
MY_MINGW = $(MY_MINGW_LIB) $(MY_MINGW_DEF)

.PHONY: all clean
all: dll

else

.PHONY: all clean
all: so

endif


exe: $(MY_TARGET_EXE)
lib: $(MY_TARGET_A)
so: $(MY_TARGET_SO)
dll: $(MY_TARGET_DLL)

%.c.o: %.c
	$(CC) $(MY_A_PARAMS) $(MY_LIB_MINI) -c -o $@ $<

%.c.oo: %.c
	$(CC) $(MY_SO_PARAMS) -c -o $@ $<

$(MY_TARGET_EXE): $(MY_LIB_A_OBJS)
	$(my_tip)
	$(CC) -o $@ $(MY_LIB_A_OBJS) $(MY_A_PARAMS) $(MY_LINK_MINI)

$(MY_TARGET_A): $(MY_LIB_A_OBJS)
	$(my_tip)
	$(CAR) rs $(MY_TARGET_A) $(MY_LIB_A_OBJS)
	$(CRANLIB) $(MY_TARGET_A)
	
$(MY_TARGET_SO): $(MY_LIB_SO_OBJS)
	$(my_tip)
	$(CC) -shared -fPIC -o $@ $(MY_LIB_SO_OBJS) $(MY_SO_PARAMS) $(CXX_LINK_MINI)

$(MY_TARGET_DLL): $(MY_LIB_SO_OBJS)
	$(my_tip)
	$(CC) -shared -fPIC -o $@ $(MY_LIB_SO_OBJS) $(MY_SO_PARAMS) $(CXX_LINK_MINI) $(MY_MINGW)

clean:
	@echo "++++++ make clean ++++++"
	@echo "+ MY_DIRS = $(MY_DIRS)"
	@echo "++ RM = $(RM)"
	$(RM) $(MY_LIB_A_OBJS)
	$(RM) $(MY_LIB_SO_OBJS)
	$(RM) $(MY_TARGET_EXE)
	$(RM) $(MY_TARGET_A)
	$(RM) $(MY_TARGET_SO)
	@echo "+++++++++++++++++++++++++"


define my_tip
	@echo "++++++ make tip ++++++"
	@echo "+ CC = $(CC)"
	@echo "+ CXX = $(CXX)"
	@echo "+ MY_SOURCES = $(MY_SOURCES)"
	@echo "+ MY_DIRS = $(MY_DIRS)"
	@echo "+ MY_CFLAGS = $(MY_CFLAGS)"
	@echo "+ MY_TARGET_EXE = $(MY_TARGET_EXE)"
	@echo "+ MY_TARGET_A = $(MY_TARGET_A)"
	@echo "+ MY_TARGET_SO = $(MY_TARGET_SO)"
	@echo "++++++++++++++++++++++"
endef
