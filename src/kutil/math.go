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
