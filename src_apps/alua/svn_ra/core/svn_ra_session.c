// Doc Encode : UTF-8 BOM, Unix(LF)
#include "alua/svn_ra/core/svn_ra_priv.h"

svn_ra_t* svn_ra_open(const char* p_hostport, const char* p_user,
                      const char* p_pass, const char* p_repo)
{
    svn_ra_t* p_ra = NULL;
    klb_socket_fd fd = INVALID_SOCKET;
    svn_sexpr_t* p_greet_params = NULL;
    svn_sexpr_t* p_auth_params = NULL;
    svn_sexpr_t* p_repos_params = NULL;
    const svn_sexpr_t* p_uuid = NULL;
    svn_buf_t greet = { 0 };
    char url[512] = { 0 };

    if (NULL == p_hostport || NULL == p_repo || '\0' == p_repo[0])
    {
        return NULL;
    }

    p_ra = KLB_MALLOCZ(svn_ra_t, 1, 0);

    if (0 == svn_ra_parse_hostport(p_hostport, p_ra->host, (int)sizeof(p_ra->host), &p_ra->port))
    {
        KLB_FREE(p_ra);
        return NULL;
    }

    if (NULL != p_user)
    {
        snprintf(p_ra->user, sizeof(p_ra->user), "%s", p_user);
    }
    if (NULL != p_pass)
    {
        snprintf(p_ra->pass, sizeof(p_ra->pass), "%s", p_pass);
    }
    snprintf(p_ra->repo, sizeof(p_ra->repo), "%s", p_repo);

    fd = klb_socket_connect(p_ra->host, p_ra->port, SVN_RA_CONNECT_TIMEOUT_MS);
    if (INVALID_SOCKET == fd)
    {
        KLB_FREE(p_ra);
        return NULL;
    }

    p_ra->p_socket = klb_socket_async_create(fd);
    if (NULL == p_ra->p_socket)
    {
        KLB_FREE(p_ra);
        return NULL;
    }

    // sync ra_svn: async_create forces nonblock; restore blocking IO
    klb_socket_set_block(fd, true);
    p_ra->p_socket->nonblock = 0;

    if (0 == svn_ra_read_cmd_response(p_ra, &p_greet_params))
    {
        svn_ra_close(p_ra);
        return NULL;
    }
    svn_sexpr_free(p_greet_params);

    if (0 == svn_ra_build_url(p_ra->host, p_ra->port, p_ra->repo, url, (int)sizeof(url)))
    {
        svn_ra_close(p_ra);
        return NULL;
    }

    // 对齐官方客户端: ( 2 ( caps ) url client-version ( ) )
    svn_buf_open_list(&greet);
    svn_buf_append_number(&greet, 2);
    svn_buf_open_list(&greet);
    svn_buf_append_word(&greet, "edit-pipeline");
    // 写路径发 svndiff0; 勿广告 svndiff1 (否则服务端期望压缩窗)
    svn_buf_append_word(&greet, "absent-entries");
    svn_buf_append_word(&greet, "depth");
    svn_buf_append_word(&greet, "mergeinfo");
    // 不广告 log-revprops: commit 用经典 logmsg 形参
    svn_buf_close_list(&greet);
    svn_buf_append_string(&greet, url);
    svn_buf_append_string(&greet, "alua/ksvnra");
    svn_buf_open_list(&greet);
    svn_buf_close_list(&greet);
    svn_buf_close_list(&greet);

    if (0 == svn_ra_send_item(p_ra, &greet))
    {
        svn_buf_free(&greet);
        svn_ra_close(p_ra);
        return NULL;
    }
    svn_buf_free(&greet);

    if (0 == svn_ra_read_cmd_response(p_ra, &p_auth_params))
    {
        svn_ra_close(p_ra);
        return NULL;
    }

    if (0 == svn_ra_handle_auth_request(p_ra, p_auth_params))
    {
        svn_sexpr_free(p_auth_params);
        svn_ra_close(p_ra);
        return NULL;
    }
    svn_sexpr_free(p_auth_params);

    if (0 == svn_ra_read_cmd_response(p_ra, &p_repos_params))
    {
        svn_ra_close(p_ra);
        return NULL;
    }

    if (NULL == p_repos_params || SVN_SEXPR_LIST != p_repos_params->type
        || p_repos_params->child_count < 1)
    {
        svn_sexpr_free(p_repos_params);
        svn_ra_close(p_ra);
        return NULL;
    }

    p_uuid = svn_sexpr_child(p_repos_params, 0);
    if (NULL == p_uuid || SVN_SEXPR_STRING != p_uuid->type)
    {
        svn_sexpr_free(p_repos_params);
        svn_ra_close(p_ra);
        return NULL;
    }

    snprintf(p_ra->uuid, sizeof(p_ra->uuid), "%s", p_uuid->p_string);
    svn_sexpr_free(p_repos_params);

    if (NULL != getenv("KSVNRA_COMMIT_DEBUG"))
    {
        fprintf(stderr, "ksvnra open: rx_len after handshake=%d\n", p_ra->rx_len);
        fflush(stderr);
    }

    return p_ra;
}


void svn_ra_close(svn_ra_t* p_ra)
{
    if (NULL == p_ra)
    {
        return;
    }

    if (NULL != p_ra->p_socket)
    {
        klb_socket_destroy(p_ra->p_socket);
        p_ra->p_socket = NULL;
    }

    KLB_FREE(p_ra->p_rx_buf);
    KLB_FREE(p_ra);
}


