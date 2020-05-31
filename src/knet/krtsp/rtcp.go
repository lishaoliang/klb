package krtsp

// RtcpHead rtcp head
type RtcpHead struct {
	Version    uint8  // protocol version
	Padding    uint8  // padding flag
	Count      uint8  // varies by packet type
	PacketType uint8  // RTCP packet type
	Length     uint16 // packet len in words, w/o this word
}

// RtcpReport rtcp report
type RtcpReport struct {
	Ssrc         uint32 // data source being reported
	FractionLost uint8  // fraction lost since last SR/RR
	NumPackLost  uint32 // cumul. no. pkts lost (signed!)
	ExtSequence  uint32 // extended last seq. no. received
	Jitter       uint32 // interarrival jitter
	Lsr          uint32 // last SR packet from this source
	Dlsr         uint32 // delay since last SR packet
}
