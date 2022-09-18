///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    CMultiplex.hpp
/// @brief   socket/fd 复用
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_MULTIPLEX_HPP__
#define __KLB_MULTIPLEX_HPP__


#include "klb_type.h"
#include "klbnet/klb_multiplex.h"


namespace klb {

KLB_EXTERN class KLB_API_CPP CMultiplex
{
public:
    CMultiplex();
    CMultiplex(int64_t tc);
    ~CMultiplex();

    /// @brief 获取 klb_multiplex_t*
    klb_multiplex_t* GetMultiplex();

    /// @brief 放入socket
    /// @param [in]  *p_socket              socket
    /// @param [in]  *p_ops                 附加的参数
    /// @return int 返回ID编号; 大于0的正整数
    int PushSocket(klb_socket_t* p_socket, const klb_multiplex_ops_t* p_ops);

    /// @brief 移除id对应
    /// @param [in]  id                     ID编号
    /// @return int 0.成功; 非0.失败
    /// @note 必须等到 klb_multiplex_obj_t.cb_remove 函数被调用, 才正式移除完成
    int Remove(int id);

    /// @brief 调用一次; 需要定期调用
    /// @param [in] now                     当前系统滴答数
    /// @return int 0
    int LoopOnce(int64_t now);

private:
    klb_multiplex_t*       m_multiplex;     ///< 复用对象
};

} // namespace klb

#endif // __KLB_MULTIPLEX_HPP__