static bool svn_ra_sexpr_pick_rev(const svn_sexpr_t* p_node, int64_t* p_rev)
{
    int i = 0;

    if (NULL == p_node || NULL == p_rev)
    {
        return false;
    }

    if (SVN_SEXPR_NUMBER == p_node->type)
    {
        *p_rev = p_node->number;
        return true;
    }

    if (SVN_SEXPR_LIST != p_node->type)
    {
        return false;
    }

    for (i = 0; i < p_node->child_count; ++i)
    {
        if (svn_ra_sexpr_pick_rev(p_node->pp_children[i], p_rev))
        {
            return true;
        }
    }

    return false;
}


int svn_ra_youngest(svn_ra_t* p_ra, int64_t* p_rev)
{
    svn_sexpr_t* p_resp = NULL;
    int64_t use_rev = -1;
    int pass = 0;

    if (NULL == p_ra || NULL == p_rev)
    {
        return -1;
    }

    if (0 == svn_ra_send_command(p_ra, "get-latest-rev", NULL))
    {
        return -1;
    }

    for (pass = 0; pass < 4; ++pass)
    {
        int64_t got = -1;

        if (0 == svn_ra_read_auth_then_response(p_ra, &p_resp))
        {
            return -1;
        }

        if (NULL != p_resp && svn_ra_sexpr_pick_rev(p_resp, &got))
        {
            use_rev = got;
        }

        svn_sexpr_free(p_resp);
        p_resp = NULL;

        if (p_ra->rx_len <= 0)
        {
            break;
        }
    }

    if (use_rev < 0)
    {
        return -1;
    }

    *p_rev = use_rev;
    return 0;
}


int svn_ra_uuid(svn_ra_t* p_ra, char* p_buf, int buf_len)
{
    if (NULL == p_ra || NULL == p_buf || buf_len <= 0)
    {
        return -1;
    }

    if ('\0' == p_ra->uuid[0])
    {
        return -1;
    }

    snprintf(p_buf, (size_t)buf_len, "%s", p_ra->uuid);
    return 0;
}


int svn_ra_list(svn_ra_t* p_ra, const char* p_path, int64_t rev,
                svn_ra_dirent_t** pp_ents, int* p_count)
{
    svn_dirent_arr_t arr = { 0 };
    const char* p_use_path = p_path;

    if (NULL == p_ra || NULL == pp_ents || NULL == p_count)
    {
        return -1;
    }

    if (NULL == p_use_path)
    {
        p_use_path = "";
    }

    if (0 == svn_ra_list_internal(p_ra, p_use_path, rev, &arr, false, ""))
    {
        KLB_FREE(arr.p_items);
        return -1;
    }

    *pp_ents = arr.p_items;
    *p_count = arr.count;
    return 0;
}


int svn_ra_list_r(svn_ra_t* p_ra, const char* p_path, int64_t rev,
                  svn_ra_dirent_t** pp_ents, int* p_count)
{
    svn_dirent_arr_t arr = { 0 };
    const char* p_use_path = p_path;

    if (NULL == p_ra || NULL == pp_ents || NULL == p_count)
    {
        return -1;
    }

    if (NULL == p_use_path)
    {
        p_use_path = "";
    }

    if (0 == svn_ra_list_internal(p_ra, p_use_path, rev, &arr, true, p_use_path))
    {
        KLB_FREE(arr.p_items);
        return -1;
    }

    *pp_ents = arr.p_items;
    *p_count = arr.count;
    return 0;
}


// @brief 临时调试: 打印 sexpr 树 (KSVNRA_PROPS_DEBUG)
static void svn_ra_debug_dump_sexpr(const svn_sexpr_t* p_node, int depth)
{
    int i = 0;

    if (NULL == p_node)
    {
        fprintf(stderr, "%*sNULL\n", depth * 2, "");
        return;
    }

    if (SVN_SEXPR_LIST == p_node->type)
    {
        fprintf(stderr, "%*s(\n", depth * 2, "");
        for (i = 0; i < p_node->child_count; ++i)
        {
            svn_ra_debug_dump_sexpr(p_node->pp_children[i], depth + 1);
        }
        fprintf(stderr, "%*s)\n", depth * 2, "");
    }
    else if (SVN_SEXPR_NUMBER == p_node->type)
    {
        fprintf(stderr, "%*s%lld\n", depth * 2, "", (long long)p_node->number);
    }
    else if (SVN_SEXPR_WORD == p_node->type)
    {
        fprintf(stderr, "%*s%s\n", depth * 2, "", (NULL != p_node->p_word) ? p_node->p_word : "");
    }
    else if (SVN_SEXPR_STRING == p_node->type)
    {
        fprintf(stderr, "%*s'%s'\n", depth * 2, "", (NULL != p_node->p_string) ? p_node->p_string : "");
    }
    else
    {
        fprintf(stderr, "%*s?type=%d\n", depth * 2, "", (int)p_node->type);
    }
}


