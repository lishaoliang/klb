# �������� : make

SHELL = /bin/bash


# ����һЩ��������
-include ../mk_def
MY_LIB_PATH = ../$(MY_LIB_NAME)


# ��Ŀ¼������Ҫ�����c�ļ�
MY_DIRS := ./hash ./list ./log ./mem ./obj 
MY_DIRS += ./socket ./string ./graph ./media ./file
MY_DIRS += ./algorithm ./klua ./mnp


# ��������
MY_DIRS += ../third/lua-5.3.5/src ../third/lua-cjson-2.1.0 ../third/lpeg-1.0.2

# STM32
ifeq ($(MY_OS), stm32)
MY_DIRS += ./stm32
else ifeq ($(MY_OS), emscripten)
MY_DIRS += ./emscripten
else
MY_DIRS += ./thread ./time ../third/luafilesystem-2.0/src
endif

# ����ѡ��
#MY_CFLAGS := $(my_flags)
#MY_CFLAGS += -std=c99


# ����ͷ�ļ�
MY_INCLUDES := -I ../inc
MY_INCLUDES += -I ./hash -I./list -I ./log -I ./mem -I ./obj 
MY_INCLUDES += -I ./socket -I ./string -I ./thread -I ./time -I ./graph
MY_INCLUDES += -I ./media -I ./file -I ./algorithm -I ./klua -I ./mnp
MY_INCLUDES += -I ./stm32


# ��������ͷ�ļ�
MY_INCLUDES += -I ../third/lua-5.3.5/src


# ���õľ�̬��
MY_LIB_STATIC := -L $(MY_LIB_PATH) -Bstatic

# ���õĶ�̬��
MY_LIB_DYNAMIC := -L $(MY_LIB_PATH) -Bdynamic


# ����Ŀ������
MY_TARGET_NAME := klb_c
MY_TARGET_EXE := $(MY_LIB_PATH)/$(MY_TARGET_NAME)
MY_TARGET_A := $(MY_LIB_PATH)/lib$(MY_TARGET_NAME).a
MY_TARGET_SO := $(MY_LIB_PATH)/lib$(MY_TARGET_NAME).so


# ���б����ļ�
MY_FIND_FILES_C = $(wildcard $(dir)/*.c)
MY_SOURCES = $(foreach dir, $(MY_DIRS), $(MY_FIND_FILES_C))


MY_LIB_A_OBJS := $(addsuffix .o, $(MY_SOURCES))
MY_A_PARAMS := $(MY_INCLUDES) $(MY_CFLAGS) $(MY_LIB_STATIC) $(MY_LIB_DYNAMIC)

MY_LIB_SO_OBJS := $(addsuffix .oo, $(MY_SOURCES))
MY_SO_PARAMS := -fPIC $(MY_INCLUDES) $(MY_CFLAGS) $(MY_LIB_STATIC) $(MY_LIB_DYNAMIC) 


# ���뾲̬��ʱ��,ʹcompilerΪÿ��function��data item���������section
MY_LIB_MINI = -ffunction-sections -fdata-sections

# ���붯̬���ִ�е�ʱ,ʹcompilerɾ������δ��ʹ�õ�function��data,������֮����ļ���С��
MY_LINK_MINI = -Wl,--gc-sections


.PHONY: all clean

all: lib

exe: $(MY_TARGET_EXE)
lib: $(MY_TARGET_A)
so: $(MY_TARGET_SO)

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