// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klua/CKluaEnv.hpp"
#include "klbmem/klb_mem.h"
#include <assert.h>

namespace klb {

CKluaEnv::CKluaEnv(lua_CFunction cb_pre_load)
{
    m_is_share_env = false;
    m_env = klua_env_create(cb_pre_load);
}

// 共享 *p_env 指针, *p_env 的生命周期由创建者维护
CKluaEnv::CKluaEnv(klua_env_t* p_env, bool share)
{
    m_is_share_env = true;
    m_env = p_env;
}

CKluaEnv::~CKluaEnv()
{
    if (!m_is_share_env)
    {
        KLB_FREE_BY(m_env, klua_env_destroy);
    }
}

/////////////////////////////////////

void CKluaEnv::SetUserdata(void* p_udata)
{
    klua_env_set_udata(m_env, p_udata);
}

void* CKluaEnv::GetUserdata()
{
    return klua_env_get_udata(m_env);
}

int64_t CKluaEnv::GetTickCount()
{
    return klua_env_get_tick_count(m_env);
}

void CKluaEnv::UpdateTickCount()
{
    klua_env_update_tick_count(m_env);
}

int CKluaEnv::DoFile(const char* p_entry)
{
    return klua_env_dofile(m_env, p_entry);
}

int CKluaEnv::DoLibrary(const char* p_entry)
{
    return klua_env_dolibrary(m_env, p_entry);
}

int CKluaEnv::DoEnd()
{
    return klua_env_doend(m_env);
}

lua_State* CKluaEnv::GetL()
{
    return klua_env_get_L(m_env);
}

int CKluaEnv::Report(int status)
{
    return klua_env_report(m_env, status);
}

int CKluaEnv::LoopOnce()
{
    return klua_env_loop_once(m_env);
}

bool CKluaEnv::IsExit()
{
    return klua_env_is_exit(m_env);
}

void CKluaEnv::Exit()
{
    klua_env_exit(m_env);
}

void CKluaEnv::SetName(const char* p_name, size_t name_len)
{
    klua_env_set_name(m_env, p_name, name_len);
}

void CKluaEnv::GetName(std::string& name)
{
    const sds str = klua_env_get_name(m_env);
    if (str)
    {
        name = str;
    }
}

void CKluaEnv::SetArgs(const char* p_data, int data_len)
{
    klua_env_set_args(m_env, p_data, data_len);
}

const klb_buf_t* CKluaEnv::GetArgs()
{
    return klua_env_get_args(m_env);
}

/////////////////////////////////////


} // namespace klb