int svn_ra_get_dir_props(svn_ra_t* p_ra, const char* p_path, int64_t rev,
                         svn_ra_prop_t** pp_props, int* p_count)
{
    svn_sexpr_t* p_resp = NULL;
    const svn_sexpr_t* p_props = NULL;
    svn_ra_prop_t* p_out = NULL;
    int count = 0;
    int i = 0;

    if (NULL == p_ra || NULL == pp_props || NULL == p_count)
    {
        return -1;
    }

    *pp_props = NULL;
    *p_count = 0;

    if (0 == svn_ra_get_dir_response(p_ra, p_path, rev, &p_resp))
    {
        return -1;
    }

    if (NULL != getenv("KSVNRA_PROPS_DEBUG"))
    {
        svn_ra_debug_dump_sexpr(p_resp, 0);
        fflush(stderr);
    }

    // props 段: ( ( name value ) ... ); 无属性时为 0 元素 list
    p_props = svn_sexpr_child(p_resp, 1);
    if (NULL == p_props || SVN_SEXPR_LIST != p_props->type)
    {
        svn_sexpr_free(p_resp);
        return 0;
    }

    if (NULL != getenv("KSVNRA_PROPS_DEBUG"))
    {
        fprintf(stderr, "get_dir_props: resp_children=%d props_children=%d\n",
            p_resp->child_count, p_props->child_count);
        for (i = 0; i < p_props->child_count; ++i)
        {
            const svn_sexpr_t* p_it = p_props->pp_children[i];
            fprintf(stderr, "  props[%d] type=%d children=%d word=%s str=%s\n", i,
                (int)p_it->type, p_it->child_count,
                (NULL != p_it->p_word) ? p_it->p_word : "-",
                (NULL != p_it->p_string) ? p_it->p_string : "-");
        }
        fflush(stderr);
    }

    if (p_props->child_count > 0)
    {
        p_out = KLB_MALLOCZ(svn_ra_prop_t, p_props->child_count, 0);
        if (NULL == p_out)
        {
            svn_sexpr_free(p_resp);
            return -1;
        }
    }

    for (i = 0; i < p_props->child_count; ++i)
    {
        const svn_sexpr_t* p_item = p_props->pp_children[i];
        const svn_sexpr_t* p_name = NULL;
        const svn_sexpr_t* p_value = NULL;
        const char* p_name_s = NULL;
        const char* p_value_s = NULL;
        int value_len = 0;

        if (NULL == p_item || SVN_SEXPR_LIST != p_item->type || p_item->child_count < 2)
        {
            continue;
        }

        p_name = svn_sexpr_child(p_item, 0);
        p_value = svn_sexpr_child(p_item, 1);
        p_name_s = svn_sexpr_word_cstr(p_name);
        if (NULL == p_name_s)
        {
            p_name_s = svn_sexpr_string_cstr(p_name);
        }
        if (NULL == p_name_s)
        {
            continue;
        }

        // 属性名/值可能是 word 或 string, 两种都接受
        if (NULL != p_value && SVN_SEXPR_STRING == p_value->type && NULL != p_value->p_string)
        {
            p_value_s = p_value->p_string;
            value_len = p_value->string_len;
        }
        else if (NULL != p_value && SVN_SEXPR_WORD == p_value->type && NULL != p_value->p_word)
        {
            p_value_s = p_value->p_word;
            value_len = (int)strlen(p_value_s);
        }

        svn_ra_copy_string_field(p_out[count].name, (int)sizeof(p_out[count].name), p_name_s);
        if (NULL != p_value_s)
        {
            p_out[count].p_value = (char*)KLB_MALLOC(char, value_len + 1, 0);
            if (NULL == p_out[count].p_value)
            {
                svn_ra_props_free(p_out, count);
                svn_sexpr_free(p_resp);
                return -1;
            }
            memcpy(p_out[count].p_value, p_value_s, (size_t)value_len);
            p_out[count].p_value[value_len] = '\0';
            p_out[count].value_len = value_len;
        }
        ++count;
    }

    svn_sexpr_free(p_resp);
    *pp_props = p_out;
    *p_count = count;
    return 0;
}


void svn_ra_props_free(svn_ra_prop_t* p_props, int count)
{
    int i = 0;

    if (NULL == p_props)
    {
        return;
    }

    for (i = 0; i < count; ++i)
    {
        KLB_FREE(p_props[i].p_value);
    }
    KLB_FREE(p_props);
}


/// @brief 从 get-mergeinfo 响应 params 中挑选目标路径的 mergeinfo 文本
/// @param [in] *p_params  read_cmd_response 返回的 params
/// @param [in] *p_path    请求路径 (相对会话根)
/// @param [out] **pp_text 命中的文本 (指向 sexpr 内, 不另分配); NULL=未命中
/// @return true=该帧为 catalog 帧; false=非 catalog 帧 (如空 auth 成功帧)
static bool svn_ra_mergeinfo_pick(const svn_sexpr_t* p_params, const char* p_path,
                                  const char** pp_text)
{
    const svn_sexpr_t* p_catalog = NULL;
    int i = 0;

    *pp_text = NULL;

    // catalog 帧: params 仅 1 个 list 子节点 ( ( path info ) ... )
    if (NULL == p_params || SVN_SEXPR_LIST != p_params->type || 1 != p_params->child_count)
    {
        return false;
    }

    p_catalog = svn_sexpr_child(p_params, 0);
    if (NULL == p_catalog || SVN_SEXPR_LIST != p_catalog->type)
    {
        return false;
    }

    for (i = 0; i < p_catalog->child_count; ++i)
    {
        const svn_sexpr_t* p_entry = p_catalog->pp_children[i];
        const char* p_entry_path = NULL;
        const char* p_info = NULL;
        const char* p_use_path = NULL;

        if (NULL == p_entry || SVN_SEXPR_LIST != p_entry->type || p_entry->child_count < 2)
        {
            continue;
        }

        p_entry_path = svn_sexpr_string_cstr(svn_sexpr_child(p_entry, 0));
        if (NULL == p_entry_path)
        {
            p_entry_path = svn_sexpr_word_cstr(svn_sexpr_child(p_entry, 0));
        }

        p_info = svn_sexpr_string_cstr(svn_sexpr_child(p_entry, 1));
        if (NULL == p_info)
        {
            p_info = svn_sexpr_word_cstr(svn_sexpr_child(p_entry, 1));
        }

        // 响应 path 可能带前导 /, 统一按仓内相对路径比较
        p_use_path = (NULL != p_entry_path) ? p_entry_path : "";
        if ('/' == p_use_path[0])
        {
            ++p_use_path;
        }

        if (NULL != p_info && 0 == strcmp(p_use_path, p_path))
        {
            *pp_text = p_info;
            return true;
        }
    }

    return true;
}


