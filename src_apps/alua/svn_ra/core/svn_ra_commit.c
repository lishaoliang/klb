// Doc Encode : UTF-8 BOM, Unix(LF)
#include "alua/svn_ra/core/svn_ra_priv.h"
#include "klbthird/zlib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// commit (editor driver)

/// 单条 textdelta-chunk 上限; svnserve 对超长单字符串会断连, 须分块发送
#define SVN_RA_TEXTDELTA_CHUNK      (1024 * 1024)


static void svn_ra_commit_trace(const char* p_where);
static void svn_ra_commit_debug_item(const char* p_label, const svn_sexpr_t* p_item);
static void svn_ra_commit_append_copyfrom_slot(svn_buf_t* p_params, const char* p_copyfrom_path,
                                                      int64_t copyfrom_rev);
static void svn_ra_commit_append_optional_rev(svn_buf_t* p_params, int64_t rev);
static void svn_ra_commit_append_optional_checksum(svn_buf_t* p_params, const char* p_checksum);
static void svn_ra_commit_append_optional_string_slot(svn_buf_t* p_params, const char* p_value);
static void svn_ra_commit_free_put_old(uint8_t** pp_put_old, int* p_put_old_len, int op_count);
static bool svn_ra_read_edit_empty_ack(svn_ra_t* p_ra);
static bool svn_ra_svndiff_apply_windows(const uint8_t* p_src, int src_len,
                                                const uint8_t* p_win, const uint8_t* p_win_end,
                                                svn_buf_t* p_out);
static bool svn_ra_svndiff0_apply(const uint8_t* p_src, int src_len,
                                         const uint8_t* p_diff, int diff_len,
                                         svn_buf_t* p_out);
static bool svn_ra_svndiff0_digest_new(const uint8_t* p_diff, int diff_len, char out_hex[33]);
static bool svn_ra_data_looks_svndiff(const uint8_t* p_data, int len);


static void svn_ra_commit_free_put_old(uint8_t** pp_put_old, int* p_put_old_len, int op_count)
{
    int i = 0;

    if (NULL == pp_put_old || NULL == p_put_old_len || op_count <= 0)
    {
        KLB_FREE(pp_put_old);
        KLB_FREE(p_put_old_len);
        return;
    }

    for (i = 0; i < op_count; ++i)
    {
        KLB_FREE(pp_put_old[i]);
    }
    KLB_FREE(pp_put_old);
    KLB_FREE(p_put_old_len);
}


static void svn_ra_commit_trace(const char* p_where)
{
    if (NULL != getenv("KSVNRA_COMMIT_DEBUG"))
    {
        fprintf(stderr, "ksvnra commit: %s\n", p_where);
        fflush(stderr);
    }
}


bool svn_buf_append_string_n(svn_buf_t* p_buf, const void* p_data, int n)
{
    char hdr[32] = { 0 };

    if (n < 0)
    {
        return false;
    }

    if (NULL == p_data)
    {
        n = 0;
        p_data = "";
    }

    snprintf(hdr, sizeof(hdr), "%d:", n);
    if (0 == svn_buf_append_cstr(p_buf, hdr))
    {
        return false;
    }

    if (n > 0 && 0 == svn_buf_append_raw(p_buf, (const char*)p_data, n))
    {
        return false;
    }

    return svn_buf_append_ch(p_buf, ' ');
}


/// @brief 头部是否确为 svndiff 流 (SVN + 版本 0..3); 仅前三字节为 SVN 的明文不算
static bool svn_ra_data_looks_svndiff(const uint8_t* p_data, int len)
{
    if (NULL == p_data || len < 4)
    {
        return false;
    }

    if (0 != memcmp(p_data, "SVN", 3))
    {
        return false;
    }

    return (p_data[3] <= 3);
}


/// @brief 追加 svndiff 可变长整数 (大端 7 位分组, 除末字节外置 0x80)
static bool svn_ra_svndiff0_varint(svn_buf_t* p_out, int value)
{
    uint8_t bytes[5] = { 0 };
    int n = 0;

    if (NULL == p_out)
    {
        return false;
    }

    do
    {
        bytes[n] = (uint8_t)(value & 0x7F);
        value >>= 7;
        ++n;
    }
    while (value > 0);

    for (int i = n - 1; i >= 0; --i)
    {
        uint8_t byte = bytes[i];

        if (i > 0)
        {
            byte = (uint8_t)(byte | 0x80);
        }
        if (0 == svn_buf_append_raw(p_out, (const char*)&byte, 1))
        {
            return false;
        }
    }

    return true;
}


/// @brief 追加 svndiff0 窗口头 (sview_off, sview_len, tview_len, ins_len, new_len)
static bool svn_ra_svndiff0_window(svn_buf_t* p_out, int sview_off, int sview_len,
                                   int tview_len, int ins_len, int new_len)
{
    if (0 == svn_ra_svndiff0_varint(p_out, sview_off))
    {
        return false;
    }
    if (0 == svn_ra_svndiff0_varint(p_out, sview_len))
    {
        return false;
    }
    if (0 == svn_ra_svndiff0_varint(p_out, tview_len))
    {
        return false;
    }
    if (0 == svn_ra_svndiff0_varint(p_out, ins_len))
    {
        return false;
    }

    return svn_ra_svndiff0_varint(p_out, new_len);
}


/// @brief 明文 → svndiff0 全新文件窗 (与 dav.lua _svndiff_new_file 对齐)
bool svn_ra_make_svndiff0_new(const uint8_t* p_data, int len, svn_buf_t* p_out)
{
    int off = 0;

    if (NULL == p_out)
    {
        return false;
    }

    if (NULL == p_data)
    {
        p_data = (const uint8_t*)"";
        len = 0;
    }

    if (0 == svn_buf_append_raw(p_out, "SVN\0", 4))
    {
        return false;
    }

    if (0 == len)
    {
        // 空目标: 无源视图/无指令/无新数据 (指令 0x80 单独出现会被解析为长度 0 + 待续 varint)
        return svn_ra_svndiff0_window(p_out, 0, 0, 0, 0, 0);
    }

    while (off < len)
    {
        int chunk = len - off;
        uint8_t hdr[6] = { 0 };

        if (chunk > 62)
        {
            chunk = 62;
        }

        // sview_off=0 sview_len=0 tview_len=chunk instr_len=1 newdata_len=chunk
        hdr[0] = 0;
        hdr[1] = 0;
        hdr[2] = (uint8_t)chunk;
        hdr[3] = 1;
        hdr[4] = (uint8_t)chunk;
        hdr[5] = (uint8_t)(0x80 | chunk);

        if (0 == svn_buf_append_raw(p_out, (const char*)hdr, 6))
        {
            return false;
        }

        if (0 == svn_buf_append_raw(p_out, (const char*)(p_data + off), chunk))
        {
            return false;
        }

        off += chunk;
    }

    return true;
}


/// @brief open-file 用: 丢弃旧全文、插入新全文 (与 dav.lua _svndiff_replace 对齐)
bool svn_ra_make_svndiff0_replace(const uint8_t* p_old, int old_len,
                                         const uint8_t* p_new, int new_len,
                                         svn_buf_t* p_out)
{
    int off = 0;

    if (NULL == p_out)
    {
        return false;
    }

    if (NULL == p_old)
    {
        p_old = (const uint8_t*)"";
        old_len = 0;
    }
    if (NULL == p_new)
    {
        p_new = (const uint8_t*)"";
        new_len = 0;
    }

    if (old_len == new_len && 0 == memcmp(p_old, p_new, (size_t)old_len))
    {
        return false;
    }

    if (0 == old_len)
    {
        return svn_ra_make_svndiff0_new(p_new, new_len, p_out);
    }

    if (0 == svn_buf_append_raw(p_out, "SVN\0", 4))
    {
        return false;
    }

    if (0 == new_len)
    {
        // 清空目标: 本流程总以 delete+add 重建文件, 源视图为空; 无指令无数据
        return svn_ra_svndiff0_window(p_out, 0, 0, 0, 0, 0);
    }

    if (old_len <= 62 && new_len <= 62)
    {
        // 小文件: 单窗全量插入 (copy from new)
        if (0 == svn_ra_svndiff0_window(p_out, 0, 0, new_len, 1, new_len))
        {
            return false;
        }

        char cmd = (char)(0x80 | new_len);

        if (0 == svn_buf_append_raw(p_out, &cmd, 1))
        {
            return false;
        }

        return svn_buf_append_raw(p_out, (const char*)p_new, new_len);
    }

    while (off < new_len)
    {
        int chunk = new_len - off;

        if (chunk > 62)
        {
            chunk = 62;
        }

        if (0 == svn_ra_svndiff0_window(p_out, 0, 0, chunk, 1, chunk))
        {
            return false;
        }

        char cmd = (char)(0x80 | chunk);

        if (0 == svn_buf_append_raw(p_out, &cmd, 1))
        {
            return false;
        }
        if (0 == svn_buf_append_raw(p_out, (const char*)(p_new + off), chunk))
        {
            return false;
        }
        off += chunk;
    }

    return true;
}


bool svn_ra_norm_path(const char* p_in, char* p_out, int out_len)
{
    const char* p = p_in;
    int n = 0;

    if (NULL == p_out || out_len <= 1)
    {
        return false;
    }

    if (NULL == p)
    {
        p = "";
    }

    while ('/' == *p)
    {
        ++p;
    }

    while ('\0' != *p && n < out_len - 1)
    {
        p_out[n++] = *p++;
    }
    p_out[n] = '\0';

    while (n > 0 && '/' == p_out[n - 1])
    {
        p_out[--n] = '\0';
    }

    return true;
}


bool svn_ra_path_parent(const char* p_path, char* p_parent, int parent_len)
{
    const char* p_slash = NULL;
    int n = 0;

    if (NULL == p_path || NULL == p_parent || parent_len <= 0)
    {
        return false;
    }

    p_slash = strrchr(p_path, '/');
    if (NULL == p_slash)
    {
        p_parent[0] = '\0';
        return true;
    }

    n = (int)(p_slash - p_path);
    if (n >= parent_len)
    {
        return false;
    }

    memcpy(p_parent, p_path, (size_t)n);
    p_parent[n] = '\0';
    return true;
}


