///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/08/24
//
/// @file    mnp_bin.go
/// @author  lishaoliang
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief	mnp_media
///////////////////////////////////////////////////////////////////////////
package kmnp

// MnpBinH klb_mnp_binh_t
type MnpBinH struct {
	Extra uint16
	Resv1 uint16

	Sequence uint32
	UID      uint32
}