int svn_ra_get_mergeinfo(svn_ra_t* p_ra, const char* p_path, int64_t rev,
                         bool inherited, char** pp_mergeinfo)
{
    svn_buf_t params = { 0 };
    svn_sexpr_t* p_resp = NULL;
    int64_t use_rev = rev;
    const char* p_text = NULL;
    int pass = 0;

    if (NULL == p_ra || NULL == p_path || NULL == pp_mergeinfo)
    {
        return -1;
    }

    *pp_mergeinfo = NULL;

    if (use_rev < 0)
    {
        if (0 != svn_ra_youngest(p_ra, &use_rev))
        {
            return -1;
        }
    }

    // 实测 ra_svn 形: ( get-mergeinfo ( ( path ) rev-list inherit include-removed ) )
    svn_buf_open_list(&params);
    svn_buf_append_string(&params, p_path);
    svn_buf_close_list(&params);
    svn_buf_append_rev_list(&params, use_rev);
    svn_buf_append_word(&params, (true == inherited) ? "inherited" : "explicit");
    svn_buf_append_word(&params, "false");

    if (0 == svn_ra_send_command(p_ra, "get-mergeinfo", &params))
    {
        svn_buf_free(&params);
        return -1;
    }
    svn_buf_free(&params);

    // 服务端常在结果帧前加空 auth 成功帧; 逐帧读至 catalog
    for (pass = 0; pass < 4; ++pass)
    {
        bool is_catalog = false;

        if (0 == svn_ra_read_auth_then_response(p_ra, &p_resp))
        {
            return -1;
        }

        if (0 != p_ra->err_code)
        {
            svn_sexpr_free(p_resp);
            return -1;
        }

        is_catalog = svn_ra_mergeinfo_pick(p_resp, p_path, &p_text);
        if (NULL != p_text)
        {
            int len = (int)strlen(p_text);

            *pp_mergeinfo = KLB_MALLOCZ(char, (size_t)len + 1, 0);
            if (NULL == *pp_mergeinfo)
            {
                svn_sexpr_free(p_resp);
                return -1;
            }
            memcpy(*pp_mergeinfo, p_text, (size_t)len);
            svn_sexpr_free(p_resp);
            return 0;
        }

        svn_sexpr_free(p_resp);
        p_resp = NULL;

        if (is_catalog)
        {
            // catalog 帧已到但无该路径条目: 无 mergeinfo
            return 0;
        }

        if (p_ra->rx_len <= 0)
        {
            break;
        }
    }

    return 0;
}


void svn_ra_locsegs_free(svn_ra_locseg_t* p_segs, int count)
{
    int i = 0;

    if (NULL == p_segs)
    {
        return;
    }

    for (i = 0; i < count; ++i)
    {
        KLB_FREE(p_segs[i].p_path);
    }
    KLB_FREE(p_segs);
}


int svn_ra_get_location_segments(svn_ra_t* p_ra, const char* p_path, int64_t peg_rev,
                                 int64_t start_rev, int64_t end_rev,
                                 svn_ra_locseg_t** pp_segs, int* p_count)
{
    svn_buf_t params = { 0 };
    svn_sexpr_t* p_item = NULL;
    svn_sexpr_t* p_done_resp = NULL;
    svn_ra_locseg_t* p_out = NULL;
    int count = 0;
    int cap = 0;

    if (NULL == p_ra || NULL == p_path || NULL == pp_segs || NULL == p_count)
    {
        return -1;
    }

    *pp_segs = NULL;
    *p_count = 0;

    // svnserve 形参 "c(?r)(?r)(?r)": 路径 + peg/start/end (缺省为空表)
    if (0 == svn_buf_append_string(&params, p_path)
        || 0 == svn_buf_append_opt_rev(&params, peg_rev)
        || 0 == svn_buf_append_opt_rev(&params, start_rev)
        || 0 == svn_buf_append_opt_rev(&params, end_rev))
    {
        svn_buf_free(&params);
        return -1;
    }

    if (0 == svn_ra_send_command(p_ra, "get-location-segments", &params))
    {
        svn_buf_free(&params);
        return -1;
    }
    svn_buf_free(&params);

    // 段条目流式到达, 以 done 收尾; 其后才是 success 响应
    if (0 == svn_ra_read_auth_only(p_ra))
    {
        return -1;
    }

    while (1)
    {
        const svn_sexpr_t* p_start_node = NULL;
        const svn_sexpr_t* p_end_node = NULL;
        const char* p_seg_path = NULL;

        svn_sexpr_free(p_item);
        p_item = NULL;

        if (0 == svn_ra_read_item(p_ra, &p_item))
        {
            svn_ra_locsegs_free(p_out, count);
            return -1;
        }

        if (svn_sexpr_is_word(p_item, "done"))
        {
            svn_sexpr_free(p_item);
            p_item = NULL;
            break;
        }

        if (SVN_SEXPR_LIST != p_item->type || p_item->child_count < 2)
        {
            svn_sexpr_free(p_item);
            p_item = NULL;
            svn_ra_locsegs_free(p_out, count);
            return -1;
        }

        p_start_node = svn_sexpr_child(p_item, 0);
        p_end_node = svn_sexpr_child(p_item, 1);
        if (NULL == p_start_node || SVN_SEXPR_NUMBER != p_start_node->type
            || NULL == p_end_node || SVN_SEXPR_NUMBER != p_end_node->type)
        {
            svn_sexpr_free(p_item);
            p_item = NULL;
            svn_ra_locsegs_free(p_out, count);
            return -1;
        }

        if (count >= cap)
        {
            svn_ra_locseg_t* p_new = NULL;
            int new_cap = (0 == cap) ? 8 : (cap * 2);

            p_new = KLB_REALLOCZ(p_out, svn_ra_locseg_t, new_cap, 0);
            if (NULL == p_new)
            {
                svn_sexpr_free(p_item);
                p_item = NULL;
                svn_ra_locsegs_free(p_out, count);
                return -1;
            }
            p_out = p_new;
            cap = new_cap;
        }

        p_out[count].range_start = p_start_node->number;
        p_out[count].range_end = p_end_node->number;

        // 段路径可缺省 (该区间内路径不存在)
        p_seg_path = svn_sexpr_opt_string_child(p_item, 2);
        if (NULL != p_seg_path)
        {
            int len = (int)strlen(p_seg_path);

            p_out[count].p_path = KLB_MALLOCZ(char, (size_t)len + 1, 0);
            if (NULL == p_out[count].p_path)
            {
                svn_sexpr_free(p_item);
                p_item = NULL;
                svn_ra_locsegs_free(p_out, count);
                return -1;
            }
            memcpy(p_out[count].p_path, p_seg_path, (size_t)len);
        }
        ++count;
    }

    // 收尾 success
    if (0 == svn_ra_read_cmd_response(p_ra, &p_done_resp))
    {
        svn_ra_locsegs_free(p_out, count);
        return -1;
    }
    svn_sexpr_free(p_done_resp);

    *pp_segs = p_out;
    *p_count = count;
    return 0;
}


