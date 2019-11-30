///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   log.go
/// @author lishaoliang
/// @brief	log
///////////////////////////////////////////////////////////////////////////

package kutil

import (
	"log"
	"runtime"
)

// Log log
// 普通打印
func Log(v ...interface{}) {
	_, filename, line, ok := runtime.Caller(1)
	if ok {
		log.Println(filename, line, v)
	} else {
		log.Println("/runtime.Caller/error/xxxx.go", 0, v)
	}
	//fmt.Println(filename, line, ok)
}

// LogD log debug
// 调试打印
func LogD(v ...interface{}) {
	_, filename, line, ok := runtime.Caller(1)
	if ok {
		log.Println(filename, line, v)
	} else {
		log.Println("/runtime.Caller/error/xxxx.go", 0, v)
	}
}

// LogE log error
// 错误打印
func LogE(v ...interface{}) {
	_, filename, line, ok := runtime.Caller(1)
	if ok {
		log.Println(filename, line, v)
	} else {
		log.Println("/runtime.Caller/error/xxxx.go", 0, v)
	}
}

// LogS log sys error,panic
// 系统错误打印, 必须重启
func LogS(v ...interface{}) {
	_, filename, line, ok := runtime.Caller(1)
	if ok {
		log.Println(filename, line, v)
	} else {
		log.Println("/runtime.Caller/error/xxxx.go", 0, v)
	}
}
