# make

SHELL = /bin/bash
PWD = `pwd`


# include make param
-include ./make_define

INSTALL_PATH_VERSION = $(INSTALL_LIB)/klb1.0

.PHONY: all clean strip doc install uninstall


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
	mkdir -p $(INSTALL_PATH_VERSION)/
	$(CP_R)  $(MY_LIB_PATH)/klb $(INSTALL_PATH_VERSION)/
	$(CP_RF) $(MY_LIB_PATH)/base/ $(INSTALL_PATH_VERSION)/
	$(CP_RF) $(MY_LIB_PATH)/html/ $(INSTALL_PATH_VERSION)/
	$(CP_RF) $(MY_LIB_PATH)/http/ $(INSTALL_PATH_VERSION)/
	$(CP_RF) $(MY_LIB_PATH)/tls/ $(INSTALL_PATH_VERSION)/
	$(CP_RF) $(MY_LIB_PATH)/rtsp/ $(INSTALL_PATH_VERSION)/
	$(CP_RF) $(MY_LIB_PATH)/server/ $(INSTALL_PATH_VERSION)/
	$(CP_RF) $(MY_LIB_PATH)/util/ $(INSTALL_PATH_VERSION)/
	$(CP_R)  $(MY_LIB_PATH)/*.lua $(INSTALL_PATH_VERSION)/
	ln -s $(INSTALL_PATH_VERSION)/klb $(INSTALL_BIN)/klb
	ln -s $(INSTALL_PATH_VERSION)/ $(INSTALL_LIB)/klb1

uninstall:
	$(RM_RF) $(INSTALL_PATH_VERSION)/
	$(RM_R)  $(INSTALL_BIN)/klb
	$(RM_R)  $(INSTALL_LIB)/klb1
