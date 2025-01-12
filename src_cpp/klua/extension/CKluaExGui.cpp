// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klua/extension/CKluaExGui.hpp"
#include "klua/klua_gui.h"
#include "klbgui/klb_gui.h"
#include <assert.h>

namespace klb {

/////////////////////////////////////////////////
// 构造 / 析构

CKluaExGui::CKluaExGui(klb_gui_t* p_gui, bool share) : m_cpp_gui(p_gui, share)
{

}

CKluaExGui::~CKluaExGui()
{

}

/////////////////////////////////////////////////
// 响应逻辑

int CKluaExGui::OnCtrl(klua_env_t* p_env, int opt, uint8_t* p_param_in_out, int param_size)
{
    return 0;
}

int CKluaExGui::OnMsg(klua_env_t* p_env, int64_t now, klua_msg_t* p_msg)
{
    return 0;
}

int CKluaExGui::OnLoopOnce(klua_env_t* p_env, int64_t last_tc, int64_t now)
{
    return 0;
}

/////////////////////////////////////////////////
// 扩展 适配函数

void* CKluaExGui::OptCreate(klua_env_t* p_env)
{
    klb_gui_t* p_gui = klua_gui_get(p_env);
    CKluaExGui* p_gui_ex = new CKluaExGui(p_gui, true);

    return p_gui_ex;
}

void CKluaExGui::OptDestroy(void* ptr)
{
    assert(NULL != ptr);
    CKluaExGui* p_gui_ex = (CKluaExGui*)ptr;

    delete p_gui_ex;
}

int CKluaExGui::OptCtrl(void* ptr, klua_env_t* p_env, int opt, uint8_t* p_param_in_out, int param_size)
{
    CKluaExGui* p_gui_ex = (CKluaExGui*)ptr;

    return p_gui_ex->OnCtrl(p_env, opt, p_param_in_out, param_size);
}

int CKluaExGui::OptMsg(void* ptr, klua_env_t* p_env, int64_t now, klua_msg_t* p_msg)
{
    CKluaExGui* p_gui_ex = (CKluaExGui*)ptr;

    return p_gui_ex->OnMsg(p_env, now, p_msg);
}

int CKluaExGui::OptLoopOnce(void* ptr, klua_env_t* p_env, int64_t last_tc, int64_t now)
{
    CKluaExGui* p_gui_ex = (CKluaExGui*)ptr;

    return p_gui_ex->OnLoopOnce(p_env, last_tc, now);
}

/////////////////////////////////////////////////
// 注册 / 获取

int CKluaExGui::RegisterGuiEx(klua_env_t* p_env)
{
    klua_env_extension_t ex = { 0 };

    ex.cb_create = CKluaExGui::OptCreate;
    ex.cb_destroy = CKluaExGui::OptDestroy;
    ex.cb_ctrl = CKluaExGui::OptCtrl;
    ex.cb_msg = CKluaExGui::OptMsg;
    ex.cb_loop_once = CKluaExGui::OptLoopOnce;

    klua_env_register_extension(p_env, CKLUA_EX_GUI_NAME, &ex);

    return 0;
}

CKluaExGui* CKluaExGui::GetGuiEx(klua_env_t* p_env)
{
    CKluaExGui* p_ex = (CKluaExGui*)klua_env_get_extension(p_env, CKLUA_EX_GUI_NAME);
    return p_ex;
}

CKluaExGui* CKluaExGui::GetGuiExByL(lua_State* L)
{
    return GetGuiEx(klua_env_get_by_L(L));
}


} // namespace klb