bool svn_ra_send_editor_cmd(svn_ra_t* p_ra, const char* p_cmd, svn_buf_t* p_params)
{
    svn_sexpr_t* p_item = NULL;

    // 服务端可能在任意 editor 步后推 failure; 先读再发避免半开连接
    if (p_ra->rx_len > 0)
    {
        if (0 == svn_ra_read_item(p_ra, &p_item))
        {
            if (NULL != getenv("KSVNRA_COMMIT_DEBUG"))
            {
                fprintf(stderr, "ksvnra commit: pre-send read fail after prior cmd rx=%d\n",
                    p_ra->rx_len);
            }
            return false;
        }

        if (NULL != p_item && SVN_SEXPR_LIST == p_item->type && p_item->child_count >= 1
            && svn_sexpr_is_word(p_item->pp_children[0], "failure"))
        {
            if (NULL != getenv("KSVNRA_COMMIT_DEBUG"))
            {
                fprintf(stderr, "ksvnra commit: editor failure before %s\n",
                    (NULL != p_cmd) ? p_cmd : "?");
            }
            svn_sexpr_free(p_item);
            return false;
        }

        if (NULL != getenv("KSVNRA_COMMIT_DEBUG"))
        {
            svn_ra_commit_debug_item("pre-send-unexpected", p_item);
        }
        svn_sexpr_free(p_item);
    }

    if (0 == svn_ra_send_command(p_ra, p_cmd, p_params))
    {
        if (NULL != getenv("KSVNRA_COMMIT_DEBUG"))
        {
            fprintf(stderr, "ksvnra commit: send %s failed\n",
                (NULL != p_cmd) ? p_cmd : "?");
        }
        return false;
    }

    // editor 正常无逐步 ack; close-edit 应答由 svn_ra_commit 读, 勿在此吞掉
    return true;
}


/// @brief parent 是否为 child 的祖先路径 (含相等); 空串视为仓根祖先
bool svn_ra_path_is_ancestor(const char* p_parent, const char* p_child)
{
    int plen = 0;

    if (NULL == p_parent || NULL == p_child)
    {
        return false;
    }

    plen = (int)strlen(p_parent);
    if (0 == plen)
    {
        return true;
    }

    if (0 != strncmp(p_parent, p_child, (size_t)plen))
    {
        return false;
    }

    if ('\0' == p_child[plen] || '/' == p_child[plen])
    {
        return true;
    }

    return false;
}


bool svn_ra_dir_stack_close_to(svn_ra_t* p_ra, svn_ra_dir_frame_t* p_stack, int* p_depth,
                                      const char* p_keep_prefix)
{
    if (NULL == p_keep_prefix)
    {
        p_keep_prefix = "";
    }

    // 关掉非 keep 祖先的栈顶; 保留仓根帧
    while (*p_depth > 1)
    {
        svn_ra_dir_frame_t* p_top = &p_stack[*p_depth - 1];
        svn_buf_t params = { 0 };

        if (0 != svn_ra_path_is_ancestor(p_top->path, p_keep_prefix))
        {
            break;
        }

        svn_buf_append_string(&params, p_top->token);
        if (0 == svn_ra_send_editor_cmd(p_ra, "close-dir", &params))
        {
            svn_buf_free(&params);
            return false;
        }
        svn_buf_free(&params);
        --(*p_depth);
    }

    return true;
}


bool svn_ra_dir_stack_ensure(svn_ra_t* p_ra, svn_ra_dir_frame_t* p_stack, int* p_depth,
                                    int* p_tok_seq, const char* p_dir_path, int64_t base_rev)
{
    char want[SVN_RA_PATH_MAX] = { 0 };
    char cur[SVN_RA_PATH_MAX] = { 0 };
    const char* p = NULL;

    if (0 == svn_ra_norm_path(p_dir_path, want, (int)sizeof(want)))
    {
        return false;
    }

    if (0 == svn_ra_dir_stack_close_to(p_ra, p_stack, p_depth, want))
    {
        return false;
    }

    // current open tip path
    if (*p_depth > 0)
    {
        snprintf(cur, sizeof(cur), "%s", p_stack[*p_depth - 1].path);
    }

    if (0 == strcmp(cur, want))
    {
        return true;
    }

    // open remaining segments from cur toward want
    p = want;
    if ('\0' != cur[0])
    {
        int clen = (int)strlen(cur);
        if (0 != strncmp(want, cur, (size_t)clen) || ('\0' != want[clen] && '/' != want[clen]))
        {
            return false;
        }
        p = want + clen;
        if ('/' == *p)
        {
            ++p;
        }
    }

    while ('\0' != *p)
    {
        char seg[SVN_RA_PATH_MAX] = { 0 };
        char next_path[SVN_RA_PATH_MAX] = { 0 };
        char child_tok[32] = { 0 };
        const char* p_slash = strchr(p, '/');
        int seglen = 0;
        svn_buf_t params = { 0 };
        const char* p_parent_tok = NULL;

        if (NULL != p_slash)
        {
            seglen = (int)(p_slash - p);
        }
        else
        {
            seglen = (int)strlen(p);
        }

        if (seglen <= 0 || seglen >= (int)sizeof(seg))
        {
            return false;
        }

        memcpy(seg, p, (size_t)seglen);
        seg[seglen] = '\0';

        if ('\0' == cur[0])
        {
            snprintf(next_path, sizeof(next_path), "%s", seg);
        }
        else
        {
            snprintf(next_path, sizeof(next_path), "%s/%s", cur, seg);
        }

        if (*p_depth <= 0 || *p_depth >= SVN_RA_DIR_STACK_MAX)
        {
            return false;
        }

        p_parent_tok = p_stack[*p_depth - 1].token;
        snprintf(child_tok, sizeof(child_tok), "d%d", ++(*p_tok_seq));

        svn_buf_append_string(&params, next_path);
        svn_buf_append_string(&params, p_parent_tok);
        svn_buf_append_string(&params, child_tok);
        svn_ra_commit_append_optional_rev(&params, base_rev);

        if (0 == svn_ra_send_editor_cmd(p_ra, "open-dir", &params))
        {
            svn_buf_free(&params);
            return false;
        }
        svn_buf_free(&params);

        snprintf(p_stack[*p_depth].path, sizeof(p_stack[*p_depth].path), "%s", next_path);
        snprintf(p_stack[*p_depth].token, sizeof(p_stack[*p_depth].token), "%s", child_tok);
        ++(*p_depth);
        snprintf(cur, sizeof(cur), "%s", next_path);

        p += seglen;
        if ('/' == *p)
        {
            ++p;
        }
    }

    return true;
}


