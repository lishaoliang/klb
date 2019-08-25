///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/08/18
//
/// @file    mnp.go
/// @author  lishaoliang
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief	mnp
///////////////////////////////////////////////////////////////////////////
package kmnp

// copy from */mnp/klb_mnp.h

// MnpT klb_mnp_t
type MnpT struct {
	Magic     uint16
	MagicUser uint16

	Size     uint16
	Subtype  uint8
	Opt      uint8
	Packtype uint8
	Resv1    uint8

	Resv2 uint32
}

// from klb_mnp.h
const (
	// magic
	MNPmagic     = uint16(0xDCE6) // KLB_MNP_MAGIC
	MNPmagicUser = uint16(0x6ECD) // KLB_MNP_MAGIC_USER

	// klb_mnp_opt_e
	MNPbegin    = uint8(0x0) // KLB_MNP_BEGIN
	MNPcontinue = uint8(0x1) // KLB_MNP_CONTINUE
	MNPend      = uint8(0x2) // KLB_MNP_END
	MNPfull     = uint8(0x3) // KLB_MNP_FULL
	MNPoptMax   = uint8(0x3) // KLB_MNP_OPT_MAX

	// klb_mnp_packtype_e
	MNPheart       = uint8(0x0) // KLB_MNP_HEART
	MNPtxt         = uint8(0x1) // KLB_MNP_TXT
	MNPbin         = uint8(0x2) // KLB_MNP_BIN
	MNPmedia       = uint8(0x3) // KLB_MNP_MEDIA
	MNPpacktypeMax = uint8(0x7) // KLB_MNP_PACKTYPE_MAX

	// klb_mnp_subtype_e
	MNPsubMedia   = uint8(0x00) // KLB_MNP_SUB_MEDIA
	MNPsubtypeMax = uint8(0xFF) // KLB_MNP_SUBTYPE_MAX

	MNPTsize = 12 // sizeof(klb_mnp_t)
)

// Pack to bin
// MNPTsize byte
func (m *MnpT) Pack(b []byte) []byte {
	if nil == b {
		b = make([]byte, MNPTsize)
	}

	// LE
	b[0] = uint8(m.Magic)
	b[1] = uint8(m.Magic >> 8)

	b[2] = uint8(m.MagicUser)
	b[3] = uint8(m.MagicUser >> 8)

	b[4] = uint8(m.Size)
	b[5] = uint8(m.Size >> 8)

	b[6] = m.Subtype

	optPtype := (m.Resv1 << 5) | (m.Packtype << 2) | m.Opt
	b[7] = optPtype

	b[8] = uint8(m.Resv2)
	b[9] = uint8(m.Resv2 >> 8)
	b[10] = uint8(m.Resv2 >> 16)
	b[11] = uint8(m.Resv2 >> 24)

	return b
}

// Unpack from []byte
func (m *MnpT) Unpack(b []byte) {
	// LE
	m.Magic = uint16(b[0]) | (uint16(b[1]) << 8)
	m.MagicUser = uint16(b[2]) | (uint16(b[3]) << 8)

	m.Size = uint16(b[4]) | (uint16(b[5]) << 8)
	m.Subtype = b[6]

	optPtype := b[7]
	m.Opt = optPtype & 0x3
	m.Packtype = (optPtype >> 2) & 0x7
	m.Resv1 = (optPtype >> 5) & 0x7

	m.Resv2 = uint32(b[8]) | (uint32(b[9]) << 8) | (uint32(b[10]) << 16) | (uint32(b[11]) << 24)
}

// IsOk is ok
func (m *MnpT) IsOk() bool {

	if MNPmagic != m.Magic {
		return false
	}

	if MNPmagicUser != m.MagicUser {
		return false
	}

	if MNPsubMedia != m.Subtype {
		return false
	}

	if MNPoptMax < m.Opt {
		return false
	}

	if MNPheart != m.Packtype &&
		MNPtxt != m.Packtype &&
		MNPbin != m.Packtype &&
		MNPmedia != m.Packtype {
		return false
	}

	return true
}

// Default default
func (m *MnpT) Default() {
	m.Magic = MNPmagic
	m.MagicUser = MNPmagicUser

	m.Size = MNPTsize
	m.Opt = MNPfull
}
