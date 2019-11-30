///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   fh264.go
/// @author lishaoliang
/// @brief	fh264
///////////////////////////////////////////////////////////////////////////

package kgstream

import (
	"bytes"
	"errors"
	"fmt"
	"io/ioutil"
	"os"
	"unsafe"

	"github.com/lishaoliang/klb/src/kutil"
)

// fh264Frame fh264 frame
type fh264Frame struct {
	sps []byte
	pps []byte
	img []byte

	pSps unsafe.Pointer
	pPps unsafe.Pointer
	pImg unsafe.Pointer
}

// fh264 fh264
type fh264 struct {
	data []byte

	count  int
	idx    int
	frames []fh264Frame

	pData unsafe.Pointer
}

func fh264Load(path string) (*fh264, error) {
	f, err := os.Open(path)
	if err != nil {
		return nil, err
	}

	defer f.Close()

	var fh fh264
	fh.frames = make([]fh264Frame, 0)
	fh.data, _ = ioutil.ReadAll(f)

	dataLen := len(fh.data)
	if 0 < dataLen {
		fh.pData = kutil.Malloc(dataLen)
		kutil.MemcpyB(fh.pData, fh.data)
	}

	fmt.Println("kgstream.fh264.fh264Load read file len:", len(fh.data), path)

	fh.init()
	return &fh, nil
}

///////////////////////////////////////////////////////////////////////////////

// Unload unload
func (fh *fh264) Unload() {

	if nil != fh.pData {
		kutil.Free(fh.pData)
		fh.pData = nil
	}

	fh.count = 0
	fh.idx = 0
	fh.data = nil
	fh.frames = nil
}

// Count count
func (fh *fh264) Count() int {
	return fh.count
}

// Get get
func (fh *fh264) Get(idx int) (*fh264Frame, error) {
	if fh.count <= idx {
		return nil, errors.New("kgstream.fh264.Get,out of idx")
	}

	return &fh.frames[idx], nil
}

// GetNext get next
func (fh *fh264) GetNext() (*fh264Frame, error) {
	idx := fh.idx

	next := idx + 1
	if fh.count <= next {
		next = 0
	}

	fh.idx = next
	return fh.Get(idx)
}

///////////////////////////////////////////////////////////////////////////////

const (
	h264P   = 1
	h264I   = 5
	h264SPS = 7
	h264PPS = 8
)

func h264Type(v uint8) int {
	return int(v & 0x1f)
}

func h264Scan(data []byte) (int, int, []byte, []byte, []byte, int, int, int) {
	h3 := []byte{0x00, 0x00, 0x01}
	h4 := []byte{0x00, 0x00, 0x00, 0x01}

	var sps, pps, img []byte
	var posSps, posPps, posImg int

	lastOff := -1
	lastType := -1
	first := -1

	off := 0
	nal := 0
	l := len(data)

	for {
		if l < off+4 {
			break
		}

		d3 := data[off : off+3]
		d4 := data[off : off+4]

		if 0 == bytes.Compare(d3, h3) {
			nal = off
			off += 3
		} else if 0 == bytes.Compare(d4, h4) {
			nal = off
			off += 4
		} else {
			off++
			continue
		}

		if first < 0 {
			first = nal
		}

		if h264P == lastType {
			if 0 < len(img) {
				break
			}

			img = data[lastOff:nal]
			posImg = lastOff
		} else if h264I == lastType {
			if 0 < len(img) {
				break
			}

			img = data[lastOff:nal]
			posImg = lastOff
		} else if h264SPS == lastType {
			sps = data[lastOff:nal]
			posSps = lastOff
		} else if h264PPS == lastType {
			pps = data[lastOff:nal]
			posPps = lastOff
		}

		lastType = h264Type(data[off])
		lastOff = nal
	}

	return first, lastOff, sps, pps, img, posSps, posPps, posImg
}

func (fh *fh264) init() {
	pos := 0
	for {
		s, e, sps, pps, img, posSps, posPps, posImg := h264Scan(fh.data[pos:])
		if 0 <= s && s < e {
			var frame fh264Frame
			frame.sps = sps
			frame.pps = pps
			frame.img = img

			frame.pSps = kutil.PointerAdd(fh.pData, pos+posSps)
			frame.pPps = kutil.PointerAdd(fh.pData, pos+posPps)
			frame.pImg = kutil.PointerAdd(fh.pData, pos+posImg)

			fh.frames = append(fh.frames, frame)
			fh.count++

			//fmt.Println("frame:", pos, s, e, posSps, posPps, posImg, len(frame.img))
			pos += e
		} else {
			break
		}
	}
}