void svn_ra_edit_evt_free(svn_ra_edit_evt_t* p_evt)
{
    if (NULL == p_evt)
    {
        return;
    }

    KLB_FREE(p_evt->p_path);
    KLB_FREE(p_evt->p_token);
    KLB_FREE(p_evt->p_parent_token);
    KLB_FREE(p_evt->p_name);
    KLB_FREE(p_evt->p_value);
    KLB_FREE(p_evt->p_checksum);
    KLB_FREE(p_evt->p_copy_path);
    KLB_FREE(p_evt->p_chunk);
    memset(p_evt, 0, sizeof(*p_evt));
}


static char* svn_ra_dup_bytes(const char* p_s, int len)
{
    char* p_out = NULL;

    p_out = KLB_MALLOCZ(char, (size_t)len + 1, 0);
    if (NULL == p_out)
    {
        return NULL;
    }

    if (len > 0)
    {
        memcpy(p_out, p_s, (size_t)len);
    }
    return p_out;
}


static int svn_ra_evt_take_str(char** pp_dst, const svn_sexpr_t* p_list, int idx)
{
    const char* p_s = svn_sexpr_opt_string_child(p_list, idx);

    *pp_dst = NULL;
    if (NULL == p_s)
    {
        return 0;
    }

    *pp_dst = svn_ra_dup_bytes(p_s, (int)strlen(p_s));
    return (NULL == *pp_dst) ? -1 : 0;
}


static void svn_ra_evt_take_rev(svn_ra_edit_evt_t* p_evt, const svn_sexpr_t* p_list, int idx)
{
    const svn_sexpr_t* p_node = svn_sexpr_child(p_list, idx);

    p_evt->has_rev = 0;
    p_evt->rev = -1;
    if (NULL == p_node)
    {
        return;
    }

    if (SVN_SEXPR_NUMBER == p_node->type)
    {
        p_evt->has_rev = 1;
        p_evt->rev = p_node->number;
        return;
    }

    // 可选修订在 wire 上为 ( rev ) 或空表 ( )
    if (SVN_SEXPR_LIST == p_node->type && p_node->child_count >= 1
        && SVN_SEXPR_NUMBER == p_node->pp_children[0]->type)
    {
        p_evt->has_rev = 1;
        p_evt->rev = p_node->pp_children[0]->number;
    }
}


static int svn_ra_evt_abort(svn_sexpr_t* p_item, svn_ra_edit_evt_t* p_evt)
{
    svn_sexpr_free(p_item);
    svn_ra_edit_evt_free(p_evt);
    return -1;
}


int svn_ra_replay_begin(svn_ra_t* p_ra, int64_t rev, int64_t low_water_mark, bool send_deltas)
{
    svn_buf_t params = { 0 };

    if (NULL == p_ra)
    {
        return -1;
    }

    // ra_svn: replay ( rev low-water-mark send-deltas )
    if (0 == svn_buf_append_number(&params, rev)
        || 0 == svn_buf_append_number(&params, low_water_mark)
        || 0 == svn_buf_append_word(&params, send_deltas ? "true" : "false"))
    {
        svn_buf_free(&params);
        return -1;
    }

    if (0 == svn_ra_send_command(p_ra, "replay", &params))
    {
        svn_buf_free(&params);
        return -1;
    }
    svn_buf_free(&params);

    // 认证交换完成后, 服务器切换到编辑器命令集
    if (0 == svn_ra_read_auth_only(p_ra))
    {
        return -1;
    }
    return 0;
}


