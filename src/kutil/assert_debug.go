///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   assert_debug.go
/// @author lishaoliang
/// @brief	assert debug
///  usage: go build -tags="debug"
///////////////////////////////////////////////////////////////////////////

// +build debug

package kutil

// Assert assert
// 断言, 类似c/c++
func Assert(expression ...bool) {
	for _, v := range expression {
		if !(v) {
			panic(v)
		}
	}
}
