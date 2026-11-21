// Doc Encode : UTF-8 BOM, Unix(LF)
#include "alua/svn_ra/core/svn_ra_priv.h"

//////////////////////////////////////////////////////////////////////////
// s-expression

void svn_sexpr_free(svn_sexpr_t* p_node)
{
    int i = 0;

    if (NULL == p_node)
    {
        return;
    }

    if (SVN_SEXPR_LIST == p_node->type)
    {
        for (i = 0; i < p_node->child_count; ++i)
        {
            svn_sexpr_free(p_node->pp_children[i]);
        }
        KLB_FREE(p_node->pp_children);
    }
    else if (SVN_SEXPR_WORD == p_node->type)
    {
        KLB_FREE(p_node->p_word);
    }
    else if (SVN_SEXPR_STRING == p_node->type)
    {
        KLB_FREE(p_node->p_string);
    }

    KLB_FREE(p_node);
}


bool svn_sexpr_is_word(const svn_sexpr_t* p_node, const char* p_word)
{
    if (NULL == p_node || SVN_SEXPR_WORD != p_node->type || NULL == p_node->p_word)
    {
        return false;
    }

    return (0 == strcmp(p_node->p_word, p_word));
}


const svn_sexpr_t* svn_sexpr_child(const svn_sexpr_t* p_node, int idx)
{
    if (NULL == p_node || SVN_SEXPR_LIST != p_node->type)
    {
        return NULL;
    }
    if (idx < 0 || idx >= p_node->child_count)
    {
        return NULL;
    }

    return p_node->pp_children[idx];
}


const char* svn_sexpr_word_cstr(const svn_sexpr_t* p_node)
{
    if (NULL == p_node || SVN_SEXPR_WORD != p_node->type)
    {
        return NULL;
    }

    return p_node->p_word;
}


const char* svn_sexpr_string_cstr(const svn_sexpr_t* p_node)
{
    if (NULL == p_node || SVN_SEXPR_STRING != p_node->type)
    {
        return NULL;
    }

    return p_node->p_string;
}


//////////////////////////////////////////////////////////////////////////
// IO / buffer

bool svn_ra_rx_grow(svn_ra_t* p_ra, int need_len)
{
    int new_cap = 0;
    uint8_t* p_new = NULL;

    if (p_ra->rx_cap >= need_len)
    {
        return true;
    }

    new_cap = (p_ra->rx_cap > 0) ? p_ra->rx_cap : SVN_RA_RX_INIT_CAP;
    while (new_cap < need_len)
    {
        new_cap *= 2;
    }

    p_new = (uint8_t*)KLB_REALLOC(p_ra->p_rx_buf, uint8_t, new_cap, 0);
    if (NULL == p_new)
    {
        return false;
    }

    p_ra->p_rx_buf = p_new;
    p_ra->rx_cap = new_cap;
    return true;
}


bool svn_ra_rx_fill(svn_ra_t* p_ra)
{
    int n = 0;

    if (0 == svn_ra_rx_grow(p_ra, p_ra->rx_len + 1))
    {
        return false;
    }

    n = klb_socket_recv(p_ra->p_socket, p_ra->p_rx_buf + p_ra->rx_len,
                        p_ra->rx_cap - p_ra->rx_len);
    if (0 >= n)
    {
        return false;
    }

    p_ra->rx_len += n;
    return true;
}


bool svn_ra_rx_ensure(svn_ra_t* p_ra, int min_avail)
{
    while (p_ra->rx_len < min_avail)
    {
        if (0 == svn_ra_rx_fill(p_ra))
        {
            return false;
        }
    }

    return true;
}


void svn_ra_rx_consume(svn_ra_t* p_ra, int n)
{
    if (n <= 0)
    {
        return;
    }
    if (n >= p_ra->rx_len)
    {
        p_ra->rx_len = 0;
        return;
    }

    memmove(p_ra->p_rx_buf, p_ra->p_rx_buf + n, (size_t)(p_ra->rx_len - n));
    p_ra->rx_len -= n;
}


/// @brief 跳过当前缓冲内空白; 缓冲空时不阻塞读 (避免吃掉尾空白后挂死)
bool svn_ra_skip_ws(svn_ra_t* p_ra)
{
    int i = 0;

    for (i = 0; i < p_ra->rx_len; ++i)
    {
        uint8_t c = p_ra->p_rx_buf[i];
        if (' ' == c || '\n' == c || '\r' == c || '\t' == c)
        {
            continue;
        }
        break;
    }

    if (i > 0)
    {
        svn_ra_rx_consume(p_ra, i);
    }

    return true;
}


/// @brief 确保缓冲有非空白字节可读 (必要时阻塞 fill)
bool svn_ra_need_data(svn_ra_t* p_ra)
{
    while (1)
    {
        if (0 == svn_ra_skip_ws(p_ra))
        {
            return false;
        }

        if (p_ra->rx_len > 0)
        {
            return true;
        }

        if (0 == svn_ra_rx_fill(p_ra))
        {
            return false;
        }
    }
}


bool svn_ra_send_all(svn_ra_t* p_ra, const uint8_t* p_data, int len)
{
    int sent = 0;
    int n = 0;

    while (sent < len)
    {
        n = klb_socket_send(p_ra->p_socket, p_data + sent, len - sent);
        if (0 >= n)
        {
            if (NULL != getenv("KSVNRA_COMMIT_DEBUG"))
            {
                fprintf(stderr, "ksvnra wire: send_all fail sent=%d len=%d errno=%d\n",
                    sent, len, errno);
                fflush(stderr);
            }
            snprintf(p_ra->err_msg, sizeof(p_ra->err_msg),
                "send failed: sent=%d len=%d errno=%d", sent, len, errno);
            return false;
        }
        sent += n;
    }

    return true;
}


