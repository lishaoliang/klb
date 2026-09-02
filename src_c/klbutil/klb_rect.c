// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbutil/klb_rect.h"
#include <assert.h>


bool klb_pt_in_rect(const klb_rect_t* p_rect, int x, int y)
{
    assert(NULL != p_rect);

    if ((p_rect->x <= x) && (x < p_rect->x + p_rect->w) &&
        (p_rect->y <= y) && (y < p_rect->y + p_rect->h))
    {
        return true;
    }

    return false;
}

void klb_rect_init(klb_rect_t* p_dst, int x, int y, int w, int h)
{
    assert(NULL != p_dst);

    p_dst->x = x;
    p_dst->y = y;
    p_dst->w = w;
    p_dst->h = h;
}

void klb_rect_union(klb_rect_t* p_dst, const klb_rect_t* p_rect1, const klb_rect_t* p_rect2)
{
    assert(NULL != p_dst);
    assert(NULL != p_rect1);
    assert(NULL != p_rect2);

    int l = (p_rect1->x <= p_rect2->x) ? p_rect1->x : p_rect2->x;
    int t = (p_rect1->y <= p_rect2->y) ? p_rect1->y : p_rect2->y;

    int r1 = p_rect1->x + p_rect1->w;
    int r2 = p_rect2->x + p_rect2->w;

    int b1 = p_rect1->y + p_rect1->h;
    int b2 = p_rect2->y + p_rect2->h;

    int r = (r1 <= r2) ? r2 : r1;
    int b = (b1 <= b2) ? b2 : b1;

    p_dst->x = l;
    p_dst->y = t;
    p_dst->w = r - l;
    p_dst->h = b - t;
}

bool klb_rect_intersect(klb_rect_t* p_dst, const klb_rect_t* p_rect1, const klb_rect_t* p_rect2)
{
    assert(NULL != p_rect1);
    assert(NULL != p_rect2);

    int l = (p_rect1->x <= p_rect2->x) ? p_rect2->x : p_rect1->x;
    int t = (p_rect1->y <= p_rect2->y) ? p_rect2->y : p_rect1->y;

    int r1 = p_rect1->x + p_rect1->w;
    int r2 = p_rect2->x + p_rect2->w;

    int b1 = p_rect1->y + p_rect1->h;
    int b2 = p_rect2->y + p_rect2->h;

    int r = (r1 <= r2) ? r1 : r2;
    int b = (b1 <= b2) ? b1 : b2;

    if ((l < r) && (t < b))
    {
        if (NULL != p_dst)
        {
            p_dst->x = l;
            p_dst->y = t;
            p_dst->w = r - l;
            p_dst->h = b - t;
        }

        return true; // 有
    }

    return false;   // 无
}

// end
