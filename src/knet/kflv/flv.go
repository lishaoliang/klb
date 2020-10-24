package kflv

import (
	"encoding/binary"

	"github.com/lishaoliang/klb/src/kutil"
)

// flv head,tag
// FLV文件/网络流, 是大端格式
const (
	FlvHeadSize = 9                // Head大小
	FlvVersion  = uint8(0x01)      // flv版本
	FlvFlag     = uint32(0x464C56) // flv flag值(小端)

	FlvTagSize   = 11        // Tag大小
	FlvTagAudio  = uint8(8)  // 音频tag
	FlvTagVideo  = uint8(9)  // 视频tag
	FlvTagScript = uint8(18) // 脚本tag

	FlvVideoSize = 1        // Video大小
	FlvCodecAvc  = uint8(7) // AVC视频类型
	FlvAvcFrameI = uint8(1) // I帧;关键帧(AVC)
	FlvAvcFrameP = uint8(2) // P帧;非关键帧(AVC)

	FlvAudioSize   = 1 // Audio大小
	FlvAudioLPCMPE = uint8(0)
	FlvAudioADPCM  = uint8(1)
	FlvAudioAAC    = uint8(10)

	FlvAudio5dot5KHZ = uint8(0)
	FlvAudio11KHZ    = uint8(1)
	FlvAudio22KHZ    = uint8(2)
	FlvAudio44KHZ    = uint8(3)

	FlvAudioSamples8Bit  = uint8(0)
	FlvAudioSamples16Bit = uint8(1)

	FlvAudioMono   = uint8(0)
	FlvAudioStereo = uint8(1)
)

// Head flv head
// flv头部
type Head struct {
	Version uint8  // 版本: 0x01
	Flag    uint32 // 标记: "FLV"

	Video uint8 // 是否带视频
	resv1 uint8 // 0
	Audio uint8 // 是否带音频
	resv2 uint8 // 0

	Offset uint32 // 数据偏移: 9
}

// Tag flv tag
// tag部分
type Tag struct {
	DataSize uint32 // 数据长度: 不包含本结构体
	TagType  uint8  // tag类型
	Filter   uint8  // 0.未加密; 1.加密
	resv1    uint8  // 0

	TimestampEx uint32 // 扩展时间戳
	Timestamp   uint32 // 时间戳

	resv2 [3]uint8 // 0: StreamID 24位
}

// Video flv tag.video
// tag.video部分
type Video struct {
	CodecID   uint8 // 视频类型
	FrameType uint8 // 帧类型
}

// Audio flv tag.audio
type Audio struct {
	SoundFormat uint8 // 音频格式
	SoundRate   uint8 // 音频频率
	SoundSize   uint8 // 8比特,16比特
	SoundType   uint8 // Mono Stereo
}

// Pack pack Head
// 将结构打包成二进制
func (m *Head) Pack(b []byte) []byte {
	if nil == b {
		b = make([]byte, FlvHeadSize)
	}

	// 4
	flagVer := uint32((FlvFlag&0xFFFFFF)<<8) | uint32((FlvVersion)&0xFF)
	binary.BigEndian.PutUint32(b[0:4], flagVer)

	// 1
	b[4] = ((m.Audio & 0x1) << 2) | (m.Video & 0x1)

	// 4
	binary.BigEndian.PutUint32(b[5:9], FlvHeadSize)

	return b
}

// Unpack unpack Head
// 将二进制解析为结构体
func (m *Head) Unpack(b []byte) {
	kutil.Assert(FlvHeadSize == len(b))

	// 4
	flagVer := binary.BigEndian.Uint32(b[0:4])
	m.Version = uint8(flagVer & 0xFF)
	m.Flag = uint32((flagVer >> 8) & 0xFFFFFF)

	// 1
	av := b[4]
	m.Video = av & 0x1
	m.Audio = (av >> 2) & 0x1

	// 4
	m.Offset = binary.BigEndian.Uint32(b[5:9])
}

// IsOk is ok
// 是否正常
func (m *Head) IsOk() bool {
	if FlvVersion != m.Version || FlvFlag != m.Flag {
		return false
	}

	if m.Offset < FlvHeadSize {
		return false
	}

	return true
}

// Pack pack Tag
// 将结构打包成二进制
func (m *Tag) Pack(b []byte) []byte {
	if nil == b {
		b = make([]byte, FlvTagSize)
	}

	// 4
	dtf := (uint32(m.Filter&0x1) << 29) | (uint32(m.TagType&0x1F) << 24) | (m.DataSize & 0xFFFFFF)
	binary.BigEndian.PutUint32(b[0:4], dtf)

	// 4
	time := ((m.Timestamp & 0xFFFFFF) << 8) | (m.TimestampEx & 0xFF)
	binary.BigEndian.PutUint32(b[4:8], time)

	// 3
	b[8] = 0
	b[9] = 0
	b[10] = 0

	return b
}

// Unpack unpack Tag
// 将二进制解析为结构体
func (m *Tag) Unpack(b []byte) {
	kutil.Assert(FlvTagSize == len(b))

	// 4
	dtf := binary.BigEndian.Uint32(b[0:4])
	m.DataSize = dtf & 0xFFFFFF
	m.TagType = uint8((dtf >> 24) & 0x1F)
	m.Filter = uint8((dtf >> 29) & 0x1)

	// 4
	time := binary.BigEndian.Uint32(b[4:8])
	m.TimestampEx = time & 0xFF
	m.Timestamp = (time >> 8) & 0xFFFFFF
}

// Pack pack Video
// 将结构打包成二进制
func (m *Video) Pack(b []byte) []byte {
	if nil == b {
		b = make([]byte, FlvVideoSize)
	}

	// 1
	b[0] = ((m.FrameType & 0xF) << 4) | (m.CodecID & 0xF)

	return b
}

// Unpack unpack Video
// 将二进制解析为结构体
func (m *Video) Unpack(b []byte) {
	kutil.Assert(FlvVideoSize == len(b))

	v := b[0]

	m.CodecID = v & 0xF
	m.FrameType = (v >> 4) & 0xF
}

// Pack pack Audio
// 将结构打包成二进制
func (m *Audio) Pack(b []byte) []byte {
	if nil == b {
		b = make([]byte, FlvAudioSize)
	}

	// 1
	b[0] = ((m.SoundFormat & 0xF) << 4) | ((m.SoundRate & 0x3) << 2) | ((m.SoundSize & 0x1) << 1) | (m.SoundType & 0x1)

	return b
}

// Unpack unpack Audio
// 将二进制解析为结构体
func (m *Audio) Unpack(b []byte) {
	kutil.Assert(FlvAudioSize == len(b))

	v := b[0]

	m.SoundType = v & 0x1
	m.SoundSize = (v >> 1) & 0x1
	m.SoundRate = (v >> 2) & 0x3
	m.SoundFormat = (v >> 4) & 0xF
}
