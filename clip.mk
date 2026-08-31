# Doc-Encode UTF8, Unix(LF)
# 处理代码裁剪:
# @param [in]		$(MY_CLIP)			裁剪参数: eg. "min-core" / "no-gui" / "min-core use-zlib"
# @param [out]		$(MY_CLIP_TAG)		归一后的 no-* 列表 (全集的子集; 空=全量)
# @param [out]		$(MY_CLIP_FLAGS)	处理裁剪参数之后的 宏定义等
# @param [out]		$(MY_CLIP_DIRS)		处理裁剪之后, 需要加入编译的目录
# @param [out]		$(MY_CLIP_SOURCES)	处理裁剪之后, 需要加入编译的源文件(非目录)
# @param [out]		$(MY_CLIP_INC)		处理裁剪之后, 需要引用的头文件目录
# @param [out]		$(MY_CLIP_SOURCES_EXCLUDE)	裁剪之后, 排除的源文件

##################################################################
# 入参/出参

# @param [in] $(MY_CLIP)	裁剪参数: no-* 减法, 或 min-core + use-* 加回
# @param [out] $(MY_CLIP_TAG)	归一后的 no-* (见下节)

# @param [out] $(MY_CLIP_FLAGS)	处理裁剪参数之后的 宏定义等
export MY_CLIP_FLAGS :=

# @param [out] $(MY_CLIP_DIRS)	处理裁剪之后, 需要加入编译的目录
export MY_CLIP_DIRS :=

# @param [out] $(MY_CLIP_SOURCES)	处理裁剪之后, 需要加入编译的源文件(非目录)
export MY_CLIP_SOURCES :=

# @param [out] $(MY_CLIP_INC)	处理裁剪之后, 需要引用的头文件目录
export MY_CLIP_INC :=

# @param [out] $(MY_CLIP_SOURCES_EXCLUDE)	裁剪之后, 排除的源文件
export MY_CLIP_SOURCES_EXCLUDE :=

##################################################################
# 归一: 两种入参 → 同一套 no-* (all 裁剪)
#   1. 减法: MY_CLIP="no-gui no-zlib" / "--disable-gui"
#   2. 加回: MY_CLIP="min-core use-zlib" / "use-wui-embed" (仅 use-* 时隐含 min-core)
# 最终 MY_CLIP_TAG 只含 no-*; 同条中 use-* 覆盖对应 no-* (enable 优先)
# 记名: no-* 同 --disable-*; use-* 同 --enable-*; no-all 同 min-core

# 全集 (min-core 基准; 不含 no-wui-sim)
KLB_CLIP_ALL_NO := no-pcre2 no-lpeg no-sqlite no-zlib no-wui no-cpp no-gui no-format no-qrencode no-net-proto

# --disable-X -> no-X; --enable-X -> use-X; no-all -> min-core
define KLB_CLIP_MAP_WORD
$(strip $(if $(findstring --disable-,$(1)),$(patsubst --disable-%,no-%,$(1)),$(if $(findstring --enable-,$(1)),$(patsubst --enable-%,use-%,$(1)),$(if $(filter no-all,$(1)),min-core,$(1)))))
endef

# use-* -> 要从 TAG 删掉的 no-* (wui / wui-embed 连带 gui)
define KLB_CLIP_USE_NOS
$(strip $(if $(filter use-wui-embed,$(1)),no-wui no-gui,$(if $(filter use-wui use-klbwui use-wui-sim,$(1)),no-wui no-gui,$(if $(filter use-gui use-klbgui use-kgui,$(1)),no-gui,$(if $(filter use-zlib,$(1)),no-zlib,$(if $(filter use-lpeg,$(1)),no-lpeg,$(if $(filter use-sqlite use-lsqlite3,$(1)),no-sqlite,$(if $(filter use-pcre2,$(1)),no-pcre2,$(if $(filter use-cpp,$(1)),no-cpp,$(if $(filter use-format use-kh26x use-klbformat,$(1)),no-format,$(if $(filter use-qrencode use-qr,$(1)),no-qrencode,$(if $(filter use-net-proto use-netproto use-net use-protocol,$(1)),no-net-proto,$(patsubst use-%,no-%,$(1))))))))))))))
endef