bool svn_buf_reserve(svn_buf_t* p_buf, int need_cap)
{
    char* p_new = NULL;

    if (p_buf->cap >= need_cap)
    {
        return true;
    }

    if (p_buf->cap <= 0)
    {
        p_buf->cap = 256;
    }
    while (p_buf->cap < need_cap)
    {
        p_buf->cap *= 2;
    }

    p_new = (char*)KLB_REALLOC(p_buf->p_data, char, p_buf->cap, 0);
    if (NULL == p_new)
    {
        return false;
    }

    p_buf->p_data = p_new;
    return true;
}


bool svn_buf_append_raw(svn_buf_t* p_buf, const char* p_s, int n)
{
    if (0 == svn_buf_reserve(p_buf, p_buf->len + n + 1))
    {
        return false;
    }

    memcpy(p_buf->p_data + p_buf->len, p_s, (size_t)n);
    p_buf->len += n;
    p_buf->p_data[p_buf->len] = '\0';
    return true;
}


bool svn_buf_append_cstr(svn_buf_t* p_buf, const char* p_s)
{
    if (NULL == p_s)
    {
        p_s = "";
    }

    return svn_buf_append_raw(p_buf, p_s, (int)strlen(p_s));
}


bool svn_buf_append_ch(svn_buf_t* p_buf, char ch)
{
    return svn_buf_append_raw(p_buf, &ch, 1);
}


/// @brief 开列表 "( " (svnserve 要求 '(' 后有空白)
bool svn_buf_open_list(svn_buf_t* p_buf)
{
    return svn_buf_append_cstr(p_buf, "( ");
}


/// @brief 闭列表 ") " (svnserve 要求 ')' 前有空白)
bool svn_buf_close_list(svn_buf_t* p_buf)
{
    return svn_buf_append_cstr(p_buf, ") ");
}


bool svn_buf_append_word(svn_buf_t* p_buf, const char* p_word)
{
    if (0 == svn_buf_append_cstr(p_buf, p_word))
    {
        return false;
    }

    return svn_buf_append_ch(p_buf, ' ');
}


bool svn_buf_append_number(svn_buf_t* p_buf, int64_t num)
{
    char tmp[32] = { 0 };

    snprintf(tmp, sizeof(tmp), "%lld", (long long)num);
    return svn_buf_append_word(p_buf, tmp);
}


bool svn_buf_append_string(svn_buf_t* p_buf, const char* p_str)
{
    char hdr[32] = { 0 };
    int slen = 0;

    if (NULL == p_str)
    {
        p_str = "";
    }

    slen = (int)strlen(p_str);
    snprintf(hdr, sizeof(hdr), "%d:", slen);
    if (0 == svn_buf_append_cstr(p_buf, hdr))
    {
        return false;
    }
    if (0 == svn_buf_append_raw(p_buf, p_str, slen))
    {
        return false;
    }

    return svn_buf_append_ch(p_buf, ' ');
}


void svn_buf_clear(svn_buf_t* p_buf)
{
    p_buf->len = 0;
    if (NULL != p_buf->p_data)
    {
        p_buf->p_data[0] = '\0';
    }
}


void svn_buf_free(svn_buf_t* p_buf)
{
    KLB_FREE(p_buf->p_data);
    p_buf->len = 0;
    p_buf->cap = 0;
}


//////////////////////////////////////////////////////////////////////////
// parser

bool svn_ra_parse_word(svn_ra_t* p_ra, svn_sexpr_t* p_out)
{
    int i = 0;
    char* p_copy = NULL;

    if (0 == svn_ra_skip_ws(p_ra))
    {
        return false;
    }

    // step1. 扫到分隔符; 缓冲用尽时继续填充 (单词可能跨多次读)
    while (1)
    {
        for (; i < p_ra->rx_len; ++i)
        {
            uint8_t c = p_ra->p_rx_buf[i];
            if (isalnum(c) || '-' == c)
            {
                continue;
            }
            if (' ' == c || '\n' == c || '\r' == c || '\t' == c)
            {
                break;
            }
            return false;
        }

        if (i < p_ra->rx_len)
        {
            break;
        }

        if (0 == svn_ra_rx_fill(p_ra))
        {
            break;
        }
    }

    if (0 == i)
    {
        return false;
    }

    p_copy = (char*)KLB_MALLOC(char, i + 1, 0);
    if (NULL == p_copy)
    {
        return false;
    }
    memcpy(p_copy, p_ra->p_rx_buf, (size_t)i);
    p_copy[i] = '\0';
    svn_ra_rx_consume(p_ra, i);

    p_out->type = SVN_SEXPR_WORD;
    p_out->p_word = p_copy;
    return svn_ra_skip_ws(p_ra);
}


bool svn_ra_parse_digits(svn_ra_t* p_ra, int* p_end_pos)
{
    int i = 0;

    if (0 == svn_ra_skip_ws(p_ra))
    {
        return false;
    }

    // step1. 数字串可能跨多次读; 缓冲用尽时继续填充
    while (1)
    {
        for (; i < p_ra->rx_len; ++i)
        {
            if (isdigit(p_ra->p_rx_buf[i]))
            {
                continue;
            }
            break;
        }

        if (i < p_ra->rx_len)
        {
            *p_end_pos = i;
            return true;
        }

        if (0 == svn_ra_rx_fill(p_ra))
        {
            break;
        }
    }

    if (0 == i)
    {
        return false;
    }

    *p_end_pos = i;
    return true;
}


bool svn_ra_parse_number(svn_ra_t* p_ra, svn_sexpr_t* p_out)
{
    int end = 0;
    char tmp[64] = { 0 };

    if (0 == svn_ra_parse_digits(p_ra, &end))
    {
        return false;
    }

    if (end >= (int)sizeof(tmp))
    {
        return false;
    }

    memcpy(tmp, p_ra->p_rx_buf, (size_t)end);
    tmp[end] = '\0';

    if (end < p_ra->rx_len && ':' == p_ra->p_rx_buf[end])
    {
        return false;
    }

    p_out->type = SVN_SEXPR_NUMBER;
    p_out->number = (int64_t)atoll(tmp);
    svn_ra_rx_consume(p_ra, end);
    return svn_ra_skip_ws(p_ra);
}


