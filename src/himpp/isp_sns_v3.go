///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   isp_sns_v3.go
/// @author lishaoliang
/// @brief	isp sns v3
///  usage: go build -tags="himppv3"
///////////////////////////////////////////////////////////////////////////

// +build himppv3

package himpp

/*
#include "himpp/klb_himpp.h"

HI_S32 klua_go_himpp_pfnRegisterCallback(ISP_SNS_OBJ_S* p_sns_obj, ISP_DEV IspDev, ALG_LIB_S *pstAeLib, ALG_LIB_S *pstAwbLib)
{
	return p_sns_obj->pfnRegisterCallback(IspDev, pstAeLib, pstAwbLib);
}

HI_S32 klua_go_himpp_pfnUnRegisterCallback(ISP_SNS_OBJ_S* p_sns_obj, ISP_DEV IspDev, ALG_LIB_S *pstAeLib, ALG_LIB_S *pstAwbLib)
{
	return p_sns_obj->pfnUnRegisterCallback(IspDev, pstAeLib, pstAwbLib);
}

*/
import "C"
import "fmt"

// IspSnsObj C.ISP_SNS_OBJ_S
type IspSnsObj C.ISP_SNS_OBJ_S

// IspSns isp sensor
type IspSns struct {
	SensorID int
	snsObj   IspSnsObj

	ae  AlgLib
	af  AlgLib
	awb AlgLib
}

// IspSnsCreate create isp sensor
func IspSnsCreate(sensorID int, obj *IspSnsObj) *IspSns {
	var m IspSns

	m.SensorID = sensorID
	m.snsObj = *obj

	m.ae.ID = sensorID
	m.ae.Name = C.HI_AE_LIB_NAME

	m.af.ID = sensorID
	m.af.Name = C.HI_AF_LIB_NAME

	m.awb.ID = sensorID
	m.awb.Name = C.HI_AWB_LIB_NAME

	return &m
}

///////////////////////////////////////////////////////////////////////////////

// Destroy destroy
func (m *IspSns) Destroy() {

}

// Register register
func (m *IspSns) Register() int {

	var ae, af, awb ALGLIBS
	AlgLibCopyC(&ae, &m.ae)
	AlgLibCopyC(&af, &m.af)
	AlgLibCopyC(&awb, &m.awb)

	ret1 := C.klua_go_himpp_pfnRegisterCallback((*C.ISP_SNS_OBJ_S)(&m.snsObj), (C.ISP_DEV)(m.SensorID), (*C.ALG_LIB_S)(&ae), (*C.ALG_LIB_S)(&awb))
	ret2 := C.HI_MPI_AE_Register((C.ISP_DEV)(m.SensorID), (*C.ALG_LIB_S)(&ae))
	ret3 := C.HI_MPI_AWB_Register((C.ISP_DEV)(m.SensorID), (*C.ALG_LIB_S)(&awb))
	ret4 := C.HI_MPI_AF_Register((C.ISP_DEV)(m.SensorID), (*C.ALG_LIB_S)(&af))

	if 0 != ret1 || 0 != ret2 || 0 != ret3 || 0 != ret4 {
		fmt.Printf("himpp.is_sns.Register error!rets=[0x%X,0x%X,0x%X,0x%X],isp sensor id=%d\n", uint(ret1), uint(ret2), uint(ret3), uint(ret4), m.SensorID)
		return -1
	}

	fmt.Printf("himpp.is_sns.Register ok!isp sensor id=%d\n", m.SensorID)
	return 0
}

// UnRegister UnRegister
func (m *IspSns) UnRegister() int {

	var ae, af, awb ALGLIBS
	AlgLibCopyC(&ae, &m.ae)
	AlgLibCopyC(&af, &m.af)
	AlgLibCopyC(&awb, &m.awb)

	ret4 := C.HI_MPI_AF_UnRegister((C.ISP_DEV)(m.SensorID), (*C.ALG_LIB_S)(&af))
	ret3 := C.HI_MPI_AWB_UnRegister((C.ISP_DEV)(m.SensorID), (*C.ALG_LIB_S)(&awb))
	ret2 := C.HI_MPI_AE_UnRegister((C.ISP_DEV)(m.SensorID), (*C.ALG_LIB_S)(&ae))
	ret1 := C.klua_go_himpp_pfnUnRegisterCallback((*C.ISP_SNS_OBJ_S)(&m.snsObj), (C.ISP_DEV)(m.SensorID), (*C.ALG_LIB_S)(&ae), (*C.ALG_LIB_S)(&awb))

	if 0 != ret1 || 0 != ret2 || 0 != ret3 || 0 != ret4 {
		fmt.Printf("himpp.is_sns.Register error!rets=[0x%X,0x%X,0x%X,0x%X],isp sensor id=%d\n", uint(ret1), uint(ret2), uint(ret3), uint(ret4), m.SensorID)
		return -1
	}

	fmt.Printf("himpp.is_sns.UnRegister ok!isp sensor id=%d\n", m.SensorID)
	return 0
}
