///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2020, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_hmap.h
/// @brief   hash table查找算法;槽(slot)大小自适应
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
///  \n 槽(slot)大小自适应过程中不可避免产生hash重排等动作; 较klb_htab_t而言,这些时间消耗是额外的
///  \n 适用于: 小数量级, 槽(slot)不固定
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_HMAP_H__
#define __KLB_HMAP_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @def   KLB_HMAP_AUTO_MALLOC
/// @brief hash table模块内部自动分配节点内存
#define KLB_HMAP_AUTO_MALLOC        0


/// @def   KLB_HMAP_NOT_MALLOC
/// @brief hash table模块内部不要分配节点内存
#define KLB_HMAP_NOT_MALLOC        1


/// @struct klb_hmap_node_t
/// @brief  hash table node节点
typedef struct klb_hmap_node_t_ klb_hmap_node_t;

#pragma pack(4)

/// @struct klb_hmap_node_t
/// @brief  hash table node节点
typedef struct klb_hmap_node_t_
{
    klb_hmap_node_t* p_next;    ///< 下一个节点

    void*       p_key;          ///< key
    uint32_t    key_len;        ///< key长度

    uint32_t    hash_idx;       ///< 第一hash值, 算索引位置
    uint32_t    hash_cmp;       ///< 第二hash值, 链表中匹配

    void*       p_data;         ///< 节点数据
}klb_hmap_node_t;

#pragma pack()


/// @struct klb_hmap_t
/// @brief  hash table查找
typedef struct klb_hmap_t_ klb_hmap_t;


/// @brief 创建hmap对象
/// @param [in] auto_malloc  节点内存使用方式: KLB_HMAP_AUTO_MALLOC.内部申请; KLB_HMAP_NOT_MALLOC.调用者传入
/// @return klb_htab_t* htab对象
klb_hmap_t* klb_hmap_create(int auto_malloc);


/// @brief 销毁hmap对象
/// @param [in] *p_htab     htab对象
/// @return 无
void klb_hmap_destroy(klb_hmap_t* p_hmap);


/// @brief 清空hmap回调函数
/// @param [in] *p_obj      传参对象
/// @param [in] *p_data     数据
/// @return int 0
typedef int(*klb_hmap_clean_cb)(void* p_obj, void* p_data);


/// @brief 清空hmap
/// @param [in] *p_list     hmap对象
/// @param [in] cb_clean    清理回调函数
/// @param [in] *p_obj      传参对象
/// @return 无
int klb_hmap_clean(klb_hmap_t* p_hmap, klb_hmap_clean_cb cb_clean, void* p_obj);


/// @brief 向hmap对象压入数据
/// @param [in] *p_htab      hmap对象
/// @param [in] *p_key       key关键字(非NULL)
/// @param [in] key_len      key长度(按char计算)
/// @param [in] *p_data      数据
/// @param [in] *p_htab_node 当KLB_HTAB_NOT_MALLOC时,传入节点指针
/// @return int 0.成功; 非0.失败
/// @note 1. htab对象并不负责数据释放
///  \n   2. 已经存在完全一致的key时, 返回失败
///  \n   3. p_key指针被缓存, 用于hash冲突时匹配
int klb_hmap_push(klb_hmap_t* p_hmap, void* p_key, uint32_t key_len, void* p_data, klb_hmap_node_t* p_hmap_node);


/// @brief 按key移除
/// @param [in] *p_hmap      hmap对象
/// @param [in] *p_key       key关键字(非NULL)
/// @param [in] key_len      key长度
/// @return void* 数据指针 或 NULL(未找到)
void* klb_hmap_remove(klb_hmap_t* p_hmap, void* p_key, uint32_t key_len);


/// @brief 按key寻找值
/// @param [in] *p_hmap      hmap对象
/// @param [in] *p_key       key关键字(非NULL)
/// @param [in] key_len      key长度
/// @return void* 数据指针 或 NULL(未找到)
void* klb_hmap_find(klb_hmap_t* p_hmap, const void* p_key, uint32_t key_len);


/// @brief 获取当前节点数目
/// @param [in] *p_hmap      hmap对象
/// @return uint32_t 当前节点数
uint32_t klb_hmap_size(klb_hmap_t* p_hmap);


#ifdef __cplusplus
}
#endif

#endif // __KLB_HMAP_H__
//end
