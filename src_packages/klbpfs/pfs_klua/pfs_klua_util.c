// Doc Encode : UTF-8 BOM, Unix(LF)
#include "pfs_klua_util.h"
#include "pfsmtd.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int pfs_klua_push_rc(lua_State *L, int rc)
{
    lua_pushinteger(L, rc);
    lua_pushstring(L, pfs_strerror(rc));
    return 2;
}

int pfs_klua_push_blkio_rc(lua_State *L, int rc)
{
    if (PFS_BLKIO_RET_OK == rc)
    {
        return pfs_klua_push_rc(L, PFS_OK);
    }
    else if (PFS_BLKIO_RET_PARAM == rc)
    {
        return pfs_klua_push_rc(L, PFS_EINVAL);
    }
    else if (PFS_BLKIO_RET_IO == rc)
    {
        return pfs_klua_push_rc(L, PFS_EIO);
    }
    else if (PFS_BLKIO_RET_STATE == rc)
    {
        return pfs_klua_push_rc(L, PFS_EBUSY);
    }
    else if (PFS_BLKIO_RET_PROBE == rc)
    {
        return pfs_klua_push_rc(L, PFS_EOPNOTSUPP);
    }

    return pfs_klua_push_rc(L, PFS_EIO);
}

int pfs_klua_push_info_rc_fail(lua_State *L, int rc)
{
    lua_createtable(L, 0, 0);
    lua_pushinteger(L, rc);
    lua_pushstring(L, pfs_strerror(rc));
    return 3;
}

int pfs_klua_push_info_rc_ok(lua_State *L)
{
    lua_pushinteger(L, PFS_OK);
    lua_pushstring(L, pfs_strerror(PFS_OK));
    return 3;
}

void pfs_klua_push_blkio_container(lua_State *L, const char *path, pfs_blkio_t *p_blkio)
{
    const char *name = "?";

    if (NULL != p_blkio && NULL != p_blkio->ops && NULL != p_blkio->ops->name && '\0' != p_blkio->ops->name[0])
    {
        name = p_blkio->ops->name;
    }

    int sector_size = pfs_blkio_logical_sector_size(p_blkio);

    if (sector_size <= 0)
    {
        sector_size = 512;
    }
    int64_t size = pfs_blkio_get_size(p_blkio);
    int type_id = (NULL != p_blkio && NULL != p_blkio->ops) ? (int)p_blkio->ops->type : (int)PFS_BLKIO_RAW;

    if (NULL != path && '\0' != path[0])
    {
        lua_pushstring(L, path);
    }
    else
    {
        lua_pushstring(L, "");
    }
    lua_setfield(L, -2, "path");
    lua_pushstring(L, name);
    lua_setfield(L, -2, "container");
    lua_pushinteger(L, type_id);
    lua_setfield(L, -2, "type");
    lua_pushinteger(L, sector_size);
    lua_setfield(L, -2, "sector_size");
    lua_pushinteger(L, (lua_Integer)size);
    lua_setfield(L, -2, "size");
}

int pfs_klua_parse_opts_table(lua_State *L, const char **p_path, int *p_opts_idx)
{
    if (NULL == L || NULL == p_path || NULL == p_opts_idx)
    {
        return PFS_EINVAL;
    }

    if (0 == lua_istable(L, 1))
    {
        return PFS_EINVAL;
    }

    *p_opts_idx = 1;
    *p_path = pfs_klua_opts_string(L, 1, "path", NULL);
    if (NULL == *p_path || '\0' == (*p_path)[0])
    {
        return PFS_EINVAL;
    }

    return PFS_OK;
}

int pfs_klua_parse_path_opts(lua_State *L, const char **p_path, int *p_opts_idx)
{
    if (NULL == L || NULL == p_path || NULL == p_opts_idx)
    {
        return -1;
    }

    *p_opts_idx = 0;

    if (lua_type(L, 1) == LUA_TSTRING)
    {
        *p_path = luaL_checkstring(L, 1);
        if (lua_gettop(L) >= 2 && 0 != lua_istable(L, 2))
        {
            *p_opts_idx = 2;
        }
        return 0;
    }

    if (0 != lua_istable(L, 1))
    {
        *p_opts_idx = 1;
        *p_path = pfs_klua_opts_string(L, 1, "path", NULL);
        if (NULL == *p_path || '\0' == (*p_path)[0])
        {
            return -1;
        }
        return 0;
    }

    return -1;
}