bool svn_ra_commit_put_one(svn_ra_t* p_ra, svn_ra_dir_frame_t* p_stack, int* p_depth,
                                  int* p_tok_seq, const svn_ra_commit_op_t* p_op, int64_t base_rev,
                                  const uint8_t* p_old_data, int old_data_len)
{
    char path[SVN_RA_PATH_MAX] = { 0 };
    char parent[SVN_RA_PATH_MAX] = { 0 };
    char file_tok[32] = { 0 };
    const char* p_dir_tok = NULL;
    svn_buf_t params = { 0 };
    svn_buf_t diff = { 0 };
    const uint8_t* p_diff = NULL;
    int diff_len = 0;
    char md5hex[33] = { 0 };
    int64_t use_rev = base_rev;
    bool use_del_add = false;

    if (0 == svn_ra_norm_path(p_op->p_path, path, (int)sizeof(path)) || '\0' == path[0])
    {
        return false;
    }

    if (0 == svn_ra_path_parent(path, parent, (int)sizeof(parent)))
    {
        return false;
    }

    if (0 == svn_ra_dir_stack_ensure(p_ra, p_stack, p_depth, p_tok_seq, parent, base_rev))
    {
        return false;
    }

    p_dir_tok = p_stack[*p_depth - 1].token;
    snprintf(file_tok, sizeof(file_tok), "f%d", ++(*p_tok_seq));

    if (p_op->base_rev >= 0)
    {
        use_rev = p_op->base_rev;
    }

    if (0 == p_op->is_add)
    {
        use_del_add = true;
    }

    // svndiff 编解码须在任何 editor 命令前完成: 失败若落在 apply-textdelta 之后,
    // abort-edit 会被 svnserve 报成 "svndiff 输入意料结束", 真实原因被掩盖
    if (0 != use_del_add)
    {
        svn_buf_t plain = { 0 };
        bool applied = false;

        // add-file 须相对空白的 new-file svndiff; 勿转发 ra_serf replace 窗
        if (0 != p_op->data_is_svndiff)
        {
            applied = svn_ra_svndiff0_apply(p_old_data, old_data_len,
                p_op->p_data, p_op->data_len, &plain);
            if (!applied && old_data_len > 0)
            {
                svn_buf_clear(&plain);
                applied = svn_ra_svndiff0_apply(NULL, 0,
                    p_op->p_data, p_op->data_len, &plain);
                if (applied)
                {
                    svn_ra_commit_trace("put del_add apply empty-src ok");
                }
            }
        }
        else
        {
            applied = svn_buf_append_raw(&plain, (const char*)p_op->p_data, p_op->data_len);
        }

        if (!applied)
        {
            if (NULL != getenv("KSVNRA_COMMIT_DEBUG"))
            {
                fprintf(stderr, "ksvnra commit: put del_add apply/plain fail path=%s"
                    " svndiff=%d old=%d diff=%d\n",
                    path, p_op->data_is_svndiff, old_data_len, p_op->data_len);
                fflush(stderr);
            }
            svn_buf_free(&plain);
            return false;
        }

        if (!svn_ra_make_svndiff0_new((const uint8_t*)plain.p_data, plain.len, &diff))
        {
            svn_ra_commit_trace("put del_add make svndiff fail");
            svn_buf_free(&plain);
            return false;
        }
        p_diff = (const uint8_t*)diff.p_data;
        diff_len = diff.len;
        svn_ra_md5_hex((const uint8_t*)plain.p_data, plain.len, md5hex);
        svn_buf_free(&plain);
    }
    else if (0 != p_op->data_is_svndiff)
    {
        if (0 == svn_ra_data_looks_svndiff(p_op->p_data, p_op->data_len))
        {
            // 上层按 "SVN" 前缀误判明文为 svndiff: 按明文重建, 勿让服务端解析越界
            if (NULL != getenv("KSVNRA_COMMIT_DEBUG"))
            {
                fprintf(stderr, "ksvnra commit: put svndiff misjudged plain path=%s len=%d\n",
                    path, p_op->data_len);
                fflush(stderr);
            }

            if (!svn_ra_make_svndiff0_new(p_op->p_data, p_op->data_len, &diff))
            {
                return false;
            }
            p_diff = (const uint8_t*)diff.p_data;
            diff_len = diff.len;
            svn_ra_md5_hex(p_op->p_data, p_op->data_len, md5hex);
        }
        else
        {
            p_diff = p_op->p_data;
            diff_len = p_op->data_len;
            if (0 == svn_ra_svndiff0_digest_new(p_diff, diff_len, md5hex))
            {
                md5hex[0] = '\0';
            }
        }
    }
    else
    {
        bool made = false;

        if (0 != p_op->is_add)
        {
            made = svn_ra_make_svndiff0_new(p_op->p_data, p_op->data_len, &diff);
        }
        else
        {
            made = svn_ra_make_svndiff0_replace(p_old_data, old_data_len,
                p_op->p_data, p_op->data_len, &diff);
        }

        if (!made)
        {
            svn_buf_free(&diff);
            return false;
        }
        p_diff = (const uint8_t*)diff.p_data;
        diff_len = diff.len;
        svn_ra_md5_hex(p_op->p_data, p_op->data_len, md5hex);
    }

    if (NULL != getenv("KSVNRA_COMMIT_DEBUG"))
    {
        fprintf(stderr, "ksvnra commit: put path=%s is_add=%d svndiff=%d old=%d new=%d diff=%d\n",
            path, p_op->is_add, p_op->data_is_svndiff, old_data_len, p_op->data_len, diff_len);
        if (NULL != p_diff && diff_len >= 4)
        {
            fprintf(stderr, "ksvnra commit: put diff head path=%s %02x %02x %02x %02x\n",
                path, (unsigned)p_diff[0], (unsigned)p_diff[1],
                (unsigned)p_diff[2], (unsigned)p_diff[3]);
        }
        fflush(stderr);
    }

    if (0 != use_del_add)
    {
        svn_ra_commit_op_t del_op = { 0 };

        // open-file 在 commit editor 下部分 svnserve 会 close-edit failure; delete+add 等价
        del_op.type = SVN_RA_OP_DELETE;
        del_op.p_path = path;
        del_op.base_rev = base_rev;
        if (0 == svn_ra_commit_delete_one(p_ra, p_stack, p_depth, p_tok_seq, &del_op, base_rev))
        {
            svn_buf_free(&diff);
            return false;
        }
    }

    // ra_svn: add-file ( path parent token [ copy-path copy-rev ] )
    svn_buf_clear(&params);
    svn_buf_append_string(&params, path);
    svn_buf_append_string(&params, p_dir_tok);
    svn_buf_append_string(&params, file_tok);
    svn_ra_commit_append_copyfrom_slot(&params, NULL, -1);
    if (0 == svn_ra_send_editor_cmd(p_ra, "add-file", &params))
    {
        svn_buf_free(&params);
        svn_buf_free(&diff);
        return false;
    }
    svn_buf_free(&params);

    // ra_svn: apply-textdelta ( token ( [ base-checksum ] ) )
    svn_buf_clear(&params);
    svn_buf_append_string(&params, file_tok);
    svn_ra_commit_append_optional_checksum(&params, NULL);
    if (0 == svn_ra_send_editor_cmd(p_ra, "apply-textdelta", &params))
    {
        svn_buf_free(&params);
        svn_buf_free(&diff);
        return false;
    }
    svn_buf_free(&params);

    // textdelta-chunk (分块发送; 单条过大将导致 svnserve 解析断连)
    // 多个 textdelta-chunk 在服务端按 svndiff 流顺序拼接, 等价于单条
    {
        int off = 0;

        do
        {
            int chunk_len = diff_len - off;

            if (chunk_len > SVN_RA_TEXTDELTA_CHUNK)
            {
                chunk_len = SVN_RA_TEXTDELTA_CHUNK;
            }

            svn_buf_clear(&params);
            svn_buf_append_string(&params, file_tok);
            if (0 == svn_buf_append_string_n(&params,
                    (0 < chunk_len) ? (p_diff + off) : NULL, chunk_len))
            {
                svn_buf_free(&params);
                svn_buf_free(&diff);
                return false;
            }
            if (0 == svn_ra_send_editor_cmd(p_ra, "textdelta-chunk", &params))
            {
                svn_buf_free(&params);
                svn_buf_free(&diff);
                return false;
            }

            off += chunk_len;
        }
        while (off < diff_len);

        svn_buf_free(&params);
    }

    svn_buf_clear(&params);
    svn_buf_append_string(&params, file_tok);
    if (0 == svn_ra_send_editor_cmd(p_ra, "textdelta-end", &params))
    {
        svn_buf_free(&params);
        svn_buf_free(&diff);
        return false;
    }
    svn_buf_free(&params);

    // commit 驱动 editor: 中间步无应答 (仅 close-edit / 早期 failure)

    // ra_svn: close-file ( token ( [ text-checksum ] ) )
    svn_buf_clear(&params);
    svn_buf_append_string(&params, file_tok);
    svn_ra_commit_append_optional_checksum(&params, ('\0' != md5hex[0]) ? md5hex : NULL);
    if (0 == svn_ra_send_editor_cmd(p_ra, "close-file", &params))
    {
        svn_buf_free(&params);
        svn_buf_free(&diff);
        return false;
    }
    svn_buf_free(&params);
    svn_buf_free(&diff);
    return true;
}


typedef struct svn_ra_txdelta_op_t_
{
    int action;     ///< 0=source 1=target 2=new
    int offset;
    int length;
} svn_ra_txdelta_op_t;


#define SVN_RA_TXDELTA_NEW 2


static bool svn_ra_buf_append_zeros(svn_buf_t* p_buf, int n)
{
    int i = 0;

    for (i = 0; i < n; ++i)
    {
        if (0 == svn_buf_append_ch(p_buf, '\0'))
        {
            return false;
        }
    }

    return true;
}


static bool svn_ra_build_sview(const svn_buf_t* p_stream, int sview_off, int sview_len,
                                      svn_buf_t* p_sview)
{
    int have = 0;
    int need_pad = 0;

    if (sview_len <= 0)
    {
        return true;
    }

    if (sview_off < 0)
    {
        return false;
    }

    if (sview_off < p_stream->len)
    {
        have = p_stream->len - sview_off;
        if (have > sview_len)
        {
            have = sview_len;
        }
        if (have > 0
            && 0 == svn_buf_append_raw(p_sview, p_stream->p_data + sview_off, have))
        {
            return false;
        }
    }

    need_pad = sview_len - p_sview->len;
    if (need_pad > 0 && !svn_ra_buf_append_zeros(p_sview, need_pad))
    {
        return false;
    }

    return (p_sview->len == sview_len);
}


/// @brief 解码 svndiff varint (大端, 对齐 libsvn_delta svndiff 格式)
/// @note  高位 7-bit 组在前, 除末组外置 continuation bit (0x80)
static const uint8_t* svn_ra_decode_uint(const uint8_t* p, const uint8_t* end, uint64_t* p_val)
{
    uint64_t v = 0;

    if (NULL == p || NULL == end || NULL == p_val || p >= end)
    {
        return NULL;
    }

    while (p < end)
    {
        uint8_t b = *p++;

        if (v > (UINT64_MAX >> 7))
        {
            return NULL;
        }

        v = (v << 7) | (uint64_t)(b & 0x7f);
        if (0 == (b & 0x80))
        {
            *p_val = v;
            return p;
        }
    }

    return NULL;
}


static const uint8_t* svn_ra_decode_instruction(const uint8_t* p, const uint8_t* end,
                                                         svn_ra_txdelta_op_t* p_op)
{
    uint64_t tmp = 0;
    uint8_t c = 0;

    if (NULL == p || NULL == end || NULL == p_op || p >= end)
    {
        return NULL;
    }

    c = *p++;
    p_op->action = (int)((c >> 6) & 0x03);
    if (p_op->action >= 3)
    {
        return NULL;
    }

    p_op->length = (int)(c & 0x3f);
    if (0 == p_op->length)
    {
        p = svn_ra_decode_uint(p, end, &tmp);
        if (NULL == p)
        {
            return NULL;
        }
        p_op->length = (int)tmp;
    }

    p_op->offset = 0;
    if (SVN_RA_TXDELTA_NEW != p_op->action)
    {
        p = svn_ra_decode_uint(p, end, &tmp);
        if (NULL == p)
        {
            return NULL;
        }
        p_op->offset = (int)tmp;
    }

    return p;
}


