# Doc-Encode UTF8, Unix(LF)
# @param [out]		$(MY_PATH)		当前文件绝对路径


# 导出: $(MY_PATH)
# 当前文件绝对路径
export MY_PATH := $(abspath $(dir $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))))
