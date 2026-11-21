// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_getopt.h
/// @brief   getopt 转接导出 (ANSI); 对接 src_3rdparty/getopt
/// @version 0.1
/// @history 修改历史
///  \n 2026 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_GETOPT_H__
#define __KLB_GETOPT_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @def   KLB_no_argument / KLB_required_argument / KLB_optional_argument
/// @brief long option 的 has_arg 取值 (与 GNU getopt 一致)
#define KLB_no_argument         0
#define KLB_required_argument   1
#define KLB_optional_argument   2


/// @struct klb_option_t
/// @brief  long option 描述 (对应上游 option_a)
typedef struct klb_option_t_
{
    const char* name;       ///< 长选项名 (不含前导 --)
    int         has_arg;    ///< KLB_no_argument / KLB_required_argument / KLB_optional_argument
    int*        flag;       ///< 非 NULL 时写入 val, 函数返回 0; NULL 时函数返回 val
    int         val;        ///< 与 flag 配合的返回值或写入值
} klb_option_t;


/// @brief 当前选项的参数字符串; 无参时为 NULL
/// @note  Win: KLB_API 无 extern, 变量声明须再写 extern; Linux: KLB_API 已含 extern
#if defined(_WIN32)
KLB_API extern char* klb_optarg;
KLB_API extern int klb_optind;
KLB_API extern int klb_opterr;
KLB_API extern int klb_optopt;
#else
KLB_API char* klb_optarg;
KLB_API int klb_optind;
KLB_API int klb_opterr;
KLB_API int klb_optopt;
#endif


/// @brief 解析短选项
/// @param [in]  argc           参数个数
/// @param [in]  argv           参数数组
/// @param [in]  *optstring     短选项串 (如 "ab:")
/// @return int 选项字符; 结束返回 -1; 非法选项返回 '?'
KLB_API int klb_getopt(int argc, char* const* argv, const char* optstring);


/// @brief 解析短选项与长选项
/// @param [in]  argc               参数个数
/// @param [in]  argv               参数数组
/// @param [in]  *options           短选项串
/// @param [in]  *long_options      长选项表, 以 name==NULL 结尾
/// @param [out] *opt_index         命中长选项时写入下标; 可为 NULL
/// @return int 选项字符或 val; 结束返回 -1; 非法选项返回 '?'
KLB_API int klb_getopt_long(int argc, char* const* argv, const char* options,
                            const klb_option_t* long_options, int* opt_index);


/// @brief 解析短选项与长选项; 单横线也可匹配长选项
/// @param [in]  argc               参数个数
/// @param [in]  argv               参数数组
/// @param [in]  *options           短选项串
/// @param [in]  *long_options      长选项表, 以 name==NULL 结尾
/// @param [out] *opt_index         命中长选项时写入下标; 可为 NULL
/// @return int 选项字符或 val; 结束返回 -1; 非法选项返回 '?'
KLB_API int klb_getopt_long_only(int argc, char* const* argv, const char* options,
                                const klb_option_t* long_options, int* opt_index);


#if defined(__cplusplus)
}
#endif

#endif // __KLB_GETOPT_H__

// end
