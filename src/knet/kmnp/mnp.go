///////////////////////////////////////////////////////////////////////////
//	Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file	mnp.go
/// @author	lishaoliang
/// @brief	mnp
///////////////////////////////////////////////////////////////////////////

package kmnp

// from */mnp/klb_mnp.h

// Mnp klb_mnp_t
// 网络封包头: 小端对齐
// 固定8字节, 封包头可以被写入文件, 需要精简大小
// 缓存大小:
// 	媒体包缓存: [4K, 8K, 16K, 32K]
type Mnp struct {
	Magic    uint32 // [bit32]魔数: MnpMagic
	Size     uint16 // [bit16]单个数据包大小(包含本结构体) <= 32K
	Opt      uint8  // [bit2]包组合方式: MnpOptBegin,MnpOptContinue,MnpOptEnd,MnpOptFull
	Packtype uint8  // [bit5]包类型: MnpPackHeart,MnpPackText,MnpPackBin,MnpPackMedia
	Resv1    uint8  // [bit1]0
	Resv2    uint8  // [bit8]0
}

// MNP协议头常量定义
const (
	MnpHeadSize = 8 //Mnp, sizeof(klb_mnp_t)

	// magic
	MnpMagic        = uint32(0x504E4DEB) // KLB_MNP_MAGIC, 魔数
	MnpBlockSizeMax = 32 * 1024          // 分包最大32K

	// klb_mnp_opt_e
	MnpOptBegin    = uint8(0x0) // KLB_MNP_BEGIN,开始包
	MnpOptContinue = uint8(0x1) // KLB_MNP_CONTINUE,中间包
	MnpOptEnd      = uint8(0x2) // KLB_MNP_END,结束包
	MnpOptFull     = uint8(0x3) // KLB_MNP_FULL,完整包
	MnpOptMax      = uint8(0x3) // KLB_MNP_OPT_MAX,opt项最大值

	// klb_mnp_packtype_e
	MnpPackHeart = uint8(0x0)  // KLB_MNP_HEART,心跳包
	MnpPackText  = uint8(0x1)  // KLB_MNP_TXT,文本包
	MnpPackBin   = uint8(0x2)  // KLB_MNP_BIN,二进制包
	MnpPackMedia = uint8(0x3)  // KLB_MNP_MEDIA,媒体包
	MnpPackMax   = uint8(0x1F) // KLB_MNP_PACKTYPE_MAX,包类型最大值
)

// Pack to []byte
// 打包封包头,大小MnpHeadSize
func (m *Mnp) Pack(b []byte) []byte {
	if nil == b {
		b = make([]byte, MnpHeadSize)
	}

	// LE
	b[0] = uint8(m.Magic)
	b[1] = uint8(m.Magic >> 8)
	b[2] = uint8(m.Magic >> 16)
	b[3] = uint8(m.Magic >> 24)

	b[4] = uint8(m.Size)
	b[5] = uint8(m.Size >> 8)

	b[6] = ((m.Resv1 & 0x1) << 7) | ((m.Packtype & 0x1F) << 2) | (m.Opt & 0x3)
	b[7] = m.Resv2

	return b
}

// Unpack from []byte
// 解包封包头,大小MnpHeadSize
func (m *Mnp) Unpack(b []byte) {
	// LE
	m.Magic = uint32(b[0]) | (uint32(b[1]) << 8) | (uint32(b[2]) << 16) | (uint32(b[3]) << 24)

	m.Size = uint16(b[4]) | (uint16(b[5]) << 8)

	resv2PtOpt := b[6]
	m.Opt = resv2PtOpt & 0x3
	m.Packtype = (resv2PtOpt >> 2) & 0x1F
	m.Resv1 = (resv2PtOpt >> 7) & 0x1

	m.Resv2 = b[7]
}

// IsOk is ok
// 封包格式是否正确
func (m *Mnp) IsOk() bool {

	if MnpMagic != m.Magic {
		return false
	}

	if MnpOptMax < m.Opt {
		return false
	}

	return true
}

// Default default
// 默认封包参数
func (m *Mnp) Default() {
	m.Magic = MnpMagic
	m.Size = MnpHeadSize
	m.Opt = MnpOptFull
	m.Packtype = MnpPackHeart
	m.Resv1 = 0
	m.Resv2 = 0
}
