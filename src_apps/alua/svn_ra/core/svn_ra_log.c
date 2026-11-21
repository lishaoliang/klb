// Doc Encode : UTF-8 BOM, Unix(LF)
#include "alua/svn_ra/core/svn_ra_priv.h"

/// @brief 仅消化 auth 请求 (其后可能是流式条目而非 success)
bool svn_ra_read_auth_only(svn_ra_t* p_ra)
{
    svn_sexpr_t* p_auth_params = NULL;

    if (0 == svn_ra_read_cmd_response(p_ra, &p_auth_params))
    {
        return false;
    }

    if (0 == svn_ra_handle_auth_request(p_ra, p_auth_params))
    {
        svn_sexpr_free(p_auth_params);
        return false;
    }

    svn_sexpr_free(p_auth_params);
    return true;
}


bool svn_buf_append_opt_rev(svn_buf_t* p_buf, int64_t rev)
{
    if (rev < 0)
    {
        if (0 == svn_buf_open_list(p_buf))
        {
            return false;
        }
        return svn_buf_close_list(p_buf);
    }

    return svn_buf_append_rev_list(p_buf, rev);
}


const char* svn_sexpr_opt_string_child(const svn_sexpr_t* p_list, int idx)
{
    const svn_sexpr_t* p = svn_sexpr_child(p_list, idx);
    const char* p_s = NULL;

    if (NULL == p)
    {
        return NULL;
    }

    p_s = svn_sexpr_string_cstr(p);
    if (NULL != p_s)
    {
        return p_s;
    }

    if (SVN_SEXPR_LIST == p->type && p->child_count >= 1)
    {
        return svn_sexpr_string_cstr(p->pp_children[0]);
    }

    return NULL;
}


typedef struct svn_log_arr_t_
{
    svn_ra_log_entry_t* p_items;
    int                 count;
    int                 cap;
} svn_log_arr_t;


int svn_ra_sexpr_bool_opt(const svn_sexpr_t* p_node, int default_val)
{
    if (NULL == p_node)
    {
        return default_val;
    }

    if (svn_sexpr_is_word(p_node, "true"))
    {
        return 1;
    }

    if (svn_sexpr_is_word(p_node, "false"))
    {
        return 0;
    }

    return default_val;
}


const char* svn_ra_sexpr_path_cstr(const svn_sexpr_t* p_node)
{
    const char* p_s = NULL;

    if (NULL == p_node)
    {
        return NULL;
    }

    p_s = svn_sexpr_string_cstr(p_node);
    if (NULL != p_s)
    {
        return p_s;
    }

    return svn_sexpr_word_cstr(p_node);
}


bool svn_ra_parse_changed_path_copyfrom(const svn_sexpr_t* p_copy,
                                               svn_ra_log_changed_path_t* p_out)
{
    const char* p_cpath = NULL;
    const svn_sexpr_t* p_crev = NULL;

    if (NULL == p_copy || SVN_SEXPR_LIST != p_copy->type || p_copy->child_count < 2)
    {
        return true;
    }

    p_cpath = svn_ra_sexpr_path_cstr(svn_sexpr_child(p_copy, 0));
    p_crev = svn_sexpr_child(p_copy, 1);
    if (NULL == p_cpath || NULL == p_crev || SVN_SEXPR_NUMBER != p_crev->type)
    {
        return true;
    }

    svn_ra_copy_string_field(p_out->copyfrom_path, (int)sizeof(p_out->copyfrom_path), p_cpath);
    p_out->copyfrom_rev = p_crev->number;
    return true;
}


bool svn_ra_parse_changed_path_extra(const svn_sexpr_t* p_extra,
                                            svn_ra_log_changed_path_t* p_out)
{
    const char* p_kind = NULL;

    if (NULL == p_extra || SVN_SEXPR_LIST != p_extra->type || p_extra->child_count < 1)
    {
        return true;
    }

    p_kind = svn_sexpr_word_cstr(svn_sexpr_child(p_extra, 0));
    if (NULL != p_kind)
    {
        svn_ra_copy_string_field(p_out->node_kind, (int)sizeof(p_out->node_kind), p_kind);
    }

    if (p_extra->child_count >= 2)
    {
        p_out->text_mods = svn_ra_sexpr_bool_opt(svn_sexpr_child(p_extra, 1), -1);
    }

    if (p_extra->child_count >= 3)
    {
        p_out->prop_mods = svn_ra_sexpr_bool_opt(svn_sexpr_child(p_extra, 2), -1);
    }

    return true;
}


