///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   himpp_hi3516av100.go
/// @author lishaoliang
/// @brief	himpp hi3516av100
///  usage: go build -tags="hi3516av100 himppv2"
///////////////////////////////////////////////////////////////////////////

// +build hi3516av100

package himpp

/*
#cgo CFLAGS: -std=c11 -D__HI3516AV100__ -D__HIMPPV2__
#cgo CFLAGS: -I ${SRCDIR}/../../inc -I ${SRCDIR}/../../hisdk/hi3516av100/include

#cgo LDFLAGS: -L ${SRCDIR}/../../lib -L ${SRCDIR}/../../hisdk/hi3516av100/lib

#cgo LDFLAGS: -lsns_ov4689

#cgo LDFLAGS: -l_cmoscfg -l_iniparser
#cgo LDFLAGS: -lisp -l_hiae -l_hiaf -l_hiawb
#cgo LDFLAGS: -lmpi -ldnvqe -live
#cgo LDFLAGS: -l_hidefog -ltde -lupvqe -lVoiceEngine

#cgo LDFLAGS: -lpthread -ldl -lm
*/
import "C"