bool svn_ra_parse_string(svn_ra_t* p_ra, svn_sexpr_t* p_out)
{
    int end = 0;
    char tmp[32] = { 0 };
    int slen = 0;
    char* p_copy = NULL;

    if (0 == svn_ra_parse_digits(p_ra, &end))
    {
        return false;
    }

    if (end >= (int)sizeof(tmp))
    {
        return false;
    }
    if (end >= p_ra->rx_len || ':' != p_ra->p_rx_buf[end])
    {
        return false;
    }

    memcpy(tmp, p_ra->p_rx_buf, (size_t)end);
    tmp[end] = '\0';
    slen = atoi(tmp);
    if (slen < 0)
    {
        return false;
    }

    svn_ra_rx_consume(p_ra, end + 1);

    if (0 == svn_ra_rx_ensure(p_ra, slen))
    {
        return false;
    }

    p_copy = (char*)KLB_MALLOC(char, slen + 1, 0);
    if (slen > 0)
    {
        memcpy(p_copy, p_ra->p_rx_buf, (size_t)slen);
    }
    p_copy[slen] = '\0';
    svn_ra_rx_consume(p_ra, slen);

    p_out->type = SVN_SEXPR_STRING;
    p_out->p_string = p_copy;
    p_out->string_len = slen;
    return svn_ra_skip_ws(p_ra);
}


bool svn_ra_parse_item(svn_ra_t* p_ra, svn_sexpr_t** pp_out);


bool svn_ra_parse_list(svn_ra_t* p_ra, svn_sexpr_t* p_out)
{
    svn_sexpr_t* p_child = NULL;

    if (0 == svn_ra_need_data(p_ra))
    {
        return false;
    }
    if ('(' != p_ra->p_rx_buf[0])
    {
        return false;
    }

    svn_ra_rx_consume(p_ra, 1);

    p_out->type = SVN_SEXPR_LIST;
    p_out->pp_children = NULL;
    p_out->child_count = 0;

    while (1)
    {
        if (0 == svn_ra_need_data(p_ra))
        {
            return false;
        }
        if (')' == p_ra->p_rx_buf[0])
        {
            svn_ra_rx_consume(p_ra, 1);
            break;
        }

        if (0 == svn_ra_parse_item(p_ra, &p_child))
        {
            return false;
        }

        p_out->pp_children = (svn_sexpr_t**)KLB_REALLOC(p_out->pp_children, svn_sexpr_t*, p_out->child_count + 1, 0);
        if (NULL == p_out->pp_children)
        {
            svn_sexpr_free(p_child);
            return false;
        }
        p_out->pp_children[p_out->child_count] = p_child;
        ++p_out->child_count;
    }

    return svn_ra_skip_ws(p_ra);
}


bool svn_ra_parse_item(svn_ra_t* p_ra, svn_sexpr_t** pp_out)
{
    svn_sexpr_t* p_node = NULL;

    if (NULL == pp_out)
    {
        return false;
    }

    if (0 == svn_ra_need_data(p_ra))
    {
        return false;
    }

    p_node = KLB_MALLOCZ(svn_sexpr_t, 1, 0);

    if ('(' == p_ra->p_rx_buf[0])
    {
        if (0 == svn_ra_parse_list(p_ra, p_node))
        {
            svn_sexpr_free(p_node);
            return false;
        }
    }
    else if (isdigit(p_ra->p_rx_buf[0]))
    {
        int end = 0;
        if (0 == svn_ra_parse_digits(p_ra, &end))
        {
            svn_sexpr_free(p_node);
            return false;
        }
        if (end < p_ra->rx_len && ':' == p_ra->p_rx_buf[end])
        {
            if (0 == svn_ra_parse_string(p_ra, p_node))
            {
                svn_sexpr_free(p_node);
                return false;
            }
        }
        else
        {
            if (0 == svn_ra_parse_number(p_ra, p_node))
            {
                svn_sexpr_free(p_node);
                return false;
            }
        }
    }
    else if (isalpha((unsigned char)p_ra->p_rx_buf[0]))
    {
        if (0 == svn_ra_parse_word(p_ra, p_node))
        {
            svn_sexpr_free(p_node);
            return false;
        }
    }
    else
    {
        svn_sexpr_free(p_node);
        return false;
    }

    *pp_out = p_node;
    return true;
}


bool svn_ra_read_item(svn_ra_t* p_ra, svn_sexpr_t** pp_item)
{
    if (NULL == pp_item)
    {
        return false;
    }

    *pp_item = NULL;

    return svn_ra_parse_item(p_ra, pp_item);
}


/// @brief 下一 token 是否为列表 (命令状态帧); 必要时阻塞读
bool svn_ra_next_is_list(svn_ra_t* p_ra)
{
    if (NULL == p_ra)
    {
        return false;
    }

    return svn_ra_need_data(p_ra) && '(' == p_ra->p_rx_buf[0];
}


bool svn_ra_send_item(svn_ra_t* p_ra, svn_buf_t* p_buf)
{
    // ra_svn 项以空白分隔; 官方客户端不加 '\n'
    if (p_buf->len <= 0 || ' ' != p_buf->p_data[p_buf->len - 1])
    {
        if (0 == svn_buf_append_ch(p_buf, ' '))
        {
            return false;
        }
    }

    if (0 == svn_ra_send_all(p_ra, (const uint8_t*)p_buf->p_data, p_buf->len))
    {
        return false;
    }

    svn_buf_clear(p_buf);
    return true;
}


//////////////////////////////////////////////////////////////////////////
// protocol helpers

bool svn_ra_mech_list_has(const svn_sexpr_t* p_mechs, const char* p_mech)
{
    int i = 0;

    if (NULL == p_mechs || SVN_SEXPR_LIST != p_mechs->type)
    {
        return false;
    }

    for (i = 0; i < p_mechs->child_count; ++i)
    {
        if (svn_sexpr_is_word(p_mechs->pp_children[i], p_mech))
        {
            return true;
        }
    }

    return false;
}


