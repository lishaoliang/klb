///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2020, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_mnp_dev.h
/// @brief   mnp硬件设备相关定义
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_MNP_DEV_H__
#define __KLB_MNP_DEV_H__

#include "klb_type.h"
#include "mnp/klb_mnp_stream.h"
#include "image/klb_canvas.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @def   KLB_MNP_DEV_V1
/// @brief klb_mnp_dev_interface_t 接口版本
#define KLB_MNP_DEV_V1                  1


/// @brief GUI消息传递回调函数
/// @param [in] msg         消息: 参见"./klb/inc/gui/klb_msg.h"
/// @param [in] x1          点1坐标x
/// @param [in] y1          点1坐标y
/// @param [in] x2          点2坐标x
/// @param [in] y2          点2坐标y
/// @param [in] lparam      参数1
/// @param [in] lparam      参数2
/// @return 无
typedef void(*klb_mnp_msg_cb)(int msg, int x1, int y1, int x2, int y2, int lparam, int wparam);


/// @struct klb_mnp_dev_interface_t
/// @brief  设备控制接口v1
typedef struct klb_mnp_dev_interface_t_
{
    /// @brief 获取库文件版本
    /// @return uint32_t 库版本
    ///  \n eg. "v1.1.1" = 10101
    uint32_t(*version)();


    /// @brief 获取项目代号
    /// @return const char* 项目代号
    const char*(*project)();


    /// @brief 初始化
    /// @param [in] *p_json_param    json格式的参数
    /// @return int 0.成功; 非0.错误码
    int(*init)(const char* p_json_param);


    /// @brief 启动线程, 开始运行
    /// @param [in] *p_json_param    json格式的参数
    /// @return int 0.成功; 非0.错误码
    int(*start)(const char* p_json_param);


    /// @brief 关闭线程
    /// @return 无
    void(*stop)();


    /// @brief 退出
    /// @return 无
    void(*quit)();


    /// @brief 输入/输出控制命令
    /// @param [in] *p_cmd      控制命令
    /// @param [in] *p_lparam   json格式的参数1
    /// @param [in] *p_wparam   json格式的参数2
    /// @return char* 返回json字符串
    char* (*control)(const char* p_cmd, const char* p_lparam, const char* p_wparam);


    /// @brief 释放control等函数申请的内存
    /// @param [in] *ptr        内存指针
    /// @return 无
    void(*free)(char* ptr);


    /// @brief 设置数据接收者
    /// @param [in] index     接收者序号[0,16)
    /// @param [in] cb_stream 当有数据的回调函数
    /// @return 无
    void (*set_stream_receiver)(int index, klb_mnp_stream_cb cb_stream);
#define KLB_MNP_DEV_RECEIVER_MAX     16


    /// @brief 放入数据
    /// @param [in] chnn 通道
    /// @param [in] sidx 流序号
    /// @param [in] *p_stream 流数据
    /// @return int 0.成功; 非0.失败
    int(*push_stream)(uint32_t chnn, uint32_t sidx, const klb_mnp_stream_t* p_stream);


    /// @brief 获取(GUI)显存画布
    /// @param [in] index     显存序号
    /// @return klb_canvas_t* 画布指针
    ///  gui相关
    klb_canvas_t* (*get_canvas)(int index);


    /// @brief 设置(GUI)消息接收者
    /// @param [in] index     接收者序号[0,16)
    /// @param [in] cb_msg    当有消息事件时回调函数: 来源于鼠标,按键,键盘等
    /// @return 无
    ///  gui相关
    void(*set_msg_receiver)(int index, klb_mnp_msg_cb cb_msg);
}klb_mnp_dev_interface_t;


/// @brief 打开设备控制接口
/// @param [in] version         接口版本
/// @param [out] *p_interface   接口指针
/// @param [out] interface_size 接口结构体大小
/// @return int 0.成功; 非0.失败
///  \n 加载函数名称为: "klb_mnp_dev_open"
#if defined(__KLB_USE_KMNP_DEV_STATIC__)
KLB_API int klb_mnp_dev_open(uint32_t version, void* p_interface, int interface_size);
#endif
typedef int(*klb_mnp_dev_open_interface)(uint32_t version, void* p_interface, int interface_size);



#ifdef __cplusplus
}
#endif


#endif // __KLB_MNP_DEV_H__
//end