bool svn_ra_parse_changed_path_entry(const svn_sexpr_t* p_ent,
                                            svn_ra_log_changed_path_t* p_out)
{
    const char* p_path = NULL;
    const char* p_action = NULL;

    if (NULL == p_ent || SVN_SEXPR_LIST != p_ent->type || p_ent->child_count < 2)
    {
        return false;
    }

    memset(p_out, 0, sizeof(*p_out));
    p_out->copyfrom_rev = -1;
    p_out->text_mods = -1;
    p_out->prop_mods = -1;
    svn_ra_copy_string_field(p_out->node_kind, (int)sizeof(p_out->node_kind), "file");

    p_path = svn_ra_sexpr_path_cstr(svn_sexpr_child(p_ent, 0));
    p_action = svn_sexpr_word_cstr(svn_sexpr_child(p_ent, 1));
    if (NULL == p_path || NULL == p_action || '\0' == p_action[0])
    {
        return false;
    }

    svn_ra_copy_string_field(p_out->path, (int)sizeof(p_out->path), p_path);
    p_out->action = p_action[0];

    if (!svn_ra_parse_changed_path_copyfrom(svn_sexpr_child(p_ent, 2), p_out))
    {
        return false;
    }

    if (!svn_ra_parse_changed_path_extra(svn_sexpr_child(p_ent, 3), p_out))
    {
        return false;
    }

    return true;
}


bool svn_ra_parse_changed_paths(const svn_sexpr_t* p_changes,
                                       svn_ra_log_changed_path_t** pp_paths,
                                       int* p_count)
{
    int i = 0;

    if (NULL == pp_paths || NULL == p_count)
    {
        return false;
    }

    *pp_paths = NULL;
    *p_count = 0;

    if (NULL == p_changes || SVN_SEXPR_LIST != p_changes->type
        || p_changes->child_count <= 0)
    {
        return true;
    }

    *pp_paths = (svn_ra_log_changed_path_t*)KLB_MALLOC(
        svn_ra_log_changed_path_t, p_changes->child_count, 0);
    if (NULL == *pp_paths)
    {
        return false;
    }

    for (i = 0; i < p_changes->child_count; ++i)
    {
        if (0 == svn_ra_parse_changed_path_entry(p_changes->pp_children[i], &(*pp_paths)[i]))
        {
            KLB_FREE(*pp_paths);
            *pp_paths = NULL;
            *p_count = 0;
            return false;
        }
    }

    *p_count = p_changes->child_count;
    return true;
}


void svn_ra_log_entries_free(svn_ra_log_entry_t* p_ents, int count)
{
    int i = 0;

    if (NULL == p_ents)
    {
        return;
    }

    for (i = 0; i < count; ++i)
    {
        KLB_FREE(p_ents[i].p_changed_paths);
    }

    KLB_FREE(p_ents);
}


void svn_ra_log_arr_free(svn_log_arr_t* p_arr)
{
    if (NULL == p_arr)
    {
        return;
    }

    svn_ra_log_entries_free(p_arr->p_items, p_arr->count);
    p_arr->p_items = NULL;
    p_arr->count = 0;
    p_arr->cap = 0;
}