bool svn_ra_send_auth_response(svn_ra_t* p_ra, const char* p_mech)
{
    svn_buf_t buf = { 0 };

    svn_buf_open_list(&buf);
    svn_buf_append_word(&buf, p_mech);
    svn_buf_open_list(&buf);
    svn_buf_close_list(&buf);
    svn_buf_close_list(&buf);

    if (0 == svn_ra_send_item(p_ra, &buf))
    {
        svn_buf_free(&buf);
        return false;
    }

    svn_buf_free(&buf);
    return true;
}


bool svn_ra_send_step_response(svn_ra_t* p_ra, const char* p_resp)
{
    svn_buf_t buf = { 0 };

    // 客户端对 CRAM challenge 的应答是裸 string 项, 不是 ( step ( ... ) )
    svn_buf_append_string(&buf, p_resp);

    if (0 == svn_ra_send_item(p_ra, &buf))
    {
        svn_buf_free(&buf);
        return false;
    }

    svn_buf_free(&buf);
    return true;
}


bool svn_ra_do_cram_md5(svn_ra_t* p_ra)
{
    svn_sexpr_t* p_challenge = NULL;
    const svn_sexpr_t* p_kind = NULL;
    const svn_sexpr_t* p_payload = NULL;
    const char* p_chal = NULL;
    char resp[512] = { 0 };
    uint8_t digest[16] = { 0 };
    char hex[33] = { 0 };

    if (0 == svn_ra_send_auth_response(p_ra, "CRAM-MD5"))
    {
        return false;
    }

    while (1)
    {
        if (0 == svn_ra_read_item(p_ra, &p_challenge))
        {
            return false;
        }

        if (SVN_SEXPR_LIST != p_challenge->type || p_challenge->child_count < 1)
        {
            svn_sexpr_free(p_challenge);
            return false;
        }

        p_kind = p_challenge->pp_children[0];
        if (svn_sexpr_is_word(p_kind, "success"))
        {
            svn_sexpr_free(p_challenge);
            return true;
        }
        if (svn_sexpr_is_word(p_kind, "failure"))
        {
            svn_sexpr_free(p_challenge);
            return false;
        }
        if (0 == svn_sexpr_is_word(p_kind, "step"))
        {
            svn_sexpr_free(p_challenge);
            return false;
        }

        p_payload = svn_sexpr_child(p_challenge, 1);
        if (NULL == p_payload || SVN_SEXPR_LIST != p_payload->type
            || p_payload->child_count < 1)
        {
            svn_sexpr_free(p_challenge);
            return false;
        }

        p_chal = svn_sexpr_string_cstr(p_payload->pp_children[0]);
        if (NULL == p_chal)
        {
            svn_sexpr_free(p_challenge);
            return false;
        }

        svn_hmac_md5(p_ra->pass, p_chal, digest);
        svn_hex_lower(digest, 16, hex, (int)sizeof(hex));
        snprintf(resp, sizeof(resp), "%s %s", p_ra->user, hex);

        svn_sexpr_free(p_challenge);
        p_challenge = NULL;

        if (0 == svn_ra_send_step_response(p_ra, resp))
        {
            return false;
        }
    }
}


bool svn_ra_do_anonymous(svn_ra_t* p_ra)
{
    svn_sexpr_t* p_challenge = NULL;
    const svn_sexpr_t* p_kind = NULL;

    if (0 == svn_ra_send_auth_response(p_ra, "ANONYMOUS"))
    {
        return false;
    }

    if (0 == svn_ra_read_item(p_ra, &p_challenge))
    {
        return false;
    }

    if (SVN_SEXPR_LIST != p_challenge->type || p_challenge->child_count < 1)
    {
        svn_sexpr_free(p_challenge);
        return false;
    }

    p_kind = p_challenge->pp_children[0];
    if (svn_sexpr_is_word(p_kind, "success"))
    {
        svn_sexpr_free(p_challenge);
        return true;
    }

    svn_sexpr_free(p_challenge);
    return false;
}


bool svn_ra_handle_auth_request(svn_ra_t* p_ra, const svn_sexpr_t* p_params)
{
    const svn_sexpr_t* p_mechs = NULL;
    const char* p_mech = NULL;

    if (NULL == p_params)
    {
        return false;
    }

    if (SVN_SEXPR_LIST != p_params->type)
    {
        return false;
    }

    if (0 == p_params->child_count)
    {
        return true;
    }

    p_mechs = svn_sexpr_child(p_params, 0);
    if (NULL == p_mechs || SVN_SEXPR_LIST != p_mechs->type)
    {
        return false;
    }

    if (0 == p_mechs->child_count)
    {
        return true;
    }

    // 有凭据时优先 CRAM-MD5; 勿选 ANONYMOUS 空握手 (写操作会 170001)
    if ('\0' != p_ra->user[0] && '\0' != p_ra->pass[0]
        && svn_ra_mech_list_has(p_mechs, "CRAM-MD5"))
    {
        p_mech = "CRAM-MD5";
    }
    else if (svn_ra_mech_list_has(p_mechs, "ANONYMOUS"))
    {
        p_mech = "ANONYMOUS";
    }
    else if (svn_ra_mech_list_has(p_mechs, "CRAM-MD5"))
    {
        p_mech = "CRAM-MD5";
    }
    else
    {
        return false;
    }

    if (NULL != getenv("KSVNRA_COMMIT_DEBUG"))
    {
        int mi = 0;

        fprintf(stderr, "ksvnra wire: auth choose %s (n=%d)\n", p_mech, p_mechs->child_count);
        for (mi = 0; mi < p_mechs->child_count && mi < 6; ++mi)
        {
            if (SVN_SEXPR_WORD == p_mechs->pp_children[mi]->type
                && NULL != p_mechs->pp_children[mi]->p_word)
            {
                fprintf(stderr, "ksvnra wire: auth mech[%d] %s\n", mi,
                    p_mechs->pp_children[mi]->p_word);
            }
        }
        fflush(stderr);
    }

    if (0 == strcmp(p_mech, "CRAM-MD5"))
    {
        if (0 == svn_ra_do_cram_md5(p_ra))
        {
            return false;
        }
        return true;
    }

    if (0 == svn_ra_do_anonymous(p_ra))
    {
        return false;
    }
    return true;
}