KLB_CLIP_WORDS := $(foreach _w,$(MY_CLIP),$(call KLB_CLIP_MAP_WORD,$(_w)))
KLB_CLIP_HAS_MIN := $(filter min-core,$(KLB_CLIP_WORDS))
KLB_CLIP_USE := $(filter use-%,$(KLB_CLIP_WORDS))
KLB_CLIP_NO := $(filter no-%,$(KLB_CLIP_WORDS))

ifeq ($(KLB_CLIP_HAS_MIN)$(KLB_CLIP_USE),)
	KLB_CLIP_BASE :=
else
	KLB_CLIP_BASE := $(KLB_CLIP_ALL_NO)
endif

KLB_CLIP_USE_REMOVE := $(foreach _u,$(KLB_CLIP_USE),$(call KLB_CLIP_USE_NOS,$(_u)))
MY_CLIP_TAG := $(sort $(filter-out $(KLB_CLIP_USE_REMOVE),$(KLB_CLIP_BASE) $(KLB_CLIP_NO)))

# min-core + wui-embed: 加回 embed 后仍裁 sim; 与 use-wui 同时出现时全量 wui 优先
ifeq ($(filter use-wui use-klbwui use-wui-sim,$(KLB_CLIP_USE)),)
ifeq ($(filter use-wui-embed,$(KLB_CLIP_USE)),)
else
	MY_CLIP_TAG := $(sort $(MY_CLIP_TAG) no-wui-sim)
endif
endif


##################################################################
# 分别处理