const char *pfs_klua_opts_string(lua_State *L, int opts_idx, const char *key, const char *p_default)
{
    if (NULL == L || NULL == key || opts_idx <= 0 || 0 == lua_istable(L, opts_idx))
    {
        return p_default;
    }

    lua_getfield(L, opts_idx, key);
    if (0 != lua_isstring(L, -1))
    {
        const char *p = lua_tostring(L, -1);

        lua_pop(L, 1);
        if (NULL != p && '\0' != p[0])
        {
            return p;
        }
        return p_default;
    }

    lua_pop(L, 1);
    return p_default;
}

int pfs_klua_opts_bool(lua_State *L, int opts_idx, const char *key, int v_default)
{
    if (NULL == L || NULL == key || opts_idx <= 0 || 0 == lua_istable(L, opts_idx))
    {
        return v_default;
    }

    lua_getfield(L, opts_idx, key);
    if (0 != lua_isboolean(L, -1))
    {
        int v = lua_toboolean(L, -1);

        lua_pop(L, 1);
        return v;
    }

    lua_pop(L, 1);
    return v_default;
}

lua_Integer pfs_klua_opts_integer(lua_State *L, int opts_idx, const char *key, lua_Integer v_default)
{
    if (NULL == L || NULL == key || opts_idx <= 0 || 0 == lua_istable(L, opts_idx))
    {
        return v_default;
    }

    lua_getfield(L, opts_idx, key);
    if (0 != lua_isinteger(L, -1) || 0 != lua_isnumber(L, -1))
    {
        lua_Integer v = lua_tointeger(L, -1);

        lua_pop(L, 1);
        return v;
    }

    lua_pop(L, 1);
    return v_default;
}

int pfs_klua_opts_require_force(lua_State *L, int opts_idx)
{
    if (0 == pfs_klua_opts_bool(L, opts_idx, "force", 0))
    {
        return PFS_EINVAL;
    }
    return PFS_OK;
}

bool pfs_klua_str_ieq(const char *a, const char *b)
{
    if (NULL == a || NULL == b)
    {
        return false;
    }

    while ('\0' != *a && '\0' != *b)
    {
        char ca = *a;
        char cb = *b;

        if (ca >= 'A' && ca <= 'Z')
        {
            ca = (char)(ca - 'A' + 'a');
        }
        if (cb >= 'A' && cb <= 'Z')
        {
            cb = (char)(cb - 'A' + 'a');
        }
        if (ca != cb)
        {
            return false;
        }
        a++;
        b++;
    }

    return ('\0' == *a && '\0' == *b) ? true : false;
}

int pfs_klua_parse_fs_type(const char *text, pfs_fs_type_e *p_out)
{
    if (NULL == text || NULL == p_out || '\0' == text[0])
    {
        return -1;
    }
    if (pfs_klua_str_ieq(text, "fat12"))
    {
        *p_out = PFS_FAT12;
        return 0;
    }
    else if (pfs_klua_str_ieq(text, "fat16"))
    {
        *p_out = PFS_FAT16;
        return 0;
    }
    else if (pfs_klua_str_ieq(text, "fat32"))
    {
        *p_out = PFS_FAT32;
        return 0;
    }
    else if (pfs_klua_str_ieq(text, "exfat"))
    {
        *p_out = PFS_EXFAT;
        return 0;
    }
    else if (pfs_klua_str_ieq(text, "ntfs"))
    {
        *p_out = PFS_NTFS;
        return 0;
    }
    else if (pfs_klua_str_ieq(text, "none"))
    {
        *p_out = PFS_NONE;
        return 0;
    }
    else if (pfs_klua_str_ieq(text, "isofs"))
    {
        *p_out = PFS_ISOFS;
        return 0;
    }
    else if (pfs_klua_str_ieq(text, "udf"))
    {
        *p_out = PFS_UDF;
        return 0;
    }
    else if (pfs_klua_str_ieq(text, "ext2"))
    {
        *p_out = PFS_EXT2;
        return 0;
    }
    else if (pfs_klua_str_ieq(text, "ext3"))
    {
        *p_out = PFS_EXT3;
        return 0;
    }
    else if (pfs_klua_str_ieq(text, "ext4"))
    {
        *p_out = PFS_EXT4;
        return 0;
    }
    return -1;
}