/// @brief 从 failure 帧递归取首个错误码/消息 (深度受限)
static void svn_ra_failure_scan(const svn_sexpr_t* p_node, int depth,
                                int64_t* p_code, const char** pp_msg)
{
    int i = 0;

    if (NULL == p_node || depth > 3)
    {
        return;
    }

    if (SVN_SEXPR_NUMBER == p_node->type && 0 == *p_code)
    {
        *p_code = p_node->number;
        return;
    }

    if (SVN_SEXPR_STRING == p_node->type && NULL != p_node->p_string
        && NULL == *pp_msg)
    {
        *pp_msg = p_node->p_string;
        return;
    }

    if (SVN_SEXPR_LIST != p_node->type)
    {
        return;
    }

    for (i = 0; i < p_node->child_count; ++i)
    {
        svn_ra_failure_scan(p_node->pp_children[i], depth + 1, p_code, pp_msg);
    }
}


bool svn_ra_read_cmd_response(svn_ra_t* p_ra, svn_sexpr_t** pp_params)
{
    svn_sexpr_t* p_item = NULL;
    svn_sexpr_t* p_params = NULL;

    if (NULL == pp_params)
    {
        return false;
    }

    *pp_params = NULL;

    if (0 == svn_ra_read_item(p_ra, &p_item))
    {
        if (NULL != getenv("KSVNRA_COMMIT_DEBUG"))
        {
            fprintf(stderr, "ksvnra wire: read_cmd_response eof rx=%d\n", p_ra->rx_len);
            fflush(stderr);
        }
        return false;
    }

    if (NULL != getenv("KSVNRA_COMMIT_DEBUG"))
    {
        int di = 0;
        const svn_sexpr_t* p0 = NULL;

        fprintf(stderr, "ksvnra wire: read_cmd_response item type=%d children=%d rx=%d\n",
            (int)p_item->type,
            (SVN_SEXPR_LIST == p_item->type) ? p_item->child_count : 0,
            p_ra->rx_len);
        if (SVN_SEXPR_LIST == p_item->type && p_item->child_count > 0)
        {
            p0 = p_item->pp_children[0];
            fprintf(stderr, "ksvnra wire:   [0] type=%d\n",
                (NULL != p0) ? (int)p0->type : -1);
            if (NULL != p0 && SVN_SEXPR_WORD == p0->type && NULL != p0->p_word)
            {
                fprintf(stderr, "ksvnra wire:   [0] word %s\n", p0->p_word);
            }
        }
        if (p_ra->rx_len > 0 && p_ra->rx_len < 64)
        {
            fprintf(stderr, "ksvnra wire: rx tail: %.*s\n", p_ra->rx_len, p_ra->p_rx_buf);
        }
        fflush(stderr);
    }

    if (SVN_SEXPR_LIST != p_item->type)
    {
        if (NULL != getenv("KSVNRA_COMMIT_DEBUG"))
        {
            fprintf(stderr, "ksvnra wire: read_cmd_response not-list type=%d\n",
                (int)p_item->type);
            fflush(stderr);
        }
        svn_sexpr_free(p_item);
        return false;
    }

    // commit 等命令可回裸 ( ); 视为空 params 成功
    if (0 == p_item->child_count)
    {
        svn_sexpr_free(p_item);
        return true;
    }

    if (svn_sexpr_is_word(p_item->pp_children[0], "failure"))
    {
        int64_t err_code = 0;
        const char* p_err_msg = NULL;

        svn_ra_failure_scan(p_item, 0, &err_code, &p_err_msg);
        p_ra->err_code = err_code;
        p_ra->err_msg[0] = '\0';
        if (NULL != p_err_msg)
        {
            snprintf(p_ra->err_msg, sizeof(p_ra->err_msg), "%s", p_err_msg);
        }

        if (NULL != getenv("KSVNRA_COMMIT_DEBUG"))
        {
            int fi = 0;

            fprintf(stderr, "ksvnra wire: failure frame children=%d\n", p_item->child_count);
            for (fi = 1; fi < p_item->child_count && fi < 4; ++fi)
            {
                const svn_sexpr_t* p_err = p_item->pp_children[fi];

                if (NULL != p_err && SVN_SEXPR_STRING == p_err->type && NULL != p_err->p_string)
                {
                    fprintf(stderr, "ksvnra wire: failure str[%d] %s\n", fi, p_err->p_string);
                }
                else if (NULL != p_err && SVN_SEXPR_LIST == p_err->type)
                {
                    int fj = 0;
                    int fk = 0;

                    fprintf(stderr, "ksvnra wire: failure list[%d] n=%d\n", fi, p_err->child_count);
                    for (fj = 0; fj < p_err->child_count && fj < 4; ++fj)
                    {
                        const svn_sexpr_t* p_sub = p_err->pp_children[fj];

                        if (NULL != p_sub && SVN_SEXPR_NUMBER == p_sub->type)
                        {
                            fprintf(stderr, "ksvnra wire: failure code %lld\n",
                                (long long)p_sub->number);
                        }
                        else if (NULL != p_sub && SVN_SEXPR_STRING == p_sub->type
                            && NULL != p_sub->p_string)
                        {
                            fprintf(stderr, "ksvnra wire: failure msg %s\n", p_sub->p_string);
                        }
                        else if (NULL != p_sub && SVN_SEXPR_LIST == p_sub->type)
                        {
                            for (fk = 0; fk < p_sub->child_count && fk < 6; ++fk)
                            {
                                const svn_sexpr_t* p_leaf = p_sub->pp_children[fk];

                                if (NULL != p_leaf && SVN_SEXPR_NUMBER == p_leaf->type)
                                {
                                    fprintf(stderr, "ksvnra wire: failure code %lld\n",
                                        (long long)p_leaf->number);
                                }
                                else if (NULL != p_leaf && SVN_SEXPR_STRING == p_leaf->type
                                    && NULL != p_leaf->p_string)
                                {
                                    fprintf(stderr, "ksvnra wire: failure msg %s\n",
                                        p_leaf->p_string);
                                }
                            }
                        }
                    }
                }
            }
            fflush(stderr);
        }
        svn_sexpr_free(p_item);
        return false;
    }

    if (0 == svn_sexpr_is_word(p_item->pp_children[0], "success"))
    {
        // 无 success 包裹: 整帧即 params, 如 commit ack ( ( ) "" ) 或 ( 1 )
        if (NULL != getenv("KSVNRA_COMMIT_DEBUG"))
        {
            const svn_sexpr_t* p0 = p_item->pp_children[0];

            fprintf(stderr, "ksvnra wire: read_cmd_response bare params children=%d t0=%d\n",
                p_item->child_count, (NULL != p0) ? (int)p0->type : -1);
            fflush(stderr);
        }
        *pp_params = p_item;
        return true;
    }

    if (p_item->child_count >= 2)
    {
        p_params = p_item->pp_children[1];
        if (SVN_SEXPR_LIST == p_params->type || SVN_SEXPR_NUMBER == p_params->type)
        {
            *pp_params = p_params;
            p_item->pp_children[1] = NULL;
        }
    }

    svn_sexpr_free(p_item);
    return true;
}


