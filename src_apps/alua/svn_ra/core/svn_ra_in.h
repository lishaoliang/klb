// Doc Encode : UTF-8 BOM, Unix(LF)
/// @file    svn_ra_in.h
/// @brief   svn_ra 包内 C 接口 (不对外; 供 core / svn_ra_klua)
#ifndef __SVN_RA_IN_H__
#define __SVN_RA_IN_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct svn_ra_t_ svn_ra_t;


/// @struct svn_ra_dirent_t
/// @brief  get-dir 条目
typedef struct svn_ra_dirent_t_
{
    char        name[256];          ///< 名 (无路径)
    char        kind[16];           ///< "file" / "dir"
    int64_t     size;               ///< 字节; 目录可为 0
    int64_t     created_rev;        ///< 最后修改修订
    char        date[64];           ///< ISO 日期; 可空
    char        author[128];        ///< 作者; 可空
}svn_ra_dirent_t;


/// @brief 打开到 svnserve 的会话并完成握手/鉴权
/// @param [in] *p_hostport  host:port
/// @param [in] *p_user      用户名
/// @param [in] *p_pass      口令
/// @param [in] *p_repo      仓名 (URL 末段)
/// @return svn_ra_t* 成功非 NULL
svn_ra_t* svn_ra_open(const char* p_hostport, const char* p_user,
                      const char* p_pass, const char* p_repo);


/// @brief 关闭会话
void svn_ra_close(svn_ra_t* p_ra);


/// @brief 取最近一次失败的错误码与消息
/// @param [out] *p_code    svn 错误码; 0=无失败
/// @param [out] *p_buf     消息缓冲; 可空
/// @param [in]  buf_len    缓冲字节数
/// @return 0 成功; -1 参数错
int svn_ra_last_error(svn_ra_t* p_ra, int64_t* p_code, char* p_buf, int buf_len);


/// @brief 按日期取修订 (get-dated-rev)
/// @param [in]  *p_date    ISO 日期
/// @param [out] *p_rev     修订号
/// @return 0 成功; -1 失败
int svn_ra_get_dated_rev(svn_ra_t* p_ra, const char* p_date, int64_t* p_rev);


/// @brief 取路径在某修订被删除的修订 (get-deleted-rev)
/// @param [in]  *p_path    仓内路径
/// @param [in]  peg_rev    锚修订
/// @param [in]  end_rev    搜索上界 (<=0 时取 peg_rev)
/// @param [out] *p_del_rev  删除修订; 未删除时可为无效修订
/// @return 0 成功; -1 失败
int svn_ra_get_deleted_rev(svn_ra_t* p_ra, const char* p_path, int64_t peg_rev,
                           int64_t end_rev, int64_t* p_del_rev);


/// @brief youngest revision
int svn_ra_youngest(svn_ra_t* p_ra, int64_t* p_rev);


/// @brief 仓库 UUID (缓存于 open)
int svn_ra_uuid(svn_ra_t* p_ra, char* p_buf, int buf_len);


/// @brief 列目录 (单层)
/// @param [out] *pp_ents  条目数组; 调用方 KLB_FREE
/// @param [out] *p_count  条数
int svn_ra_list(svn_ra_t* p_ra, const char* p_path, int64_t rev,
                svn_ra_dirent_t** pp_ents, int* p_count);


/// @brief 递归列目录 (相对仓根路径, 无前导 /)
/// @note  path 字段写入 name 槽为相对仓根完整路径
int svn_ra_list_r(svn_ra_t* p_ra, const char* p_path, int64_t rev,
                  svn_ra_dirent_t** pp_ents, int* p_count);


/// @struct svn_ra_prop_t
/// @brief  目录属性条目 (get-dir 响应 props 段)
typedef struct svn_ra_prop_t_
{
    char        name[128];          ///< 属性名
    char*       p_value;            ///< 属性值; 调用方经 svn_ra_props_free 释放
    int         value_len;          ///< 值字节数
}svn_ra_prop_t;


/// @brief 取目录属性 (get-dir 响应 props 段; 值不分块)
/// @param [in] *p_path     仓内路径; 空串=仓根
/// @param [in] rev         修订; <=0 取 youngest
/// @param [out] *pp_props  属性数组; 调用方 svn_ra_props_free; 无属性时可为 NULL
/// @param [out] *p_count   条数
/// @return 0 成功; -1 失败
int svn_ra_get_dir_props(svn_ra_t* p_ra, const char* p_path, int64_t rev,
                         svn_ra_prop_t** pp_props, int* p_count);