int svn_ra_replay_next(svn_ra_t* p_ra, svn_ra_edit_evt_t* p_evt, bool* p_done)
{
    svn_sexpr_t* p_item = NULL;
    svn_sexpr_t* p_resp = NULL;
    const svn_sexpr_t* p_node = NULL;
    const char* p_cmd = NULL;

    if (NULL == p_ra || NULL == p_evt || NULL == p_done)
    {
        return -1;
    }

    memset(p_evt, 0, sizeof(*p_evt));
    *p_done = false;

    if (0 == svn_ra_read_item(p_ra, &p_item))
    {
        if (NULL != getenv("KSVNRA_COMMIT_DEBUG"))
        {
            fprintf(stderr, "ksvnra replay: read_item failed rx=%d head=%.*s\n",
                p_ra->rx_len, (p_ra->rx_len > 80) ? 80 : p_ra->rx_len, p_ra->p_rx_buf);
            fflush(stderr);
        }
        return -1;
    }

    if (NULL != getenv("KSVNRA_COMMIT_DEBUG"))
    {
        fprintf(stderr, "ksvnra replay: item type=%d children=%d\n",
            (int)p_item->type, (SVN_SEXPR_LIST == p_item->type) ? p_item->child_count : 0);
        fflush(stderr);
    }

    if (SVN_SEXPR_LIST != p_item->type || p_item->child_count < 1)
    {
        return svn_ra_evt_abort(p_item, p_evt);
    }

    p_cmd = svn_sexpr_word_cstr(p_item->pp_children[0]);
    if (NULL == p_cmd)
    {
        p_cmd = svn_sexpr_string_cstr(p_item->pp_children[0]);
    }
    if (NULL == p_cmd)
    {
        return svn_ra_evt_abort(p_item, p_evt);
    }

    if (0 == strcmp(p_cmd, "failure"))
    {
        // 服务器拒绝 (authz/修订无效等): 流式命令无 success 包裹, 需就地记录错误
        svn_ra_capture_failure(p_ra, p_item);
        return svn_ra_evt_abort(p_item, p_evt);
    }

    // finish-replay 无应答; 其后才是 replay 命令的 success 响应 (帧为 ( finish-replay ( ) ))
    if (0 == strcmp(p_cmd, "finish-replay"))
    {
        svn_sexpr_free(p_item);
        if (0 == svn_ra_read_cmd_response(p_ra, &p_resp))
        {
            return -1;
        }
        svn_sexpr_free(p_resp);
        p_evt->type = SVN_RA_EDIT_EVT_FINISH_REPLAY;
        *p_done = true;
        return 0;
    }

    if (0 == strcmp(p_cmd, "target-rev"))
    {
        p_evt->type = SVN_RA_EDIT_EVT_TARGET_REV;
        svn_ra_evt_take_rev(p_evt, p_item, 1);
    }
    else if (0 == strcmp(p_cmd, "open-root"))
    {
        p_evt->type = SVN_RA_EDIT_EVT_OPEN_ROOT;
        svn_ra_evt_take_rev(p_evt, p_item, 1);
        if (0 != svn_ra_evt_take_str(&p_evt->p_token, p_item, 2))
        {
            return svn_ra_evt_abort(p_item, p_evt);
        }
    }
    else if (0 == strcmp(p_cmd, "delete-entry"))
    {
        p_evt->type = SVN_RA_EDIT_EVT_DELETE_ENTRY;
        if (0 != svn_ra_evt_take_str(&p_evt->p_path, p_item, 1))
        {
            return svn_ra_evt_abort(p_item, p_evt);
        }
        svn_ra_evt_take_rev(p_evt, p_item, 2);
        if (0 != svn_ra_evt_take_str(&p_evt->p_token, p_item, 3))
        {
            return svn_ra_evt_abort(p_item, p_evt);
        }
    }
    else if (0 == strcmp(p_cmd, "add-dir") || 0 == strcmp(p_cmd, "open-dir"))
    {
        p_evt->type = (0 == strcmp(p_cmd, "add-dir")) ? SVN_RA_EDIT_EVT_ADD_DIR
                                                       : SVN_RA_EDIT_EVT_OPEN_DIR;
        if (0 != svn_ra_evt_take_str(&p_evt->p_path, p_item, 1)
            || 0 != svn_ra_evt_take_str(&p_evt->p_parent_token, p_item, 2)
            || 0 != svn_ra_evt_take_str(&p_evt->p_token, p_item, 3))
        {
            return svn_ra_evt_abort(p_item, p_evt);
        }
        // add-dir [copy-path copy-rev] / open-dir rev
        if (SVN_RA_EDIT_EVT_OPEN_DIR == p_evt->type)
        {
            svn_ra_evt_take_rev(p_evt, p_item, 4);
        }
        else if (0 == svn_ra_evt_take_str(&p_evt->p_copy_path, p_item, 4))
        {
            svn_ra_evt_take_rev(p_evt, p_item, 5);
        }
        else
        {
            return svn_ra_evt_abort(p_item, p_evt);
        }
    }
    else if (0 == strcmp(p_cmd, "change-dir-prop"))
    {
        p_evt->type = SVN_RA_EDIT_EVT_CHANGE_DIR_PROP;
        if (0 != svn_ra_evt_take_str(&p_evt->p_token, p_item, 1)
            || 0 != svn_ra_evt_take_str(&p_evt->p_name, p_item, 2)
            || 0 != svn_ra_evt_take_str(&p_evt->p_value, p_item, 3))
        {
            return svn_ra_evt_abort(p_item, p_evt);
        }
    }
    else if (0 == strcmp(p_cmd, "close-dir"))
    {
        p_evt->type = SVN_RA_EDIT_EVT_CLOSE_DIR;
        if (0 != svn_ra_evt_take_str(&p_evt->p_token, p_item, 1))
        {
            return svn_ra_evt_abort(p_item, p_evt);
        }
    }
    else if (0 == strcmp(p_cmd, "absent-dir") || 0 == strcmp(p_cmd, "absent-file"))
    {
        p_evt->type = (0 == strcmp(p_cmd, "absent-dir")) ? SVN_RA_EDIT_EVT_ABSENT_DIR
                                                         : SVN_RA_EDIT_EVT_ABSENT_FILE;
        if (0 != svn_ra_evt_take_str(&p_evt->p_path, p_item, 1)
            || 0 != svn_ra_evt_take_str(&p_evt->p_parent_token, p_item, 2))
        {
            return svn_ra_evt_abort(p_item, p_evt);
        }
    }
    else if (0 == strcmp(p_cmd, "add-file") || 0 == strcmp(p_cmd, "open-file"))
    {
        p_evt->type = (0 == strcmp(p_cmd, "add-file")) ? SVN_RA_EDIT_EVT_ADD_FILE
                                                       : SVN_RA_EDIT_EVT_OPEN_FILE;
        if (0 != svn_ra_evt_take_str(&p_evt->p_path, p_item, 1)
            || 0 != svn_ra_evt_take_str(&p_evt->p_parent_token, p_item, 2)
            || 0 != svn_ra_evt_take_str(&p_evt->p_token, p_item, 3))
        {
            return svn_ra_evt_abort(p_item, p_evt);
        }
        // add-file [copy-path copy-rev] / open-file rev
        if (SVN_RA_EDIT_EVT_OPEN_FILE == p_evt->type)
        {
            svn_ra_evt_take_rev(p_evt, p_item, 4);
        }
        else if (0 == svn_ra_evt_take_str(&p_evt->p_copy_path, p_item, 4))
        {
            svn_ra_evt_take_rev(p_evt, p_item, 5);
        }
        else
        {
            return svn_ra_evt_abort(p_item, p_evt);
        }
    }
    else if (0 == strcmp(p_cmd, "apply-textdelta"))
    {
        p_evt->type = SVN_RA_EDIT_EVT_APPLY_TEXTDELTA;
        if (0 != svn_ra_evt_take_str(&p_evt->p_token, p_item, 1)
            || 0 != svn_ra_evt_take_str(&p_evt->p_checksum, p_item, 2))
        {
            return svn_ra_evt_abort(p_item, p_evt);
        }
    }
    else if (0 == strcmp(p_cmd, "textdelta-chunk"))
    {
        p_evt->type = SVN_RA_EDIT_EVT_TEXTDELTA_CHUNK;
        if (0 != svn_ra_evt_take_str(&p_evt->p_token, p_item, 1))
        {
            return svn_ra_evt_abort(p_item, p_evt);
        }

        p_node = svn_sexpr_child(p_item, 2);
        if (NULL != p_node && SVN_SEXPR_STRING == p_node->type && p_node->string_len > 0)
        {
            p_evt->p_chunk = svn_ra_dup_bytes(p_node->p_string, p_node->string_len);
            if (NULL == p_evt->p_chunk)
            {
                return svn_ra_evt_abort(p_item, p_evt);
            }
            p_evt->chunk_len = p_node->string_len;
        }
    }
    else if (0 == strcmp(p_cmd, "textdelta-end"))
    {
        p_evt->type = SVN_RA_EDIT_EVT_TEXTDELTA_END;
        if (0 != svn_ra_evt_take_str(&p_evt->p_token, p_item, 1))
        {
            return svn_ra_evt_abort(p_item, p_evt);
        }
    }
    else if (0 == strcmp(p_cmd, "change-file-prop"))
    {
        p_evt->type = SVN_RA_EDIT_EVT_CHANGE_FILE_PROP;
        if (0 != svn_ra_evt_take_str(&p_evt->p_token, p_item, 1)
            || 0 != svn_ra_evt_take_str(&p_evt->p_name, p_item, 2)
            || 0 != svn_ra_evt_take_str(&p_evt->p_value, p_item, 3))
        {
            return svn_ra_evt_abort(p_item, p_evt);
        }
    }
    else if (0 == strcmp(p_cmd, "close-file"))
    {
        p_evt->type = SVN_RA_EDIT_EVT_CLOSE_FILE;
        if (0 != svn_ra_evt_take_str(&p_evt->p_token, p_item, 1)
            || 0 != svn_ra_evt_take_str(&p_evt->p_checksum, p_item, 2))
        {
            return svn_ra_evt_abort(p_item, p_evt);
        }
    }
    else if (0 == strcmp(p_cmd, "close-edit"))
    {
        p_evt->type = SVN_RA_EDIT_EVT_CLOSE_EDIT;
        return svn_ra_evt_abort(p_item, p_evt);
    }
    else
    {
        return svn_ra_evt_abort(p_item, p_evt);
    }

    svn_sexpr_free(p_item);
    return 0;
}


