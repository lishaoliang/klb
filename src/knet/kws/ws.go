package kws

import (
	"encoding/binary"
	"errors"
)

// Ws head
type Ws struct {
	Fin        uint8
	Rsv1       uint8
	Rsv2       uint8
	Rsv3       uint8
	Opcode     uint8
	Mask       uint8
	PayloadLen uint64
	MaskingKey []byte
	HeadLen    int
}

// Ws Head lenght
const (
	WsHeadSizeMin = 2
	WsHeadSizeMax = 14
)

// Ws.Opcode define
// RFC 6455  5.2. Base Framing Protocol
const (
	WsOpcodeNil    = 0x0 // denotes a continuation frame
	WsOpcodeText   = 0x1 // denotes a text frame
	WsOpcodeBinary = 0x2 // denotes a binary frame
	WsOpcodeClose  = 0x8 // denotes a connection close
	WsOpcodePing   = 0x9 // denotes a ping
	WsOpcodePong   = 0xA // denotes a pong
	WsOpcodeMax    = 0xF // Opcode Max
)

// Ws.Fin, Ws.Mask
const (
	WsFin  = 0x1
	WsMask = 0x1
)

// Pack to []byte
// 打包封包头,大小MNPheadSize
func (m *Ws) Pack(b []byte) []byte {
	headLen := WsHeadSizeMin
	var payloadLen uint8

	if 65536 < m.PayloadLen {
		headLen = WsHeadSizeMin + 8
		payloadLen = 127
	} else if 125 < m.PayloadLen {
		headLen = WsHeadSizeMin + 2
		payloadLen = 126
	} else {
		headLen = WsHeadSizeMin
		payloadLen = uint8(m.PayloadLen)
	}

	if 0x1 == (m.Mask & 0x1) {
		headLen += 4
	}

	if nil == b {
		b = make([]byte, headLen)
	}

	b[0] = ((m.Fin & 0x1) << 7) | ((m.Rsv1 & 0x1) << 6) | ((m.Rsv2 & 0x1) << 5) | ((m.Rsv3 & 0x1) << 4) | (m.Opcode & 0xF)
	b[1] = ((m.Mask & 0x1) << 7) | (payloadLen & 0x7F)

	pos := WsHeadSizeMin // 2

	// 扩展长度
	if 126 == payloadLen {
		binary.BigEndian.PutUint16(b[pos:], uint16(m.PayloadLen))
		pos += 2
	} else if 127 == payloadLen {
		binary.BigEndian.PutUint64(b[pos:], m.PayloadLen)
		pos += 8
	}

	// mask key
	if 0x1 == (m.Mask & 0x1) {
		copy(b[pos:], m.MaskingKey[:4])
		pos += 4
	}

	return b[:headLen]
}

// Unpack from []byte
// 解包封包头,大小MNPheadSize
func (m *Ws) Unpack(b []byte) error {
	blen := len(b)
	if blen < WsHeadSizeMin {
		return errors.New("kws.ws.Unpack,less byte,pos 1")
	}

	b0 := b[0]

	m.Fin = (b0 & 0x80) >> 7
	m.Rsv1 = (b0 & 0x40) >> 6
	m.Rsv2 = (b0 & 0x20) >> 5
	m.Rsv3 = (b0 & 0x10) >> 4
	m.Opcode = b0 & 0xF

	b1 := b[1]
	m.Mask = (b1 & 0x80) >> 7
	payloadLen := int(b1 & 0x7F)

	pos := 2
	if 126 == payloadLen {
		if blen < pos+2 {
			return errors.New("kws.ws.Unpack,less byte,pos 2")
		}

		// 扩展2字节长度
		m.PayloadLen = uint64(binary.BigEndian.Uint16(b[pos : pos+2]))
		pos += 2
	} else if 127 == payloadLen {
		if blen < pos+8 {
			return errors.New("kws.ws.Unpack,less byte,pos 3")
		}

		// 扩展8字节
		m.PayloadLen = uint64(binary.BigEndian.Uint64(b[pos : pos+8]))
		pos += 8
	} else {
		m.PayloadLen = uint64(payloadLen)
	}

	if 0x1 == (m.Mask & 0x1) {
		if blen < pos+4 {
			return errors.New("kws.ws.Unpack,less byte,pos 4")
		}

		//m.MaskingKey = binary.BigEndian.Uint32(b[pos : pos+4])
		m.MaskingKey = make([]byte, 4)
		copy(m.MaskingKey, b[pos:pos+4])

		pos += 4
	}

	m.HeadLen = pos

	return nil
}

// IsOk is ok
// 封包格式是否正确
func (m *Ws) IsOk() bool {

	return true
}