int pfs_klua_parse_mtd_fs_type(const char *text, pfs_fs_type_e *p_out)
{
    if (NULL == text || NULL == p_out || '\0' == text[0])
    {
        return -1;
    }
    if (pfs_klua_str_ieq(text, "squashfs"))
    {
        *p_out = (pfs_fs_type_e)PFSMTD_SQUASHFS;
        return 0;
    }
    else if (pfs_klua_str_ieq(text, "ubifs"))
    {
        *p_out = (pfs_fs_type_e)PFSMTD_UBIFS;
        return 0;
    }
    else if (pfs_klua_str_ieq(text, "jffs2"))
    {
        *p_out = (pfs_fs_type_e)PFSMTD_JFFS2;
        return 0;
    }
    else if (pfs_klua_str_ieq(text, "yaffs") || pfs_klua_str_ieq(text, "yaffs2"))
    {
        *p_out = (pfs_fs_type_e)PFSMTD_YAFFS;
        return 0;
    }
    else if (pfs_klua_str_ieq(text, "erofs"))
    {
        *p_out = (pfs_fs_type_e)PFSMTD_EROFS;
        return 0;
    }
    else if (pfs_klua_str_ieq(text, "cramfs"))
    {
        *p_out = (pfs_fs_type_e)PFSMTD_CRAMFS;
        return 0;
    }
    else if (pfs_klua_str_ieq(text, "romfs"))
    {
        *p_out = (pfs_fs_type_e)PFSMTD_ROMFS;
        return 0;
    }
    return -1;
}

int pfs_klua_parse_part_scheme(const char *text, pfs_part_scheme_e *p_out)
{
    if (NULL == text || NULL == p_out || '\0' == text[0])
    {
        return -1;
    }
    if (pfs_klua_str_ieq(text, "mbr"))
    {
        *p_out = PFS_PART_SCHEME_MBR;
        return 0;
    }
    else if (pfs_klua_str_ieq(text, "gpt"))
    {
        *p_out = PFS_PART_SCHEME_GPT;
        return 0;
    }
    return -1;
}

static int pfs_klua_size_suffix_mul(const char *suffix, uint64_t *p_mul)
{
    if (NULL == suffix || NULL == p_mul)
    {
        return -1;
    }
    if (0 == strcmp(suffix, "TiB") || 0 == strcmp(suffix, "T"))
    {
        *p_mul = 1024ull * 1024ull * 1024ull * 1024ull;
        return 0;
    }
    if (0 == strcmp(suffix, "GiB") || 0 == strcmp(suffix, "G"))
    {
        *p_mul = 1024ull * 1024ull * 1024ull;
        return 0;
    }
    if (0 == strcmp(suffix, "MiB") || 0 == strcmp(suffix, "M"))
    {
        *p_mul = 1024ull * 1024ull;
        return 0;
    }
    if (0 == strcmp(suffix, "KiB") || 0 == strcmp(suffix, "K"))
    {
        *p_mul = 1024ull;
        return 0;
    }
    return -1;
}

static int pfs_klua_parse_size_text(const char *text, uint64_t *p_out)
{
    char *end;
    uint64_t val;
    uint64_t mul;
    const char *suffix;

    if (NULL == text || NULL == p_out || '\0' == text[0])
    {
        return -1;
    }

    end = NULL;
    val = strtoull(text, &end, 0);
    if (NULL == end || end == text)
    {
        return -1;
    }

    suffix = end;
    while (' ' == *suffix || '\t' == *suffix)
    {
        suffix++;
    }
    if ('\0' == *suffix)
    {
        *p_out = val;
        return 0;
    }

    if (0 != pfs_klua_size_suffix_mul(suffix, &mul))
    {
        return -1;
    }
    if (val > 0 && mul > (UINT64_MAX / val))
    {
        return -1;
    }
    *p_out = val * mul;
    return 0;
}

