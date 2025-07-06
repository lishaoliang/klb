# Doc-Encode UTF8, Unix(LF)
# 处理代码裁剪:
# @param [in]		$(MY_CLIP)			裁剪参数: eg. "no-all"
# @param [out]		$(MY_CLIP_FLAGS)	处理裁剪参数之后的 宏定义等
# @param [out]		$(MY_CLIP_DIRS)		处理裁剪之后, 需要加入编译的目录文件
# @param [out]		$(MY_CLIP_INC)		处理裁剪之后, 需要引用的头文件目录

##################################################################
# 入参/出参

# @param [in] $(MY_CLIP)	裁剪参数: 
MY_CLIP_TAG := $(MY_CLIP)

# @param [out] $(MY_CLIP_FLAGS)	处理裁剪参数之后的 宏定义等
export MY_CLIP_FLAGS :=

# @param [out] $(MY_CLIP_DIRS)	处理裁剪之后, 需要加入编译的目录
export MY_CLIP_DIRS :=

# @param [out] $(MY_CLIP_INC)	处理裁剪之后, 需要引用的头文件目录
export MY_CLIP_INC :=

##################################################################
# 默认所有

# 默认所有可裁剪参数 MY_CLIP = no-all
ifeq ($(filter no-all, $(MY_CLIP_TAG)), )
	MY_CLIP_TAG += 
else
	MY_CLIP_TAG += no-pcre2 no-lpeg no-sqlite no-zlib no-packages
endif


##################################################################
# 分别处理

# 可裁剪参数: MY_CLIP = no-pcre2
ifeq ($(filter no-pcre2, $(MY_CLIP_TAG)), )
	MY_CLIP_DIRS += ./src_c/klbthird/pcre2/src
	MY_CLIP_INC += -I ./src_c/klbthird/pcre2/src
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


# 可裁剪参数: MY_CLIP = no-packages
ifeq ($(filter no-packages, $(MY_CLIP_TAG)), )
	MY_CLIP_DIRS += ./src_packages/kpa_flv
	MY_CLIP_DIRS += ./src_packages/kpa_http
	MY_CLIP_DIRS += ./src_packages/kpa_mgui
	MY_CLIP_DIRS += ./src_packages/kpa_mnp
	MY_CLIP_DIRS += ./src_packages/kpa_rtsp
	MY_CLIP_DIRS += ./src_packages/kpa_sip
	MY_CLIP_DIRS += ./src_packages/kpa_ws
	MY_CLIP_INC += -I ./src_packages
else
	MY_CLIP_FLAGS += -D__KLB_NO_PACKAGES__
endif

