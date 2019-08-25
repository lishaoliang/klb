///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/08/18
//
/// @file    buf.go
/// @author  lishaoliang
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief	buf
///////////////////////////////////////////////////////////////////////////
package kmnp

import "errors"

type mnpBuf struct {
	buf    []byte
	bufLen int
	pos    int
}

func mnpBufCreate(bufLen int) *mnpBuf {
	var buf mnpBuf
	buf.bufLen = bufLen
	buf.buf = make([]byte, bufLen)

	return &buf
}

func (mb *mnpBuf) Reset() {
	mb.pos = 0
}

func (mb *mnpBuf) Data() []byte {
	return mb.buf[:mb.pos]
}

func (mb *mnpBuf) LeftBuf() []byte {
	return mb.buf[mb.pos:]
}

func (mb *mnpBuf) LeftBufBy(num int) []byte {
	e := mb.pos + num
	if mb.bufLen < e {
		e = mb.bufLen
	}

	return mb.buf[mb.pos:e]
}

func (mb *mnpBuf) AddPos(n int) error {
	mb.pos += n

	if mb.bufLen < mb.pos {
		mb.pos = mb.bufLen
		return errors.New("kmnp.mnpBuf.AddPos,error!out of pos")
	}

	return nil
}

func (mb *mnpBuf) IsFull() bool {
	return mb.pos == mb.bufLen
}