int pfs_klua_parse_size_bytes(lua_State *L, int opts_idx, const char *key, uint64_t *p_out, int *p_have)
{
    if (NULL == L || NULL == key || NULL == p_out || NULL == p_have || opts_idx <= 0 || 0 == lua_istable(L, opts_idx))
    {
        return -1;
    }

    *p_have = 0;
    lua_getfield(L, opts_idx, key);
    if (0 != lua_isinteger(L, -1) || 0 != lua_isnumber(L, -1))
    {
        *p_out = (uint64_t)lua_tointeger(L, -1);
        *p_have = 1;
        lua_pop(L, 1);
        return 0;
    }
    if (0 != lua_isstring(L, -1))
    {
        const char *text = lua_tostring(L, -1);

        lua_pop(L, 1);
        if (NULL != text && 0 == pfs_klua_parse_size_text(text, p_out))
        {
            *p_have = 1;
            return 0;
        }
        return -1;
    }

    lua_pop(L, 1);
    return 0;
}

const char *pfs_klua_fs_type_name(pfs_fs_type_e t)
{
    if (PFS_NONE == t)
    {
        return "NONE";
    }
    else if (PFS_FAT12 == t)
    {
        return "FAT12";
    }
    else if (PFS_FAT16 == t)
    {
        return "FAT16";
    }
    else if (PFS_FAT32 == t)
    {
        return "FAT32";
    }
    else if (PFS_EXFAT == t)
    {
        return "exFAT";
    }
    else if (PFS_NTFS == t)
    {
        return "NTFS";
    }
    else if (PFS_ISOFS == t)
    {
        return "ISOFS";
    }
    else if (PFS_UDF == t)
    {
        return "UDF";
    }
    else if (PFS_EXT2 == t)
    {
        return "ext2";
    }
    else if (PFS_EXT3 == t)
    {
        return "ext3";
    }
    else if (PFS_EXT4 == t)
    {
        return "ext4";
    }
    else if (PFSMTD_SQUASHFS == t)
    {
        return "squashfs";
    }
    else if (PFSMTD_UBIFS == t)
    {
        return "ubifs";
    }
    else if (PFSMTD_JFFS2 == t)
    {
        return "jffs2";
    }
    else if (PFSMTD_YAFFS == t)
    {
        return "yaffs";
    }
    else if (PFSMTD_EROFS == t)
    {
        return "erofs";
    }
    else if (PFSMTD_CRAMFS == t)
    {
        return "cramfs";
    }
    else if (PFSMTD_ROMFS == t)
    {
        return "romfs";
    }
    return "?";
}

const char *pfs_klua_part_scheme_str(pfs_part_scheme_e scheme)
{
    if (PFS_PART_SCHEME_NONE == scheme)
    {
        return "none";
    }
    else if (PFS_PART_SCHEME_MBR == scheme)
    {
        return "mbr";
    }
    else if (PFS_PART_SCHEME_GPT == scheme)
    {
        return "gpt";
    }
    return "?";
}

static int pfs_klua_sector_size_normalize(int sector_size)
{
    if (PFS_BLKIO_ALIGN_512 == sector_size || PFS_BLKIO_ALIGN_1K == sector_size ||
            PFS_BLKIO_ALIGN_2K == sector_size || PFS_BLKIO_ALIGN_4K == sector_size)
    {
        return sector_size;
    }
    return -1;
}

int pfs_klua_query_vol_bytes_at_offset(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t *p_vol)
{
    if (NULL == p_blkio || NULL == p_vol || part_offset < 0)
    {
        return -1;
    }

    int sector_size = pfs_blkio_logical_sector_size(p_blkio);
    if (sector_size <= 0)
    {
        sector_size = 512;
    }

    pfs_part_table_t part_tbl;
    int rc = pfs_part_probe(p_blkio, &part_tbl);
    if (PFS_OK == rc && part_tbl.count >= 1)
    {
        for (int i = 0; i < part_tbl.count; i++)
        {
            int64_t start;

            if (0 == part_tbl.entries[i].size_lba)
            {
                continue;
            }
            if (part_tbl.entries[i].start_lba > (uint64_t)INT64_MAX / (uint64_t)sector_size)
            {
                continue;
            }
            start = (int64_t)(part_tbl.entries[i].start_lba * (uint64_t)sector_size);
            if (start == part_offset)
            {
                if (part_tbl.entries[i].size_lba > (uint64_t)UINT64_MAX / (uint64_t)sector_size)
                {
                    return -1;
                }
                *p_vol = part_tbl.entries[i].size_lba * (uint64_t)sector_size;
                return 0;
            }
        }
    }

    int64_t size_bytes = pfs_blkio_get_size(p_blkio);
    if (size_bytes < 0 || size_bytes <= part_offset)
    {
        return -1;
    }
    *p_vol = (uint64_t)(size_bytes - part_offset);
    return 0;
}

