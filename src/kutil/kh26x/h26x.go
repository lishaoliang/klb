package kh26x

import "bytes"

// ScanNal ScanNal
// 从h26x数据中寻找到 0001 起始标记
// ok bool			是否找到
// nalPos int		找到的0001起始位置
// nalLen int		整个nal长度(含0001)
// nalHeadLen int	nal头长度: 4, 3
// isEnd bool		是否结束了
func ScanNal(h26x []byte) (ok bool, nalPos int, nalLen int, nalHeadLen int, isEnd bool) {

	h4 := []byte{0x0, 0x0, 0x0, 0x1}
	h3 := []byte{0x0, 0x0, 0x1}

	b := h26x
	bLen := len(b)
	pos := 0

	flag := -1 // 第一次找到 0001 的位置
	headLen := 4

	for pos+3 <= bLen {
		if 0 == bytes.Compare(h3, b[pos:pos+3]) {
			headLen = 3

			if 0 < pos && 0 == bytes.Compare(h4, b[pos-1:pos+3]) {
				headLen = 4
				pos--
			}

			if 0 <= flag {
				// 第二次找到 0001
				nalPos = flag
				nalLen = pos - flag
				isEnd = false
				ok = true
				return // 找到了, 处于中间
			}

			// 第一次找到 0001 位置
			flag = pos
			nalHeadLen = headLen

			pos += headLen
		} else {
			pos++
		}
	}

	if 0 <= flag {
		// 只找到了一次 0001
		nalPos = flag
		nalLen = bLen - flag
		isEnd = true
		ok = true
		return // 找到了, 处于末尾
	}

	// 一个 0001 都没有找到
	return false, 0, 0, 0, true
}
