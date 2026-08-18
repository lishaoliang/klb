# Doc-Encode UTF8, Unix(LF)
# 处理代码裁剪:
# @param [in]		$(MY_CLIP)			裁剪参数: eg. "min-core" / "no-all"
# @param [out]		$(MY_CLIP_FLAGS)	处理裁剪参数之后的 宏定义等
# @param [out]		$(MY_CLIP_DIRS)		处理裁剪之后, 需要加入编译的目录
# @param [out]		$(MY_CLIP_SOURCES)	处理裁剪之后, 需要加入编译的源文件(非目录)
# @param [out]		$(MY_CLIP_INC)		处理裁剪之后, 需要引用的头文件目录
# @param [out]		$(MY_CLIP_SOURCES_EXCLUDE)	裁剪之后, 排除的源文件

##################################################################
# 入参/出参

# @param [in] $(MY_CLIP)	裁剪参数: 
MY_CLIP_TAG := $(MY_CLIP)

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
# 默认所有

# 预设 min-core / no-all: 展开 clip.mk 内全部 no-*
#   MY_CLIP = min-core  (推荐, 最小核心预设)
#   MY_CLIP = no-all    (同 min-core, 兼容旧用法)
# min-core + 可选库: 勿写 "min-core no-zlib"; 改用手动 no-* / --enable / use-* — 见 klb-min-core
# 记名: no-* 与 --disable-* 等价; use-* 与 --enable-* 等价 (enable 须 clip-build / klb-clip 组合)

# --disable-* -> no-* (make 直传别名)
define KLB_CLIP_MAP_DISABLE
$(if $(findstring --disable-,$(1)),$(patsubst --disable-%,no-%,$(1)),$(1))
endef
MY_CLIP_TAG := $(foreach _w,$(MY_CLIP_TAG),$(call KLB_CLIP_MAP_DISABLE,$(_w)))

ifeq ($(filter min-core no-all,$(MY_CLIP_TAG)),)
	MY_CLIP_TAG +=
else
	MY_CLIP_TAG += no-pcre2 no-lpeg no-sqlite no-zlib no-packages no-cpp no-gui no-format no-qrencode no-net-proto
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


# src_packages: 子目录已迁 backup/src_packages/，待重整；暂不编入
# 恢复扩展包时: 在此追加 MY_CLIP_DIRS / MY_CLIP_INC，并视情况去掉下行
MY_CLIP_FLAGS += -D__KLB_NO_PACKAGES__


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
