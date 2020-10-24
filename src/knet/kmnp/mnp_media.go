///////////////////////////////////////////////////////////////////////////
//	Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file	mnp_media.go
/// @author	lishaoliang
/// @brief	mnp_txt
///////////////////////////////////////////////////////////////////////////

package kmnp

// MnpMedia klb_mnp_md_t
// 媒体包头
type MnpMedia struct {
	Size  uint32 // 完整数据长度(data size, 包含本结构体)
	Dtype uint32 // 数据类型(data type): klb_mnp_dtype_e
	Chnn  uint32 // 通道
	Sidx  uint32 // 流序号(stream index): klb_mnp_sidx_e

	Time int64 // 时间戳(基于1970年基准,微妙)(范围约:[-292471年, 292471年]

	Resv               uint64 // 0
	Vtype              uint8  // 视频(video type): klb_mnp_vtype_e
	Tracks             uint8  // 音频通道数:1 单声道, 2 立体声
	BitsPerCodedSample uint8  // 采样位宽: 1, 2, 4
	Samples            uint32 // 采样率: 44100
}

// MnpMedia
// 数据类型常量定义
const (
	MnpMediaSize = 32 // klb_mnp_md_t
)

// MnpMedia.Vtype klb_mnp_vtype_e
// 视频(video type)
const (
	MnpVtypeP   = 0x00 // KLB_MNP_VTYPE_P, P帧
	MnpVtypeI   = 0x01 // KLB_MNP_VTYPE_I, I帧
	MnpVtypeB   = 0x02 // KLB_MNP_VTYPE_B, B帧
	MnpVtypeMax = 0xFF // KLB_MNP_VTYPE_MAX, MAX
)

// MnpMedia.Sidx klb_mnp_sidx_e
// 媒体流序号
const (
	MnpSidxNil = 0x0000 // KLB_MNP_SIDX_NULL

	MnpSidxV1 = 0x0001 // KLB_MNP_SIDX_V1, Video 1
	MnpSidxV2 = 0x0002 // KLB_MNP_SIDX_V2, Video 2
	MnpSidxV3 = 0x0003 // KLB_MNP_SIDX_V3, Video 3

	MnpSidxA1 = 0x0021 // KLB_MNP_SIDX_A1, Audio 1
	MnpSidxA2 = 0x0022 // KLB_MNP_SIDX_A2, Audio 2
	MnpSidxA3 = 0x0023 // KLB_MNP_SIDX_A3, Audio 3

	MnpSidxP1 = 0x0041 // KLB_MNP_SIDX_P1, Picture 1
	MnpSidxP2 = 0x0042 // KLB_MNP_SIDX_P2, Picture 2
	MnpSidxP3 = 0x0043 // KLB_MNP_SIDX_P3, Picture 3

	MnpSidxI1 = 0x0061 // KLB_MNP_SIDX_I1, Image 1
	MnpSidxI2 = 0x0062 // KLB_MNP_SIDX_I2, Image 2
	MnpSidxI3 = 0x0063 // KLB_MNP_SIDX_I3, Image 3
)

// MnpMedia.Dtype klb_mnp_dtype_e
// 数据类型
const (
	MnpDtypeNil = 0x0000

	MnpDtypeH264 = 0x0001
	MnpDtypeH265 = 0x0002

	MnpDtypeAAC = 0x1001

	MnpDtypeJpeg = 0x2001

	MnpDtypeMax = 0x7FFFFFFF
)

///////////////////////////////////////////////////////////////////////////////

// Pack to []byte
// 打包媒体包头,大小MNPmdSize
func (m *MnpMedia) Pack(b []byte) []byte {
	if nil == b {
		b = make([]byte, MnpMediaSize)
	}

	// LE
	b[0] = uint8(m.Size)
	b[1] = uint8(m.Size >> 8)
	b[2] = uint8(m.Size >> 16)
	b[3] = uint8(m.Size >> 24)

	b[4] = uint8(m.Dtype)
	b[5] = uint8(m.Dtype >> 8)
	b[6] = uint8(m.Dtype >> 16)
	b[7] = uint8(m.Dtype >> 24)

	b[8] = uint8(m.Chnn)
	b[9] = uint8(m.Chnn >> 8)
	b[10] = uint8(m.Chnn >> 16)
	b[11] = uint8(m.Chnn >> 24)

	b[12] = uint8(m.Sidx)
	b[13] = uint8(m.Sidx >> 8)
	b[14] = uint8(m.Sidx >> 16)
	b[15] = uint8(m.Sidx >> 24)

	b[16] = uint8(m.Time)
	b[17] = uint8(m.Time >> 8)
	b[18] = uint8(m.Time >> 16)
	b[19] = uint8(m.Time >> 24)
	b[20] = uint8(m.Time >> 32)
	b[21] = uint8(m.Time >> 40)
	b[22] = uint8(m.Time >> 48)
	b[23] = uint8(m.Time >> 56)

	// union resv
	b[24] = m.Vtype
	b[25] = 0
	b[26] = 0
	b[27] = 0
	b[28] = 0
	b[29] = 0
	b[30] = 0
	b[31] = 0

	//fmt.Println("kmnp.mnp_media.Pack", b)
	return b
}

// Unpack from []byte
// 解包媒体包头,大小MNPmdSize
func (m *MnpMedia) Unpack(b []byte) {
	// LE
	m.Size = uint32(b[0]) | (uint32(b[1]) << 8) | (uint32(b[2]) << 16) | (uint32(b[3]) << 24)

	m.Dtype = uint32(b[4]) | (uint32(b[5]) << 8) | (uint32(b[6]) << 16) | (uint32(b[7]) << 24)

	m.Chnn = uint32(b[8]) | (uint32(b[9]) << 8) | (uint32(b[10]) << 16) | (uint32(b[11]) << 24)
	m.Sidx = uint32(b[12]) | (uint32(b[13]) << 8) | (uint32(b[14]) << 16) | (uint32(b[15]) << 24)

	m.Time = int64(uint64(b[16]) | (uint64(b[17]) << 8) | (uint64(b[18]) << 16) | (uint64(b[19]) << 24) |
		(uint64(b[20]) << 32) | (uint64(b[21]) << 40) | (uint64(b[22]) << 48) | (uint64(b[23]) << 56))

	m.Resv = uint64(b[24]) | (uint64(b[25]) << 8) | (uint64(b[26]) << 16) | (uint64(b[27]) << 24) |
		(uint64(b[28]) << 32) | (uint64(b[29]) << 40) | (uint64(b[30]) << 48) | (uint64(b[31]) << 56)

	m.Vtype = b[24]
}

// IsOk is ok
// 封包格式是否正确
func (m *MnpMedia) IsOk() bool {
	return true
}

// Default default
// 默认封包参数
func (m *MnpMedia) Default() {
	m.Size = 0
	m.Dtype = 0
	m.Chnn = 0
	m.Sidx = 0
	m.Time = 0
	m.Vtype = 0

	m.Resv = 0
}