int pfs_klua_resolve_format_part_offset(pfs_blkio_t *p_blkio, int64_t cli_offset, int have_cli_offset,
        int64_t *p_out)
{
    if (NULL == p_blkio || NULL == p_out || cli_offset < 0)
    {
        return -1;
    }
    if (0 != have_cli_offset)
    {
        *p_out = cli_offset;
        return 0;
    }

    int sector_size = pfs_blkio_logical_sector_size(p_blkio);

    if (sector_size <= 0)
    {
        sector_size = 512;
    }

    pfs_part_table_t part_tbl;
    int rc = pfs_part_probe(p_blkio, &part_tbl);

    if (PFS_OK == rc && part_tbl.count >= 1 &&
            0 != part_tbl.entries[0].size_lba &&
            part_tbl.entries[0].start_lba <= (uint64_t)INT64_MAX / (uint64_t)sector_size)
    {
        int64_t part_offset = (int64_t)(part_tbl.entries[0].start_lba * (uint64_t)sector_size);

        *p_out = part_offset;
        return 0;
    }

    *p_out = cli_offset;
    return 0;
}

int pfs_klua_resolve_open_sector_size(const char *open_path)
{
    if (NULL == open_path || '\0' == open_path[0])
    {
        return PFS_BLKIO_ALIGN_512;
    }

    int sector_size = pfs_blkio_sector_size(open_path);

    if (pfs_klua_sector_size_normalize(sector_size) < 0)
    {
        return PFS_BLKIO_ALIGN_512;
    }
    return sector_size;
}

int pfs_klua_parse_mount_mode_string(const char *text, int *p_style)
{
    if (NULL == p_style)
    {
        return -1;
    }

    *p_style = PFS_KLUA_MNT_STYLE_R;
    if (NULL == text || '\0' == text[0])
    {
        return 0;
    }

    if (pfs_klua_str_ieq(text, "r"))
    {
        *p_style = PFS_KLUA_MNT_STYLE_R;
        return 0;
    }
    else if (pfs_klua_str_ieq(text, "rw") || pfs_klua_str_ieq(text, "w"))
    {
        *p_style = PFS_KLUA_MNT_STYLE_RW;
        return 0;
    }

    *p_style = PFS_KLUA_MNT_STYLE_R;
    return 0;
}

int pfs_klua_parse_mount_mode_lua(lua_State *L, int idx, int *p_style)
{
    if (NULL == L || NULL == p_style || 0 == lua_isstring(L, idx))
    {
        return -1;
    }

    return pfs_klua_parse_mount_mode_string(lua_tostring(L, idx), p_style);
}

const char *pfs_klua_mount_mode_to_string(int mount_mode)
{
    if (PFS_MNT_RDWR == mount_mode)
    {
        return "rw";
    }

    return "r";
}

bool pfs_klua_mount_name_ok(const char *p_name)
{
    const char *p;

    if (NULL == p_name || '\0' == p_name[0])
    {
        return false;
    }

    for (p = p_name; '\0' != *p; p++)
    {
        char c = *p;

        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
        {
            continue;
        }
        return false;
    }

    return true;
}

void pfs_klua_push_mount_part_item(lua_State *L, int lua_index, const pfs_disk_part_t *p_part)
{
    if (NULL == L)
    {
        return;
    }

    lua_createtable(L, 0, 8);
    lua_pushinteger(L, lua_index);
    lua_setfield(L, -2, "index");
    lua_pushinteger(L, (lua_Integer)(NULL != p_part ? p_part->part_offset : 0));
    lua_setfield(L, -2, "part_offset");
    lua_pushinteger(L, (lua_Integer)(NULL != p_part ? p_part->start_lba : 0));
    lua_setfield(L, -2, "start_lba");
    lua_pushinteger(L, (lua_Integer)(NULL != p_part ? p_part->size_lba : 0));
    lua_setfield(L, -2, "size_lba");

    if (NULL != p_part && 0 != p_part->mounted)
    {
        lua_pushstring(L, pfs_klua_fs_type_name(p_part->fs_type));
    }
    else
    {
        lua_pushstring(L, "");
    }
    lua_setfield(L, -2, "fs_type");
    lua_pushboolean(L, (NULL != p_part && 0 != p_part->mounted) ? 1 : 0);
    lua_setfield(L, -2, "mounted");
    lua_pushinteger(L, (NULL != p_part) ? p_part->mount_err : PFS_OK);
    lua_setfield(L, -2, "mount_err");
    lua_pushstring(L, pfs_klua_mount_mode_to_string((NULL != p_part) ? p_part->mount_mode : 0));
    lua_setfield(L, -2, "mode");
}

