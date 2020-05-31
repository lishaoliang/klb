package krtsp

import (
	"bytes"
	"regexp"
	"strconv"
)

const rtpToken = 0x24                           // '$'
var myCRLF = []byte{0x0D, 0x0A}                 // "\r\n"
var myCRLFCRLF = []byte{0x0D, 0x0A, 0x0D, 0x0A} // "\r\n\r\n"

// Parser parser
func Parser(b []byte) (bool, int, int) {
	if len(b) < 4 {
		return false, 0, 0
	}

	if rtpToken == b[0] {

	} else {
		pos := bytes.Index(b, myCRLFCRLF)
		if 0 <= pos {
			pos += 4
			length := 0

			h := b[0:pos]
			re := regexp.MustCompile(`Content-Length[ ]*:[ ]*[\d]+`)
			sctlen := re.Find(h)
			//fmt.Println(string(sctlen))

			if 0 < len(sctlen) {
				reNum := regexp.MustCompile(`[\d]+`)
				sl := reNum.Find(sctlen)
				l, err := strconv.Atoi(string(sl))
				//fmt.Println(l, err)
				if nil == err {
					length = l
				}
			}

			return true, pos, length
		}
	}

	return false, 0, 0
}
