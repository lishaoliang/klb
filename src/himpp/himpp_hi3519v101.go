///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   himpp_hi3519v101.go
/// @author lishaoliang
/// @brief	himpp hi3519v101
///  usage: go build -tags="hi3519v101 himppv3"
///////////////////////////////////////////////////////////////////////////

// +build hi3519v101

package himpp

/*
#cgo CFLAGS: -std=c11 -D__HI3519V101__ -D__HIMPPV3__
#cgo CFLAGS: -I ${SRCDIR}/../../inc -I ${SRCDIR}/../../hisdk/hi3519v101/include

#cgo LDFLAGS: -L ${SRCDIR}/../../lib -L ${SRCDIR}/../../hisdk/hi3519v101/lib

#cgo LDFLAGS: -lisp -l_hiae -l_hiaf -l_hiawb -l_hiawb_dv
#cgo LDFLAGS: -lmpi -ldnvqe -lhi_cipher -lhifisheyecalibrate -lhistitch -live -lmd -lpciv
#cgo LDFLAGS: -l_hidefog -ltde -lupvqe -lVoiceEngine

#cgo LDFLAGS: -lpthread -ldl -lm
*/
import "C"
