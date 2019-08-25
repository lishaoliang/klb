///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/08/24
//
/// @file    mnp_media.go
/// @author  lishaoliang
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief	mnp_txt
///////////////////////////////////////////////////////////////////////////
package kmnp

// MnpMH klb_mnp_mh_t
type MnpMH struct {
	Extra uint16

	Vtype uint16

	Chnn  uint16
	Sidx  uint16
	Dtype uint16
	Time  int64
}
