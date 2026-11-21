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

	# 先清空再拷: 避免安装树残留旧文件盖住新模块 (如扁平 *.lua vs */init.lua)
	rm -rf $(INSTALL_SHARE)/klua/klbcore
	$(MKDIR_P) $(INSTALL_SHARE)/klua/klbcore
	$(CP_RLPT) ./bin/klbcore $(INSTALL_SHARE)/klua/klbcore

	rm -rf $(INSTALL_SHARE)/klua/klbagent
	$(MKDIR_P) $(INSTALL_SHARE)/klua/klbagent
	$(CP_RLPT) ./bin/klbagent $(INSTALL_SHARE)/klua/klbagent
	# 可选包: INSTALL_ALUA_OPTIONAL_PACKAGES=0|embedded 时剔除 tier=optional
	@if [ "$(INSTALL_ALUA_OPTIONAL_PACKAGES)" = "0" ] || [ "$(INSTALL_ALUA_EMBEDDED)" = "1" ]; then \
		for d in $(INSTALL_SHARE)/klua/klbagent/packages/*/ ; do \
			[ -f "$$d/package.lua" ] || continue; \
			if grep -qE 'tier[[:space:]]*=[[:space:]]*"optional"' "$$d/package.lua" 2>/dev/null; then \
				rm -rf "$$d"; \
			fi; \
		done; \
	fi

	$(MKDIR_P) $(INSTALL_LIB)/klua

	$(CP_F) ./lib/libklb.a $(INSTALL_LIB)/
	# 原子安装: 直接覆盖运行中进程已 mmap 的 elf/so 会令文本页失效 (SIGBUS), 故先写临时名再 rename
	cp -f ./lib/libklb.so $(INSTALL_LIB)/.libklb.so.new
	mv -f $(INSTALL_LIB)/.libklb.so.new $(INSTALL_LIB)/libklb.so
	cp -f ./lib/klua $(INSTALL_BIN)/.klua.new
	mv -f $(INSTALL_BIN)/.klua.new $(INSTALL_BIN)/klua
	cp -f ./lib/alua $(INSTALL_BIN)/.alua.new
	mv -f $(INSTALL_BIN)/.alua.new $(INSTALL_BIN)/alua

	chmod 755 $(INSTALL_INC)/klb -R
	chmod 755 $(INSTALL_SHARE)/klua/klbcore -R
	chmod 755 $(INSTALL_SHARE)/klua/klbagent -R
	chmod 755 $(INSTALL_LIB)/klua -R
	chmod 755 $(INSTALL_LIB)/libklb.a
	chmod 755 $(INSTALL_LIB)/libklb.so
	chmod 755 $(INSTALL_BIN)/klua
	chmod 755 $(INSTALL_BIN)/alua
endef

# eg. (call DO_install_by_path, ./install)
define DO_install_by_path
	@echo $(1)

	$(MKDIR_P) $(1)/include/klb
	$(CP_RLPT) ./inc $(1)/include/klb

	rm -rf $(1)/share/klua/klbcore
	$(MKDIR_P) $(1)/share/klua/klbcore
	$(CP_RLPT) ./bin/klbcore $(1)/share/klua/klbcore

	rm -rf $(1)/share/klua/klbagent
	$(MKDIR_P) $(1)/share/klua/klbagent
	$(CP_RLPT) ./bin/klbagent $(1)/share/klua/klbagent
	@if [ "$(INSTALL_ALUA_OPTIONAL_PACKAGES)" = "0" ] || [ "$(INSTALL_ALUA_EMBEDDED)" = "1" ]; then \
		for d in $(1)/share/klua/klbagent/packages/*/ ; do \
			[ -f "$$d/package.lua" ] || continue; \
			if grep -qE 'tier[[:space:]]*=[[:space:]]*"optional"' "$$d/package.lua" 2>/dev/null; then \
				rm -rf "$$d"; \
			fi; \
		done; \
	fi

	$(MKDIR_P) $(1)/lib/klua
	$(CP_F) ./lib/libklb.a $(1)/lib/libklb.a
	# 原子安装: 直接覆盖运行中进程已 mmap 的 elf/so 会令文本页失效 (SIGBUS), 故先写临时名再 rename
	cp -f ./lib/libklb.so $(1)/lib/.libklb.so.new
	mv -f $(1)/lib/.libklb.so.new $(1)/lib/libklb.so

	$(MKDIR_P) $(1)/bin
	cp -f ./lib/klua $(1)/bin/.klua.new
	mv -f $(1)/bin/.klua.new $(1)/bin/klua
	cp -f ./lib/alua $(1)/bin/.alua.new
	mv -f $(1)/bin/.alua.new $(1)/bin/alua

	chmod 755 $(1)/include/klb -R
	chmod 755 $(1)/share/klua/klbcore -R
	chmod 755 $(1)/share/klua/klbagent -R
	chmod 755 $(1)/lib/klua -R
	chmod 755 $(1)/lib/libklb.a
	chmod 755 $(1)/lib/libklb.so
	chmod 755 $(1)/bin/klua
	chmod 755 $(1)/bin/alua
endef
