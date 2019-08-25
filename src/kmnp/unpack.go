///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/08/18
//
/// @file    unpack.go
/// @author  lishaoliang
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief	unpack
///////////////////////////////////////////////////////////////////////////
package kmnp

import "errors"

// getPackTye get []byte type
func getPackTye(b []byte) (uint8, error) {
	return MNPheart, nil
}

// unpackTxt unpack text
func unpackTxt(b []byte) (uint32, uint32, []byte, []byte, error) {
	bLen := len(b)

	posTxt := MNPTsize
	posExtra := posTxt + MNPTxtHsize

	if bLen < posExtra {
		return 0, 0, nil, nil, errors.New("kmnp.unpack.unpackTxt,error!less byte buf")
	}

	var mnpt MnpT
	bMnpT := b[:posTxt]
	mnpt.Unpack(bMnpT)

	var mnpTxtH MnpTxtH
	bMnpTxtH := b[posTxt:posExtra]
	mnpTxtH.Unpack(bMnpTxtH)

	extraLen := int(mnpTxtH.Extra)
	msgLen := int(mnpt.Size) - posExtra - extraLen

	posMsg := posExtra + extraLen
	posEnd := posMsg + msgLen

	return mnpTxtH.Sequence, mnpTxtH.UID, b[posExtra:posMsg], b[posMsg:posEnd], nil
}
