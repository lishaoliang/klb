///////////////////////////////////////////////////////////////////////////
//	Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file	mnp_bin.go
/// @author	lishaoliang
/// @brief	mnp_media
///////////////////////////////////////////////////////////////////////////

package kmnp

// MnpBinH klb_mnp_binh_t
type MnpBinH struct {
	Extra uint16
	Resv  uint16

	Sequence uint32
	UID      uint32
}
