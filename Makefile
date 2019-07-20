# 编译命令 : make

SHELL = /bin/bash
PWD = `pwd`

# 引入一些环境参数
-include ./mk_def


.PHONY: all clean strip doc install


all:
	if [ -d ./src_c ]; then $(MAKE) -C ./src_c; fi

ifeq ($(MY_OS), stm32)
else ifeq ($(MY_OS), emscripten)
else
	if [ -d ./src ]; then $(MAKE) -C ./src; fi
endif


clean:
	if [ -d ./src_c ]; then $(MAKE) -C ./src_c clean; fi

ifeq ($(MY_OS), stm32)
else ifeq ($(MY_OS), emscripten)
else
	if [ -d ./src ]; then $(MAKE) -C ./src clean; fi
endif


strip:
	

doc:
	doxygen


install:
	$(CP) $(MY_LIB_PATH)/klua $(INSTALL_BIN)
