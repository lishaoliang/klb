#include "image/klb_resolution.h"
#include <assert.h>


bool klb_resolution_size(int resolution/*klb_resolution_e*/, int* p_w, int* p_h)
{
    assert(NULL != p_w);
    assert(NULL != p_h);

    switch (resolution)
    {
    // 4:3
    case KLB_RESOLUTION_256x190:
    {
        *p_w = 256;
        *p_h = 190;
        return true;
    }
    case KLB_RESOLUTION_320x240:
    {
        *p_w = 320;
        *p_h = 240;
        return true;
    }
    case KLB_RESOLUTION_480x360:
    {
        *p_w = 480;
        *p_h = 360;
        return true;
    }
    case KLB_RESOLUTION_640x480:
    {
        *p_w = 640;
        *p_h = 480;
        return true;
    }
    case KLB_RESOLUTION_768x576:
    {
        *p_w = 768;
        *p_h = 576;
        return true;
    }
    case KLB_RESOLUTION_1024x768:
    {
        *p_w = 1024;
        *p_h = 768;
        return true;
    }
    case KLB_RESOLUTION_1400x1050:
    {
        *p_w = 1400;
        *p_h = 1050;
        return true;
    }
    case KLB_RESOLUTION_1600x1200:
    {
        *p_w = 1600;
        *p_h = 1200;
        return true;
    }
    case KLB_RESOLUTION_2048x1536:
    {
        *p_w = 2048;
        *p_h = 1536;
        return true;
    }


    // 16:9
    case KLB_RESOLUTION_256x144:
    {
        *p_w = 256;
        *p_h = 144;
        return true;
    }
    case KLB_RESOLUTION_384x216:
    {
        *p_w = 384;
        *p_h = 216;
        return true;
    }
    case KLB_RESOLUTION_480x272:
    {
        *p_w = 480;
        *p_h = 272;
        return true;
    }
    case KLB_RESOLUTION_640x360:
    {
        *p_w = 640;
        *p_h = 360;
        return true;
    }
    case KLB_RESOLUTION_960x540:
    {
        *p_w = 960;
        *p_h = 540;
        return true;
    }
    case KLB_RESOLUTION_1280x720:
    {
        *p_w = 1280;
        *p_h = 720;
        return true;
    }
    case KLB_RESOLUTION_1600x900:
    {
        *p_w = 1600;
        *p_h = 900;
        return true;
    }
    case KLB_RESOLUTION_1920x1080:
    {
        *p_w = 1920;
        *p_h = 1080;
        return true;
    }
    case KLB_RESOLUTION_2560x1440:
    {
        *p_w = 2560;
        *p_h = 1440;
        return true;
    }
    case KLB_RESOLUTION_3200x1800:
    {
        *p_w = 3200;
        *p_h = 1800;
        return true;
    }
    case KLB_RESOLUTION_3840x2160:
    {
        *p_w = 3840;
        *p_h = 2160;
        return true;
    }
    case KLB_RESOLUTION_5120x2880:
    {
        *p_w = 5120;
        *p_h = 2880;
        return true;
    }
    case KLB_RESOLUTION_7680x4320:
    {
        *p_w = 7680;
        *p_h = 4320;
        return true;
    }
    case KLB_RESOLUTION_9600x5400:
    {
        *p_w = 9600;
        *p_h = 5400;
        return true;
    }
    case KLB_RESOLUTION_10240x5760:
    {
        *p_w = 10240;
        *p_h = 5760;
        return true;
    }
    default:
        break;
    }

    return false;
}