void pfs_klua_push_mount_disk(lua_State *L, const char *p_name, int name_len, pfs_disk_t *p_disk)
{
    if (NULL == L || NULL == p_disk)
    {
        return;
    }

    lua_createtable(L, 0, 8);

    if (NULL != p_name && name_len > 0)
    {
        lua_pushlstring(L, p_name, (size_t)name_len);
    }
    else
    {
        lua_pushstring(L, "");
    }
    lua_setfield(L, -2, "name");

    const char *path = p_disk->image_path;
    if (NULL == path)
    {
        path = "";
    }
    pfs_klua_push_blkio_container(L, path, &p_disk->blkio);
    lua_pushstring(L, pfs_klua_part_scheme_str(p_disk->part_scheme));
    lua_setfield(L, -2, "scheme");

    int part_count = pfs_disk_part_count(p_disk);
    lua_pushinteger(L, part_count);
    lua_setfield(L, -2, "part_count");

    lua_createtable(L, part_count, 0);
    for (int i = 1; i <= part_count; i++)
    {
        const pfs_disk_part_t *p_part = pfs_disk_part_info(p_disk, i);

        if (NULL == p_part)
        {
            continue;
        }

        pfs_klua_push_mount_part_item(L, i, p_part);
        lua_rawseti(L, -2, i);
    }
    lua_setfield(L, -2, "parts");
}

int pfs_klua_lua_part_to_c(int lua_part)
{
    if (lua_part < 1)
    {
        return -1;
    }
    if (lua_part > PFS_PART_ENTRY_MAX)
    {
        return -1;
    }
    return lua_part - 1;
}

bool pfs_klua_opts_is_part_array(lua_State *L, int opts_idx)
{
    if (NULL == L || opts_idx <= 0 || 0 == lua_istable(L, opts_idx))
    {
        return false;
    }

    lua_rawgeti(L, opts_idx, 1);
    if (0 == lua_istable(L, -1))
    {
        lua_pop(L, 1);
        return false;
    }
    lua_pop(L, 1);
    return true;
}

int pfs_klua_opts_part_array_count(lua_State *L, int opts_idx)
{
    if (NULL == L || opts_idx <= 0 || 0 == lua_istable(L, opts_idx))
    {
        return 0;
    }

    int count = 0;

    for (int i = 1; i <= PFS_PART_ENTRY_MAX; i++)
    {
        lua_rawgeti(L, opts_idx, i);
        if (0 == lua_istable(L, -1))
        {
            lua_pop(L, 1);
            break;
        }
        lua_pop(L, 1);
        count = i;
    }

    return count;
}

int pfs_klua_resolve_part_offset_by_lua_index(pfs_blkio_t *p_blkio, int lua_part_index, int64_t *p_out)
{
    if (NULL == p_blkio || NULL == p_out || lua_part_index < 1)
    {
        return -1;
    }

    int c_idx = pfs_klua_lua_part_to_c(lua_part_index);

    if (c_idx < 0)
    {
        return -1;
    }

    int sector_size = pfs_blkio_logical_sector_size(p_blkio);

    if (sector_size <= 0)
    {
        sector_size = 512;
    }

    pfs_part_table_t part_tbl;
    int rc = pfs_part_probe(p_blkio, &part_tbl);

    if (PFS_OK != rc || part_tbl.count <= 0 || c_idx >= part_tbl.count)
    {
        return -1;
    }

    if (0 == part_tbl.entries[c_idx].size_lba)
    {
        return -1;
    }
    if (part_tbl.entries[c_idx].start_lba > (uint64_t)INT64_MAX / (uint64_t)sector_size)
    {
        return -1;
    }

    *p_out = (int64_t)(part_tbl.entries[c_idx].start_lba * (uint64_t)sector_size);
    return 0;
}

