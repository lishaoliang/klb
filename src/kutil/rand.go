///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   rand.go
/// @author lishaoliang
/// @brief	rand
///////////////////////////////////////////////////////////////////////////

package kutil

import (
	"math/rand"
	"time"
)

// Rand rand
// 随机字符串
type Rand struct {
	r *rand.Rand // 伪随机因子

	chars    []byte // 字符集[A-Za-z0-9]
	charsLen int    // []byte长度

	nums    []byte // 字符集[0-9]
	numsLen int    // nums长度
}

///////////////////////////////////////////////////////////////////////////////

// RandCreate create
// 创建
func RandCreate() *Rand {
	var m Rand

	m.r = rand.New(rand.NewSource(time.Now().UnixNano()))

	chs := "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
	m.chars = []byte(chs)
	m.charsLen = len(m.chars)

	ns := "0123456789"
	m.nums = []byte(ns)
	m.numsLen = len(m.nums)

	return &m
}

// Destroy destroy
// 销毁
func (m *Rand) Destroy() {
	m.r = nil
	m.chars = nil
	m.nums = nil
}

// Str 随机长度字符串
// 字符集[A-Za-z0-9]
func (m *Rand) Str(sl int) string {
	return string(m.Str64B(int64(sl)))
}

// Str64 随机长度字符串
// 字符集[A-Za-z0-9]
func (m *Rand) Str64(sl int64) string {
	return string(m.Str64B(sl))
}

// StrB 随机长度字符串
// 字符集[A-Za-z0-9]
func (m *Rand) StrB(sl int) []byte {
	return m.Str64B(int64(sl))
}

// Str64B 随机长度字符串
// 字符集[A-Za-z0-9]
func (m *Rand) Str64B(sl int64) []byte {
	ret := []byte{}
	for i := int64(0); i < sl; i++ {
		ret = append(ret, m.chars[m.r.Intn(m.charsLen)])
	}

	return ret
}

// StrNum 随机长度数值字符串, 2位以上不以"0"开头
// 字符集[0-9]
func (m *Rand) StrNum(sl int) string {
	return string(m.StrNum64B(int64(sl)))
}

// StrNum64 随机长度数值字符串, 2位以上不以"0"开头
// 字符集[0-9]
func (m *Rand) StrNum64(sl int64) string {
	return string(m.StrNum64B(sl))
}

// StrNumB 随机长度数值字符串, 2位以上不以"0"开头
// 字符集[0-9]
func (m *Rand) StrNumB(sl int) []byte {
	return m.StrNum64B(int64(sl))
}

// StrNum64B 随机长度数值字符串, 2位以上不以"0"开头
// 字符集[0-9]
func (m *Rand) StrNum64B(sl int64) []byte {
	ret := []byte{}

	if 1 == sl {
		// 1位数, 可以为[0-9]
		ret = append(ret, m.nums[m.r.Intn(m.numsLen)])
	} else if 1 < sl {
		// 2位数以上, 必须以[1-9]开头
		for {
			ch := m.nums[m.r.Intn(m.numsLen)]
			if '0' != ch {
				ret = append(ret, ch)
				break
			}
		}

		for i := int64(1); i < sl; i++ {
			ret = append(ret, m.nums[m.r.Intn(m.numsLen)])
		}
	}

	return ret
}