# 可裁剪参数: MY_CLIP = no-pcre2
# exclude pcre2 tool sources: grep/dftables/fuzz (contain main or CLI-only code)
ifeq ($(filter no-pcre2, $(MY_CLIP_TAG)), )
	MY_PCRE2_SKIP := ./src_c/klbthird/pcre2/src/pcre2grep.c ./src_c/klbthird/pcre2/src/pcre2_dftables.c ./src_c/klbthird/pcre2/src/pcre2_fuzzsupport.c
	MY_CLIP_SOURCES += $(filter-out $(MY_PCRE2_SKIP),$(wildcard ./src_c/klbthird/pcre2/src/*.c))
	MY_CLIP_INC += -I ./src_c/klbthird/pcre2/src
	MY_CLIP_FLAGS += -DHAVE_CONFIG_H
else
	MY_CLIP_FLAGS += -D__KLB_NO_PCRE2__
endif


# 可裁剪参数: MY_CLIP = no-lpeg
ifeq ($(filter no-lpeg, $(MY_CLIP_TAG)), )
	MY_CLIP_DIRS += ./src_c/klua/lpeg-1.0.2
else
	MY_CLIP_FLAGS += -D__KLB_NO_LPEG__
endif


# 可裁剪参数: MY_CLIP = no-sqlite
ifeq ($(filter no-sqlite, $(MY_CLIP_TAG)), )
	MY_CLIP_DIRS += ./src_c/klua/lsqlite3
	MY_CLIP_INC += -I ./src_c/klua/lsqlite3
else
	MY_CLIP_FLAGS += -D__KLB_NO_SQLITE__
endif


# 可裁剪参数: MY_CLIP = no-zlib
ifeq ($(filter no-zlib, $(MY_CLIP_TAG)), )
	MY_CLIP_DIRS += ./src_c/zlib-1.2.11
	MY_CLIP_DIRS += ./src_c/klua/lua-zlib-1.3

	MY_CLIP_INC += -I ./src_c/zlib-1.2.11
else
	MY_CLIP_FLAGS += -D__KLB_NO_ZLIB__
endif


# 可裁剪参数: MY_CLIP = no-cpp
ifeq ($(filter no-cpp, $(MY_CLIP_TAG)), )
	MY_CLIP_DIRS += ./src_cpp/klbplatform ./src_cpp/klbmem ./src_cpp/klbutil ./src_cpp/klbbase
	MY_CLIP_DIRS += ./src_cpp/klbnet
	MY_CLIP_DIRS += ./src_cpp/klbgui ./src_cpp/klbgui/wnd ./src_cpp/klbgui/widgets
	MY_CLIP_DIRS += ./src_cpp/klua ./src_cpp/klua/extension
	MY_CLIP_DIRS += ./src_cpp/klbapp
	MY_CLIP_INC += -I ./src_cpp -I ./inc_hpp
else
	MY_CLIP_FLAGS += -D__KLB_NO_CPP__
endif


# 可裁剪参数: MY_CLIP = no-wui / no-wui-sim
# 正向: MY_CLIP = use-wui / --enable-wui (连带 gui; sim 含 embed) / use-wui-embed / --enable-wui-embed
# wui: src_packages/klbwui (依赖 klbgui; no-gui 时跳过)
ifeq ($(filter no-wui, $(MY_CLIP_TAG)), )
ifeq ($(filter no-gui, $(MY_CLIP_TAG)), )
	MY_CLIP_DIRS += ./src_packages/klbwui/core
	MY_CLIP_DIRS += ./src_packages/klbwui/embed_wnd
	MY_CLIP_DIRS += ./src_packages/klbwui/embed_widgets
	MY_CLIP_DIRS += ./src_packages/klbwui/embed_shwnd
ifeq ($(filter no-wui-sim, $(MY_CLIP_TAG)), )
	MY_CLIP_DIRS += ./src_packages/klbwui/sim_wnd
	MY_CLIP_DIRS += ./src_packages/klbwui/sim_widgets
	MY_CLIP_DIRS += ./src_packages/klbwui/sim_shwnd
else
	MY_CLIP_FLAGS += -D__KLB_NO_WUI_SIM__
endif
	MY_CLIP_INC += -I ./src_packages
endif
else
	MY_CLIP_FLAGS += -D__KLB_NO_WUI__
endif


# 可裁剪参数: MY_CLIP = no-qrencode
ifeq ($(filter no-qrencode, $(MY_CLIP_TAG)), )
	MY_CLIP_DIRS += ./src_c/qrencode-4.1.1
	MY_CLIP_INC += -I ./src_c/qrencode-4.1.1
else
	MY_CLIP_FLAGS += -D__KLB_NO_QRENCODE__
endif


# 可裁剪参数: MY_CLIP = no-gui
ifeq ($(filter no-gui, $(MY_CLIP_TAG)), )
	MY_CLIP_DIRS += ./src_c/klbgui ./src_c/klbgui/extensions ./src_c/klbgui/widgets ./src_c/klbgui/subviews ./src_c/klbgui/wnd
	MY_CLIP_DIRS += ./src_c/klbgui/shwnd
else
	MY_CLIP_FLAGS += -D__KLB_NO_GUI__
	MY_CLIP_SOURCES_EXCLUDE += ./src_c/klua/klua_base/klua_kgui.c ./src_c/klua/extension/klua_ex_gui.c
endif


# 可裁剪参数: MY_CLIP = no-format
ifeq ($(filter no-format, $(MY_CLIP_TAG)), )
	MY_CLIP_DIRS += ./src_c/klbformat
	MY_CLIP_DIRS += ./src_c/klua/klua_format
else
	MY_CLIP_FLAGS += -D__KLB_NO_FORMAT__
endif


# 可裁剪参数: MY_CLIP = no-net-proto
# min-core 保留 klbnet(socket/iopoll/netconn/netmulti) 与 kurl; 裁协议栈与 klua 网络绑定
ifeq ($(filter no-net-proto, $(MY_CLIP_TAG)), )
	MY_CLIP_DIRS += ./src_c/klbnet/klbmnp ./src_c/klbnet/klbrtsp ./src_c/klbnet/klbsmp
	MY_CLIP_DIRS += ./src_c/klua/klua_net
else
	MY_CLIP_FLAGS += -D__KLB_NO_NET_PROTO__
	MY_CLIP_SOURCES_EXCLUDE += ./src_c/klbnet/klb_http.c ./src_c/klbnet/klb_websocket.c
	MY_CLIP_SOURCES += ./src_c/klua/klua_net/klua_kurl.c
endif