/// @brief 记录 failure 帧的 svn 错误码/消息 (供流式命令如 replay 就地判失败)
/// @param [in] p_item  已读到的帧; 首元素非 word failure 时返回 false
/// @return true=已记录 (p_ra 的 err_code/err_msg 被更新); false=非 failure 帧
bool svn_ra_capture_failure(svn_ra_t* p_ra, const svn_sexpr_t* p_item)
{
    if (NULL == p_ra || NULL == p_item || SVN_SEXPR_LIST != p_item->type)
    {
        return false;
    }

    if (0 == svn_sexpr_is_word(svn_sexpr_child(p_item, 0), "failure"))
    {
        return false;
    }

    int64_t err_code = 0;
    const char* p_err_msg = NULL;

    svn_ra_failure_scan(p_item, 0, &err_code, &p_err_msg);
    p_ra->err_code = err_code;
    p_ra->err_msg[0] = '\0';
    if (NULL != p_err_msg)
    {
        snprintf(p_ra->err_msg, sizeof(p_ra->err_msg), "%s", p_err_msg);
    }

    if (NULL != getenv("KSVNRA_COMMIT_DEBUG"))
    {
        fprintf(stderr, "ksvnra wire: failure code=%lld msg=%s\n",
            (long long)err_code, p_ra->err_msg);
        fflush(stderr);
    }

    return true;
}


/// @brief 是否为 commit/editor 启动时的占位帧 ( ( ) "" )
bool svn_ra_params_looks_like_edit_bootstrap(const svn_sexpr_t* p_params)
{
    const svn_sexpr_t* p_slot = NULL;

    if (NULL == p_params || SVN_SEXPR_LIST != p_params->type || 2 != p_params->child_count)
    {
        return false;
    }

    p_slot = svn_sexpr_child(p_params, 0);
    if (NULL == p_slot || SVN_SEXPR_LIST != p_slot->type || 0 != p_slot->child_count)
    {
        return false;
    }

    if (NULL == p_params->pp_children[1])
    {
        return false;
    }

    return (SVN_SEXPR_STRING == p_params->pp_children[1]->type);
}


/// @brief params 是否为 auth-request ( ( mech ... ) ), 非命令结果如 ( 42 ) 或空 ( )
static bool svn_ra_params_looks_like_auth(const svn_sexpr_t* p_params)
{
    const svn_sexpr_t* p_mechs = NULL;
    int i = 0;

    if (NULL == p_params || SVN_SEXPR_LIST != p_params->type)
    {
        return false;
    }

    if (0 == p_params->child_count)
    {
        return false;
    }

    p_mechs = svn_sexpr_child(p_params, 0);
    if (NULL == p_mechs || SVN_SEXPR_LIST != p_mechs->type || 0 == p_mechs->child_count)
    {
        return false;
    }

    for (i = 0; i < p_mechs->child_count; ++i)
    {
        if (SVN_SEXPR_WORD != p_mechs->pp_children[i]->type)
        {
            return false;
        }
    }

    return true;
}


bool svn_ra_read_auth_then_response(svn_ra_t* p_ra, svn_sexpr_t** pp_params)
{
    svn_sexpr_t* p_auth_params = NULL;

    if (0 == svn_ra_read_cmd_response(p_ra, &p_auth_params))
    {
        return false;
    }

    if (0 == svn_ra_params_looks_like_auth(p_auth_params))
    {
        if (svn_ra_params_looks_like_edit_bootstrap(p_auth_params) && p_ra->rx_len > 0)
        {
            if (NULL != getenv("KSVNRA_COMMIT_DEBUG"))
            {
                fprintf(stderr, "ksvnra wire: skip edit bootstrap frame rx=%d\n", p_ra->rx_len);
                fflush(stderr);
            }
            svn_sexpr_free(p_auth_params);
            return svn_ra_read_auth_then_response(p_ra, pp_params);
        }

        if (NULL != getenv("KSVNRA_COMMIT_DEBUG"))
        {
            fprintf(stderr, "ksvnra wire: read_auth_then_response direct params\n");
            fflush(stderr);
        }
        *pp_params = p_auth_params;
        return true;
    }

    if (NULL != getenv("KSVNRA_COMMIT_DEBUG"))
    {
        fprintf(stderr, "ksvnra wire: read_auth_then_response handle auth\n");
        fflush(stderr);
    }

    if (0 == svn_ra_handle_auth_request(p_ra, p_auth_params))
    {
        svn_sexpr_free(p_auth_params);
        return false;
    }

    svn_sexpr_free(p_auth_params);
    return svn_ra_read_cmd_response(p_ra, pp_params);
}


