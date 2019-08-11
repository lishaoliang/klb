///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/08/11
//
/// @file    rand.go
/// @author  lishaoliang
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief	kgrand
///////////////////////////////////////////////////////////////////////////
package kgrand

import (
	"math/rand"
	"time"
)

type myRand struct {
	r *rand.Rand // 伪随机因子

	bytes    []byte // 字符集[A-Za-z0-9]
	bytesLen int    // bytes长度

	nums    []byte // 字符集[0-9]
	numsLen int    // nums长度
}

var gMyRand myRand

func randInit() {
	gMyRand.r = rand.New(rand.NewSource(time.Now().UnixNano()))

	chs := "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
	gMyRand.bytes = []byte(chs)
	gMyRand.bytesLen = len(gMyRand.bytes)

	ns := "0123456789"
	gMyRand.nums = []byte(ns)
	gMyRand.numsLen = len(gMyRand.nums)
}

// Str 随机长度字符串
// 字符集[A-Za-z0-9]
func Str(sl int) string {
	return Str64(int64(sl))
}

// Str64 随机长度字符串
// 字符集[A-Za-z0-9]
func Str64(sl int64) string {
	ret := []byte{}
	for i := int64(0); i < sl; i++ {
		ret = append(ret, gMyRand.bytes[gMyRand.r.Intn(gMyRand.bytesLen)])
	}

	return string(ret)
}

// StrNum 随机长度数值字符串, 2位以上不以"0"开头
// 字符集[0-9]
func StrNum(sl int) string {
	return StrNum64(int64(sl))
}

// StrNum64 随机长度数值字符串, 2位以上不以"0"开头
// 字符集[0-9]
func StrNum64(sl int64) string {
	ret := []byte{}

	if 1 == sl {
		// 1位数, 可以为[0-9]
		ret = append(ret, gMyRand.nums[gMyRand.r.Intn(gMyRand.numsLen)])
	} else if 1 < sl {
		// 2位数以上, 必须以[1-9]开头
		for true {
			ch := gMyRand.nums[gMyRand.r.Intn(gMyRand.numsLen)]
			if '0' != ch {
				ret = append(ret, ch)
				break
			}
		}

		for i := int64(1); i < sl; i++ {
			ret = append(ret, gMyRand.nums[gMyRand.r.Intn(gMyRand.numsLen)])
		}
	}

	return string(ret)
}
