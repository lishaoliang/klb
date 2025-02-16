///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    CKluaEnv.hpp
/// @brief   klua evn
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __CKLUA_ENV_HPP__
#define __CKLUA_ENV_HPP__

#include "klb_type.h"
#include "klua/klua.h"
#include "klua/klua_env.h"
#include <string>

namespace klb {

/// @class CKluaEnv
/// @brief klua env
KLB_EXTERN class KLB_API_CPP CKluaEnv
{
public:
    CKluaEnv(lua_CFunction cb_pre_load);
    CKluaEnv(klua_env_t* p_env, bool share); // 共享 *p_env 指针, *p_env 的生命周期由创建者维护
    ~CKluaEnv();

public:
    /////////////////////////////////////

    /// @brief 设置用户数据指针(user data)
    /// @param [in] *p_udata           用户数据指针(user data)
    /// @return 无
    void SetUserdata(void* p_udata);

    /// @brief 获取用户数据指针(user data)
    /// @return void* 用户数据指针
    void* GetUserdata();

    /// @brief 获取当前lua环境滴答数
    /// @return int64_t tc
    int64_t GetTickCount();

    /// @brief 立即更新lua环境滴答数
    /// @return 无
    void UpdateTickCount();

    /// @brief 按路径方式加载一个脚本文件
    /// @param [in] *p_entry           入口lua文件路径; eg. "./test.lua"
    /// @return int 0.成功; 非0.失败
    int DoFile(const char* p_entry);

    /// @brief 按库方式加载一个脚本文件
    /// @param [in] *p_entry           入口lua文件; eg. "server.entry_gui"
    /// @return int 0.成功; 非0.失败
    int DoLibrary(const char* p_entry);

    /// @brief 结束dofile/dolibrary
    /// @param [in] *p_env             lua环境
    /// @return int 0.成功; 非0.失败
    int DoEnd();

    /// @brief 获取原始lua_State*
    /// @return lua_State* L
    lua_State* GetL();

    /// @brief 报错
    /// @param [in] status              非0时, 报错
    /// @return int status
    int Report(int status);

    /// @brief 调用一次lua环境; 需要定期调用
    /// @return int 0
    int LoopOnce();

    /// @brief 是否退出状态
    /// @return bool true.退出; false.不退出
    bool IsExit();

    /// @brief 设置退出状态
    /// @return 无
    void Exit();

    /// @brief 设置名称
    /// @return 无
    void SetName( const char* p_name, size_t name_len);

    /// @brief 获取名称
    /// @return 无
    void GetName(std::string& name);

    /// @brief 设置全局参数: 数据格式参考 luaseri_pack/luaseri_pack_from
    /// @return 无
    void SetArgs(const char* p_data, int data_len);

    /// @brief 获取全局参数: 数据格式参考 luaseri_pack/luaseri_pack_from
    /// @return const klb_buf_t*
    const klb_buf_t* GetArgs();

private:
    bool            m_is_share_env; ///< 是否共享控制
    klua_env_t*     m_env;          ///< lua 环境
};

} // namespace klb

#endif // __CKLUA_ENV_HPP__
