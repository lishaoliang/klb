# Doc-Encode UTF8, Unix(LF)
# mingw-w64 交叉片段 (非默认)
# 主编译仍为 Linux: make / make linux
# 入口: make mingw  /  make MY_HOST=mingw  /  make MY_TOOL_CHAIN=x86_64-w64-mingw32-
# 32 位: MY_TOOL_CHAIN=i686-w64-mingw32-
#
# 被 Makefile、proj/klua/Makefile include 两次:
#   首次: 识别 MY_HOST, 覆盖工具链前缀与 tmp/lib 目录
#   再次: 覆盖编译/链接/产物与 install (仅 MY_HOST=mingw)
#         主编译: libklb.dll; proj/klua: klua.exe

##################################################################
# 识别宿主

ifeq ($(filter mingw,$(MAKECMDGOALS)),mingw)
	MY_HOST := mingw
endif
ifneq ($(findstring mingw,$(MY_TOOL_CHAIN)),)
	MY_HOST := mingw
endif

ifndef MINGW_MK_LOADED
MINGW_MK_LOADED := 1

# mingw.mk 所在目录 (主编译为 . ; proj/klua 为 ../..)
MINGW_MK_DIR := $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))

##################################################################
# 首次 include: 工具链与目录

ifeq ($(MY_HOST),mingw)
ifeq ($(strip $(MY_TOOL_CHAIN)),)
	MY_TOOL_CHAIN := x86_64-w64-mingw32-
endif
	MY_OUT_DIR := $(MINGW_MK_DIR)/lib/mingw
	MY_TMP_DIR := $(MINGW_MK_DIR)/tmp/mingw
endif

# 非默认目标: 仅当唯一目标时编 all; `make mingw info` 只看变量
ifeq ($(MAKECMDGOALS),mingw)
mingw: all
else
mingw:
	@true
endif

else

##################################################################
# 再次 include: 编译/链接/产物 (仅 mingw)

ifeq ($(MY_HOST),mingw)

ifneq ($(strip $(MY_TARGET_EXE)),)

	# proj/klua: 可执行消费 dll
	MY_TMP_DIR := $(MINGW_MK_DIR)/tmp/mingw/$(MY_TARGET_NAME)

	MY_LIB_DYNAMIC := -L $(MY_OUT_DIR) -Bdynamic -lklb
	MY_LIB_DYNAMIC += -lws2_32 -lwsock32 -lwinmm -lpsapi -lcrypt32 -lbcrypt
	MY_LIB_DYNAMIC += -lgdi32 -luser32 -ladvapi32 -lshell32 -liphlpapi
	MY_LIB_STATIC := -L $(MY_OUT_DIR) -Bstatic

	MY_CFLAGS := $(filter-out -D_GNU_SOURCE -DLUA_USE_LINUX,$(MY_CFLAGS))
	MY_CFLAGS += -D__KLB_USE_DLL__ -DWIN32_LEAN_AND_MEAN -D_WIN32_WINNT=0x0601

	MY_TARGET_EXE := $(MY_OUT_DIR)/$(MY_TARGET_NAME).exe

else

	MY_CLIP_SOURCES_EXCLUDE += ./src_c/klua/lua-5.4.6/src/lua.c
	MY_CLIP_SOURCES_EXCLUDE += ./src_c/klua/lua-5.4.6/src/luac.c

	MY_LIB_DYNAMIC := -L $(MY_OUT_DIR) -Bdynamic
	MY_LIB_DYNAMIC += -lws2_32 -lwsock32 -lwinmm -lpsapi -lcrypt32 -lbcrypt
	MY_LIB_DYNAMIC += -lgdi32 -luser32 -ladvapi32 -lshell32 -liphlpapi
	MY_LIB_STATIC := -L $(MY_OUT_DIR) -Bstatic

	MY_CFLAGS := $(filter-out -D_GNU_SOURCE -DLUA_USE_LINUX -D__KLB_SYMBOL_HIDING__ -fvisibility=hidden,$(MY_CFLAGS))
	MY_LDFLAGS := $(filter-out -rdynamic,$(MY_LDFLAGS))
	MY_CFLAGS += -D__KLB_BUILD_DLL__ -DWIN32_LEAN_AND_MEAN -D_WIN32_WINNT=0x0601
	MY_CFLAGS += -DPCRE2_STATIC -fgnu89-inline

	MY_SO_PARAMS :=
	MY_STD_C99 := -std=gnu99

	MY_TARGET_SO := $(MY_OUT_DIR)/lib$(MY_TARGET_NAME).dll
	MY_TARGET_IMPLIB := $(MY_OUT_DIR)/lib$(MY_TARGET_NAME).dll.a

	MY_SO_LINK = $(CXX) -shared $(MY_LIB_A_OBJS) $(MY_LINK_PARAMS) $(MY_LINK_MINI) -o $@ -Wl,--out-implib,$(MY_TARGET_IMPLIB)

define DO_host_install
	@echo "mingw-w64: skip install (Linux prefix /usr/local)"
endef

define DO_host_local
	@echo "mingw-w64: skip local install"
endef

endif

endif

endif