bool svn_ra_log_arr_push(svn_log_arr_t* p_arr, const svn_ra_log_entry_t* p_ent)
{
    if (NULL == p_arr || NULL == p_ent)
    {
        return false;
    }

    if (p_arr->count >= p_arr->cap)
    {
        int new_cap = (p_arr->cap > 0) ? p_arr->cap * 2 : 16;
        svn_ra_log_entry_t* p_new = (svn_ra_log_entry_t*)KLB_REALLOC(
            p_arr->p_items, svn_ra_log_entry_t, new_cap, 0);

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


int svn_ra_log(svn_ra_t* p_ra, const char** pp_paths, int path_count,
               int64_t start_rev, int64_t end_rev, int limit, int changed_paths,
               svn_ra_log_entry_t** pp_ents, int* p_count)
{
    svn_buf_t params = { 0 };
    svn_sexpr_t* p_item = NULL;
    svn_sexpr_t* p_done_resp = NULL;
    svn_log_arr_t arr = { 0 };
    int i = 0;

    if (NULL == p_ra || NULL == pp_ents || NULL == p_count)
    {
        return -1;
    }

    *pp_ents = NULL;
    *p_count = 0;

    if (path_count < 0)
    {
        path_count = 0;
    }

    if (limit < 0)
    {
        limit = 0;
    }

    // step1. ( ( path... ) [start] [end] changed strict limit include-merged
    //         revprops ( svn:author svn:date svn:log ) )
    // 勿发 all-revprops: svnserve 1.13 会断连; 对齐官方客户端命名 revprops
    if (0 == svn_buf_open_list(&params))
    {
        return -1;
    }

    if (NULL != pp_paths)
    {
        for (i = 0; i < path_count; ++i)
        {
            if (0 == svn_buf_append_string(&params,
                    (NULL != pp_paths[i]) ? pp_paths[i] : ""))
            {
                svn_buf_free(&params);
                return -1;
            }
        }
    }

    if (0 == svn_buf_close_list(&params))
    {
        svn_buf_free(&params);
        return -1;
    }

    if (0 == svn_buf_append_opt_rev(&params, start_rev)
        || 0 == svn_buf_append_opt_rev(&params, end_rev)
        || 0 == svn_buf_append_word(&params, (0 != changed_paths) ? "true" : "false")
        || 0 == svn_buf_append_word(&params, "false")
        || 0 == svn_buf_append_number(&params, (int64_t)limit)
        || 0 == svn_buf_append_word(&params, "false")
        || 0 == svn_buf_append_word(&params, "revprops")
        || 0 == svn_buf_open_list(&params)
        || 0 == svn_buf_append_string(&params, "svn:author")
        || 0 == svn_buf_append_string(&params, "svn:date")
        || 0 == svn_buf_append_string(&params, "svn:log")
        || 0 == svn_buf_close_list(&params))
    {
        svn_buf_free(&params);
        return -1;
    }

    if (0 == svn_ra_send_command(p_ra, "log", &params))
    {
        svn_buf_free(&params);
        return -1;
    }
    svn_buf_free(&params);

    // step2. auth, 然后流式 log-entry 直到 done
    if (0 == svn_ra_read_auth_only(p_ra))
    {
        return -1;
    }

    while (1)
    {
        svn_ra_log_entry_t ent = { 0 };
        const svn_sexpr_t* p_rev_node = NULL;
        const svn_sexpr_t* p_changes = NULL;
        const char* p_author = NULL;
        const char* p_date = NULL;
        const char* p_msg = NULL;

        if (0 == svn_ra_read_item(p_ra, &p_item))
        {
            if (NULL != getenv("KSVNRA_COMMIT_DEBUG"))
            {
                fprintf(stderr, "ksvnra log: read_item fail\n");
                fflush(stderr);
            }
            svn_ra_log_arr_free(&arr);
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
            if (NULL != getenv("KSVNRA_COMMIT_DEBUG"))
            {
                fprintf(stderr, "ksvnra log: bad item type=%d children=%d\n",
                    (int)p_item->type, p_item->child_count);
                fflush(stderr);
            }
            svn_sexpr_free(p_item);
            svn_ra_log_arr_free(&arr);
            return -1;
        }

        p_changes = svn_sexpr_child(p_item, 0);
        p_rev_node = svn_sexpr_child(p_item, 1);
        if (NULL == p_rev_node || SVN_SEXPR_NUMBER != p_rev_node->type)
        {
            if (NULL != getenv("KSVNRA_COMMIT_DEBUG"))
            {
                fprintf(stderr, "ksvnra log: rev node bad\n");
                fflush(stderr);
            }
            svn_sexpr_free(p_item);
            svn_ra_log_arr_free(&arr);
            return -1;
        }

        ent.rev = p_rev_node->number;
        p_author = svn_sexpr_opt_string_child(p_item, 2);
        p_date = svn_sexpr_opt_string_child(p_item, 3);
        p_msg = svn_sexpr_opt_string_child(p_item, 4);
        svn_ra_copy_string_field(ent.author, (int)sizeof(ent.author), p_author);
        svn_ra_copy_string_field(ent.date, (int)sizeof(ent.date), p_date);
        svn_ra_copy_string_field(ent.message, (int)sizeof(ent.message), p_msg);

        if (0 != changed_paths)
        {
            if (0 == svn_ra_parse_changed_paths(p_changes, &ent.p_changed_paths,
                                                &ent.changed_path_count))
            {
                svn_sexpr_free(p_item);
                svn_ra_log_arr_free(&arr);
                return -1;
            }
        }

        if (0 == svn_ra_log_arr_push(&arr, &ent))
        {
            KLB_FREE(ent.p_changed_paths);
            svn_sexpr_free(p_item);
            svn_ra_log_arr_free(&arr);
            return -1;
        }

        svn_sexpr_free(p_item);
        p_item = NULL;
    }

    // step3. 收尾 success
    if (0 == svn_ra_read_cmd_response(p_ra, &p_done_resp))
    {
        svn_ra_log_arr_free(&arr);
        return -1;
    }
    svn_sexpr_free(p_done_resp);

    *pp_ents = arr.p_items;
    *p_count = arr.count;
    return 0;
}

// end
