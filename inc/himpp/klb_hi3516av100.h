#ifndef __KLB_HI3516AV100_H__
#define __KLB_HI3516AV100_H__

// http://www.hisilicon.com/en/Products/ProductList/Camera

#if defined(__cplusplus)
extern "C" {
#endif

// 参考: hi3516*/mpp/sample/common/sample_comm.h
#include "hi_common.h"
#include "hi_comm_sys.h"
#include "hi_comm_vb.h"
#include "hi_comm_isp.h"
#include "hi_comm_vi.h"
#include "hi_comm_vo.h"
#include "hi_comm_venc.h"
#include "hi_comm_vpss.h"
#include "hi_comm_vdec.h"
#include "hi_comm_vda.h"
#include "hi_comm_region.h"
#include "hi_comm_adec.h"
#include "hi_comm_aenc.h"
#include "hi_comm_ai.h"
#include "hi_comm_ao.h"
#include "hi_comm_aio.h"
#include "hi_defines.h"

#include "mpi_sys.h"
#include "mpi_vb.h"
#include "mpi_vi.h"
#include "mpi_vo.h"
#include "mpi_venc.h"
#include "mpi_vpss.h"
#include "mpi_vdec.h"
#include "mpi_vda.h"
#include "mpi_region.h"
#include "mpi_adec.h"
#include "mpi_aenc.h"
#include "mpi_ai.h"
#include "mpi_ao.h"
#include "mpi_isp.h"
#include "mpi_ae.h"
#include "mpi_awb.h"
#include "mpi_af.h"

#include <sys/ioctl.h>
#include <sys/prctl.h>

#include "hi_vreg.h"
#include "hi_sns_ctrl.h"
#include "hi_mipi.h"
#include "hi_sns_ctrl.h"


// 对比 v3 缺少的定义
typedef void    ISP_SNS_OBJ_S;


#ifndef ALG_LIB_NAME_SIZE_MAX
    #define ALG_LIB_NAME_SIZE_MAX   20
#endif


#if defined(__cplusplus)
}
#endif

#endif // __KLB_HI3516AV100_H__
//end