int svn_ra_cat_begin(svn_ra_t* p_ra, const char* p_path, int64_t rev)
{
    svn_buf_t params = { 0 };
    svn_sexpr_t* p_resp = NULL;

    if (NULL == p_ra || NULL == p_path || 0 != p_ra->cat_active)
    {
        return -1;
    }

    {
        int64_t use_rev = rev;

        if (use_rev <= 0)
        {
            if (0 != svn_ra_youngest(p_ra, &use_rev))
            {
                return -1;
            }
        }

        // get-file ( path rev want-props want-contents )
        svn_buf_append_string(&params, p_path);
        svn_buf_append_rev_list(&params, use_rev);
        svn_buf_append_word(&params, "false");
        svn_buf_append_word(&params, "true");
    }

    if (0 == svn_ra_send_command(p_ra, "get-file", &params))
    {
        svn_buf_free(&params);
        return -1;
    }
    svn_buf_free(&params);

    // get-file 状态帧可能不止一帧; 读到正文分块为止。失败帧置 err_code, 此时无正文
    while (svn_ra_next_is_list(p_ra))
    {
        if (0 == svn_ra_read_cmd_response(p_ra, &p_resp))
        {
            return -1;
        }
        svn_sexpr_free(p_resp);
        p_resp = NULL;

        if (0 != p_ra->err_code)
        {
            return -1;
        }
    }

    p_ra->cat_active = 1;
    p_ra->cat_done = 0;
    return 0;
}


int svn_ra_cat_next(svn_ra_t* p_ra, uint8_t** pp_chunk, int* p_len, bool* p_eof)
{
    svn_sexpr_t* p_item = NULL;

    if (NULL == p_ra || NULL == pp_chunk || NULL == p_len || NULL == p_eof)
    {
        return -1;
    }

    *pp_chunk = NULL;
    *p_len = 0;
    *p_eof = false;

    if (0 == p_ra->cat_active)
    {
        return -1;
    }

    if (0 == svn_ra_read_item(p_ra, &p_item))
    {
        return -1;
    }

    if (SVN_SEXPR_STRING != p_item->type)
    {
        svn_sexpr_free(p_item);
        return -1;
    }

    if (p_item->string_len <= 0 || NULL == p_item->p_string)
    {
        svn_sexpr_free(p_item);
        p_ra->cat_done = 1;
        *p_eof = true;
        return 0;
    }

    // 二进制内容可含 NUL, 整块移交调用方; 勿再拷贝
    *pp_chunk = (uint8_t*)p_item->p_string;
    *p_len = p_item->string_len;
    p_item->p_string = NULL;
    svn_sexpr_free(p_item);
    return 0;
}