/// @brief 释放 svn_ra_get_dir_props 返回的属性数组
void svn_ra_props_free(svn_ra_prop_t* p_props, int count);


/// @brief 取路径的 svn:mergeinfo 文本 (get-mergeinfo, 单路径)
/// @param [in] *p_path          仓内相对路径 (相对会话根; 空串=根)
/// @param [in] rev              修订; <0 取 youngest (0 为合法空树修订)
/// @param [in] inherited        true=继承 (inherited); false=显式 (explicit)
/// @param [out] **pp_mergeinfo  mergeinfo 文本; 调用方 KLB_FREE; 无 mergeinfo 时保持 NULL
/// @return 0 成功; -1 失败
int svn_ra_get_mergeinfo(svn_ra_t* p_ra, const char* p_path, int64_t rev,
                         bool inherited, char** pp_mergeinfo);


/// @struct svn_ra_locseg_t
/// @brief  get-location-segments 的单个历史段
typedef struct svn_ra_locseg_t_
{
    int64_t     range_start;        ///< 段较新端修订
    int64_t     range_end;          ///< 段较旧端修订 (<= range_start)
    char*       p_path;             ///< 段内仓内路径 (无前导 /); NULL=该段路径不存在
}svn_ra_locseg_t;


/// @brief 取路径的历史段 (ra_svn get-location-segments)
/// @param [in] *p_path     仓内相对路径 (相对会话根; 空串=根)
/// @param [in] peg_rev     锚修订; <0 交由 svnserve 取 youngest
/// @param [in] start_rev   区间较新端; <0 交由 svnserve 取 youngest
/// @param [in] end_rev     区间较旧端; <0 交由 svnserve 取 0
/// @param [out] **pp_segs  段数组; 调用方 svn_ra_locsegs_free
/// @param [out] *p_count   段数
/// @return 0 成功; -1 失败
int svn_ra_get_location_segments(svn_ra_t* p_ra, const char* p_path, int64_t peg_rev,
                                 int64_t start_rev, int64_t end_rev,
                                 svn_ra_locseg_t** pp_segs, int* p_count);


/// @brief 释放 svn_ra_get_location_segments 返回的段数组
void svn_ra_locsegs_free(svn_ra_locseg_t* p_segs, int count);


/// @enum svn_ra_edit_evt_e
/// @brief  replay 编辑器事件类型 (ra_svn 编辑器命令集)
typedef enum svn_ra_edit_evt_e_
{
    SVN_RA_EDIT_EVT_TARGET_REV = 0,     ///< target-rev
    SVN_RA_EDIT_EVT_OPEN_ROOT,          ///< open-root
    SVN_RA_EDIT_EVT_DELETE_ENTRY,       ///< delete-entry
    SVN_RA_EDIT_EVT_ADD_DIR,            ///< add-dir
    SVN_RA_EDIT_EVT_OPEN_DIR,           ///< open-dir
    SVN_RA_EDIT_EVT_CHANGE_DIR_PROP,    ///< change-dir-prop
    SVN_RA_EDIT_EVT_CLOSE_DIR,          ///< close-dir
    SVN_RA_EDIT_EVT_ABSENT_DIR,         ///< absent-dir
    SVN_RA_EDIT_EVT_ADD_FILE,           ///< add-file
    SVN_RA_EDIT_EVT_OPEN_FILE,          ///< open-file
    SVN_RA_EDIT_EVT_APPLY_TEXTDELTA,    ///< apply-textdelta
    SVN_RA_EDIT_EVT_TEXTDELTA_CHUNK,    ///< textdelta-chunk
    SVN_RA_EDIT_EVT_TEXTDELTA_END,      ///< textdelta-end
    SVN_RA_EDIT_EVT_CHANGE_FILE_PROP,   ///< change-file-prop
    SVN_RA_EDIT_EVT_CLOSE_FILE,         ///< close-file
    SVN_RA_EDIT_EVT_ABSENT_FILE,        ///< absent-file
    SVN_RA_EDIT_EVT_CLOSE_EDIT,         ///< close-edit
    SVN_RA_EDIT_EVT_FINISH_REPLAY,      ///< finish-replay (replay 专用收尾)
}svn_ra_edit_evt_e;


