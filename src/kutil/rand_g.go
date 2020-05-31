///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   rand_g.go
/// @author lishaoliang
/// @brief	rand G
///////////////////////////////////////////////////////////////////////////

package kutil

var gMyRand *Rand

func randInit() {
	gMyRand = NewRand()
}

// RandStr 随机长度字符串
// 字符集[A-Za-z0-9]
func RandStr(sl int) string {
	return gMyRand.Str64(int64(sl))
}

// RandStr64 随机长度字符串
// 字符集[A-Za-z0-9]
func RandStr64(sl int64) string {
	return gMyRand.Str64(sl)
}

// RandStrNum 随机长度数值字符串, 2位以上不以"0"开头
// 字符集[0-9]
func RandStrNum(sl int) string {
	return gMyRand.StrNum64(int64(sl))
}

// RandStrNum64 随机长度数值字符串, 2位以上不以"0"开头
// 字符集[0-9]
func RandStrNum64(sl int64) string {
	return gMyRand.StrNum64(sl)
}
