package krtsp

// RtpHead rtp head
type RtpHead struct {
	Version     uint8  // version : 2
	Padding     uint8  // padding flag
	FlagExt     uint8  // header extension flag
	CsrcCount   uint8  // CSRC count
	Mark        uint8  // marker bit
	PayloadType uint8  // payload type
	Sequence    uint16 // sequence number
	Timestamp   uint32 // timestamp
	Ssrc        uint32 // synchronization source
}
