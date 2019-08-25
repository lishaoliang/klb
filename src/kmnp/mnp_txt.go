///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/08/24
//
/// @file    mnp.go
/// @author  lishaoliang
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief	mnp_txt
///////////////////////////////////////////////////////////////////////////
package kmnp

// MnpTxtH klb_mnp_txth_t
type MnpTxtH struct {
	Extra uint16
	Resv1 uint16

	Sequence uint32
	UID      uint32
}

// from klb_mnp_txt.h
const (
	MNPTxtHsize = 12 // sizeof(klb_mnp_txth_t)
)

// Pack to bin
// MNPTxtHsize byte
func (mtxt *MnpTxtH) Pack(b []byte) []byte {
	if nil == b {
		b = make([]byte, MNPTxtHsize)
	}

	// LE
	b[0] = uint8(mtxt.Extra)
	b[1] = uint8(mtxt.Extra >> 8)

	b[2] = uint8(mtxt.Resv1)
	b[3] = uint8(mtxt.Resv1 >> 8)

	b[4] = uint8(mtxt.Sequence)
	b[5] = uint8(mtxt.Sequence >> 8)
	b[6] = uint8(mtxt.Sequence >> 16)
	b[7] = uint8(mtxt.Sequence >> 24)

	b[8] = uint8(mtxt.UID)
	b[9] = uint8(mtxt.UID >> 8)
	b[10] = uint8(mtxt.UID >> 16)
	b[11] = uint8(mtxt.UID >> 24)

	return b
}

// Unpack from []byte
func (mtxt *MnpTxtH) Unpack(b []byte) {
	// LE
	mtxt.Extra = uint16(b[0]) | (uint16(b[1]) << 8)
	mtxt.Resv1 = uint16(b[2]) | (uint16(b[3]) << 8)

	mtxt.Sequence = uint32(b[4]) | (uint32(b[5]) << 8) | (uint32(b[6]) << 16) | (uint32(b[7]) << 24)
	mtxt.UID = uint32(b[8]) | (uint32(b[9]) << 8) | (uint32(b[10]) << 16) | (uint32(b[11]) << 24)
}

// IsOk is ok
func (mtxt *MnpTxtH) IsOk() bool {
	return true
}
