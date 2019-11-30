///////////////////////////////////////////////////////////////////////////
//	Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file	mnp.go
/// @author	lishaoliang
/// @brief	mnp
///////////////////////////////////////////////////////////////////////////

package kmnp

// from */mnp/klb_mnp.h

// MnpT klb_mnp_t
// 网络封包头: 小端对齐
// 固定8字节, 封包头可以被写入文件, 需要精简大小
// 缓存大小:
// 	媒体包缓存: [4K, 8K, 16K, 32K]
type MnpT struct {
	Magic     uint16 // [bit16]魔数: MNPmagic
	MagicUser uint16 // [bit16]magic random, 自定义魔数: MNPmagicUser

	Size     uint16 // [bit16]单个数据包大小(包含本结构体)
	Opt      uint8  // [bit2]包组合方式: MNPbegin,MNPcontinue,MNPend,MNPfull
	Packtype uint8  // [bit3]包类型: MNPheart,MNPtxt,MNPbin,MNPmedia
	Subtype  uint8  // [bit3]子协议类型: MNPsubMedia,MNPsubtypeMax
	Resv     uint8  // [bit8]0,保留
}

// MNP协议头常量定义
const (
	// magic
	MNPmagic     = uint16(0xDCE6) // KLB_MNP_MAGIC, 魔数
	MNPmagicUser = uint16(0x6ECD) // KLB_MNP_MAGIC_USER, 自定义魔数

	// klb_mnp_opt_e
	MNPbegin    = uint8(0x0) // KLB_MNP_BEGIN,开始包
	MNPcontinue = uint8(0x1) // KLB_MNP_CONTINUE,中间包
	MNPend      = uint8(0x2) // KLB_MNP_END,结束包
	MNPfull     = uint8(0x3) // KLB_MNP_FULL,完整包
	MNPoptMax   = uint8(0x3) // KLB_MNP_OPT_MAX,opt项最大值

	// klb_mnp_packtype_e
	MNPheart       = uint8(0x0) // KLB_MNP_HEART,心跳包
	MNPtxt         = uint8(0x1) // KLB_MNP_TXT,文本包
	MNPbin         = uint8(0x2) // KLB_MNP_BIN,二进制包
	MNPmedia       = uint8(0x3) // KLB_MNP_MEDIA,媒体包
	MNPpacktypeMax = uint8(0x7) // KLB_MNP_PACKTYPE_MAX,包类型最大值

	// klb_mnp_subtype_e
	MNPsubMedia   = uint8(0x0) // KLB_MNP_SUB_MEDIA, 媒体子协议
	MNPsubtypeMax = uint8(0xF) // KLB_MNP_SUBTYPE_MAX

	MNPTsize = 8 //MnpT, sizeof(klb_mnp_t)
)

// Pack to []byte
// 打包封包头,大小MNPTsize
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

	b[6] = ((m.Subtype & 0x7) << 5) | ((m.Packtype & 0x7) << 2) | (m.Opt & 0x3)
	b[7] = m.Resv

	return b
}

// Unpack from []byte
// 解包封包头,大小MNPTsize
func (m *MnpT) Unpack(b []byte) {
	// LE
	m.Magic = uint16(b[0]) | (uint16(b[1]) << 8)
	m.MagicUser = uint16(b[2]) | (uint16(b[3]) << 8)

	m.Size = uint16(b[4]) | (uint16(b[5]) << 8)

	optType := b[6]
	m.Opt = optType & 0x3
	m.Packtype = (optType >> 2) & 0x7
	m.Subtype = (optType >> 5) & 0x7

	m.Resv = b[7]
}

// IsOk is ok
// 封包格式是否正确
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
// 默认封包参数
func (m *MnpT) Default() {
	m.Magic = MNPmagic
	m.MagicUser = MNPmagicUser

	m.Size = MNPTsize
	m.Opt = MNPfull
	m.Packtype = MNPheart
	m.Subtype = MNPsubMedia

	m.Resv = 0
}

// InitHeart 心跳包
// 初始化为心跳包
func (m *MnpT) InitHeart() {
	m.Magic = MNPmagic
	m.MagicUser = MNPmagicUser

	m.Size = MNPTsize
	m.Opt = MNPfull
	m.Packtype = MNPheart
	m.Subtype = MNPsubMedia

	m.Resv = 0
}

// InitTxt text包
// 初始化为文本包
func (m *MnpT) InitTxt(size uint16, opt uint8) {
	m.Magic = MNPmagic
	m.MagicUser = MNPmagicUser

	m.Size = size //MNPTsize
	m.Opt = opt   //MNPfull
	m.Packtype = MNPtxt
	m.Subtype = MNPsubMedia

	m.Resv = 0
}

// InitMedia media包
// 初始化为媒体包
func (m *MnpT) InitMedia(size uint16, opt uint8) {
	m.Magic = MNPmagic
	m.MagicUser = MNPmagicUser

	m.Size = size //MNPTsize
	m.Opt = opt   //MNPfull
	m.Packtype = MNPmedia
	m.Subtype = MNPsubMedia

	m.Resv = 0
}