bool svn_ra_send_command(svn_ra_t* p_ra, const char* p_cmd, svn_buf_t* p_params_buf)
{
    svn_buf_t buf = { 0 };

    if (NULL != p_ra)
    {
        p_ra->err_code = 0;
        p_ra->err_msg[0] = '\0';
    }

    svn_buf_open_list(&buf);
    svn_buf_append_word(&buf, p_cmd);
    svn_buf_open_list(&buf);
    if (NULL != p_params_buf && p_params_buf->len > 0)
    {
        svn_buf_append_raw(&buf, p_params_buf->p_data, p_params_buf->len);
    }
    svn_buf_close_list(&buf);
    svn_buf_close_list(&buf);

    if (NULL != getenv("KSVNRA_COMMIT_DEBUG") && NULL != p_cmd)
    {
        // 只打前 200 字节预览 + 总长: textdelta-chunk 可达 1MiB, 全文会冲爆 journal
        int preview = (buf.len > 200) ? 200 : buf.len;

        fprintf(stderr, "ksvnra wire: >> len=%d %.*s%s\n", buf.len,
            preview, buf.p_data, (buf.len > preview) ? " ..." : "");
        fflush(stderr);
    }

    if (0 == svn_ra_send_item(p_ra, &buf))
    {
        svn_buf_free(&buf);
        return false;
    }

    svn_buf_free(&buf);
    return true;
}


int svn_ra_last_error(svn_ra_t* p_ra, int64_t* p_code, char* p_buf, int buf_len)
{
    if (NULL == p_ra || NULL == p_code)
    {
        return -1;
    }

    *p_code = p_ra->err_code;
    if (NULL != p_buf && buf_len > 0)
    {
        snprintf(p_buf, (size_t)buf_len, "%s", p_ra->err_msg);
    }

    return 0;
}


bool svn_ra_parse_hostport(const char* p_hostport, char* p_host, int host_len, int* p_port)
{
    const char* p_colon = NULL;

    if (NULL == p_hostport || NULL == p_host || NULL == p_port)
    {
        return false;
    }

    p_colon = strrchr(p_hostport, ':');
    if (NULL != p_colon && p_colon != p_hostport)
    {
        int plen = (int)(p_colon - p_hostport);
        if (plen <= 0 || plen >= host_len)
        {
            return false;
        }
        memcpy(p_host, p_hostport, (size_t)plen);
        p_host[plen] = '\0';
        *p_port = atoi(p_colon + 1);
        if (*p_port <= 0)
        {
            return false;
        }
    }
    else
    {
        snprintf(p_host, (size_t)host_len, "%s", p_hostport);
        *p_port = SVN_RA_DEFAULT_PORT;
    }

    return true;
}


bool svn_ra_build_url(const char* p_host, int port, const char* p_repo, char* p_url, int url_len)
{
    if (SVN_RA_DEFAULT_PORT == port)
    {
        snprintf(p_url, (size_t)url_len, "svn://%s/%s", p_host, p_repo);
    }
    else
    {
        snprintf(p_url, (size_t)url_len, "svn://%s:%d/%s", p_host, port, p_repo);
    }

    return true;
}


bool svn_ra_copy_string_field(char* p_dst, int dst_len, const char* p_src)
{
    if (NULL == p_dst || dst_len <= 0)
    {
        return false;
    }

    if (NULL == p_src)
    {
        p_dst[0] = '\0';
        return true;
    }

    snprintf(p_dst, (size_t)dst_len, "%s", p_src);
    return true;
}


bool svn_ra_dirent_arr_push(svn_dirent_arr_t* p_arr, const svn_ra_dirent_t* p_ent)
{
    if (NULL == p_arr || NULL == p_ent)
    {
        return false;
    }

    if (p_arr->count >= p_arr->cap)
    {
        int new_cap = (p_arr->cap > 0) ? p_arr->cap * 2 : 16;
        svn_ra_dirent_t* p_new = (svn_ra_dirent_t*)KLB_REALLOC(p_arr->p_items, svn_ra_dirent_t, new_cap, 0);
        if (NULL == p_new)
        {
            return false;
        }
        p_arr->p_items = p_new;
        p_arr->cap = new_cap;
    }

    p_arr->p_items[p_arr->count] = *p_ent;
    ++p_arr->count;
    return true;
}


bool svn_ra_parse_dirent(const svn_sexpr_t* p_ent, svn_ra_dirent_t* p_out)
{
    const char* p_name = NULL;
    const char* p_kind = NULL;
    const char* p_date = NULL;
    const char* p_author = NULL;

    if (NULL == p_ent || SVN_SEXPR_LIST != p_ent->type || p_ent->child_count < 5)
    {
        return false;
    }

    memset(p_out, 0, sizeof(*p_out));

    p_name = svn_sexpr_string_cstr(svn_sexpr_child(p_ent, 0));
    p_kind = svn_sexpr_word_cstr(svn_sexpr_child(p_ent, 1));
    if (NULL != svn_sexpr_child(p_ent, 2)
        && SVN_SEXPR_NUMBER == svn_sexpr_child(p_ent, 2)->type)
    {
        p_out->size = svn_sexpr_child(p_ent, 2)->number;
    }
    if (p_ent->child_count >= 5 && NULL != svn_sexpr_child(p_ent, 4)
        && SVN_SEXPR_NUMBER == svn_sexpr_child(p_ent, 4)->type)
    {
        p_out->created_rev = svn_sexpr_child(p_ent, 4)->number;
    }
    if (p_ent->child_count >= 6)
    {
        const svn_sexpr_t* p_d = svn_sexpr_child(p_ent, 5);
        p_date = svn_sexpr_string_cstr(p_d);
        // 部分实现把可选字段包成单元素 list
        if (NULL == p_date && NULL != p_d && SVN_SEXPR_LIST == p_d->type
            && p_d->child_count >= 1)
        {
            p_date = svn_sexpr_string_cstr(p_d->pp_children[0]);
        }
    }
    if (p_ent->child_count >= 7)
    {
        const svn_sexpr_t* p_a = svn_sexpr_child(p_ent, 6);
        p_author = svn_sexpr_string_cstr(p_a);
        if (NULL == p_author && NULL != p_a && SVN_SEXPR_LIST == p_a->type
            && p_a->child_count >= 1)
        {
            p_author = svn_sexpr_string_cstr(p_a->pp_children[0]);
        }
    }

    svn_ra_copy_string_field(p_out->name, (int)sizeof(p_out->name), p_name);
    svn_ra_copy_string_field(p_out->kind, (int)sizeof(p_out->kind), p_kind);
    svn_ra_copy_string_field(p_out->date, (int)sizeof(p_out->date), p_date);
    svn_ra_copy_string_field(p_out->author, (int)sizeof(p_out->author), p_author);
    return true;
}


