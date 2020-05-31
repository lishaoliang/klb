package kflv

import "encoding/binary"

func packTagVideoAvcSequence(dst []byte, sps []byte, pps []byte, timestamp uint32, nalHeadLen int) int {

	total := len(sps) + len(pps) + FlvTagSize + FlvVideoSize + FlvAvcSize + FlvAvcDecCfgRecSize + 3 + 3
	b := dst

	var tag Tag
	tag.DataSize = uint32(total - FlvTagSize)
	tag.TagType = FlvTagVideo

	tag.TimestampEx = (timestamp >> 24) & 0xFF
	tag.Timestamp = timestamp & 0xFFFFFF

	tag.Pack(b[:FlvTagSize])
	b = b[FlvTagSize:]

	var video Video
	video.FrameType = FlvAvcFrameI
	video.CodecID = FlvCodecAvc

	video.Pack(b[:FlvVideoSize])
	b = b[FlvVideoSize:]

	var avc Avc
	avc.AvcPackType = FlvAvcSequence
	avc.Pack(b[:FlvAvcSize])
	b = b[FlvAvcSize:]

	var cfg AvcDecCfgRec
	cfg.Version = 1
	cfg.ProfileIndication = 100
	cfg.ProfileCompatibility = 0
	cfg.LevelIndication = 3
	cfg.SizeMinusOne = int32(nalHeadLen) // 4, 3

	cfg.Pack(b[:FlvAvcDecCfgRecSize])
	b = b[FlvAvcDecCfgRecSize:]

	// SPS
	b[0] = 1
	binary.BigEndian.PutUint16(b[1:3], uint16(len(sps)))
	b = b[3:]

	copy(b, sps)
	b = b[len(sps):]

	// PPS
	b[0] = 1
	binary.BigEndian.PutUint16(b[1:3], uint16(len(pps)))
	b = b[3:]

	copy(b, pps)
	b = b[len(pps):]

	return total
}

func packTagVideoAvcNaluHead(dst []byte, frameType uint8, naluSize int, timestamp uint32) int {

	total := FlvTagSize + FlvVideoSize + FlvAvcSize
	b := dst

	var tag Tag
	tag.DataSize = uint32(total + naluSize - FlvTagSize)
	tag.TagType = FlvTagVideo

	tag.TimestampEx = (timestamp >> 24) & 0xFF
	tag.Timestamp = timestamp & 0xFFFFFF

	tag.Pack(b[:FlvTagSize])
	b = b[FlvTagSize:]

	var video Video
	//video.FrameType = FlvAvcFrameI
	video.CodecID = FlvCodecAvc

	if FlvAvcFrameI == frameType {
		video.FrameType = FlvAvcFrameI
	} else {
		video.FrameType = FlvAvcFrameP
	}

	video.Pack(b[:FlvVideoSize])
	b = b[FlvVideoSize:]

	var avc Avc
	avc.AvcPackType = FlvAvcNalu
	avc.Pack(b[:FlvAvcSize])
	b = b[FlvAvcSize:]

	// 这里只写了头部
	// NALU数据 = uint32 + nalu

	return total
}
