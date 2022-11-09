#include "wsdl_images.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_hlist.h"


typedef struct wsdl_img_item_t_
{
    SDL_Surface* p_surface;
    SDL_Texture* p_texture;
}wsdl_img_item_t;

typedef struct wsdl_images_t_
{
    klb_hlist_t*        p_hlist;
}wsdl_images_t;


wsdl_images_t* wsdl_images_create()
{
    wsdl_images_t* p_imgs = KLB_MALLOCZ(wsdl_images_t, 1, 0);

    p_imgs->p_hlist = klb_hlist_create(0);

    return p_imgs;
}

static void free_wsdl_img_item(wsdl_img_item_t* p_item)
{
    KLB_FREE_BY(p_item->p_texture, SDL_DestroyTexture);
    KLB_FREE_BY(p_item->p_surface, SDL_FreeSurface);
    KLB_FREE(p_item);
}

void wsdl_images_destroy(wsdl_images_t* p_imgs)
{
    while (0 < klb_hlist_size(p_imgs->p_hlist))
    {
        wsdl_img_item_t* p_item = (wsdl_img_item_t*)klb_hlist_pop_head(p_imgs->p_hlist);

        KLB_FREE_BY(p_item, free_wsdl_img_item);
    }

    KLB_FREE_BY(p_imgs->p_hlist, klb_hlist_destroy);
    KLB_FREE(p_imgs);
}

int wsdl_images_load(wsdl_images_t* p_imgs, SDL_Renderer* p_render, const char* p_key, const char* p_path)
{
    int key_len = strlen(p_key);
    SDL_Surface* p_surface = SDL_LoadBMP(p_path);

    if (NULL == p_surface)
    {
        return 1;
    }

    wsdl_img_item_t* p_item = KLB_MALLOCZ(wsdl_img_item_t, 1, 0);
    p_item->p_surface = p_surface;

    if (NULL != p_render)
    {
        p_item->p_texture = SDL_CreateTextureFromSurface(p_render, p_surface);
    }

    if (NULL == klb_hlist_push_tail(p_imgs->p_hlist, p_key, key_len, p_item))
    {
        wsdl_img_item_t* p_old = (wsdl_img_item_t*)klb_hlist_update(p_imgs->p_hlist, p_key, key_len, p_item);
        KLB_FREE_BY(p_old, free_wsdl_img_item);
    }

    return 0;
}

SDL_Texture* wsdl_images_find(wsdl_images_t* p_imgs, SDL_Renderer* p_render, const char* p_path)
{
    wsdl_img_item_t* p_item = (wsdl_img_item_t*)klb_hlist_find(p_imgs->p_hlist, p_path, strlen(p_path));

    if (NULL != p_item)
    {
        if (NULL == p_item->p_texture)
        {
            p_item->p_texture = SDL_CreateTextureFromSurface(p_render, p_item->p_surface);
        }

        return p_item->p_texture;
    }

    return NULL;
}
