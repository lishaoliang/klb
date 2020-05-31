package kflv

import (
	"encoding/binary"

	"github.com/lishaoliang/klb/src/kutil"
)

// flv avc
const (
	FlvAvcSize     = 4        // Avc大小
	FlvAvcSequence = uint8(0) // AVC sequence header; SPS/PPS单元
	FlvAvcNalu     = uint8(1) // AVC NALU; I/P单元
	FlvAvcEnd      = uint8(2) // AVC end of sequence (lower level NALU sequence ender is not required or supported)

	FlvAvcDecCfgRecSize = 5 // AvcDecCfgRec大小
)

// Avc tag.video.avc
type Avc struct {
	CompositionTime uint32 // 校验时间
	AvcPackType     uint8  // avc包类型
}

// AvcDecCfgRec tag.video.avc.AvcDecCfgRec
// flv AVCDecoderConfigurationRecord
type AvcDecCfgRec struct {
	Version              uint8 // configurationVersion
	ProfileIndication    uint8 // avcProfileIndication
	ProfileCompatibility uint8 // profile_compatibility
	LevelIndication      uint8 // AVCLevelIndication
	SizeMinusOne         int32 // lengthSizeMinusOne
}

// Pack pack Avc
// 将结构打包成二进制
func (m *Avc) Pack(b []byte) []byte {
	if nil == b {
		b = make([]byte, FlvAvcSize)
	}

	// 4
	v := (uint32(m.AvcPackType&0xFF) << 24) | (m.CompositionTime & 0xFFFFFF)
	binary.BigEndian.PutUint32(b[0:4], v)

	return b
}

// Unpack unpack Avc
// 将二进制解析为结构体
func (m *Avc) Unpack(b []byte) {
	kutil.Assert(FlvAvcSize == len(b))

	// 4
	avc := binary.BigEndian.Uint32(b[0:4])

	m.CompositionTime = avc & 0xFFFFFF
	m.AvcPackType = uint8((avc >> 24) & 0xFF)
}

// Pack pack AvcDecCfgRec
// 将结构打包成二进制
func (m *AvcDecCfgRec) Pack(b []byte) []byte {
	if nil == b {
		b = make([]byte, FlvAvcDecCfgRecSize)
	}

	// 4
	b[0] = m.Version
	b[1] = m.ProfileIndication
	b[2] = m.ProfileCompatibility
	b[3] = m.LevelIndication

	// 1
	if 3 == m.SizeMinusOne {
		b[4] = 0xFE
	} else {
		b[4] = 0xFF
	}

	kutil.Assert(3 == m.SizeMinusOne || 4 == m.SizeMinusOne)

	return b
}

// Unpack unpack AvcDecCfgRec
// 将二进制解析为结构体
func (m *AvcDecCfgRec) Unpack(b []byte) {
	kutil.Assert(FlvAvcDecCfgRecSize == len(b))

	m.Version = b[0]              // configurationVersion
	m.ProfileIndication = b[1]    // avcProfileIndication
	m.ProfileCompatibility = b[2] // profile_compatibility
	m.LevelIndication = b[3]      // AVCLevelIndication

	smo := b[4]
	m.SizeMinusOne = int32(smo&0x3) + 1 // lengthSizeMinusOne

	//fmt.Println("AvcDecCfgRec", m)
	kutil.Assert(3 == m.SizeMinusOne || 4 == m.SizeMinusOne)
}