static bool svn_ra_apply_txdelta_window(const uint8_t* p_sview, int sview_len,
                                               const uint8_t* p_ins, int ins_len,
                                               const uint8_t* p_new, int new_len,
                                               int tview_len, svn_buf_t* p_window)
{
    const uint8_t* p = p_ins;
    const uint8_t* ins_end = p_ins + ins_len;
    svn_ra_txdelta_op_t ops[512] = { { 0 } };
    int nops = 0;
    int npos = 0;
    int i = 0;
    int tpos = 0;

    while (p < ins_end)
    {
        if (nops >= (int)(sizeof(ops) / sizeof(ops[0])))
        {
            return false;
        }

        p = svn_ra_decode_instruction(p, ins_end, &ops[nops]);
        if (NULL == p || ops[nops].length <= 0 || ops[nops].length > tview_len - tpos)
        {
            return false;
        }

        if (SVN_RA_TXDELTA_NEW == ops[nops].action)
        {
            ops[nops].offset = npos;
            npos += ops[nops].length;
            if (npos > new_len)
            {
                return false;
            }
        }

        tpos += ops[nops].length;
        ++nops;
    }

    if (tpos != tview_len || npos != new_len)
    {
        return false;
    }

    for (i = 0; i < nops; ++i)
    {
        const svn_ra_txdelta_op_t* op = &ops[i];

        if (0 == op->action)
        {
            if (op->offset < 0 || op->length < 0
                || op->offset + op->length > sview_len)
            {
                return false;
            }
            if (0 == svn_buf_append_raw(p_window, (const char*)(p_sview + op->offset), op->length))
            {
                return false;
            }
        }
        else if (1 == op->action)
        {
            if (op->offset < 0 || op->length < 0
                || op->offset + op->length > p_window->len)
            {
                return false;
            }
            if (0 == svn_buf_append_raw(p_window,
                    (const char*)(p_window->p_data + op->offset), op->length))
            {
                return false;
            }
        }
        else if (SVN_RA_TXDELTA_NEW == op->action)
        {
            if (op->offset < 0 || op->length < 0
                || op->offset + op->length > new_len)
            {
                return false;
            }
            if (0 == svn_buf_append_raw(p_window, (const char*)(p_new + op->offset), op->length))
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    }

    return (p_window->len == tview_len);
}


static bool svn_ra_svndiff_inflate_v1(const uint8_t* p_diff, int diff_len, svn_buf_t* p_out)
{
    z_stream strm = { 0 };
    uint8_t chunk[8192] = { 0 };
    int zret = 0;

    if (NULL == p_out || NULL == p_diff || diff_len < 5)
    {
        return false;
    }

    strm.next_in = (Bytef*)(p_diff + 5);
    strm.avail_in = (uInt)(diff_len - 5);
    if (Z_OK != inflateInit(&strm))
    {
        return false;
    }

    while (0 != strm.avail_in || Z_STREAM_END != zret)
    {
        int got = 0;

        strm.next_out = chunk;
        strm.avail_out = (uInt)sizeof(chunk);
        zret = inflate(&strm, Z_NO_FLUSH);
        if (Z_OK != zret && Z_STREAM_END != zret)
        {
            inflateEnd(&strm);
            return false;
        }

        got = (int)(sizeof(chunk) - strm.avail_out);
        if (got > 0 && 0 == svn_buf_append_raw(p_out, (const char*)chunk, got))
        {
            inflateEnd(&strm);
            return false;
        }

        if (Z_STREAM_END == zret)
        {
            break;
        }
    }

    inflateEnd(&strm);
    return (Z_STREAM_END == zret);
}


/// @brief 解析 svndiff0 窗序列 (v0 体或 svndiff1 解压后)
static bool svn_ra_svndiff_apply_windows(const uint8_t* p_src, int src_len,
                                                const uint8_t* p_win, const uint8_t* p_win_end,
                                                svn_buf_t* p_out)
{
    svn_buf_t stream = { 0 };
    svn_buf_t result = { 0 };
    const uint8_t* p = p_win;
    const uint8_t* end = p_win_end;
    int64_t last_sview_off = -1;
    int last_sview_len = -1;

    if (NULL == p_out || NULL == p_win || NULL == p_win_end || p_win > p_win_end)
    {
        return false;
    }

    if (NULL == p_src)
    {
        p_src = (const uint8_t*)"";
        src_len = 0;
    }

    if (src_len > 0
        && 0 == svn_buf_append_raw(&stream, (const char*)p_src, src_len))
    {
        return false;
    }

    while (p < end)
    {
        uint64_t sview_off_u = 0;
        uint64_t sview_len_u = 0;
        uint64_t tview_len_u = 0;
        uint64_t ins_len_u = 0;
        uint64_t new_len_u = 0;
        int sview_off = 0;
        int sview_len = 0;
        int tview_len = 0;
        int ins_len = 0;
        int new_len = 0;
        const uint8_t* p_ins = NULL;
        const uint8_t* p_new = NULL;
        svn_buf_t window = { 0 };
        svn_buf_t sview_buf = { 0 };

        p = svn_ra_decode_uint(p, end, &sview_off_u);
        if (NULL == p)
        {
            break;
        }
        p = svn_ra_decode_uint(p, end, &sview_len_u);
        if (NULL == p)
        {
            svn_buf_free(&stream);
            svn_buf_free(&result);
            return false;
        }
        p = svn_ra_decode_uint(p, end, &tview_len_u);
        if (NULL == p)
        {
            svn_buf_free(&stream);
            svn_buf_free(&result);
            return false;
        }
        p = svn_ra_decode_uint(p, end, &ins_len_u);
        if (NULL == p)
        {
            svn_buf_free(&stream);
            svn_buf_free(&result);
            return false;
        }
        p = svn_ra_decode_uint(p, end, &new_len_u);
        if (NULL == p)
        {
            svn_buf_free(&stream);
            svn_buf_free(&result);
            return false;
        }

        sview_off = (int)sview_off_u;
        sview_len = (int)sview_len_u;
        tview_len = (int)tview_len_u;
        ins_len = (int)ins_len_u;
        new_len = (int)new_len_u;

        if (sview_off < 0 || sview_len < 0 || tview_len < 0 || ins_len < 0 || new_len < 0
            || p + ins_len + new_len > end)
        {
            svn_buf_free(&stream);
            svn_buf_free(&result);
            return false;
        }

        if (sview_len > 0
            && (sview_off < last_sview_off
                || (sview_off == last_sview_off && sview_len < last_sview_len)))
        {
            svn_buf_free(&stream);
            svn_buf_free(&result);
            return false;
        }

        last_sview_off = sview_off;
        last_sview_len = sview_len;

        p_ins = p;
        p_new = p + ins_len;
        p += ins_len + new_len;

        if (!svn_ra_build_sview(&stream, sview_off, sview_len, &sview_buf))
        {
            svn_buf_free(&sview_buf);
            svn_buf_free(&stream);
            svn_buf_free(&result);
            return false;
        }

        if (!svn_ra_apply_txdelta_window((const uint8_t*)sview_buf.p_data, sview_len,
                p_ins, ins_len, p_new, new_len, tview_len, &window))
        {
            svn_buf_free(&sview_buf);
            svn_buf_free(&window);
            svn_buf_free(&stream);
            svn_buf_free(&result);
            return false;
        }

        if (0 == svn_buf_append_raw(&result, window.p_data, window.len)
            || 0 == svn_buf_append_raw(&stream, window.p_data, window.len))
        {
            svn_buf_free(&sview_buf);
            svn_buf_free(&window);
            svn_buf_free(&stream);
            svn_buf_free(&result);
            return false;
        }

        svn_buf_free(&sview_buf);
        svn_buf_free(&window);
    }

    if (p != end)
    {
        svn_buf_free(&stream);
        svn_buf_free(&result);
        return false;
    }

    svn_buf_free(&stream);
    *p_out = result;
    return true;
}


static bool svn_ra_svndiff_read_section(const uint8_t* p, int section_len, svn_buf_t* p_out)
{
    const uint8_t* end = NULL;
    uint64_t orig_len = 0;
    const uint8_t* q = NULL;
    int payload_len = 0;

    if (NULL == p_out)
    {
        return false;
    }

    if (section_len <= 0)
    {
        return true;
    }

    if (NULL == p)
    {
        return false;
    }

    end = p + section_len;
    q = svn_ra_decode_uint(p, end, &orig_len);
    if (NULL == q)
    {
        return false;
    }

    payload_len = (int)(end - q);
    if (orig_len == (uint64_t)payload_len)
    {
        return svn_buf_append_raw(p_out, (const char*)q, payload_len);
    }

    svn_ra_commit_trace("svndiff2 lz4 section unsupported");
    return false;
}


/// @brief svndiff2 窗序列 (仅未压缩 section; LZ4 段暂不支持)
static bool svn_ra_svndiff2_apply(const uint8_t* p_src, int src_len,
                                         const uint8_t* p_win, int win_len,
                                         svn_buf_t* p_out)
{
    const uint8_t* p = p_win;
    const uint8_t* end = p_win + win_len;
    svn_buf_t stream = { 0 };
    svn_buf_t result = { 0 };
    int64_t last_sview_off = -1;
    int last_sview_len = -1;

    if (NULL == p_out || NULL == p_win || win_len < 0)
    {
        return false;
    }

    if (NULL == p_src)
    {
        p_src = (const uint8_t*)"";
        src_len = 0;
    }

    if (src_len > 0
        && 0 == svn_buf_append_raw(&stream, (const char*)p_src, src_len))
    {
        return false;
    }

    while (p < end)
    {
        uint64_t sview_off_u = 0;
        uint64_t sview_len_u = 0;
        uint64_t tview_len_u = 0;
        uint64_t ins_len_u = 0;
        uint64_t new_len_u = 0;
        int sview_off = 0;
        int sview_len = 0;
        int tview_len = 0;
        int ins_len = 0;
        int new_len = 0;
        svn_buf_t ins = { 0 };
        svn_buf_t newdata = { 0 };
        svn_buf_t sview_buf = { 0 };
        svn_buf_t window = { 0 };

        p = svn_ra_decode_uint(p, end, &sview_off_u);
        if (NULL == p)
        {
            break;
        }
        p = svn_ra_decode_uint(p, end, &sview_len_u);
        if (NULL == p)
        {
            svn_buf_free(&stream);
            svn_buf_free(&result);
            return false;
        }
        p = svn_ra_decode_uint(p, end, &tview_len_u);
        if (NULL == p)
        {
            svn_buf_free(&stream);
            svn_buf_free(&result);
            return false;
        }
        p = svn_ra_decode_uint(p, end, &ins_len_u);
        if (NULL == p)
        {
            svn_buf_free(&stream);
            svn_buf_free(&result);
            return false;
        }
        p = svn_ra_decode_uint(p, end, &new_len_u);
        if (NULL == p)
        {
            svn_buf_free(&stream);
            svn_buf_free(&result);
            return false;
        }

        sview_off = (int)sview_off_u;
        sview_len = (int)sview_len_u;
        tview_len = (int)tview_len_u;
        ins_len = (int)ins_len_u;
        new_len = (int)new_len_u;

        if (sview_off < 0 || sview_len < 0 || tview_len < 0 || ins_len < 0 || new_len < 0
            || p + ins_len + new_len > end)
        {
            svn_buf_free(&stream);
            svn_buf_free(&result);
            return false;
        }

        if (sview_len > 0
            && (sview_off < last_sview_off
                || (sview_off == last_sview_off && sview_len < last_sview_len)))
        {
            svn_buf_free(&stream);
            svn_buf_free(&result);
            return false;
        }

        last_sview_off = sview_off;
        last_sview_len = sview_len;

        if (!svn_ra_svndiff_read_section(p, ins_len, &ins))
        {
            svn_buf_free(&ins);
            svn_buf_free(&stream);
            svn_buf_free(&result);
            return false;
        }
        p += ins_len;

        if (!svn_ra_svndiff_read_section(p, new_len, &newdata))
        {
            svn_buf_free(&ins);
            svn_buf_free(&newdata);
            svn_buf_free(&stream);
            svn_buf_free(&result);
            return false;
        }
        p += new_len;

        if (!svn_ra_build_sview(&stream, sview_off, sview_len, &sview_buf))
        {
            svn_buf_free(&ins);
            svn_buf_free(&newdata);
            svn_buf_free(&sview_buf);
            svn_buf_free(&stream);
            svn_buf_free(&result);
            return false;
        }

        if (!svn_ra_apply_txdelta_window((const uint8_t*)sview_buf.p_data, sview_len,
                (const uint8_t*)ins.p_data, ins.len,
                (const uint8_t*)newdata.p_data, newdata.len,
                tview_len, &window))
        {
            svn_buf_free(&ins);
            svn_buf_free(&newdata);
            svn_buf_free(&sview_buf);
            svn_buf_free(&window);
            svn_buf_free(&stream);
            svn_buf_free(&result);
            return false;
        }

        if (0 == svn_buf_append_raw(&result, window.p_data, window.len)
            || 0 == svn_buf_append_raw(&stream, window.p_data, window.len))
        {
            svn_buf_free(&ins);
            svn_buf_free(&newdata);
            svn_buf_free(&sview_buf);
            svn_buf_free(&window);
            svn_buf_free(&stream);
            svn_buf_free(&result);
            return false;
        }

        svn_buf_free(&ins);
        svn_buf_free(&newdata);
        svn_buf_free(&sview_buf);
        svn_buf_free(&window);
    }

    if (p != end)
    {
        svn_buf_free(&stream);
        svn_buf_free(&result);
        return false;
    }

    svn_buf_free(&stream);
    *p_out = result;
    return true;
}


/// @brief 将 svndiff0/1/2 应用到源全文 (对齐 libsvn_delta)
static bool svn_ra_svndiff0_apply(const uint8_t* p_src, int src_len,
                                         const uint8_t* p_diff, int diff_len,
                                         svn_buf_t* p_out)
{
    svn_buf_t inflated = { 0 };
    const uint8_t* p_win = NULL;
    const uint8_t* p_win_end = NULL;
    bool ok = false;

    if (NULL == p_out || NULL == p_diff || diff_len < 4)
    {
        return false;
    }

    if (0 != memcmp(p_diff, "SVN\0", 4))
    {
        return false;
    }

    if (diff_len >= 5 && 1 == p_diff[4])
    {
        if (!svn_ra_svndiff_inflate_v1(p_diff, diff_len, &inflated))
        {
            svn_ra_commit_trace("svndiff1 inflate fail");
            return false;
        }
        p_win = (const uint8_t*)inflated.p_data;
        p_win_end = p_win + inflated.len;
        ok = svn_ra_svndiff_apply_windows(p_src, src_len, p_win, p_win_end, p_out);
        svn_buf_free(&inflated);
        return ok;
    }

    if (diff_len >= 5 && 2 == p_diff[4])
    {
        return svn_ra_svndiff2_apply(p_src, src_len, p_diff + 5, diff_len - 5, p_out);
    }

    p_win = p_diff + 4;
    p_win_end = p_diff + diff_len;
    ok = svn_ra_svndiff_apply_windows(p_src, src_len, p_win, p_win_end, p_out);
    return ok;
}


bool svn_ra_svndiff_apply(const uint8_t* p_src, int src_len,
                          const uint8_t* p_diff, int diff_len,
                          uint8_t** pp_plain, int* p_plain_len)
{
    svn_buf_t plain = { 0 };

    if (NULL == pp_plain || NULL == p_plain_len)
    {
        return false;
    }

    *pp_plain = NULL;
    *p_plain_len = 0;

    if (svn_ra_svndiff0_apply(p_src, src_len, p_diff, diff_len, &plain))
    {
        goto ok_out;
    }

    if (src_len > 0
        && svn_ra_svndiff0_apply(NULL, 0, p_diff, diff_len, &plain))
    {
        goto ok_out;
    }

    svn_buf_free(&plain);
    return false;

ok_out:
    if (plain.len > 0)
    {
        *pp_plain = (uint8_t*)KLB_MALLOC(uint8_t, (size_t)plain.len, 0);
        if (NULL == *pp_plain)
        {
            svn_buf_free(&plain);
            return false;
        }
        memcpy(*pp_plain, plain.p_data, (size_t)plain.len);
        *p_plain_len = plain.len;
    }
    else
    {
        *pp_plain = NULL;
        *p_plain_len = 0;
    }

    svn_buf_free(&plain);
    return true;
}


/// @brief 从 svndiff 提取新全文并算 close-file text MD5
static bool svn_ra_svndiff0_digest_new(const uint8_t* p_diff, int diff_len, char out_hex[33])
{
    svn_buf_t text = { 0 };

    if (NULL == p_diff || diff_len < 4 || NULL == out_hex)
    {
        return false;
    }

    out_hex[0] = '\0';
    if (!svn_ra_svndiff0_apply(NULL, 0, p_diff, diff_len, &text))
    {
        return false;
    }

    if (text.len <= 0)
    {
        svn_buf_free(&text);
        return false;
    }

    svn_ra_md5_hex((const uint8_t*)text.p_data, text.len, out_hex);
    svn_buf_free(&text);
    return ('\0' != out_hex[0]);
}


bool svn_ra_commit_mkdir_one(svn_ra_t* p_ra, svn_ra_dir_frame_t* p_stack, int* p_depth,
                                    int* p_tok_seq, const char* p_path, int64_t base_rev)
{
    char path[SVN_RA_PATH_MAX] = { 0 };
    char parent[SVN_RA_PATH_MAX] = { 0 };
    char child_tok[32] = { 0 };
    svn_buf_t params = { 0 };

    if (0 == svn_ra_norm_path(p_path, path, (int)sizeof(path)) || '\0' == path[0])
    {
        return false;
    }

    if (0 == svn_ra_path_parent(path, parent, (int)sizeof(parent)))
    {
        return false;
    }

    if (0 == svn_ra_dir_stack_ensure(p_ra, p_stack, p_depth, p_tok_seq, parent, base_rev))
    {
        return false;
    }

    if (*p_depth >= SVN_RA_DIR_STACK_MAX)
    {
        return false;
    }

    snprintf(child_tok, sizeof(child_tok), "d%d", ++(*p_tok_seq));
    svn_buf_append_string(&params, path);
    svn_buf_append_string(&params, p_stack[*p_depth - 1].token);
    svn_buf_append_string(&params, child_tok);
    svn_ra_commit_append_copyfrom_slot(&params, NULL, -1);

    if (0 == svn_ra_send_editor_cmd(p_ra, "add-dir", &params))
    {
        svn_buf_free(&params);
        return false;
    }
    svn_buf_free(&params);

    snprintf(p_stack[*p_depth].path, sizeof(p_stack[*p_depth].path), "%s", path);
    snprintf(p_stack[*p_depth].token, sizeof(p_stack[*p_depth].token), "%s", child_tok);
    ++(*p_depth);
    return true;
}


static void svn_ra_commit_append_copyfrom_slot(svn_buf_t* p_params, const char* p_copyfrom_path,
                                                      int64_t copyfrom_rev)
{
    svn_buf_open_list(p_params);
    if (NULL != p_copyfrom_path && '\0' != p_copyfrom_path[0] && copyfrom_rev >= 0)
    {
        svn_buf_append_string(p_params, p_copyfrom_path);
        svn_buf_append_number(p_params, copyfrom_rev);
    }
    svn_buf_close_list(p_params);
}


static void svn_ra_commit_append_optional_rev(svn_buf_t* p_params, int64_t rev)
{
    svn_buf_open_list(p_params);
    if (rev >= 0)
    {
        svn_buf_append_number(p_params, rev);
    }
    svn_buf_close_list(p_params);
}


static void svn_ra_commit_append_optional_checksum(svn_buf_t* p_params, const char* p_checksum)
{
    svn_buf_open_list(p_params);
    if (NULL != p_checksum && '\0' != p_checksum[0])
    {
        svn_buf_append_string(p_params, p_checksum);
    }
    svn_buf_close_list(p_params);
}


static void svn_ra_commit_append_optional_string_slot(svn_buf_t* p_params, const char* p_value)
{
    svn_buf_open_list(p_params);
    if (NULL != p_value)
    {
        svn_buf_append_string(p_params, p_value);
    }
    svn_buf_close_list(p_params);
}


bool svn_ra_commit_copy_one(svn_ra_t* p_ra, svn_ra_dir_frame_t* p_stack, int* p_depth,
                                   int* p_tok_seq, const svn_ra_commit_op_t* p_op, int64_t base_rev,
                                   const uint8_t* p_src_body, int src_body_len)
{
    char path[SVN_RA_PATH_MAX] = { 0 };
    char parent[SVN_RA_PATH_MAX] = { 0 };
    char copyfrom[SVN_RA_PATH_MAX] = { 0 };
    char copyfrom_url[SVN_RA_PATH_MAX + 512] = { 0 };
    char repo_url[512] = { 0 };
    char child_tok[32] = { 0 };
    svn_buf_t params = { 0 };
    const char* p_cmd = NULL;
    const char* p_copyfrom_path = NULL;

    if (NULL == p_op || NULL == p_op->p_copyfrom_path || '\0' == p_op->p_copyfrom_path[0]
        || p_op->copyfrom_rev < 0)
    {
        return false;
    }

    if (0 == svn_ra_norm_path(p_op->p_copyfrom_path, copyfrom, (int)sizeof(copyfrom))
        || '\0' == copyfrom[0])
    {
        return false;
    }

    if (0 == svn_ra_build_url(p_ra->host, p_ra->port, p_ra->repo, repo_url, (int)sizeof(repo_url)))
    {
        return false;
    }

    snprintf(copyfrom_url, sizeof(copyfrom_url), "%s/%s", repo_url, copyfrom);
    p_copyfrom_path = copyfrom_url;

    if (0 == svn_ra_norm_path(p_op->p_path, path, (int)sizeof(path)) || '\0' == path[0])
    {
        return false;
    }

    if (0 == svn_ra_path_parent(path, parent, (int)sizeof(parent)))
    {
        return false;
    }

    if (0 == svn_ra_dir_stack_ensure(p_ra, p_stack, p_depth, p_tok_seq, parent, base_rev))
    {
        return false;
    }

    if (0 != p_op->copy_is_dir)
    {
        if (*p_depth >= SVN_RA_DIR_STACK_MAX)
        {
            return false;
        }

        snprintf(child_tok, sizeof(child_tok), "d%d", ++(*p_tok_seq));
        p_cmd = "add-dir";
    }
    else
    {
        snprintf(child_tok, sizeof(child_tok), "f%d", ++(*p_tok_seq));
        p_cmd = "add-file";
    }

    svn_buf_append_string(&params, path);
    svn_buf_append_string(&params, p_stack[*p_depth - 1].token);
    svn_buf_append_string(&params, child_tok);
    svn_ra_commit_append_copyfrom_slot(&params, p_copyfrom_path, p_op->copyfrom_rev);

    if (0 == svn_ra_send_editor_cmd(p_ra, p_cmd, &params))
    {
        svn_buf_free(&params);
        return false;
    }
    svn_buf_free(&params);

    if (0 != p_op->copy_is_dir)
    {
        snprintf(p_stack[*p_depth].path, sizeof(p_stack[*p_depth].path), "%s", path);
        snprintf(p_stack[*p_depth].token, sizeof(p_stack[*p_depth].token), "%s", child_tok);
        ++(*p_depth);
    }
    else
    {
        // copyfrom 无 apply-textdelta; close-file 勿带 text-checksum
        svn_buf_clear(&params);
        svn_buf_append_string(&params, child_tok);
        svn_ra_commit_append_optional_checksum(&params, NULL);
        if (0 == svn_ra_send_editor_cmd(p_ra, "close-file", &params))
        {
            svn_buf_free(&params);
            return false;
        }
        svn_buf_free(&params);
    }

    return true;
}


static bool svn_ra_commit_same_file_path(const char* p_a, const char* p_b)
{
    char path_a[SVN_RA_PATH_MAX] = { 0 };
    char path_b[SVN_RA_PATH_MAX] = { 0 };

    if (NULL == p_a || NULL == p_b)
    {
        return false;
    }

    if (0 == svn_ra_norm_path(p_a, path_a, (int)sizeof(path_a))
        || 0 == svn_ra_norm_path(p_b, path_b, (int)sizeof(path_b)))
    {
        return false;
    }

    return (0 == strcmp(path_a, path_b));
}


/// @brief txn 内是否已有同路径 PUT (其正文本即 prop 组的 body 来源)
static bool svn_ra_commit_has_put_for(const svn_ra_commit_op_t* p_ops, int op_count,
                                      const char* p_path)
{
    int j = 0;

    if (NULL == p_ops || NULL == p_path)
    {
        return false;
    }

    for (j = 0; j < op_count; ++j)
    {
        if (SVN_RA_OP_PUT == p_ops[j].type
            && svn_ra_commit_same_file_path(p_path, p_ops[j].p_path))
        {
            return true;
        }
    }

    return false;
}


static bool svn_ra_commit_put_plaintext(const svn_ra_commit_op_t* p_put,
                                               const uint8_t* p_old_data, int old_data_len,
                                               svn_buf_t* p_out)
{
    if (NULL == p_put || NULL == p_out)
    {
        return false;
    }

    if (0 != p_put->data_is_svndiff)
    {
        return svn_ra_svndiff0_apply(p_old_data, old_data_len,
            p_put->p_data, p_put->data_len, p_out);
    }

    if (NULL == p_put->p_data || p_put->data_len <= 0)
    {
        return false;
    }

    return svn_buf_append_raw(p_out, (const char*)p_put->p_data, p_put->data_len);
}


static bool svn_ra_commit_body_for_prop(const svn_ra_commit_op_t* p_ops, int op_count,
                                               int prop_index,
                                               const uint8_t** pp_put_old, const int* p_put_old_len,
                                               svn_buf_t* p_out)
{
    const svn_ra_commit_op_t* p_prop = NULL;
    int j = 0;

    if (NULL == p_ops || prop_index < 0 || prop_index >= op_count || NULL == p_out)
    {
        return false;
    }

    p_prop = &p_ops[prop_index];

    if (NULL == p_prop->p_path)
    {
        return false;
    }

    svn_buf_clear(p_out);

    for (j = 0; j < op_count; ++j)
    {
        const svn_ra_commit_op_t* p_put = &p_ops[j];
        const uint8_t* p_old = NULL;
        int old_len = 0;

        if (SVN_RA_OP_PUT != p_put->type || NULL == p_put->p_path)
        {
            continue;
        }

        if (0 == svn_ra_commit_same_file_path(p_prop->p_path, p_put->p_path))
        {
            continue;
        }

        p_old = pp_put_old[j];
        old_len = p_put_old_len[j];
        if (NULL == p_old && NULL != pp_put_old[prop_index])
        {
            p_old = pp_put_old[prop_index];
            old_len = p_put_old_len[prop_index];
        }

        return svn_ra_commit_put_plaintext(p_put, p_old, old_len, p_out);
    }

    if (NULL != pp_put_old[prop_index] && p_put_old_len[prop_index] > 0)
    {
        return svn_buf_append_raw(p_out, (const char*)pp_put_old[prop_index],
            p_put_old_len[prop_index]);
    }

    return false;
}


static bool svn_ra_commit_same_prop_file(const svn_ra_commit_op_t* p_a,
                                                const svn_ra_commit_op_t* p_b)
{
    if (NULL == p_a || NULL == p_b)
    {
        return false;
    }

    if (SVN_RA_OP_PROP != p_a->type || SVN_RA_OP_PROP != p_b->type)
    {
        return false;
    }

    if (0 != p_a->prop_is_dir || 0 != p_b->prop_is_dir)
    {
        return false;
    }

    if (NULL == p_a->p_path || NULL == p_b->p_path)
    {
        return false;
    }

    return (0 == strcmp(p_a->p_path, p_b->p_path));
}


static bool svn_ra_commit_prop_file_group(svn_ra_t* p_ra, svn_ra_dir_frame_t* p_stack,
                                                 int* p_depth, int* p_tok_seq,
                                                 const svn_ra_commit_op_t* p_ops, int op_count,
                                                 int start_i, int64_t base_rev,
                                                 const uint8_t** pp_file_body,
                                                 const int* p_file_body_len, int* p_next_i)
{
    char path[SVN_RA_PATH_MAX] = { 0 };
    char parent[SVN_RA_PATH_MAX] = { 0 };
    char file_tok[32] = { 0 };
    char md5hex[33] = { 0 };
    svn_buf_t params = { 0 };
    svn_buf_t body = { 0 };
    int64_t use_rev = base_rev;
    int end_i = start_i;
    int k = 0;

    if (NULL == p_ops || start_i < 0 || start_i >= op_count || NULL == p_next_i)
    {
        return false;
    }

    if (0 == svn_ra_norm_path(p_ops[start_i].p_path, path, (int)sizeof(path)) || '\0' == path[0])
    {
        return false;
    }

    if (p_ops[start_i].base_rev >= 0)
    {
        use_rev = p_ops[start_i].base_rev;
    }

    while (end_i < op_count && svn_ra_commit_same_prop_file(&p_ops[start_i], &p_ops[end_i]))
    {
        ++end_i;
    }

    if (0 == svn_ra_path_parent(path, parent, (int)sizeof(parent)))
    {
        return false;
    }

    if (0 == svn_ra_dir_stack_ensure(p_ra, p_stack, p_depth, p_tok_seq, parent, base_rev))
    {
        return false;
    }

    snprintf(file_tok, sizeof(file_tok), "f%d", ++(*p_tok_seq));
    if (!svn_ra_commit_body_for_prop(p_ops, op_count, start_i,
            pp_file_body, p_file_body_len, &body))
    {
        svn_buf_free(&body);
        return false;
    }

    svn_ra_md5_hex((const uint8_t*)body.p_data, body.len, md5hex);

    svn_buf_clear(&params);
    svn_buf_append_string(&params, path);
    svn_buf_append_string(&params, p_stack[*p_depth - 1].token);
    svn_buf_append_string(&params, file_tok);
    svn_ra_commit_append_optional_rev(&params, use_rev);
    if (0 == svn_ra_send_editor_cmd(p_ra, "open-file", &params))
    {
        svn_buf_free(&params);
        svn_buf_free(&body);
        return false;
    }
    svn_buf_free(&params);

    for (k = start_i; k < end_i; ++k)
    {
        const svn_ra_commit_op_t* p_op = &p_ops[k];

        svn_buf_clear(&params);
        svn_buf_append_string(&params, file_tok);
        svn_buf_append_string(&params, p_op->p_prop_name);
        svn_ra_commit_append_optional_string_slot(&params, p_op->p_prop_value);
        if (0 == svn_ra_send_editor_cmd(p_ra, "change-file-prop", &params))
        {
            svn_buf_free(&params);
            svn_buf_free(&body);
            return false;
        }
        svn_buf_free(&params);
    }

    svn_buf_clear(&params);
    svn_buf_append_string(&params, file_tok);
    svn_ra_commit_append_optional_checksum(&params,
        ('\0' != md5hex[0]) ? md5hex : NULL);
    if (0 == svn_ra_send_editor_cmd(p_ra, "close-file", &params))
    {
        svn_buf_free(&params);
        svn_buf_free(&body);
        return false;
    }
    svn_buf_free(&params);
    svn_buf_free(&body);

    *p_next_i = end_i;
    return true;
}


bool svn_ra_commit_prop_one(svn_ra_t* p_ra, svn_ra_dir_frame_t* p_stack, int* p_depth,
                                   int* p_tok_seq, const svn_ra_commit_op_t* p_ops, int op_count,
                                   int prop_index, int64_t base_rev,
                                   const uint8_t** pp_put_old, const int* p_put_old_len)
{
    char path[SVN_RA_PATH_MAX] = { 0 };
    char parent[SVN_RA_PATH_MAX] = { 0 };
    char node_tok[32] = { 0 };
    svn_buf_t params = { 0 };
    svn_buf_t body = { 0 };
    int64_t use_rev = base_rev;
    const char* p_chg_cmd = NULL;
    const char* p_close_cmd = NULL;
    const svn_ra_commit_op_t* p_op = NULL;

    if (NULL == p_ops || prop_index < 0 || prop_index >= op_count)
    {
        return false;
    }

    p_op = &p_ops[prop_index];

    if (NULL == p_op->p_prop_name || '\0' == p_op->p_prop_name[0])
    {
        return false;
    }

    // dir prop may target repo root (empty path); file prop must have a path
    if (0 == svn_ra_norm_path(p_op->p_path, path, (int)sizeof(path)))
    {
        return false;
    }

    if ('\0' == path[0] && 0 == p_op->prop_is_dir)
    {
        return false;
    }

    if (p_op->base_rev >= 0)
    {
        use_rev = p_op->base_rev;
    }

    if (0 != p_op->prop_is_dir)
    {
        if (0 == svn_ra_dir_stack_ensure(p_ra, p_stack, p_depth, p_tok_seq, path, base_rev))
        {
            return false;
        }

        snprintf(node_tok, sizeof(node_tok), "%s", p_stack[*p_depth - 1].token);
        p_chg_cmd = "change-dir-prop";
        // dir stays on stack; close-dir in commit epilogue (avoid double-close)
        p_close_cmd = NULL;
    }
    else
    {
        if (0 == svn_ra_path_parent(path, parent, (int)sizeof(parent)))
        {
            return false;
        }

        if (0 == svn_ra_dir_stack_ensure(p_ra, p_stack, p_depth, p_tok_seq, parent, base_rev))
        {
            return false;
        }

        snprintf(node_tok, sizeof(node_tok), "f%d", ++(*p_tok_seq));

        // ra_svn: open-file ( path parent token ( rev ) )
        svn_buf_clear(&params);
        svn_buf_append_string(&params, path);
        svn_buf_append_string(&params, p_stack[*p_depth - 1].token);
        svn_buf_append_string(&params, node_tok);
        svn_ra_commit_append_optional_rev(&params, use_rev);
        if (0 == svn_ra_send_editor_cmd(p_ra, "open-file", &params))
        {
            svn_buf_free(&params);
            return false;
        }
        svn_buf_free(&params);
        p_chg_cmd = "change-file-prop";
        p_close_cmd = "close-file";
    }

    svn_buf_clear(&params);
    svn_buf_append_string(&params, node_tok);
    svn_buf_append_string(&params, p_op->p_prop_name);
    svn_ra_commit_append_optional_string_slot(&params, p_op->p_prop_value);
    if (0 == svn_ra_send_editor_cmd(p_ra, p_chg_cmd, &params))
    {
        svn_buf_free(&params);
        return false;
    }
    svn_buf_free(&params);

    if (NULL == p_close_cmd)
    {
        return true;
    }

    svn_buf_clear(&params);
    svn_buf_append_string(&params, node_tok);
    if (0 == strcmp(p_close_cmd, "close-file"))
    {
        char md5hex[33] = { 0 };

        if (!svn_ra_commit_body_for_prop(p_ops, op_count, prop_index,
                pp_put_old, p_put_old_len, &body))
        {
            svn_buf_free(&body);
            svn_buf_free(&params);
            return false;
        }

        svn_ra_md5_hex((const uint8_t*)body.p_data, body.len, md5hex);
        svn_buf_free(&body);
        svn_ra_commit_append_optional_checksum(&params,
            ('\0' != md5hex[0]) ? md5hex : NULL);
    }
    if (0 == svn_ra_send_editor_cmd(p_ra, p_close_cmd, &params))
    {
        svn_buf_free(&params);
        return false;
    }
    svn_buf_free(&params);
    return true;
}


bool svn_ra_commit_delete_one(svn_ra_t* p_ra, svn_ra_dir_frame_t* p_stack, int* p_depth,
                                     int* p_tok_seq, const svn_ra_commit_op_t* p_op, int64_t base_rev)
{
    char path[SVN_RA_PATH_MAX] = { 0 };
    char parent[SVN_RA_PATH_MAX] = { 0 };
    svn_buf_t params = { 0 };
    int64_t use_rev = base_rev;

    if (0 == svn_ra_norm_path(p_op->p_path, path, (int)sizeof(path)) || '\0' == path[0])
    {
        return false;
    }

    if (p_op->base_rev >= 0)
    {
        use_rev = p_op->base_rev;
    }

    if (0 == svn_ra_path_parent(path, parent, (int)sizeof(parent)))
    {
        return false;
    }

    if (0 == svn_ra_dir_stack_ensure(p_ra, p_stack, p_depth, p_tok_seq, parent, base_rev))
    {
        return false;
    }

    // ra_svn: delete-entry ( path ( rev ) dir-token )
    svn_buf_append_string(&params, path);
    svn_ra_commit_append_optional_rev(&params, use_rev);
    svn_buf_append_string(&params, p_stack[*p_depth - 1].token);

    if (0 == svn_ra_send_editor_cmd(p_ra, "delete-entry", &params))
    {
        svn_buf_free(&params);
        return false;
    }
    svn_buf_free(&params);
    return true;
}


static void svn_ra_commit_debug_item(const char* p_label, const svn_sexpr_t* p_item)
{
    int i = 0;

    if (NULL == getenv("KSVNRA_COMMIT_DEBUG") || NULL == p_label)
    {
        return;
    }

    if (NULL == p_item)
    {
        fprintf(stderr, "ksvnra commit: %s null\n", p_label);
        return;
    }

    fprintf(stderr, "ksvnra commit: %s type=%d children=%d\n",
        p_label, (int)p_item->type, p_item->child_count);

    for (i = 0; i < p_item->child_count && i < 8; ++i)
    {
        const svn_sexpr_t* p_c = p_item->pp_children[i];

        if (NULL == p_c)
        {
            continue;
        }
        if (SVN_SEXPR_WORD == p_c->type && NULL != p_c->p_word)
        {
            fprintf(stderr, "ksvnra commit:   [%d] word %s\n", i, p_c->p_word);
        }
        else if (SVN_SEXPR_NUMBER == p_c->type)
        {
            fprintf(stderr, "ksvnra commit:   [%d] num %lld\n", i, (long long)p_c->number);
        }
        else if (SVN_SEXPR_STRING == p_c->type && NULL != p_c->p_string)
        {
            fprintf(stderr, "ksvnra commit:   [%d] str %s\n", i, p_c->p_string);
        }
        else if (SVN_SEXPR_LIST == p_c->type)
        {
            int j = 0;

            fprintf(stderr, "ksvnra commit:   [%d] list n=%d\n", i, p_c->child_count);
            for (j = 0; j < p_c->child_count && j < 4; ++j)
            {
                const svn_sexpr_t* p_sub = p_c->pp_children[j];

                if (NULL != p_sub && SVN_SEXPR_STRING == p_sub->type && NULL != p_sub->p_string)
                {
                    fprintf(stderr, "ksvnra commit:     [%d.%d] str %s\n", i, j, p_sub->p_string);
                }
                else if (NULL != p_sub && SVN_SEXPR_WORD == p_sub->type && NULL != p_sub->p_word)
                {
                    fprintf(stderr, "ksvnra commit:     [%d.%d] word %s\n", i, j, p_sub->p_word);
                }
            }
        }
    }
    fflush(stderr);
}


/// @brief 从 commit-info 帧解析 new-rev (裸 ( rev ... ) 或 success 包裹)
static bool svn_ra_commit_try_parse_rev(const svn_sexpr_t* p_item, int64_t* p_new_rev)
{
    const svn_sexpr_t* p_rev_node = NULL;
    int ci = 0;

    if (NULL == p_item || SVN_SEXPR_LIST != p_item->type || p_item->child_count < 1)
    {
        return false;
    }

    if (SVN_SEXPR_NUMBER == p_item->pp_children[0]->type)
    {
        p_rev_node = p_item->pp_children[0];
    }
    else if (svn_sexpr_is_word(p_item->pp_children[0], "success") && p_item->child_count >= 2)
    {
        const svn_sexpr_t* p_inner = p_item->pp_children[1];

        if (NULL != p_inner && SVN_SEXPR_LIST == p_inner->type)
        {
            for (ci = 0; ci < p_inner->child_count; ++ci)
            {
                const svn_sexpr_t* p_c = p_inner->pp_children[ci];

                if (NULL != p_c && SVN_SEXPR_NUMBER == p_c->type)
                {
                    p_rev_node = p_c;
                    break;
                }
            }
        }
    }

    if (NULL == p_rev_node)
    {
        return false;
    }

    if (NULL != p_new_rev)
    {
        *p_new_rev = p_rev_node->number;
    }

    return true;
}


/// @brief close-edit 后读 auth / commit-info (commit-info 常为裸 rev 帧)
static bool svn_ra_commit_read_commit_info(svn_ra_t* p_ra, int64_t* p_new_rev)
{
    int i = 0;

    for (i = 0; i < 8; ++i)
    {
        svn_sexpr_t* p_item = NULL;
        svn_sexpr_t* p_params = NULL;

        if (0 == svn_ra_read_item(p_ra, &p_item))
        {
            break;
        }

        svn_ra_commit_debug_item("post-close-edit", p_item);

        if (svn_ra_commit_try_parse_rev(p_item, p_new_rev))
        {
            svn_sexpr_free(p_item);
            return true;
        }

        if (NULL != p_item && SVN_SEXPR_LIST == p_item->type)
        {
            if (0 == p_item->child_count)
            {
                svn_sexpr_free(p_item);
                continue;
            }

            if (p_item->child_count >= 1
                && svn_sexpr_is_word(p_item->pp_children[0], "success"))
            {
                if (p_item->child_count >= 2)
                {
                    p_params = p_item->pp_children[1];
                    if (SVN_SEXPR_LIST == p_params->type)
                    {
                        p_item->pp_children[1] = NULL;
                    }
                    else
                    {
                        p_params = NULL;
                    }
                }

                svn_sexpr_free(p_item);

                if (NULL != p_params)
                {
                    if (!svn_ra_handle_auth_request(p_ra, p_params))
                    {
                        svn_sexpr_free(p_params);
                        break;
                    }
                    svn_sexpr_free(p_params);
                }
                continue;
            }

            if (!svn_ra_handle_auth_request(p_ra, p_item))
            {
                svn_sexpr_free(p_item);
                break;
            }
        }

        svn_sexpr_free(p_item);
    }

    return false;
}


/// @brief editor 阶段 ack: 裸 ( ) 或 ( success ( ) )
static bool svn_ra_read_edit_empty_ack(svn_ra_t* p_ra)
{
    svn_sexpr_t* p_item = NULL;

    if (0 == svn_ra_read_item(p_ra, &p_item))
    {
        if (NULL != getenv("KSVNRA_COMMIT_DEBUG"))
        {
            fprintf(stderr, "ksvnra commit: read_edit_empty_ack eof rx=%d\n", p_ra->rx_len);
        }
        return false;
    }

    svn_ra_commit_debug_item("edit_ack", p_item);

    if (NULL == p_item || SVN_SEXPR_LIST != p_item->type)
    {
        svn_sexpr_free(p_item);
        return false;
    }

    if (0 == p_item->child_count)
    {
        svn_sexpr_free(p_item);
        return true;
    }

    if (p_item->child_count >= 1
        && svn_sexpr_is_word(p_item->pp_children[0], "success"))
    {
        svn_sexpr_free(p_item);
        return true;
    }

    if (p_item->child_count >= 1
        && svn_sexpr_is_word(p_item->pp_children[0], "failure"))
    {
        svn_ra_commit_debug_item("edit_ack failure", p_item);
        if (p_item->child_count >= 2)
        {
            svn_ra_commit_debug_item("edit_ack failure err", p_item->pp_children[1]);
        }
        svn_sexpr_free(p_item);
        return false;
    }

    if (p_item->child_count >= 1
        && SVN_SEXPR_WORD == p_item->pp_children[0]->type)
    {
        if (svn_sexpr_is_word(p_item->pp_children[0], "true")
            || svn_sexpr_is_word(p_item->pp_children[0], "false"))
        {
            svn_sexpr_free(p_item);
            return true;
        }
    }

    svn_sexpr_free(p_item);
    return false;
}


int svn_ra_commit(svn_ra_t* p_ra, const char* p_log,
                  const svn_ra_commit_op_t* p_ops, int op_count,
                  int64_t* p_new_rev)
{
    svn_buf_t params = { 0 };
    svn_sexpr_t* p_resp = NULL;
    svn_ra_dir_frame_t stack[SVN_RA_DIR_STACK_MAX];
    int depth = 0;
    int tok_seq = 0;
    int64_t base_rev = 0;
    int i = 0;
    uint8_t** pp_put_old = NULL;
    int* p_put_old_len = NULL;

    if (NULL == p_ra || NULL == p_ops || op_count <= 0)
    {
        return -1;
    }

    if (NULL == p_log)
    {
        p_log = "";
    }

    if (0 != svn_ra_youngest(p_ra, &base_rev))
    {
        svn_ra_commit_trace("youngest fail");
        return -1;
    }

    pp_put_old = (uint8_t**)KLB_MALLOCZ(uint8_t*, (size_t)op_count, 0);
    p_put_old_len = (int*)KLB_MALLOCZ(int, (size_t)op_count, 0);
    if (NULL == pp_put_old || NULL == p_put_old_len)
    {
        KLB_FREE(pp_put_old);
        KLB_FREE(p_put_old_len);
        return -1;
    }

    for (i = 0; i < op_count; ++i)
    {
        const svn_ra_commit_op_t* p_op = &p_ops[i];
        int64_t use_rev = base_rev;

        if (SVN_RA_OP_PUT == p_op->type && 0 == p_op->is_add)
        {
            if (p_op->base_rev >= 0)
            {
                use_rev = p_op->base_rev;
            }

            if (0 != svn_ra_cat(p_ra, p_op->p_path, use_rev, &pp_put_old[i], &p_put_old_len[i]))
            {
                svn_ra_commit_free_put_old(pp_put_old, p_put_old_len, op_count);
                svn_ra_commit_trace("put prefetch cat fail");
                return -1;
            }
        }
        else if (SVN_RA_OP_PROP == p_op->type && 0 == p_op->prop_is_dir)
        {
            // 同路径已有 PUT 时 body 取 PUT 正文; 新增文件在当前基版无此路径, 预取必失败
            if (0 != svn_ra_commit_has_put_for(p_ops, op_count, p_op->p_path))
            {
                continue;
            }

            if (p_op->base_rev >= 0)
            {
                use_rev = p_op->base_rev;
            }

            if (0 != svn_ra_cat(p_ra, p_op->p_path, use_rev, &pp_put_old[i], &p_put_old_len[i]))
            {
                svn_ra_commit_free_put_old(pp_put_old, p_put_old_len, op_count);
                svn_ra_commit_trace("prop prefetch cat fail");
                return -1;
            }
        }
        else if (SVN_RA_OP_COPY == p_op->type && 0 == p_op->copy_is_dir)
        {
            int64_t cf_rev = p_op->copyfrom_rev;

            if (cf_rev < 0)
            {
                cf_rev = base_rev;
            }

            if (0 != svn_ra_cat(p_ra, p_op->p_copyfrom_path, cf_rev,
                    &pp_put_old[i], &p_put_old_len[i]))
            {
                svn_ra_commit_free_put_old(pp_put_old, p_put_old_len, op_count);
                svn_ra_commit_trace("copy prefetch cat fail");
                return -1;
            }
        }
    }

    // ra_svn: commit ( logmsg [ ( lock-path lock-token ) ... ] keep-locks [ rev-props ] )
    svn_buf_append_string(&params, p_log);
    svn_buf_open_list(&params);
    svn_buf_close_list(&params);
    svn_buf_append_word(&params, "false");

    if (0 == svn_ra_send_command(p_ra, "commit", &params))
    {
        svn_buf_free(&params);
        svn_ra_commit_trace("send commit fail");
        return -1;
    }
    svn_buf_free(&params);

    // ra_svn: commit 后首帧为编辑引导 ( success ( ( ) 0: ) ) 或裸 ( ); 已鉴权勿走 auth 分支
    svn_ra_commit_trace("after send commit");
    if (0 == svn_ra_read_cmd_response(p_ra, &p_resp))
    {
        svn_ra_commit_trace("commit resp fail");
        return -1;
    }

    svn_ra_commit_debug_item("commit resp", p_resp);
    svn_sexpr_free(p_resp);
    p_resp = NULL;

    if (p_ra->rx_len > 0)
    {
        svn_ra_commit_trace("commit extra rx");
        if (0 == svn_ra_read_edit_empty_ack(p_ra))
        {
            svn_ra_commit_trace("commit extra ack fail");
            return -1;
        }
    }

    // ra_svn: open-root ( [ rev ] root-token ); 无 rev 时发 ( )
    svn_buf_clear(&params);
    svn_buf_open_list(&params);
    svn_buf_close_list(&params);
    svn_buf_append_string(&params, "d0");
    if (0 == svn_ra_send_editor_cmd(p_ra, "open-root", &params))
    {
        svn_buf_free(&params);
        svn_ra_commit_trace("open-root fail");
        return -1;
    }
    svn_buf_free(&params);

    memset(stack, 0, sizeof(stack));
    snprintf(stack[0].path, sizeof(stack[0].path), "%s", "");
    snprintf(stack[0].token, sizeof(stack[0].token), "%s", "d0");
    depth = 1;
    tok_seq = 0;

    for (i = 0; i < op_count; ++i)
    {
        const svn_ra_commit_op_t* p_op = &p_ops[i];
        int ok_op = 0;

        if (NULL != getenv("KSVNRA_COMMIT_DEBUG"))
        {
            fprintf(stderr, "ksvnra commit: op[%d/%d] type=%d path=%s\n",
                i, op_count, (int)p_op->type, p_op->p_path);
            fflush(stderr);
        }

        if (SVN_RA_OP_MKDIR == p_op->type)
        {
            ok_op = svn_ra_commit_mkdir_one(p_ra, stack, &depth, &tok_seq, p_op->p_path, base_rev) ? 1 : 0;
        }
        else if (SVN_RA_OP_DELETE == p_op->type)
        {
            ok_op = svn_ra_commit_delete_one(p_ra, stack, &depth, &tok_seq, p_op, base_rev) ? 1 : 0;
        }
        else if (SVN_RA_OP_PUT == p_op->type)
        {
            ok_op = svn_ra_commit_put_one(p_ra, stack, &depth, &tok_seq, p_op, base_rev,
                pp_put_old[i], p_put_old_len[i]) ? 1 : 0;
        }
        else if (SVN_RA_OP_COPY == p_op->type)
        {
            ok_op = svn_ra_commit_copy_one(p_ra, stack, &depth, &tok_seq, p_op, base_rev,
                pp_put_old[i], p_put_old_len[i]) ? 1 : 0;
        }
        else if (SVN_RA_OP_PROP == p_op->type)
        {
            if (0 == p_op->prop_is_dir)
            {
                int next_i = i + 1;

                ok_op = svn_ra_commit_prop_file_group(p_ra, stack, &depth, &tok_seq, p_ops,
                    op_count, i, base_rev, (const uint8_t**)pp_put_old, p_put_old_len,
                    &next_i) ? 1 : 0;
                if (0 != ok_op)
                {
                    i = next_i - 1;
                }
            }
            else
            {
                ok_op = svn_ra_commit_prop_one(p_ra, stack, &depth, &tok_seq, p_ops, op_count, i,
                    base_rev, (const uint8_t**)pp_put_old, p_put_old_len) ? 1 : 0;
            }
        }
        else
        {
            ok_op = 0;
        }

        if (0 == ok_op)
        {
            svn_buf_clear(&params);
            svn_ra_send_editor_cmd(p_ra, "abort-edit", &params);
            svn_ra_read_cmd_response(p_ra, &p_resp);
            svn_sexpr_free(p_resp);
            svn_ra_commit_trace("op fail");
            svn_ra_commit_free_put_old(pp_put_old, p_put_old_len, op_count);
            return -1;
        }
    }

    // close open dirs (含 root d0)
    while (depth > 0)
    {
        svn_buf_clear(&params);
        svn_buf_append_string(&params, stack[depth - 1].token);
        if (0 == svn_ra_send_editor_cmd(p_ra, "close-dir", &params))
        {
            svn_buf_free(&params);
            svn_ra_commit_trace("close-dir fail");
            svn_ra_commit_free_put_old(pp_put_old, p_put_old_len, op_count);
            return -1;
        }
        svn_buf_free(&params);
        --depth;
    }

    svn_ra_commit_free_put_old(pp_put_old, p_put_old_len, op_count);

    if (p_ra->rx_len > 0)
    {
        if (NULL != getenv("KSVNRA_COMMIT_DEBUG"))
        {
            fprintf(stderr, "ksvnra commit: rx before close-edit=%d\n", p_ra->rx_len);
        }
    }

    if (0 == svn_ra_send_command(p_ra, "close-edit", NULL))
    {
        svn_ra_commit_trace("close-edit fail");
        return -1;
    }

    if (0 == svn_ra_read_edit_empty_ack(p_ra))
    {
        svn_ra_commit_trace("close-edit ack fail");
        return -1;
    }

    if (svn_ra_commit_read_commit_info(p_ra, p_new_rev))
    {
        return 0;
    }

    // 回退: commit 已在 FS 生效时取 youngest
    {
        int64_t young = 0;

        if (0 == svn_ra_youngest(p_ra, &young))
        {
            if (young > base_rev)
            {
                if (NULL != p_new_rev)
                {
                    *p_new_rev = young;
                }
                return 0;
            }
        }
    }

    svn_ra_commit_trace("finish fail");
    return -1;
}

// end
