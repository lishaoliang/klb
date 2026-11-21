// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbmp4.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klb 可裁剪包 klbmp4 对外入口 (根目录仅暴露外部接口; 实现见 core/ mp4_klua/)
/// @version 0.7
/// @history 修改历史
///  \n [2026] 创建文件; klbmp4_close 改为返回状态; 写侧落盘失败可被感知; 增加 info / seek / 打开失败错误码; 读侧记录宽高与 profile/level
///  \n [2026-11] 错码对齐 klb_basecode_e (与 klbcore.klbcode 一致); EOF=1 读完
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBMP4_H__
#define __KLBMP4_H__

#include "klb_type.h"
#include "klbbase/klb_basecode.h"
#include "klbmem/klb_buf.h"
#include "klua/klua.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klbmp4_t_ klbmp4_t;


/// @def KLBMP4_EOF
/// @brief 读侧已读完 (数值 1, 同 KLB_NETCODE_DISCONNECT / socket DISCONNECT)
#define KLBMP4_EOF      1


/// @struct klbmp4_info_t
/// @brief  MP4 探测信息
typedef struct klbmp4_info_t_
{
    int         width;          ///< 宽; 未知为 0
    int         height;         ///< 高; 未知为 0
    int         profile;        ///< avcC profile; 未知为 0
    int         level;          ///< avcC level; 未知为 0
    int         sample_count;   ///< 样本数; 写侧为已写帧数
    double      fps;            ///< 帧率; 无法计算为 0
    int64_t     timescale;      ///< 媒体时间基
    int64_t     duration_ms;    ///< 时长(ms); 未知为 0
}klbmp4_info_t;


/// @brief 打开 MP4 只读
/// @param [in] *p_path     文件路径
/// @return klbmp4_t* 成功非 NULL; 失败 NULL
/// @note 非分片单路 avc1. moov 可在文件头或尾. 其它子集返回 NULL
KLB_API klbmp4_t* klbmp4_open_read(const char* p_path);


/// @brief 打开 MP4 只读(带错误码)
/// @param [in]  *p_path    文件路径
/// @param [out] *p_err     失败时 klb_basecode_e; 可为 NULL
/// @return klbmp4_t* 成功非 NULL; 失败 NULL
KLB_API klbmp4_t* klbmp4_open_read_ex(const char* p_path, int* p_err);


/// @brief 打开 MP4 只写
/// @param [in] *p_path     文件路径
/// @return klbmp4_t* 成功非 NULL; 失败 NULL
/// @note 关闭时写出 ftyp + mdat + moov. 调用方交 Annex-B 访问单元
KLB_API klbmp4_t* klbmp4_open_write(const char* p_path);


/// @brief 打开 MP4 只写(带错误码)
/// @param [in]  *p_path    文件路径
/// @param [out] *p_err     失败时 klb_basecode_e; 可为 NULL
/// @return klbmp4_t* 成功非 NULL; 失败 NULL
KLB_API klbmp4_t* klbmp4_open_write_ex(const char* p_path, int* p_err);


/// @brief 关闭 MP4
/// @param [in] *p_mp4      句柄; 可为 NULL
/// @return int 0 成功; -1 写侧落盘失败(ftyp/mdat/moov 未写出)
/// @note 只读句柄恒返回 0. 无论成功失败句柄均已释放, 调用方不得再用
KLB_API int klbmp4_close(klbmp4_t* p_mp4);


/// @brief 读下一访问单元
/// @param [in]  *p_mp4     句柄
/// @param [out] *p_frame   成功时为 klb_buf_t: [klb_mnp_media_t][载荷]
/// @return int KLB_BASECODE_OK 成功; KLBMP4_EOF 已读完; 其它为失败码 (现多为 KLB_BASECODE_EPERM)
/// @note 成功时 *p_frame 为 klb_buf_malloc 结果, 调用方 free. 关键帧前拼 SPS/PPS
/// @note time 与 FLV demux 相同: 媒体毫秒乘 1000. 非同步样本记为 P 帧
KLB_API int klbmp4_read(klbmp4_t* p_mp4, klb_buf_t** p_frame);


/// @brief 探测 MP4 信息
/// @param [in]  *p_mp4     句柄
/// @param [out] *p_info    输出信息
/// @return int 0 成功; -1 失败
/// @note 读侧取 moov 记录; 写侧按已写帧统计(时长按末帧间隔推算)
KLB_API int klbmp4_info(klbmp4_t* p_mp4, klbmp4_info_t* p_info);


/// @brief 按时间定位读位置
/// @param [in] *p_mp4      只读句柄
/// @param [in] time_ms     目标时间(ms)
/// @return int 0 成功; -1 失败
/// @note 定位到首个 time_ms >= 目标 的样本; 超过末尾则其后 read 返回 1(读完)
/// @note 非关键帧感知; 需要从关键帧起解码时由调用方自行回退
KLB_API int klbmp4_seek(klbmp4_t* p_mp4, int64_t time_ms);


/// @brief 写一访问单元
/// @param [in] *p_mp4      句柄
/// @param [in] *p_frame    [klb_mnp_media_t][载荷]
/// @return int 0 成功; 非 0 失败
/// @note 载荷为 Annex-B, 含 NAL 头. 首帧须带 SPS 与 PPS. time 单位同 read
KLB_API int klbmp4_write(klbmp4_t* p_mp4, const klb_buf_t* p_frame);


/// @brief 扩展库"kmp4"
/// @param [in] *L          Lua状态
/// @return int 返回1
/// @note open_read / open_write; 帧为 lightuserdata(klb_buf_t*), 读侧须 free_frame
KLB_API int klua_open_kmp4(lua_State* L);


#ifdef __cplusplus
}
#endif

#endif // __KLBMP4_H__

// end