int svn_ra_cat_close(svn_ra_t* p_ra)
{
    int rc = 0;

    if (NULL == p_ra)
    {
        return -1;
    }

    if (0 == p_ra->cat_active)
    {
        return 0;
    }

    // 未读完时丢弃剩余正文, 保证会话可继续发下一命令
    while (0 == p_ra->cat_done)
    {
        svn_sexpr_t* p_item = NULL;

        if (0 == svn_ra_read_item(p_ra, &p_item))
        {
            rc = -1;
            break;
        }

        if (SVN_SEXPR_STRING != p_item->type)
        {
            svn_sexpr_free(p_item);
            rc = -1;
            break;
        }

        if (p_item->string_len <= 0)
        {
            p_ra->cat_done = 1;
        }
        svn_sexpr_free(p_item);
    }

    // 正文结束后 svnserve 仍发一帧收尾响应; 读取失败时不再追加阻塞
    if (0 == rc)
    {
        svn_sexpr_t* p_resp = NULL;

        if (0 == svn_ra_read_cmd_response(p_ra, &p_resp))
        {
            rc = -1;
        }
        else
        {
            svn_sexpr_free(p_resp);
        }
    }

    p_ra->cat_active = 0;
    p_ra->cat_done = 0;
    return rc;
}


int svn_ra_cat(svn_ra_t* p_ra, const char* p_path, int64_t rev,
               uint8_t** pp_data, int* p_len)
{
    svn_buf_t out = { 0 };

    if (NULL == p_ra || NULL == p_path || NULL == pp_data || NULL == p_len)
    {
        return -1;
    }

    if (0 != svn_ra_cat_begin(p_ra, p_path, rev))
    {
        return -1;
    }

    while (1)
    {
        uint8_t* p_chunk = NULL;
        int chunk_len = 0;
        bool eof = false;

        if (0 != svn_ra_cat_next(p_ra, &p_chunk, &chunk_len, &eof))
        {
            svn_ra_cat_close(p_ra);
            svn_buf_free(&out);
            return -1;
        }

        if (eof)
        {
            break;
        }

        if (0 == svn_buf_append_raw(&out, (const char*)p_chunk, chunk_len))
        {
            KLB_FREE(p_chunk);
            svn_ra_cat_close(p_ra);
            svn_buf_free(&out);
            return -1;
        }

        KLB_FREE(p_chunk);
    }

    if (0 != svn_ra_cat_close(p_ra))
    {
        svn_buf_free(&out);
        return -1;
    }

    *pp_data = (uint8_t*)out.p_data;
    *p_len = out.len;
    return 0;
}


int svn_ra_get_dated_rev(svn_ra_t* p_ra, const char* p_date, int64_t* p_rev)
{
    svn_buf_t params = { 0 };
    svn_sexpr_t* p_resp = NULL;
    int64_t got = -1;

    if (NULL == p_ra || NULL == p_date || NULL == p_rev)
    {
        return -1;
    }

    svn_buf_append_string(&params, p_date);

    if (0 == svn_ra_send_command(p_ra, "get-dated-rev", &params))
    {
        svn_buf_free(&params);
        return -1;
    }
    svn_buf_free(&params);

    if (0 == svn_ra_read_auth_then_response(p_ra, &p_resp))
    {
        return -1;
    }

    // failure 帧也含数字; 有错误码即失败, 勿误取错误码当版本号
    if (0 != p_ra->err_code)
    {
        svn_sexpr_free(p_resp);
        return -1;
    }

    if (NULL == p_resp || 0 == svn_ra_sexpr_pick_rev(p_resp, &got))
    {
        svn_sexpr_free(p_resp);
        return -1;
    }

    svn_sexpr_free(p_resp);
    *p_rev = got;
    return 0;
}


int svn_ra_get_deleted_rev(svn_ra_t* p_ra, const char* p_path, int64_t peg_rev,
                           int64_t end_rev, int64_t* p_del_rev)
{
    svn_buf_t params = { 0 };
    svn_sexpr_t* p_resp = NULL;
    int64_t got = -1;

    if (NULL == p_ra || NULL == p_path || NULL == p_del_rev)
    {
        return -1;
    }

    if (end_rev <= 0)
    {
        end_rev = peg_rev;
    }

    svn_buf_append_string(&params, p_path);
    // svnserve 形参 "crr": cstring + 裸 peg/end revision 数 (勿用 rev list)
    svn_buf_append_number(&params, peg_rev);
    svn_buf_append_number(&params, end_rev);

    if (0 == svn_ra_send_command(p_ra, "get-deleted-rev", &params))
    {
        svn_buf_free(&params);
        return -1;
    }
    svn_buf_free(&params);

    if (0 == svn_ra_read_auth_then_response(p_ra, &p_resp))
    {
        return -1;
    }

    // failure 帧也含数字; 有错误码即失败, 勿误取错误码当删除版本
    if (0 != p_ra->err_code)
    {
        svn_sexpr_free(p_resp);
        return -1;
    }

    if (NULL == p_resp || 0 == svn_ra_sexpr_pick_rev(p_resp, &got))
    {
        svn_sexpr_free(p_resp);
        return -1;
    }

    svn_sexpr_free(p_resp);
    *p_del_rev = got;
    return 0;
}

// end
