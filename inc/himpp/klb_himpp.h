#ifndef __KLB_HIMPP_H__
#define __KLB_HIMPP_H__

#if defined(__cplusplus)
extern "C" {
#endif

// Hi35XX
// http://www.hisilicon.com/en/Products/ProductList/Camera


#if defined(_WIN32)
#elif defined(__HI3516AV100__)
#include "himpp/klb_hi3516av100.h"
#elif defined(__HI3519V101__)
#include "himpp/klb_hi3519v101.h"
#elif defined(__HI3520DV200__)
#include "himpp/klb_hi3520dv200.h"
#elif defined(__HI3520DV300__)
#include "himpp/klb_hi3520dv300.h"
#elif defined(__HI3520DV400__)
#include "himpp/klb_hi3520dv400.h"
#elif defined(__HI3531DV100__)
#include "himpp/klb_hi3531dv100.h"
#elif defined(__HI3559AV100__)
#include "himpp/klb_hi3559av100.h"
#endif


#if defined(__cplusplus)
}
#endif

#endif // __KLB_HIMPP_H__
//end