/// @struct svn_ra_edit_evt_t
/// @brief  replay 单个编辑器事件
typedef struct svn_ra_edit_evt_t_
{
    int         type;               ///< 事件类型 (svn_ra_edit_evt_e)
    int         has_rev;            ///< 1=rev 有效
    int64_t     rev;                ///< target-rev / open-* / delete / copyfrom 修订
    char*       p_path;             ///< 节点路径 (仓内相对, 无前导 /); 无则 NULL
    char*       p_token;            ///< 节点 token
    char*       p_parent_token;     ///< 父目录 token
    char*       p_name;             ///< 属性名
    char*       p_value;            ///< 属性值; NULL 且 p_name 非空=删除属性
    char*       p_checksum;         ///< 文本/基准校验和 (MD5 hex)
    char*       p_copy_path;        ///< 复制来源路径 (add-dir/add-file); 无则 NULL
    char*       p_chunk;            ///< textdelta 分块 (原始 svndiff 字节, 可含 NUL)
    int         chunk_len;          ///< p_chunk 字节数
}svn_ra_edit_evt_t;


/// @brief 发起 replay (ra_svn replay), 之后逐条读编辑器事件
/// @param [in] rev           目标修订
/// @param [in] low_water_mark 低水位修订
/// @param [in] send_deltas   是否下发文本增量
/// @return 0 成功; -1 失败
int svn_ra_replay_begin(svn_ra_t* p_ra, int64_t rev, int64_t low_water_mark, bool send_deltas);


/// @brief 读下一个 replay 编辑器事件 (读到 finish-replay 后 done=true 并收尾响应)
/// @param [in] p_ra          ra 会话
/// @param [out] p_evt        事件 (调用方负责 svn_ra_edit_evt_free)
/// @param [out] p_done       true=replay 已结束
/// @return 0 成功; -1 失败
int svn_ra_replay_next(svn_ra_t* p_ra, svn_ra_edit_evt_t* p_evt, bool* p_done);


/// @brief 释放 replay 事件内部分配的字符串
void svn_ra_edit_evt_free(svn_ra_edit_evt_t* p_evt);


/// @brief 取文件内容
/// @param [out] *pp_data  内容; 调用方 KLB_FREE
/// @param [out] *p_len    长度
int svn_ra_cat(svn_ra_t* p_ra, const char* p_path, int64_t rev,
               uint8_t** pp_data, int* p_len);


/// @brief 开始流式取文件 (get-file 正文逐块读取)
/// @param [in] *p_path  仓内路径
/// @param [in] rev      修订; <=0 取 youngest
/// @return 0 成功; -1 失败
/// @note 须与 svn_ra_cat_next / svn_ra_cat_close 成对调用; 同一会话同时仅一个流
int svn_ra_cat_begin(svn_ra_t* p_ra, const char* p_path, int64_t rev);


/// @brief 取下一块正文
/// @param [out] *pp_chunk  块数据; 调用方 KLB_FREE; eof 时为 NULL
/// @param [out] *p_len     块字节数; eof 时为 0
/// @param [out] *p_eof     true=正文结束
/// @return 0 成功; -1 失败
int svn_ra_cat_next(svn_ra_t* p_ra, uint8_t** pp_chunk, int* p_len, bool* p_eof);


/// @brief 结束流式取文件 (未读完时丢弃剩余正文并消费收尾响应)
/// @return 0 成功; -1 失败
int svn_ra_cat_close(svn_ra_t* p_ra);


#define SVN_RA_LOG_PATH_MAX     1024


/// @struct svn_ra_log_changed_path_t
/// @brief  log 单条 changed-path
typedef struct svn_ra_log_changed_path_t_
{
    char        path[SVN_RA_LOG_PATH_MAX];      ///< 仓内绝对路径 (/foo)
    char        action;                         ///< A/D/R/M
    char        copyfrom_path[SVN_RA_LOG_PATH_MAX];
    int64_t     copyfrom_rev;                   ///< <0 表示无
    char        node_kind[16];                  ///< file / dir
    int         text_mods;                        ///< 0/1; -1 未知
    int         prop_mods;                        ///< 0/1; -1 未知
} svn_ra_log_changed_path_t;


/// @struct svn_ra_log_entry_t
/// @brief  log 条目 (作者/日期/日志 + 可选 changed-paths)
typedef struct svn_ra_log_entry_t_
{
    int64_t                     rev;                ///< 修订号
    char                        author[128];        ///< 作者; 可空
    char                        date[64];           ///< ISO 日期; 可空
    char                        message[4096];      ///< svn:log; 可空
    svn_ra_log_changed_path_t*  p_changed_paths;    ///< changed-paths; 可空
    int                         changed_path_count;
} svn_ra_log_entry_t;


/// @brief 释放 svn_ra_log 返回的条目数组 (含嵌套 changed-paths)
void svn_ra_log_entries_free(svn_ra_log_entry_t* p_ents, int count);


/// @brief 取修订日志
/// @param [in] **pp_paths     路径表 (相对仓根); 可空表示仓根
/// @param [in] path_count     路径数; <0 视为 0
/// @param [in] start_rev      起始修订; <0 表示未指定 (svnserve 默认)
/// @param [in] end_rev        结束修订; <0 表示未指定
/// @param [in] limit          条数上限; 0=不限
/// @param [in] changed_paths  1=要 changed-paths
/// @param [out] *pp_ents      条目数组; 调用方 svn_ra_log_entries_free
/// @param [out] *p_count      条数
/// @return 0 成功; -1 失败
int svn_ra_log(svn_ra_t* p_ra, const char** pp_paths, int path_count,
               int64_t start_rev, int64_t end_rev, int limit, int changed_paths,
               svn_ra_log_entry_t** pp_ents, int* p_count);


/// @brief commit 操作类型
#define SVN_RA_OP_MKDIR   1
#define SVN_RA_OP_DELETE  2
#define SVN_RA_OP_PUT     3
#define SVN_RA_OP_COPY    4
#define SVN_RA_OP_PROP    5


/// @struct svn_ra_commit_op_t
/// @brief  单次 commit 内的一条变更
typedef struct svn_ra_commit_op_t_
{
    int             type;               ///< SVN_RA_OP_*
    const char*     p_path;             ///< 相对仓根; 无前导 /
    int64_t         base_rev;           ///< delete/open 基线; <0 用 commit base
    const uint8_t*  p_data;             ///< PUT 正文或 svndiff
    int             data_len;           ///< 字节数
    int             data_is_svndiff;    ///< 1=已是 svndiff; 0=明文包成 svndiff0
    int             is_add;             ///< PUT: 1=add-file; 0=open-file
    const char*     p_copyfrom_path;    ///< COPY: 源路径 (仓内)
    int64_t         copyfrom_rev;       ///< COPY: 源修订; <0 无效
    int             copy_is_dir;        ///< COPY: 1=目录; 0=文件
    const char*     p_prop_name;      ///< PROP: 属性名
    const char*     p_prop_value;     ///< PROP: 值; NULL=删除
    int             prop_is_dir;      ///< PROP: 1=目录; 0=文件
} svn_ra_commit_op_t;


/// @brief 一次 commit (editor + svndiff0)
/// @param [in] *p_log      日志; 可空
/// @param [in] *p_ops      操作数组; 顺序执行
/// @param [in] op_count    条数; >0
/// @param [out] *p_new_rev 新修订; 可空
/// @return 0 成功; -1 失败
int svn_ra_commit(svn_ra_t* p_ra, const char* p_log,
                  const svn_ra_commit_op_t* p_ops, int op_count,
                  int64_t* p_new_rev);


/// @brief svndiff0/1 应用到源全文 (失败时尝试空源回退)
/// @param [out] *pp_plain  明文; 调用方 KLB_FREE
/// @param [out] *p_plain_len  字节数
/// @return true 成功
bool svn_ra_svndiff_apply(const uint8_t* p_src, int src_len,
                          const uint8_t* p_diff, int diff_len,
                          uint8_t** pp_plain, int* p_plain_len);

/// @brief SHA1 十六进制 (update-report sha1-checksum 用)
/// @param [in] p_data  原文; 可为 NULL
/// @param [in] len     字节数
/// @param [out] out_hex 41 字节缓冲
void svn_ra_sha1_hex(const uint8_t* p_data, int len, char out_hex[41]);


#if defined(__cplusplus)
}
#endif

#endif // __SVN_RA_IN_H__

// end