bool pfs_klua_remount_is_single_item(lua_State *L, int idx)
{
    if (NULL == L || idx <= 0 || 0 == lua_istable(L, idx))
    {
        return false;
    }

    lua_getfield(L, idx, "index");
    if (0 != lua_isnil(L, -1))
    {
        lua_pop(L, 1);
        return false;
    }
    lua_pop(L, 1);
    return true;
}

int pfs_klua_remount_item_count(lua_State *L, int idx)
{
    if (NULL == L || idx <= 0 || 0 == lua_istable(L, idx))
    {
        return 0;
    }

    int count = 0;

    for (int i = 1; i <= PFS_PART_ENTRY_MAX; i++)
    {
        lua_rawgeti(L, idx, i);
        if (0 == lua_istable(L, -1))
        {
            lua_pop(L, 1);
            break;
        }
        lua_pop(L, 1);
        count = i;
    }

    return count;
}

void pfs_klua_mount_opts_init(pfs_disk_mount_opts_t *p_opts)
{
    if (NULL == p_opts)
    {
        return;
    }

    memset(p_opts, 0, sizeof(*p_opts));
    p_opts->default_mnt_style = PFS_KLUA_MNT_STYLE_R;
    for (int i = 0; i < PFS_PART_ENTRY_MAX; i++)
    {
        p_opts->part_mnt_style[i] = PFS_DISK_MNT_STYLE_DFLT;
    }
}

int pfs_klua_mount_opts_from_lua(lua_State *L, int mode_idx, pfs_disk_mount_opts_t *p_opts)
{
    if (NULL == L || NULL == p_opts)
    {
        return -1;
    }

    pfs_klua_mount_opts_init(p_opts);

    if (mode_idx <= 0 || 0 != lua_isnoneornil(L, mode_idx))
    {
        return 0;
    }

    if (0 != lua_isstring(L, mode_idx))
    {
        int style;

        if (0 != pfs_klua_parse_mount_mode_lua(L, mode_idx, &style))
        {
            return -1;
        }
        p_opts->default_mnt_style = style;
        return 0;
    }

    if (0 == lua_istable(L, mode_idx))
    {
        return -1;
    }

    for (int i = 1; i <= PFS_PART_ENTRY_MAX; i++)
    {
        int style;

        lua_rawgeti(L, mode_idx, i);
        if (0 == lua_isstring(L, -1))
        {
            lua_pop(L, 1);
            break;
        }
        if (0 != pfs_klua_parse_mount_mode_lua(L, -1, &style))
        {
            lua_pop(L, 1);
            return -1;
        }
        lua_pop(L, 1);
        p_opts->part_mnt_style[i - 1] = style;
    }

    return 0;
}

int pfs_klua_parse_mount_extra_opts(lua_State *L, int opts_idx, pfs_disk_mount_opts_t *p_opts)
{
    if (NULL == L || NULL == p_opts || opts_idx <= 0 || 0 == lua_istable(L, opts_idx))
    {
        return -1;
    }

    if (lua_getfield(L, opts_idx, "mode") != LUA_TNIL)
    {
        int style;

        if (0 != pfs_klua_parse_mount_mode_lua(L, -1, &style))
        {
            lua_pop(L, 1);
            return -1;
        }
        lua_pop(L, 1);
        p_opts->default_mnt_style = style;
    }
    else
    {
        lua_pop(L, 1);
    }

    if (lua_getfield(L, opts_idx, "offset") != LUA_TNIL)
    {
        if (0 == lua_isinteger(L, -1) && 0 == lua_isnumber(L, -1))
        {
            lua_pop(L, 1);
            return -1;
        }
        lua_Integer v = lua_tointeger(L, -1);

        lua_pop(L, 1);
        if (v < 0)
        {
            return -1;
        }
        p_opts->part_offset = (int64_t)v;
    }
    else
    {
        lua_pop(L, 1);
    }

    if (lua_getfield(L, opts_idx, "sector_size") != LUA_TNIL)
    {
        if (0 == lua_isinteger(L, -1) && 0 == lua_isnumber(L, -1))
        {
            lua_pop(L, 1);
            return -1;
        }
        lua_Integer v = lua_tointeger(L, -1);
        int norm = pfs_klua_sector_size_normalize((int)v);

        lua_pop(L, 1);
        if (norm < 0)
        {
            return -1;
        }
        p_opts->sector_size = norm;
    }
    else
    {
        lua_pop(L, 1);
    }

    return 0;
}

