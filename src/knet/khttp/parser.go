package khttp

import (
	"bytes"
	"regexp"
	"strconv"
)

var myCRLF = []byte{0x0D, 0x0A}                 // "\r\n"
var myCRLFCRLF = []byte{0x0D, 0x0A, 0x0D, 0x0A} // "\r\n\r\n"

// Parser 解析HTTP1.X头是否完整
// return bool  true.完整; false.不完整
//		int		head头长度
//		int		body长度
//      bool	true.属于升级协议; false.不需要升级协议
func Parser(b []byte) (bool, int, int, bool) {
	if len(b) < 4 {
		return false, 0, 0, false
	}

	pos := bytes.Index(b, myCRLFCRLF)
	if 0 <= pos {
		pos += 4
		h := b[0:pos]

		body := getContentLength(h) // 附加数据长度
		upgrade := isUpgrade(h)     // 是否属于升级协议

		//fmt.Println("khttp.parser.Parser", true, pos, body, upgrade)
		return true, pos, body, upgrade
	}

	return false, 0, 0, false
}

func getContentLength(h []byte) int {
	body := 0

	re := regexp.MustCompile(`(?i)Content-Length[ ]*:[ ]*[\d]+`) // 忽略大小写匹配
	sct := re.Find(h)
	//fmt.Println(string(sctlen))

	if 0 < len(sct) {
		reNum := regexp.MustCompile(`[\d]+`)
		str := reNum.Find(sct)

		num, err := strconv.Atoi(string(str))
		//fmt.Println(l, err)
		if nil == err {
			body = num
		}
	}

	return body
}

func isUpgrade(h []byte) bool {
	// https://developer.mozilla.org/zh-CN/docs/Web/HTTP/Protocol_upgrade_mechanism
	// HTTP1.1升级协议需要包含如下2项
	// Connection: Upgrade
	// Upgrade: protocols

	// 特征1. 匹配行 "Upgrade: protocols"
	re1 := regexp.MustCompile(`(?i)Upgrade[ ]*:`) // 忽略大小写匹配
	upgrade1 := re1.Find(h)

	if 0 < len(upgrade1) {
		return true
	}

	// 特征2. 匹配行 "Connection: Upgrade"
	// 注意可能为: "Connection: xxxx,Upgrade"
	re2 := regexp.MustCompile(`(?i)Connection[ ]*:[^\t\r\n\f\v]*`) // 忽略大小写匹配
	connection := re2.Find(h)

	if 0 < len(connection) {
		re21 := regexp.MustCompile(`(?i)Upgrade`) // 忽略大小写匹配
		upgrade22 := re21.Find(connection)

		if 0 < len(upgrade22) {
			return true
		}
	}

	return false
}
