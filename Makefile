# 编译命令 : make

SHELL = /bin/bash

# 引入一些环境参数
-include ./mk_def


.PHONY: all clean strip install

all:
	if [ -d ./src_c ]; then $(MAKE) -C ./src_c; fi
	if [ -d ./src ]; then $(MAKE) -C ./src; fi

clean:
	if [ -d ./src_c ]; then $(MAKE) -C ./src_c clean; fi
	if [ -d ./src ]; then $(MAKE) -C ./src clean; fi

strip:


install:
