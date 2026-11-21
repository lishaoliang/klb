// Doc Encode : UTF-8 BOM, Unix(LF)
/// @file    svn_ra_priv.h
/// @brief   svn_ra 包内私有类型与跨模块符号 (仅 core/*.c)
#ifndef __SVN_RA_PRIV_H__
#define __SVN_RA_PRIV_H__

#include "alua/svn_ra/core/svn_ra_in.h"
#include "klbnet/klb_socket.h"
#include "klbmem/klb_mem.h"

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>


#define SVN_RA_RX_INIT_CAP          4096
#define SVN_RA_DEFAULT_PORT           3690
#define SVN_RA_CONNECT_TIMEOUT_MS     30000
#define SVN_RA_SEND_BUF_CAP           8192
#define SVN_RA_DIR_STACK_MAX          64
#define SVN_RA_PATH_MAX               1024


typedef enum svn_sexpr_type_e_
{
    SVN_SEXPR_WORD = 0,
    SVN_SEXPR_NUMBER,
    SVN_SEXPR_STRING,
    SVN_SEXPR_LIST
} svn_sexpr_type_e;


typedef struct svn_sexpr_t_
{
    svn_sexpr_type_e        type;
    char*                   p_word;
    int64_t                 number;
    char*                   p_string;
    int                     string_len;     ///< 字节数; 内容可含 NUL, 勿用 strlen
    struct svn_sexpr_t_**   pp_children;
    int                     child_count;
} svn_sexpr_t;


typedef struct svn_buf_t_
{
    char*   p_data;
    int     len;
    int     cap;
} svn_buf_t;


typedef struct svn_dirent_arr_t_
{
    svn_ra_dirent_t*    p_items;
    int                 count;
    int                 cap;
} svn_dirent_arr_t;


struct svn_ra_t_
{
    klb_socket_t*   p_socket;
    char            user[256];
    char            pass[256];
    char            uuid[64];
    char            host[256];
    char            repo[256];
    int             port;
    uint8_t*        p_rx_buf;
    int             rx_len;
    int             rx_cap;
    int64_t         err_code;           ///< 最近失败 svn 错误码; 0=无
    char            err_msg[512];       ///< 最近失败消息; 空=无
    int             cat_active;         ///< 1=get-file 正文读取中
    int             cat_done;           ///< 1=正文已结束, 待收尾响应
};


typedef struct svn_ra_dir_frame_t_
{
    char    path[SVN_RA_PATH_MAX];
    char    token[32];
} svn_ra_dir_frame_t;


// md5 (svn_ra_md5.c)
void svn_hex_lower(const uint8_t* p_in, int in_len, char* p_out, int out_len);
void svn_hmac_md5(const char* p_key, const char* p_data, uint8_t digest[16]);
void svn_ra_md5_hex(const uint8_t* p_data, int len, char out_hex[33]);

// s-expression / wire (svn_ra_wire.c)
void svn_sexpr_free(svn_sexpr_t* p_node);
bool svn_sexpr_is_word(const svn_sexpr_t* p_node, const char* p_word);
const svn_sexpr_t* svn_sexpr_child(const svn_sexpr_t* p_node, int idx);
const char* svn_sexpr_word_cstr(const svn_sexpr_t* p_node);
const char* svn_sexpr_string_cstr(const svn_sexpr_t* p_node);
const char* svn_sexpr_opt_string_child(const svn_sexpr_t* p_list, int idx);
bool svn_ra_read_item(svn_ra_t* p_ra, svn_sexpr_t** pp_item);
bool svn_ra_next_is_list(svn_ra_t* p_ra);
bool svn_ra_send_item(svn_ra_t* p_ra, svn_buf_t* p_buf);
bool svn_ra_send_all(svn_ra_t* p_ra, const uint8_t* p_data, int len);
bool svn_buf_reserve(svn_buf_t* p_buf, int need_cap);
bool svn_buf_append_raw(svn_buf_t* p_buf, const char* p_s, int n);
bool svn_buf_append_cstr(svn_buf_t* p_buf, const char* p_s);
bool svn_buf_append_ch(svn_buf_t* p_buf, char ch);
bool svn_buf_open_list(svn_buf_t* p_buf);
bool svn_buf_close_list(svn_buf_t* p_buf);
bool svn_buf_append_word(svn_buf_t* p_buf, const char* p_word);
bool svn_buf_append_number(svn_buf_t* p_buf, int64_t num);
bool svn_buf_append_string(svn_buf_t* p_buf, const char* p_str);
bool svn_buf_append_string_n(svn_buf_t* p_buf, const void* p_data, int n);
bool svn_buf_append_rev_list(svn_buf_t* p_buf, int64_t rev);
bool svn_buf_append_opt_rev(svn_buf_t* p_buf, int64_t rev);
void svn_buf_clear(svn_buf_t* p_buf);
void svn_buf_free(svn_buf_t* p_buf);
bool svn_ra_handle_auth_request(svn_ra_t* p_ra, const svn_sexpr_t* p_params);
bool svn_ra_read_cmd_response(svn_ra_t* p_ra, svn_sexpr_t** pp_params);
bool svn_ra_capture_failure(svn_ra_t* p_ra, const svn_sexpr_t* p_item);
bool svn_ra_read_auth_then_response(svn_ra_t* p_ra, svn_sexpr_t** pp_params);
bool svn_ra_params_looks_like_edit_bootstrap(const svn_sexpr_t* p_params);
bool svn_ra_read_auth_only(svn_ra_t* p_ra);
bool svn_ra_send_command(svn_ra_t* p_ra, const char* p_cmd, svn_buf_t* p_params_buf);
bool svn_ra_parse_hostport(const char* p_hostport, char* p_host, int host_len, int* p_port);
bool svn_ra_build_url(const char* p_host, int port, const char* p_repo, char* p_url, int url_len);
bool svn_ra_copy_string_field(char* p_dst, int dst_len, const char* p_src);
bool svn_ra_join_path(char* p_out, int out_len, const char* p_base, const char* p_name);
bool svn_ra_list_internal(svn_ra_t* p_ra, const char* p_path, int64_t rev,
                          svn_dirent_arr_t* p_arr, bool recurse, const char* p_rel_base);
bool svn_ra_get_dir_response(svn_ra_t* p_ra, const char* p_path, int64_t rev,
                             svn_sexpr_t** pp_resp);

// commit helpers (svn_ra_commit.c)
bool svn_ra_make_svndiff0_new(const uint8_t* p_data, int len, svn_buf_t* p_out);
bool svn_ra_make_svndiff0_replace(const uint8_t* p_old, int old_len,
                                  const uint8_t* p_new, int new_len, svn_buf_t* p_out);
bool svn_ra_norm_path(const char* p_in, char* p_out, int out_len);
bool svn_ra_path_parent(const char* p_path, char* p_parent, int parent_len);
bool svn_ra_send_editor_cmd(svn_ra_t* p_ra, const char* p_cmd, svn_buf_t* p_params);
bool svn_ra_path_is_ancestor(const char* p_parent, const char* p_child);
bool svn_ra_dir_stack_close_to(svn_ra_t* p_ra, svn_ra_dir_frame_t* p_stack, int* p_depth,
                               const char* p_keep_prefix);
bool svn_ra_dir_stack_ensure(svn_ra_t* p_ra, svn_ra_dir_frame_t* p_stack, int* p_depth,
                             int* p_tok_seq, const char* p_dir_path, int64_t base_rev);
bool svn_ra_commit_put_one(svn_ra_t* p_ra, svn_ra_dir_frame_t* p_stack, int* p_depth,
                           int* p_tok_seq, const svn_ra_commit_op_t* p_op, int64_t base_rev,
                           const uint8_t* p_old_data, int old_data_len);
bool svn_ra_commit_mkdir_one(svn_ra_t* p_ra, svn_ra_dir_frame_t* p_stack, int* p_depth,
                             int* p_tok_seq, const char* p_path, int64_t base_rev);
bool svn_ra_commit_delete_one(svn_ra_t* p_ra, svn_ra_dir_frame_t* p_stack, int* p_depth,
                              int* p_tok_seq, const svn_ra_commit_op_t* p_op, int64_t base_rev);
bool svn_ra_commit_copy_one(svn_ra_t* p_ra, svn_ra_dir_frame_t* p_stack, int* p_depth,
                                   int* p_tok_seq, const svn_ra_commit_op_t* p_op, int64_t base_rev,
                                   const uint8_t* p_src_body, int src_body_len);
bool svn_ra_commit_prop_one(svn_ra_t* p_ra, svn_ra_dir_frame_t* p_stack, int* p_depth,
                                   int* p_tok_seq, const svn_ra_commit_op_t* p_ops, int op_count,
                                   int prop_index, int64_t base_rev,
                                   const uint8_t** pp_put_old, const int* p_put_old_len);


#endif // __SVN_RA_PRIV_H__

// end
