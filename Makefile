# make

SHELL = /bin/bash
PWD = `pwd`

# include make param
-include ./make_define


.PHONY: all clean strip doc install


all:
	if [ -d ./src_c ]; then $(MAKE) -C ./src_c; fi

ifeq ($(MY_OS), stm32)
else ifeq ($(MY_OS), js)
	if [ -d ./src ]; then $(MAKE) -C ./src; fi
else
	if [ -d ./src ]; then $(MAKE) -C ./src; fi
endif


clean:
	if [ -d ./src_c ]; then $(MAKE) -C ./src_c clean; fi

ifeq ($(MY_OS), stm32)
else ifeq ($(MY_OS), js)
	if [ -d ./src ]; then $(MAKE) -C ./src clean; fi
else
	if [ -d ./src ]; then $(MAKE) -C ./src clean; fi
endif


strip:
	

doc:
	doxygen


install:
	$(CP) $(MY_LIB_PATH)/klua $(INSTALL_BIN)
