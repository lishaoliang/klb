///////////////////////////////////////////////////////////////////////////
//	Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file	pack.go
/// @author	lishaoliang
/// @brief	pack
///////////////////////////////////////////////////////////////////////////

package kmnp

import (
	"github.com/lishaoliang/klb/src/kutil"
)

// packTxt mnp pack text
// 打文本数据包
func packTxt(subProto uint8, sequence, uid uint32, extra, msg []byte) []byte {
	sizeFull := MNPTsize + MNPTxtHsize + len(extra) + len(msg)

	if sizeFull <= mnpSize64KLess {
		// 一个完整包可以搞定
		return packTxtFull(subProto, sequence, uid, extra, msg)
	}

	// 必须打BCE包方式
	return packTxtBCE(subProto, sequence, uid, extra, msg)
}

// packTxtFull mnp pack text full
// 打文本数据完整包: F包
func packTxtFull(subProto uint8, sequence, uid uint32, extra, msg []byte) []byte {
	extraLen := len(extra)
	msgLen := len(msg)

	posTxt := MNPTsize
	posExtra := posTxt + MNPTxtHsize
	posMsg := posExtra + extraLen
	posEnd := posMsg + msgLen
	kutil.Assert(posEnd <= mnpSize64KLess)

	var mnpt MnpT
	mnpt.InitTxt(uint16(posEnd), MNPfull)
	mnpt.Subtype = subProto //MNPsubMedia

	var mnpTxtH MnpTxtH
	mnpTxtH.Extra = uint16(extraLen)
	mnpTxtH.Resv = 0
	mnpTxtH.Sequence = sequence
	mnpTxtH.UID = uid

	b := make([]byte, posEnd)

	mnptB := b[:posTxt]
	mnpt.Pack(mnptB)

	mnpTxtHB := b[posTxt:posExtra]
	mnpTxtH.Pack(mnpTxtHB)

	if 0 < extraLen {
		extraB := b[posExtra:posMsg]
		copy(extraB, extra)
	}

	if 0 < msgLen {
		msgB := b[posMsg:]
		copy(msgB, msg)
	}

	//fmt.Println("kmnp.pack.packTxtFull,F", posEnd)
	return b
}

// packTxtBegin mnp pack text begin
// 打文本数据: BCE包
func packTxtBCE(subProto uint8, sequence, uid uint32, extra, msg []byte) []byte {
	extraLen := len(extra)
	msgLen := len(msg)

	size := MNPTxtHsize + extraLen + msgLen // 总有效数据长度
	kutil.Assert(mnpSize64KLess < size+MNPTsize)

	nums := (size + mnpSize64KLess - MNPTsize - 1) / (mnpSize64KLess - MNPTsize) // 需要打包个数

	totalSize := size + MNPTsize*nums // 总数据长度
	buf := make([]byte, totalSize)

	pos := 0
	srcExtra := extra
	srcMsg := msg
	for {
		end := false
		bLen := mnpSize64KLess
		if totalSize <= pos+bLen {
			bLen = totalSize - pos
			end = true
		}
		b := buf[pos : pos+bLen]
		dataB := b[MNPTsize:]

		if 0 == pos {
			dataB = b[MNPTsize+MNPTxtHsize:]

			var mnpTxtH MnpTxtH
			mnpTxtH.Extra = uint16(extraLen)
			mnpTxtH.Resv = 0
			mnpTxtH.Sequence = sequence
			mnpTxtH.UID = uid

			mnpTxtHB := b[MNPTsize : MNPTsize+MNPTxtHsize]
			mnpTxtH.Pack(mnpTxtHB)
		}

		if 0 < len(dataB) && 0 < len(srcExtra) {
			dataB, srcExtra = packTxtCopyMin(dataB, srcExtra)
		}

		if 0 < len(dataB) && 0 < len(srcMsg) {
			dataB, srcExtra = packTxtCopyMin(dataB, srcMsg)
		}

		kutil.Assert(0 == len(dataB)) // 结束时, 数据一定被填满

		if end {
			var mnpt MnpT
			mnpt.InitTxt(uint16(bLen), MNPend)
			mnpt.Subtype = subProto //MNPsubMedia

			mnpt.Pack(b[:MNPTsize])
			//fmt.Println("kmnp.pack.packTxtBCE,E", bLen)

			kutil.Assert(pos+bLen == totalSize)
			break
		}

		if 0 == pos {
			var mnpt MnpT
			mnpt.InitTxt(uint16(bLen), MNPbegin)
			mnpt.Subtype = subProto //MNPsubMedia

			mnpt.Pack(b[:MNPTsize])
			//fmt.Println("kmnp.pack.packTxtBCE,B", bLen)
		} else {
			var mnpt MnpT
			mnpt.InitTxt(uint16(bLen), MNPcontinue)
			mnpt.Subtype = subProto //MNPsubMedia

			mnpt.Pack(b[:MNPTsize])
			//fmt.Println("kmnp.pack.packTxtBCE,C", bLen)
		}

		pos += bLen
	}

	return buf
}

func packTxtCopyMin(dst, src []byte) ([]byte, []byte) {
	dstLen := len(dst)
	srcLen := len(src)

	minLen := dstLen
	if srcLen <= dstLen {
		minLen = srcLen
	}

	copy(dst[:minLen], src[:minLen])

	return dst[minLen:], src[minLen:]
}
