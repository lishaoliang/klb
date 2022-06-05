///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    CKlua.hpp
/// @brief   
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __CKLUA_HPP__
#define __CKLUA_HPP__

#include "klb_type.h"
#include "klua/klua.h"
#include "klbutil/CMap.hpp"

namespace klb {

    KLB_EXTERN class KLB_API_CPP CKlua
    {
    public:
        CKlua();
        ~CKlua();


        static bool ParseToMap(lua_State* L, int idx, CMap& map);
        static void PushMapToLua(lua_State* L, CMap& map);

    private:

    };

}

#endif // __CKLUA_HPP__
