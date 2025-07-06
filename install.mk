# Doc-Encode UTF8, Unix(LF)

PLAT := guess

INSTALL_TOP = /usr/local
INSTALL_BIN := $(INSTALL_TOP)/bin
INSTALL_INC := $(INSTALL_TOP)/include
INSTALL_LIB := $(INSTALL_TOP)/lib
INSTALL_MAN := $(INSTALL_TOP)/man
INSTALL_SHARE := $(INSTALL_TOP)/share

CP_F	:= -cp -f
CP_RF	:= -cp -rf
CP_RLPT	:= -cp -rLpT
MKDIR_P	:= mkdir -p

define DO_install
	$(MKDIR_P) $(INSTALL_INC)/klb
	$(CP_RLPT) ./inc $(INSTALL_INC)/klb
	$(CP_RLPT) ./inc_hpp $(INSTALL_INC)/klb

	$(MKDIR_P) $(INSTALL_SHARE)/klua/klbcore
	$(CP_RLPT) ./bin/klbcore $(INSTALL_SHARE)/klua/klbcore

	$(MKDIR_P) $(INSTALL_LIB)/klua

	$(CP_F) ./lib/libklb.a $(INSTALL_LIB)/
	$(CP_F) ./lib/libklb.so $(INSTALL_LIB)/
	$(CP_F) ./lib/klua $(INSTALL_BIN)/

	chmod 755 $(INSTALL_INC)/klb -R
	chmod 755 $(INSTALL_SHARE)/klua/klbcore -R
	chmod 755 $(INSTALL_LIB)/klua -R
	chmod 755 $(INSTALL_LIB)/libklb.a
	chmod 755 $(INSTALL_LIB)/libklb.so
	chmod 755 $(INSTALL_BIN)/klua
endef

# eg. (call DO_install_by_path, ./install)
define DO_install_by_path
	@echo $(1)

	$(MKDIR_P) $(1)/include/klb
	$(CP_RLPT) ./inc $(1)/include/klb
	$(CP_RLPT) ./inc_hpp $(1)/include/klb

	$(MKDIR_P) $(1)/share/klua/klbcore
	$(CP_RLPT) ./bin/klbcore $(1)/share/klua/klbcore

	$(MKDIR_P) $(1)/lib/klua
	$(CP_F) ./lib/libklb.a $(1)/lib/libklb.a
	$(CP_F) ./lib/libklb.so $(1)/lib/libklb.so

	$(MKDIR_P) $(1)/bin
	$(CP_F) ./lib/klua $(1)/bin/klua

	chmod 755 $(1)/include/klb -R
	chmod 755 $(1)/share/klua/klbcore -R
	chmod 755 $(1)/lib/klua -R
	chmod 755 $(1)/lib/libklb.a
	chmod 755 $(1)/lib/libklb.so
	chmod 755 $(1)/bin/klua
endef