int pfs_klua_parse_mkpt_entry_lua(lua_State *L, int tbl_idx, int sector_size, pfs_mkpt_entry_t *p_entry)
{
    if (NULL == L || tbl_idx <= 0 || NULL == p_entry || 0 == lua_istable(L, tbl_idx))
    {
        return -1;
    }

    if (sector_size <= 0)
    {
        sector_size = 512;
    }

    memset(p_entry, 0, sizeof(*p_entry));
    p_entry->bootable = true;

    const char *fs_str = pfs_klua_opts_string(L, tbl_idx, "fs", NULL);

    if (NULL == fs_str)
    {
        fs_str = pfs_klua_opts_string(L, tbl_idx, "type", "exfat");
    }
    if (0 != pfs_klua_parse_fs_type(fs_str, &p_entry->fs_type))
    {
        return -1;
    }

    p_entry->bootable = (0 != pfs_klua_opts_bool(L, tbl_idx, "bootable", 1)) ? true : false;
    p_entry->mbr_sys_ind = (uint8_t)pfs_klua_opts_integer(L, tbl_idx, "mbr_sys_ind", 0);

    lua_Integer start_lba = pfs_klua_opts_integer(L, tbl_idx, "start_lba", 0);

    if (start_lba < 0)
    {
        return -1;
    }
    p_entry->start_lba = (uint64_t)start_lba;

    lua_Integer size_lba = pfs_klua_opts_integer(L, tbl_idx, "size_lba", 0);

    if (size_lba < 0)
    {
        return -1;
    }
    p_entry->size_lba = (uint64_t)size_lba;

    if (0 == p_entry->size_lba)
    {
        uint64_t size_bytes;
        int have_size = 0;

        if (0 != pfs_klua_parse_size_bytes(L, tbl_idx, "size", &size_bytes, &have_size))
        {
            return -1;
        }
        if (0 != have_size)
        {
            p_entry->size_lba = size_bytes / (uint64_t)sector_size;
            if (0 == p_entry->size_lba)
            {
                p_entry->size_lba = 1;
            }
        }
    }

    return 0;
}

int pfs_klua_resolve_format_offset(lua_State *L, int tbl_idx, int array_pos, pfs_blkio_t *p_blkio, int64_t *p_offset)
 {
    if (NULL == L || tbl_idx <= 0 || NULL == p_blkio || NULL == p_offset)
    {
        return -1;
    }

    int have_offset = 0;
    int64_t offset = (int64_t)pfs_klua_opts_integer(L, tbl_idx, "offset", 0);

    lua_getfield(L, tbl_idx, "offset");
    if (0 == lua_isnil(L, -1))
    {
        have_offset = 1;
    }
    lua_pop(L, 1);

    if (0 != have_offset)
    {
        if (offset < 0)
        {
            return -1;
        }
        *p_offset = offset;
        return 0;
    }

    int lua_idx = (int)pfs_klua_opts_integer(L, tbl_idx, "index", 0);

    if (lua_idx <= 0)
    {
        if (array_pos >= 1)
        {
            lua_idx = array_pos;
        }
        else
        {
            lua_idx = 1;
        }
    }

    int c_idx = pfs_klua_lua_part_to_c(lua_idx);

    if (c_idx < 0)
    {
        return -1;
    }

    if (0 != pfs_klua_resolve_part_offset_by_lua_index(p_blkio, lua_idx, p_offset))
    {
        if (1 == lua_idx &&
                0 == pfs_klua_resolve_format_part_offset(p_blkio, 0, 0, p_offset))
        {
            return 0;
        }
        return -1;
    }

    return 0;
}

int pfs_klua_resolve_format_vol(lua_State *L, int tbl_idx, pfs_blkio_t *p_blkio, int64_t part_offset,
        uint64_t *p_vol, int *p_have_vol)
{
    if (NULL == L || tbl_idx <= 0 || NULL == p_blkio || NULL == p_vol || NULL == p_have_vol)
    {
        return -1;
    }

    *p_have_vol = 0;
    if (0 != pfs_klua_parse_size_bytes(L, tbl_idx, "size", p_vol, p_have_vol))
    {
        return -1;
    }

    if (0 == *p_have_vol)
    {
        if (0 != pfs_klua_query_vol_bytes_at_offset(p_blkio, part_offset, p_vol))
        {
            return -1;
        }
    }

    return 0;
}

// end
