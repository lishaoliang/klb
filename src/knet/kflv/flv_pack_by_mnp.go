package kflv

import (
	"encoding/binary"

	"github.com/lishaoliang/klb/src/knet/kmnp"
	"github.com/lishaoliang/klb/src/kutil/kbuf"
	"github.com/lishaoliang/klb/src/kutil/kh26x"
)

// PackMnpParser PackMnpParser
type PackMnpParser struct {
	Mnp     kmnp.Mnp      //
	Media   kmnp.MnpMedia //
	TagSize int           // 分包情况下flv tag数据大小
}

// PackByMnp PackByMnp
// 将含有mnp格式的数据打包成flv格式
func PackByMnp(parser *PackMnpParser, dst kbuf.KBuf, src kbuf.KBuf) error {
	d := src.Data()

	parser.Mnp.Unpack(d)

	d = d[kmnp.MnpHeadSize:]

	if kmnp.MnpOptFull == parser.Mnp.Opt {
		parser.Media.Unpack(d)
		d = d[kmnp.MnpMediaSize:]

		return packFull(parser, dst, d)
	} else if kmnp.MnpOptBegin == parser.Mnp.Opt {
		parser.Media.Unpack(d)
		d = d[kmnp.MnpMediaSize:]

		return packBegin(parser, dst, d)
	} else if kmnp.MnpOptContinue == parser.Mnp.Opt {
		return packContinue(parser, dst, d)
	} else if kmnp.MnpOptEnd == parser.Mnp.Opt {
		return packEnd(parser, dst, d)
	}

	return nil
}

func packFull(parser *PackMnpParser, dst kbuf.KBuf, d []byte) error {

	var sps []byte
	var pps []byte
	var nalData [][]byte

	nalHeadLen := 4
	frameType := FlvAvcFrameP

	nalNum := 0
	nalSize := 0

	for 0 < len(d) {
		ok, nalPos, nalLen, headLen, _ := kh26x.ScanNal(d)

		if !ok {
			break
		}

		nalHeadLen = headLen

		nalu := d[nalPos+headLen]
		nal := d[nalPos+headLen : nalPos+nalLen]

		if kh26x.H264IsSPS(nalu) {
			sps = nal
		} else if kh26x.H264IsPPS(nalu) {
			pps = nal
		} else if kh26x.H264IsI(nalu) {
			nalData = append(nalData, nal)

			nalNum++
			nalSize += len(nal)
			frameType = FlvAvcFrameI
		} else {
			nalData = append(nalData, nal)

			nalNum++
			nalSize += len(nal)
		}

		d = d[nalPos+nalLen:]
	}

	end := 0
	b := dst.Buf()

	if 0 < len(sps) || 0 < len(pps) {
		n := packTagVideoAvcSequence(b, sps, pps, uint32(parser.Media.Time/1000), nalHeadLen)
		b = b[n:]
		binary.BigEndian.PutUint32(b[:4], uint32(n))
		b = b[4:]

		end += n + 4
	}

	if 0 < nalNum {
		n := packTagVideoAvcNaluHead(b, frameType, nalSize+nalNum*4, uint32(parser.Media.Time/1000))
		b = b[n:]

		for i := range nalData {
			nal := nalData[i]

			binary.BigEndian.PutUint32(b[:4], uint32(len(nal)))
			b = b[4:]

			copy(b, nal)
			b = b[len(nal):]
		}

		// 末尾大小
		tagTotal := n + nalSize + nalNum*4
		binary.BigEndian.PutUint32(b[:4], uint32(tagTotal))
		b = b[4:]

		end += tagTotal + 4
	}

	dst.SetPos(0, end)

	return nil
}

func packBegin(parser *PackMnpParser, dst kbuf.KBuf, d []byte) error {

	var sps []byte
	var pps []byte
	var nalData [][]byte

	nalHeadLen := 4
	frameType := FlvAvcFrameP

	nalNum := 0
	nalSize := 0

	for 0 < len(d) {
		ok, nalPos, nalLen, headLen, _ := kh26x.ScanNal(d)

		if !ok {
			break
		}

		nalHeadLen = headLen

		nalu := d[nalPos+headLen]
		nal := d[nalPos+headLen : nalPos+nalLen]

		if kh26x.H264IsSPS(nalu) {
			sps = nal
		} else if kh26x.H264IsPPS(nalu) {
			pps = nal
		} else if kh26x.H264IsI(nalu) {
			nalData = append(nalData, nal)

			nalNum++
			nalSize += len(nal)
			frameType = FlvAvcFrameI
		} else {
			nalData = append(nalData, nal)

			nalNum++
			nalSize += len(nal)
		}

		d = d[nalPos+nalLen:]
	}

	end := 0
	b := dst.Buf()

	if 0 < len(sps) || 0 < len(pps) {
		n := packTagVideoAvcSequence(b, sps, pps, uint32(parser.Media.Time/1000), nalHeadLen)
		b = b[n:]
		binary.BigEndian.PutUint32(b[:4], uint32(n))
		b = b[4:]

		end += n + 4
	}

	if 0 < nalNum {
		cePackSize := int(parser.Media.Size) + kmnp.MnpHeadSize - int(parser.Mnp.Size) // C/E包剩余的数据量

		n := packTagVideoAvcNaluHead(b, frameType, nalSize+cePackSize+nalNum*4, uint32(parser.Media.Time/1000))
		b = b[n:]

		for i := range nalData {
			nal := nalData[i]

			tmpNalLen := uint32(len(nal))
			if i == len(nalData)-1 {
				tmpNalLen += uint32(cePackSize) // 最后一个nal单元, 需要追加C/E包的数据长度
			}

			binary.BigEndian.PutUint32(b[:4], tmpNalLen)
			b = b[4:]

			copy(b, nal)
			b = b[len(nal):]
		}

		// 末尾大小
		nalTotal := n + nalSize + nalNum*4
		parser.TagSize = nalTotal

		end += nalTotal
	}

	dst.SetPos(0, end)

	return nil
}

func packContinue(parser *PackMnpParser, dst kbuf.KBuf, d []byte) error {

	b := dst.Buf()
	dlen := len(d)

	copy(b, d)
	parser.TagSize += dlen

	dst.SetPos(0, dlen)

	return nil
}

func packEnd(parser *PackMnpParser, dst kbuf.KBuf, d []byte) error {

	b := dst.Buf()
	dlen := len(d)

	copy(b, d)
	parser.TagSize += dlen

	binary.BigEndian.PutUint32(b[dlen:dlen+4], uint32(parser.TagSize))
	b = b[dlen+4:]

	dst.SetPos(0, dlen+4)

	return nil
}