bool svn_ra_join_path(char* p_out, int out_len, const char* p_base, const char* p_name)
{
    if (NULL == p_out || out_len <= 0 || NULL == p_name)
    {
        return false;
    }

    if (NULL == p_base || '\0' == p_base[0])
    {
        snprintf(p_out, (size_t)out_len, "%s", p_name);
    }
    else
    {
        snprintf(p_out, (size_t)out_len, "%s/%s", p_base, p_name);
    }

    return true;
}


bool svn_buf_append_rev_list(svn_buf_t* p_buf, int64_t rev)
{
    // 现行 svnserve: rev 以单元素 list 传递, 如 ( 2 )
    if (0 == svn_buf_open_list(p_buf))
    {
        return false;
    }
    if (0 == svn_buf_append_number(p_buf, rev))
    {
        return false;
    }
    return svn_buf_close_list(p_buf);
}


/// @brief 发送 get-dir 并返回已校验的响应 (rev props ( entry ... ) )
bool svn_ra_get_dir_response(svn_ra_t* p_ra, const char* p_path, int64_t rev,
                             svn_sexpr_t** pp_resp)
{
    svn_buf_t params = { 0 };
    svn_sexpr_t* p_resp = NULL;
    int64_t use_rev = rev;

    if (NULL == p_ra || NULL == pp_resp)
    {
        return false;
    }

    if (use_rev <= 0)
    {
        if (0 != svn_ra_youngest(p_ra, &use_rev))
        {
            return false;
        }
    }

    // get-dir ( path rev want-props want-contents ( want... ) )
    // want-props 必须为 true, 否则响应 props 段恒为空
    svn_buf_append_string(&params, (NULL != p_path) ? p_path : "");
    svn_buf_append_rev_list(&params, use_rev);
    svn_buf_append_word(&params, "true");
    svn_buf_append_word(&params, "true");
    svn_buf_open_list(&params);
    svn_buf_append_word(&params, "kind");
    svn_buf_append_word(&params, "size");
    svn_buf_append_word(&params, "has-props");
    svn_buf_append_word(&params, "created-rev");
    svn_buf_append_word(&params, "time");
    svn_buf_append_word(&params, "last-author");
    svn_buf_close_list(&params);

    if (0 == svn_ra_send_command(p_ra, "get-dir", &params))
    {
        svn_buf_free(&params);
        return false;
    }
    svn_buf_free(&params);

    if (0 == svn_ra_read_auth_then_response(p_ra, &p_resp))
    {
        return false;
    }

    if (NULL == p_resp || SVN_SEXPR_LIST != p_resp->type || p_resp->child_count < 3)
    {
        svn_sexpr_free(p_resp);
        return false;
    }

    *pp_resp = p_resp;
    return true;
}


bool svn_ra_list_internal(svn_ra_t* p_ra, const char* p_path, int64_t rev,
                                 svn_dirent_arr_t* p_arr, bool recurse, const char* p_rel_base)
{
    svn_sexpr_t* p_resp = NULL;
    const svn_sexpr_t* p_entries = NULL;
    int i = 0;

    if (0 == svn_ra_get_dir_response(p_ra, p_path, rev, &p_resp))
    {
        return false;
    }

    // 响应: ( rev props ( entry ... ) ); 本函数只用 entry 段
    p_entries = svn_sexpr_child(p_resp, 2);
    if (NULL == p_entries || SVN_SEXPR_LIST != p_entries->type)
    {
        svn_sexpr_free(p_resp);
        return false;
    }

    for (i = 0; i < p_entries->child_count; ++i)
    {
        svn_ra_dirent_t ent = { 0 };
        char rel_path[512] = { 0 };
        char sub_path[512] = { 0 };
        char base_name[256] = { 0 };

        if (0 == svn_ra_parse_dirent(p_entries->pp_children[i], &ent))
        {
            svn_sexpr_free(p_resp);
            return false;
        }

        snprintf(base_name, sizeof(base_name), "%s", ent.name);

        if (recurse)
        {
            if (0 == svn_ra_join_path(rel_path, (int)sizeof(rel_path), p_rel_base, base_name))
            {
                svn_sexpr_free(p_resp);
                return false;
            }
            svn_ra_copy_string_field(ent.name, (int)sizeof(ent.name), rel_path);
        }

        if (0 == svn_ra_dirent_arr_push(p_arr, &ent))
        {
            svn_sexpr_free(p_resp);
            return false;
        }

        if (recurse && 0 == strcmp(ent.kind, "dir"))
        {
            if (0 == svn_ra_join_path(sub_path, (int)sizeof(sub_path), p_path, base_name))
            {
                svn_sexpr_free(p_resp);
                return false;
            }
            if (0 == svn_ra_list_internal(p_ra, sub_path, rev, p_arr, recurse, rel_path))
            {
                svn_sexpr_free(p_resp);
                return false;
            }
        }
    }

    svn_sexpr_free(p_resp);
    return true;
}

// end
