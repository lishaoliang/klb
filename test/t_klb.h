﻿///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/06/09
//
/// @file    t_klb.h
/// @brief   文件简要描述
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
///  \n 2019/06/09 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __T_KLB_H__
#define __T_KLB_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif

/// @brief 测试 klb 函数
/// @param [in]  argc       参数个数
/// @param [in]  **argv     参数列表
/// @param [out] **p_print  输出打印信息
/// @return int 0.成功; 非0.失败
/// @note 形如 int t_klb_xxx_main(int argc, char** argv, char** p_print);

KLB_API int t_klb_hlist_main(int argc, char** argv, char** p_print);

KLB_API int t_klb_htab_main(int argc, char** argv, char** p_print);

KLB_API int t_klb_list_main(int argc, char** argv, char** p_print);


#ifdef __cplusplus
}
#endif

#endif // __T_KLB_H__
//end
