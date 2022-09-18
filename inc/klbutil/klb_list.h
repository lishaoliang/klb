///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_list.h
/// @brief   精简双向链表
///  \n 参考: std::list
///  \n 插入的数据托管内存
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
    klb_nlist_t*    p_nlist;
}klb_list_t;


KLB_API void klb_list_init(klb_list_t* p_list);
KLB_API void klb_list_quit(klb_list_t* p_list);

KLB_API klb_list_t* klb_list_create();
KLB_API void klb_list_destroy(klb_list_t* p_list);


KLB_API void klb_list_push_head(klb_list_t* p_list);
KLB_API void klb_list_push_tail(klb_list_t* p_list);

KLB_API void klb_list_pop_head(klb_list_t* p_list);
KLB_API void klb_list_pop_tail(klb_list_t* p_list);

KLB_API klb_adt_t* klb_list_head(klb_list_t* p_list);
KLB_API klb_adt_t* klb_list_tail(klb_list_t* p_list);

KLB_API int klb_list_size(klb_list_t* p_list);

typedef klb_nlist_iter_t klb_list_iter_t;
KLB_API void klb_list_remove(klb_list_t* p_list, klb_list_iter_t* p_iter);

KLB_API klb_adt_t* klb_list_data(klb_list_iter_t* p_iter);

KLB_API klb_list_iter_t* klb_list_begin(klb_list_t* p_list);
KLB_API klb_list_iter_t* klb_list_next(klb_list_iter_t* p_iter);

KLB_API klb_list_iter_t* klb_list_end(klb_list_t* p_list);
KLB_API klb_list_iter_t* klb_list_prev(klb_list_iter_t* p_iter);


#ifdef __cplusplus
}
#endif

#endif // __KLB_LIST_H__
//end
