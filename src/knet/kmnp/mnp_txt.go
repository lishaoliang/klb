///////////////////////////////////////////////////////////////////////////
//	Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file	mnp.go
/// @author	lishaoliang
/// @brief	mnp_txt
///////////////////////////////////////////////////////////////////////////

package kmnp

// from */mnp/klb_mnp_txt.h

// MnpTxtH klb_mnp_txth_t
// 文本包头
type MnpTxtH struct {
	Extra uint16 // 附加数据长度
	Resv  uint16 // 0

	Sequence uint32 // 序列号
	UID      uint32 // 用户ID
}

// MNP text
// 文本部分常量定义
const (
	MNPTxtHsize = 12 // 文本包头大小,sizeof(klb_mnp_txth_t)
)

// Pack to bin
// 打文本包头,MNPTxtHsize byte
func (m *MnpTxtH) Pack(b []byte) []byte {
	if nil == b {
		b = make([]byte, MNPTxtHsize)
	}

	// LE
	b[0] = uint8(m.Extra)
	b[1] = uint8(m.Extra >> 8)

	b[2] = uint8(m.Resv)
	b[3] = uint8(m.Resv >> 8)

	b[4] = uint8(m.Sequence)
	b[5] = uint8(m.Sequence >> 8)
	b[6] = uint8(m.Sequence >> 16)
	b[7] = uint8(m.Sequence >> 24)

	b[8] = uint8(m.UID)
	b[9] = uint8(m.UID >> 8)
	b[10] = uint8(m.UID >> 16)
	b[11] = uint8(m.UID >> 24)

	return b
}

// Unpack from []byte
// 解包文本包头,MNPTxtHsize byte
func (m *MnpTxtH) Unpack(b []byte) {
	// LE
	m.Extra = uint16(b[0]) | (uint16(b[1]) << 8)
	m.Resv = uint16(b[2]) | (uint16(b[3]) << 8)

	m.Sequence = uint32(b[4]) | (uint32(b[5]) << 8) | (uint32(b[6]) << 16) | (uint32(b[7]) << 24)
	m.UID = uint32(b[8]) | (uint32(b[9]) << 8) | (uint32(b[10]) << 16) | (uint32(b[11]) << 24)
}

// IsOk is ok
// 封包格式是否正确
func (m *MnpTxtH) IsOk() bool {
	return true
}
