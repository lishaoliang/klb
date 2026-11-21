// Doc Encode : UTF-8 BOM, Unix(LF)
// VHDX GUID 常量与比较
#include "blkio_vhdx/pfs_vhdx_internal.h"
#include "pfs_endian.h"

#include <string.h>
#include <time.h>

void pfs_vhdx_guid_from_raw(const uint8_t *p, pfs_vhdx_guid_t *out)
{
    out->data1 = pfs_read_le32(p);
    out->data2 = pfs_read_le16(p + 4);
    out->data3 = pfs_read_le16(p + 6);
    memcpy(out->data4, p + 8, 8);
}

void pfs_vhdx_guid_to_raw(uint8_t *p, const pfs_vhdx_guid_t *g)
{
    pfs_write_le32(p, g->data1);
    pfs_write_le16(p + 4, g->data2);
    pfs_write_le16(p + 6, g->data3);
    memcpy(p + 8, g->data4, 8);
}

int pfs_vhdx_guid_eq(const pfs_vhdx_guid_t *a, const pfs_vhdx_guid_t *b)
{
    if (NULL == a || NULL == b)
    {
        return 0;
    }
    return (a->data1 == b->data1 && a->data2 == b->data2 && a->data3 == b->data3 &&
            0 == memcmp(a->data4, b->data4, 8)) ? 1 : 0;
}

int pfs_vhdx_guid_is_zero(const pfs_vhdx_guid_t *g)
{
    static const pfs_vhdx_guid_t z = {0, 0, 0, {0}};
    return pfs_vhdx_guid_eq(g, &z);
}

void pfs_vhdx_guid_make(pfs_vhdx_guid_t *g)
{
    uint32_t t = (uint32_t)time(NULL);

    memset(g, 0, sizeof(*g));
    g->data1 = t ^ 0xA5A5A5A5u;
    g->data2 = 0x4F50u;
    g->data3 = 0x41F1u;
    g->data4[0] = 0x8Au;
    g->data4[1] = 0xB1u;
    g->data4[2] = (uint8_t)(t >> 24);
    g->data4[3] = (uint8_t)(t >> 16);
    g->data4[4] = (uint8_t)(t >> 8);
    g->data4[5] = (uint8_t)t;
    g->data4[6] = 0x70u;
    g->data4[7] = 0x73u;
}

// BAT 2DC27766-F623-4200-9D64-115E9BFD4A08
static const pfs_vhdx_guid_t g_guid_bat = {
    0x2DC27766u, 0xF623u, 0x4200u,
    {0x9D, 0x64, 0x11, 0x5E, 0x9B, 0xFD, 0x4A, 0x08}
};

// Metadata 8B7CA206-4790-4B9A-B8FE-575F050F886E
static const pfs_vhdx_guid_t g_guid_meta = {
    0x8B7CA206u, 0x4790u, 0x4B9Au,
    {0xB8, 0xFE, 0x57, 0x5F, 0x05, 0x0F, 0x88, 0x6E}
};

// FileParameters caa16737-fa36-4d43-b3b6-33f0aa44e76b
static const pfs_vhdx_guid_t g_guid_params = {
    0xCAA16737u, 0xFA36u, 0x4D43u,
    {0xB3, 0xB6, 0x33, 0xF0, 0xAA, 0x44, 0xE7, 0x6B}
};

// VirtualDiskSize 2fa54224-cd1b-4876-b211-5dbed83bf4b8
static const pfs_vhdx_guid_t g_guid_vsize = {
    0x2FA54224u, 0xCD1Bu, 0x4876u,
    {0xB2, 0x11, 0x5D, 0xBE, 0xD8, 0x3B, 0xF4, 0xB8}
};

// LogicalSectorSize 8141bf1d-a96f-4709-ba47-f233a8faab5f
static const pfs_vhdx_guid_t g_guid_lss = {
    0x8141BF1Du, 0xA96Fu, 0x4709u,
    {0xBA, 0x47, 0xF2, 0x33, 0xA8, 0xFA, 0xAB, 0x5F}
};

// PhysicalSectorSize cda348c7-445d-4471-9cc9-e9885251c556
static const pfs_vhdx_guid_t g_guid_pss = {
    0xCDA348C7u, 0x445Du, 0x4471u,
    {0x9C, 0xC9, 0xE9, 0x88, 0x52, 0x51, 0xC5, 0x56}
};

// ParentLocator a8d35f2d-b30b-454d-abf7-d3d84834ab0c
static const pfs_vhdx_guid_t g_guid_parent = {
    0xA8D35F2Du, 0xB30Bu, 0x454Du,
    {0xAB, 0xF7, 0xD3, 0xD8, 0x48, 0x34, 0xAB, 0x0C}
};

const pfs_vhdx_guid_t *pfs_vhdx_guid_bat(void)
{
    return &g_guid_bat;
}

const pfs_vhdx_guid_t *pfs_vhdx_guid_metadata(void)
{
    return &g_guid_meta;
}

const pfs_vhdx_guid_t *pfs_vhdx_guid_file_params(void)
{
    return &g_guid_params;
}

const pfs_vhdx_guid_t *pfs_vhdx_guid_virtual_size(void)
{
    return &g_guid_vsize;
}

const pfs_vhdx_guid_t *pfs_vhdx_guid_logical_sector(void)
{
    return &g_guid_lss;
}

const pfs_vhdx_guid_t *pfs_vhdx_guid_physical_sector(void)
{
    return &g_guid_pss;
}

const pfs_vhdx_guid_t *pfs_vhdx_guid_parent_locator(void)
{
    return &g_guid_parent;
}

// end
