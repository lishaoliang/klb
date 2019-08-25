///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/08/18
//
/// @file    pack.go
/// @author  lishaoliang
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief	pack
///////////////////////////////////////////////////////////////////////////
package kmnp

func packTxt(subProto uint8, sequence, uid uint32, extra, msg []byte) []byte {
	extraLen := len(extra)
	msgLen := len(msg)

	posTxt := MNPTsize
	posExtra := posTxt + MNPTxtHsize
	posMsg := posExtra + extraLen
	posEnd := posMsg + msgLen

	var mnpt MnpT
	mnpt.Default()

	mnpt.Size = uint16(posEnd)
	mnpt.Subtype = subProto //MNPsubMedia
	mnpt.Opt = MNPfull
	mnpt.Packtype = MNPtxt

	var mnpTxtH MnpTxtH
	mnpTxtH.Extra = uint16(extraLen)
	mnpTxtH.Sequence = sequence
	mnpTxtH.UID = uid

	b := make([]byte, mnpt.Size)

	bMnpT := b[:posTxt]
	mnpt.Pack(bMnpT)

	bMnpTxtH := b[posTxt:posExtra]
	mnpTxtH.Pack(bMnpTxtH)

	if 0 < extraLen {
		bExtra := b[posExtra:posMsg]
		copy(bExtra, extra)
	}

	if 0 < msgLen {
		bMsg := b[posMsg:posEnd]
		copy(bMsg, msg)
	}

	//fmt.Println(posEnd, len(b))
	//fmt.Println(b)
	return b
}
