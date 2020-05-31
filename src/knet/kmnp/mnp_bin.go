///////////////////////////////////////////////////////////////////////////
//	Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file	mnp_bin.go
/// @author	lishaoliang
/// @brief	mnp_media
///////////////////////////////////////////////////////////////////////////

package kmnp

// MnpBin klb_mnp_bin_t
type MnpBin struct {
	Size     uint32 // 完整数据长度(data size, 包含本结构体)
	Extra    uint32 // 附加数据长度; 正式数据长度 = size - extra - sizeof(klb_mnp_bin_t)
	Sequence uint32
	UID      uint32
}

// MNP bin
// 二进制常量定义
const (
	MnpBinSize = 16 // 文本包头大小,sizeof(klb_mnp_bin_t)
)

// Pack to bin
// 打文本包头,MnpBinSize byte
func (m *MnpBin) Pack(b []byte) []byte {
	if nil == b {
		b = make([]byte, MnpBinSize)
	}

	// LE
	b[0] = uint8(m.Size)
	b[1] = uint8(m.Size >> 8)
	b[2] = uint8(m.Size >> 16)
	b[3] = uint8(m.Size >> 24)

	b[4] = uint8(m.Extra)
	b[5] = uint8(m.Extra >> 8)
	b[6] = uint8(m.Extra >> 16)
	b[7] = uint8(m.Extra >> 24)

	b[8] = uint8(m.Sequence)
	b[9] = uint8(m.Sequence >> 8)
	b[10] = uint8(m.Sequence >> 16)
	b[11] = uint8(m.Sequence >> 24)

	b[12] = uint8(m.UID)
	b[13] = uint8(m.UID >> 8)
	b[14] = uint8(m.UID >> 16)
	b[15] = uint8(m.UID >> 24)

	return b
}

// Unpack from []byte
// 解包文本包头,MnpBinSize byte
func (m *MnpBin) Unpack(b []byte) {
	// LE
	m.Size = uint32(b[0]) | (uint32(b[1]) << 8) | (uint32(b[2]) << 16) | (uint32(b[3]) << 24)
	m.Extra = uint32(b[4]) | (uint32(b[5]) << 8) | (uint32(b[6]) << 16) | (uint32(b[7]) << 24)
	m.Sequence = uint32(b[8]) | (uint32(b[9]) << 8) | (uint32(b[10]) << 16) | (uint32(b[11]) << 24)
	m.UID = uint32(b[12]) | (uint32(b[13]) << 8) | (uint32(b[14]) << 16) | (uint32(b[15]) << 24)
}

// IsOk is ok
// 封包格式是否正确
func (m *MnpBin) IsOk() bool {
	return true
}

// Default default
// 默认封包参数
func (m *MnpBin) Default() {
	m.Size = 0
	m.Extra = 0
	m.Sequence = 0
	m.UID = 0
}
