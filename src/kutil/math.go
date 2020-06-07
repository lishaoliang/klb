///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   math.go
/// @author lishaoliang
/// @brief	math
///////////////////////////////////////////////////////////////////////////

package kutil

// Max max
// 较大值
func Max(v1, v2 int) int {
	if v1 <= v2 {
		return v2
	}
	return v1
}

// Min min
// 较小值
func Min(v1, v2 int) int {
	if v1 <= v2 {
		return v1
	}
	return v2
}

// SubAbsI64 Sub Abs int64
func SubAbsI64(v1, v2 int64) int64 {
	if v1 <= v2 {
		return v2 - v1
	}

	return v1 - v2
}
