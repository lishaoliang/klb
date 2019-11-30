///////////////////////////////////////////////////////////////////////////
//	Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file	mnp_media.go
/// @author	lishaoliang
/// @brief	mnp_txt
///////////////////////////////////////////////////////////////////////////

package kmnp

// MnpMH klb_mnp_mh_t
// 媒体包头
type MnpMH struct {
	Dtype uint16 // 数据类型(data type): klb_mnp_dtype_e
	Vtype uint8  // 视频(video type): klb_mnp_vtype_e
	Resv  uint8  // 0

	Chnn uint16 // 通道
	Sidx uint16 // 流序号(stream index): klb_mnp_sidx_e

	Time int64 // 时间戳(基于1970年基准,微妙)(范围约:[-292471年, 292471年]
}

// MNPMHsize sizeof(klb_mnp_mh_t)
// 媒体包头大小
const MNPMHsize = 16

// MnpMH.Dtype klb_mnp_dtype_e
// 数据类型常量定义
const (
	MNPdtypeNull = 0x0000 // KLB_MNP_DTYPE_NULL

	MNPvideoB = 0x0000 // KLB_MNP_VIDEO_B, 视频 Begin
	MNPmjpeg  = 0x0001 // KLB_MNP_MJPEG, MJPEG
	MNPh264   = 0x0002 // KLB_MNP_H264, H264
	MNPh265   = 0x0003 // KLB_MNP_H265, H265
	MNPvideoE = 0x00FF // KLB_MNP_VIDEO_E, 视频 End

	MNPaudioB = 0x0100 // KLB_MNP_AUDIO_B, 音频 Begin
	MNPg711a  = 0x0101 // KLB_MNP_G711A, G711A
	MNPg711u  = 0x0102 // KLB_MNP_G711U, G711U
	MNPaudioE = 0x01FF // KLB_MNP_AUDIO_E, 音频 End

	MNPpicB = 0x0200 // KLB_MNP_PIC_B, 图片 Begin
	MNPjpeg = 0x0201 // KLB_MNP_JPEG, JPEG
	MNPpicE = 0x02FF // KLB_MNP_PIC_E, 图片 End

	MNPimgB     = 0x0300 // KLB_MNP_IMG_B, 图像 Begin
	MNPargb8888 = 0x0301 // KLB_MNP_ARGB8888, [ARGB][ARGB]...[ARGB]
	MNPimgE     = 0x0300 // KLB_MNP_IMG_E, 图像 End
)

// MnpMH.Vtype klb_mnp_vtype_e
// 视频(video type)
const (
	MNPvtypeI   = 0x00 // KLB_MNP_VTYPE_I, I帧
	MNPvtypeP   = 0x01 // KLB_MNP_VTYPE_P, P帧
	MNPvtypeB   = 0x02 // KLB_MNP_VTYPE_B, B帧
	MNPvtypeMax = 0xFF // KLB_MNP_VTYPE_MAX, MAX
)

// MnpMH.Sidx klb_mnp_sidx_e
// 媒体流序号
const (
	MNPsidxNull = 0x0000 // KLB_MNP_SIDX_NULL

	MNPsidxV1 = 0x0001 // KLB_MNP_SIDX_V1, Video 1
	MNPsidxV2 = 0x0002 // KLB_MNP_SIDX_V2, Video 2
	MNPsidxV3 = 0x0003 // KLB_MNP_SIDX_V3, Video 3

	MNPsidxA1 = 0x0011 // KLB_MNP_SIDX_A1, Audio 1
	MNPsidxA2 = 0x0012 // KLB_MNP_SIDX_A2, Audio 2
	MNPsidxA3 = 0x0013 // KLB_MNP_SIDX_A3, Audio 3

	MNPsidxP1 = 0x0021 // KLB_MNP_SIDX_P1, Picture 1
	MNPsidxP2 = 0x0022 // KLB_MNP_SIDX_P2, Picture 2
	MNPsidxP3 = 0x0023 // KLB_MNP_SIDX_P3, Picture 3

	MNPsidxI1 = 0x0031 // KLB_MNP_SIDX_I1, Image 1
	MNPsidxI2 = 0x0032 // KLB_MNP_SIDX_I2, Image 2
	MNPsidxI3 = 0x0033 // KLB_MNP_SIDX_I3, Image 3

	MNPsidxMAX = 0xFFFF // KLB_MNP_SIDX_MAX, MAX
)

///////////////////////////////////////////////////////////////////////////////

// Pack to []byte
// 打包媒体包头,大小MNPMHsize
func (m *MnpMH) Pack(b []byte) []byte {
	if nil == b {
		b = make([]byte, MNPMHsize)
	}

	// LE
	b[0] = uint8(m.Dtype)
	b[1] = uint8(m.Dtype >> 8)

	b[2] = m.Vtype
	b[3] = m.Resv

	b[4] = uint8(m.Chnn)
	b[5] = uint8(m.Chnn >> 8)

	b[6] = uint8(m.Sidx)
	b[7] = uint8(m.Sidx >> 8)

	b[8] = uint8(m.Time)
	b[9] = uint8(m.Time >> 8)
	b[10] = uint8(m.Time >> 16)
	b[11] = uint8(m.Time >> 24)
	b[12] = uint8(m.Time >> 32)
	b[13] = uint8(m.Time >> 40)
	b[14] = uint8(m.Time >> 48)
	b[15] = uint8(m.Time >> 56)

	//fmt.Println("kmnp.mnp_media.Pack", b)
	return b
}

// Unpack from []byte
// 解包媒体包头,大小MNPMHsize
func (m *MnpMH) Unpack(b []byte) {

}

// IsOk is ok
// 封包格式是否正确
func (m *MnpMH) IsOk() bool {
	return false
}
