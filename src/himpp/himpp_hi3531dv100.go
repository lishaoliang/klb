///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   himpp_hi3531dv100.go
/// @author lishaoliang
/// @brief	himpp hi3531dv100
///  usage: go build -tags="hi3531dv100"
///////////////////////////////////////////////////////////////////////////

// +build hi3531dv100

package himpp

/*
#cgo CFLAGS: -std=c11 -D__HI3531DV100__
#cgo CFLAGS: -I ${SRCDIR}/../../inc -I ${SRCDIR}/../../hisdk/hi3519v101/include

#cgo LDFLAGS: -L ${SRCDIR}/../../lib -L ${SRCDIR}/../../hisdk/hi3519v101/lib

#cgo LDFLAGS: -lmpi

#cgo LDFLAGS: -lpthread -ldl -lm
*/
import "C"
