// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_list.h
/// @brief   精简双向链表
///  \n 参考: std::list
///  \n 插入的数据托管内存
///  \n 底层为 klb_nlist_t; 节点数据为 klb_adt_t
/// @version 0.1
/// @history 修改历史
///  \n 2022 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_LIST_H__
#define __KLB_LIST_H__

#include "klb_type.h"
#include "klbutil/klb_adt.h"
#include "klbutil/klb_nlist.h"

#if defined(__cplusplus)
extern "C" {
#endif

/// @struct klb_list_t
/// @brief  list
typedef struct klb_list_t_
{
    klb_nlist_t*    p_nlist;    ///< 底层非侵入链表
}klb_list_t;


/// @brief 初始化list对象
/// @param [in] *p_list     list对象
/// @return 无
/// @note 用于栈上或嵌入成员; 须与 klb_list_quit 成对
KLB_API void klb_list_init(klb_list_t* p_list);

/// @brief 反初始化list对象
/// @param [in] *p_list     list对象
/// @return 无
/// @note 清空并释放托管的 adt, 以及底层 nlist
KLB_API void klb_list_quit(klb_list_t* p_list);

/// @brief 创建list对象
/// @return klb_list_t*     list对象
KLB_API klb_list_t* klb_list_create();

/// @brief 销毁list对象
/// @param [in] *p_list     list对象
/// @return 无
/// @note 含清理托管内存过程
KLB_API void klb_list_destroy(klb_list_t* p_list);


/// @brief 在最前面压入 adt
/// @param [in] *p_list     list对象
/// @param [in] *p_adt      adt数据; NULL 时压入 null 类型
/// @return 无
/// @note 1. list 托管 p_adt 生命周期, 销毁/弹出/移除时释放
///  \n   2. p_adt 非 NULL 时, 调用后由 list 接管, 调用方勿再 destroy
KLB_API void klb_list_push_head(klb_list_t* p_list, klb_adt_t* p_adt);

/// @brief 在最后面压入 adt
/// @param [in] *p_list     list对象
/// @param [in] *p_adt      adt数据; NULL 时压入 null 类型
/// @return 无
/// @note 1. list 托管 p_adt 生命周期, 销毁/弹出/移除时释放
///  \n   2. p_adt 非 NULL 时, 调用后由 list 接管, 调用方勿再 destroy
KLB_API void klb_list_push_tail(klb_list_t* p_list, klb_adt_t* p_adt);

/// @brief 弹出并释放最前面的节点
/// @param [in] *p_list     list对象
/// @return 无
/// @note 空表时无操作; 托管 adt 一并释放
KLB_API void klb_list_pop_head(klb_list_t* p_list);

/// @brief 弹出并释放最后面的节点
/// @param [in] *p_list     list对象
/// @return 无
/// @note 空表时无操作; 托管 adt 一并释放
KLB_API void klb_list_pop_tail(klb_list_t* p_list);

/// @brief 访问最前面的 adt
/// @param [in] *p_list     list对象
/// @return klb_adt_t* adt指针; 空表为 NULL
/// @note 借用指针, 仍由 list 托管
KLB_API klb_adt_t* klb_list_head(klb_list_t* p_list);

/// @brief 访问最后面的 adt
/// @param [in] *p_list     list对象
/// @return klb_adt_t* adt指针; 空表为 NULL
/// @note 借用指针, 仍由 list 托管
KLB_API klb_adt_t* klb_list_tail(klb_list_t* p_list);

/// @brief 当前节点数目
/// @param [in] *p_list     list对象
/// @return int 返回当前节点数目
KLB_API int klb_list_size(klb_list_t* p_list);

/// @struct klb_list_iter_t
/// @brief  list节点 OR 迭代子
typedef klb_nlist_iter_t klb_list_iter_t;

/// @brief 按迭代子移除节点
/// @param [in] *p_list     list对象
/// @param [in] *p_iter     iter迭代子
/// @return 无
/// @note 1. 托管 adt 由 list 释放
///  \n   2. 函数调用后, iter迭代子失效(内存被释放)
KLB_API void klb_list_remove(klb_list_t* p_list, klb_list_iter_t* p_iter);

/// @brief 获取list iter对应的 adt
/// @param [in] *p_iter     iter迭代子
/// @return klb_adt_t* 返回 adt
/// @note 借用指针, 仍由 list 托管
KLB_API klb_adt_t* klb_list_data(klb_list_iter_t* p_iter);

/// @brief 获取最前面的迭代子
/// @param [in] *p_list       list对象
/// @return klb_list_iter_t* iter迭代子
/// @note 1. 命名借鉴 STL begin; 返回首节点, 空表为 NULL
///  \n   2. 正向遍历: for(p=begin(...); NULL!=p; p=next(p))
KLB_API klb_list_iter_t* klb_list_begin(klb_list_t* p_list);

/// @brief 下一个迭代子
/// @param [in] *p_iter       iter迭代子
/// @return klb_list_iter_t* 下一个iter迭代子
/// @note 与 klb_list_begin 配合正向遍历, 至 NULL 结束
KLB_API klb_list_iter_t* klb_list_next(klb_list_iter_t* p_iter);

/// @brief 获取最后面的迭代子
/// @param [in] *p_list       list对象
/// @return klb_list_iter_t* iter迭代子
/// @note 1. 命名借鉴 STL end, 但实现不同: 返回尾节点, 非 STL 尾后哨兵; 空表为 NULL
///  \n   2. 反向遍历: for(p=end(...); NULL!=p; p=prev(p))
///  \n   3. 勿用 p!=end(...) 作正向循环结束条件
KLB_API klb_list_iter_t* klb_list_end(klb_list_t* p_list);

/// @brief 前一个迭代子
/// @param [in] *p_iter       iter迭代子
/// @return klb_list_iter_t* 前一个iter迭代子
/// @note 与 klb_list_end 配合反向遍历, 至 NULL 结束
KLB_API klb_list_iter_t* klb_list_prev(klb_list_iter_t* p_iter);

/// @brief 清空列表
/// @param [in] *p_list     list对象
/// @return 无
/// @note 释放全部托管 adt; 方向: 从头到尾
KLB_API void klb_list_clear(klb_list_t* p_list);


#ifdef __cplusplus
}
#endif

#endif // __KLB_LIST_H__

// end
