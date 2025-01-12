///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2024, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    CKluaExGui.hpp
/// @brief   cpp gui
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __CKLUAEXGUI_H__
#define __CKLUAEXGUI_H__


#include "klb_type.h"
#include "klua/klua_env.h"
#include "klbgui/CGui.hpp"

namespace klb {


#define CKLUA_EX_GUI_NAME       "_CKLUA_EX_GUI_"


/// @class CKluaExGui
/// @brief cpp gui
class CKluaExGui
{
public:
    /////////////////////////////////////////////////
    // 构造 / 析构
    CKluaExGui(klb_gui_t* p_gui, bool share);
    ~CKluaExGui();


protected:
    int OnCtrl(klua_env_t* p_env, int opt, uint8_t* p_param_in_out, int param_size);
    int OnMsg(klua_env_t* p_env, int64_t now, klua_msg_t* p_msg);
    int OnLoopOnce(klua_env_t* p_env, int64_t last_tc, int64_t now);


private:
    klbui::CGui         m_cpp_gui;      ///< CPP gui


private:   
    static void* OptCreate(klua_env_t* p_env);
    static void OptDestroy(void* ptr);
    static int OptCtrl(void* ptr, klua_env_t* p_env, int opt, uint8_t* p_param_in_out, int param_size);
    static int OptMsg(void* ptr, klua_env_t* p_env, int64_t now, klua_msg_t* p_msg);
    static int OptLoopOnce(void* ptr, klua_env_t* p_env, int64_t last_tc, int64_t now);

public:
    static int RegisterGuiEx(klua_env_t* p_env);

    static CKluaExGui* GetGuiEx(klua_env_t* p_env);
    static CKluaExGui* GetGuiExByL(lua_State* L);
};

} // namespace klb

#endif // __CKLUAEXGUI_H__
//end
