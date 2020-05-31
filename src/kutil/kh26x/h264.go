package kh26x

// H264 NALU type
const (
	H264SPS = 0x67 & 0x1F
	H264PPS = 0x68 & 0x1F
	H264I   = 0x65 & 0x1F
	H264P   = 0x61 & 0x1F
)

// H264Type get h264 type
func H264Type(nalu uint8) uint8 {
	return nalu & 0x1F
}

// H264IsSPS nalu is SPS?
func H264IsSPS(nalu uint8) bool {
	return H264Type(nalu) == H264SPS
}

// H264IsPPS nalu is PPS?
func H264IsPPS(nalu uint8) bool {
	return H264Type(nalu) == H264PPS
}

// H264IsI nalu is I(key) frame?
func H264IsI(nalu uint8) bool {
	return H264Type(nalu) == H264I
}

// H264IsP nalu is P(not key) frame?
func H264IsP(nalu uint8) bool {
	return H264Type(nalu) == H264P
}
