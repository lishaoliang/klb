///////////////////////////////////////////////////////////////////////////
//	Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file	kmnp.go
/// @author	lishaoliang
/// @brief	kmnp
///////////////////////////////////////////////////////////////////////////

package kmnp

/*
#cgo CFLAGS: -std=c11 -I ${SRCDIR}/../../../inc
*/
import "C"

// pack/unpack

// PackTxt pack text by string
func PackTxt(sequence, uid uint32, extra, msg string) []byte {
	return packTxt(MNPsubMedia, sequence, uid, []byte(extra), []byte(msg))
}

// PackTxtB pack text by []byte
func PackTxtB(sequence, uid uint32, extra, msg []byte) []byte {
	return packTxt(MNPsubMedia, sequence, uid, extra, msg)
}
