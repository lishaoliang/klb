///////////////////////////////////////////////////////////////////////////
//	Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file	mnp_stream.go
/// @author	lishaoliang
/// @brief	mnp stream
///////////////////////////////////////////////////////////////////////////

package kmnp

/*
#include "mnp/klb_mnp.h"
#include "string.h"
*/
import "C"

import (
	"errors"
	"fmt"
	"unsafe"

	"github.com/lishaoliang/klb/src/kbuf/kmpool"
	"github.com/lishaoliang/klb/src/kutil"
)

// KlbMnpStream klb_mnp_stream_t
// 码流数据(C定义)
type KlbMnpStream C.klb_mnp_stream_t

// MnpStreamPack stream pack
// 打包码流数据函数
type MnpStreamPack interface {
	PackKmBuf(stream *MnpStream, kp kmpool.Pool, chnn, sidx int) (*kmpool.KmBuf, error) // 打包成内存池格式
	PackByte(stream *MnpStream, chnn, sidx int) ([]byte, error)                         // 打包成[]byte
}

// MnpPack klb_mnp_pack_t
// 码流数据pack
type MnpPack struct {
	Phy     uintptr        // 物理地址
	Data    unsafe.Pointer // 数据指针(虚拟地址)
	DataLen int            // 数据长度
}

// MnpStream klb_mnp_stream_t
// 码流数据(go定义),来源高概率为贴近硬件的C代码传递而来
type MnpStream struct {
	Dtype int   // 媒体数据类型: klb_mnp_dtype_e
	Vtype int   // 视频(video type): klb_mnp_vtype_e
	Time  int64 // 时间戳

	Packs []MnpPack // 数据
	size  int       // 总大小: 附加数据 + 媒体数据
}

// MnpStreamCreate create
// 通过C码流结构创建go结构
func MnpStreamCreate(mnpStream *KlbMnpStream) *MnpStream {
	stream := (*C.klb_mnp_stream_t)(mnpStream)

	var m MnpStream

	m.Dtype = int(stream.dtype)
	m.Vtype = int(stream.vtype)
	m.Time = int64(stream.time)

	m.Packs = make([]MnpPack, int(stream.pack_num))

	for i := 0; i < len(m.Packs); i++ {
		m.Packs[i].Data = unsafe.Pointer(stream.packs[C.int(i)].p_data)
		m.Packs[i].DataLen = int(stream.packs[C.int(i)].data_len)

		m.size += m.Packs[i].DataLen
	}

	return &m
}

///////////////////////////////////////////////////////////////////////////////

// Size size
// 数据总大小
func (m *MnpStream) Size() int {
	return m.size
}

// PackKmBuf pack
// 打包数据成内存池格式
// 内存池中的一串数据, 直接对应于网络发送
func (m *MnpStream) PackKmBuf(pack MnpStreamPack, kp kmpool.Pool, chnn, sidx int) (*kmpool.KmBuf, error) {
	if nil != pack {
		return pack.PackKmBuf(m, kp, chnn, sidx)
	}

	return m.packKmBuf(kp, chnn, sidx)
}

// PackByte pack
// 打包数据成[]byte
func (m *MnpStream) PackByte(pack MnpStreamPack, chnn, sidx int) ([]byte, error) {
	if nil != pack {
		return pack.PackByte(m, chnn, sidx)
	}

	return nil, errors.New("kmnp.mnp_stream.PackByte error!to do")
}

///////////////////////////////////////////////////////////////////////////////

// 64K - 8
// mnp.Size为: [0,65535], 这里在末尾空闲几个字节
const mnpSize64KLess = 1024*64 - 8

// packKmBuf pack
// Mnp中间格式打包成内存池格式
// 如果内存池内存不足, 则返回nil, error
func (m *MnpStream) packKmBuf(kp kmpool.Pool, chnn, sidx int) (*kmpool.KmBuf, error) {
	size := kp.BlockSize()               //
	kutil.Assert(size <= mnpSize64KLess) // 内存池块大小 <= mnpSize64KLess

	var mh MnpMH
	mh.Dtype = uint16(m.Dtype)
	mh.Vtype = uint8(m.Vtype)
	mh.Resv = 0
	mh.Chnn = uint16(chnn)
	mh.Sidx = uint16(sidx)
	mh.Time = m.Time

	sizeFull := MNPTsize + MNPMHsize + m.size
	if sizeFull <= mnpSize64KLess && sizeFull <= size {
		return m.packKmBufFull(kp, &mh) // 完整包: F
	}

	// 分包: B[C]E
	// fmt.Println("kmnp.mnp_stream.packKmBuf,m.size:", m.size)
	buf := kp.Malloc()
	cur := buf
	end := false
	idx := 0
	pos := 0

	if nil == buf {
		goto ErrMalloc
	}

	idx, pos = m.packKmBufBegin(buf, &mh)

	for {
		next := kp.Malloc()
		if nil == next {
			goto ErrMalloc
		}

		cur.Next = next
		next.Prev = cur
		cur = next

		end, idx, pos = m.packKmBufNext(next, &mh, idx, pos)

		if end {
			break
		}
	}

	return buf, nil

ErrMalloc:
	// 如果打包某一个帧中途出现内存池内存不足时, 则放弃已经打包的所有数据
	// 丢弃这一帧, 调用者可以按丢帧处理接下来的数据
	// 出现此情况原因: 1.设计的内存池容量不足; 2.某消费者未及时归还内存池块
	if nil != buf {
		buf.UnrefAll()
	}

	return nil, errors.New("kmnp.mnp_stream.packKmBuf [BCE] error!kp.Malloc no mem")
}

// packKmBufFull pack full
// Mnp中间格式打包成内存池格式: F包
func (m *MnpStream) packKmBufFull(kp kmpool.Pool, mh *MnpMH) (*kmpool.KmBuf, error) {
	buf := kp.Malloc()
	if nil == buf {
		return nil, errors.New("kmnp.mnp_stream.packKmBufFull error!kp.Malloc no mem")
	}

	b := buf.GetBuf()

	mnptB := b[0:MNPTsize]

	mhB := b[MNPTsize : MNPTsize+MNPMHsize]
	mh.Pack(mhB)

	dataB := b[MNPTsize+MNPMHsize:]
	pos := 0
	for i := 0; i < len(m.Packs); i++ {
		kutil.Memcpy(dataB[pos:], m.Packs[i].Data, m.Packs[i].DataLen)
		pos += m.Packs[i].DataLen
	}

	var mnpt MnpT
	mnpt.InitMedia(uint16(pos+MNPTsize+MNPMHsize), MNPfull)
	mnpt.Pack(mnptB)

	buf.Start = 0
	buf.End = pos + MNPTsize + MNPMHsize

	return buf, nil
}

// packKmBufBegin pack Begin
// Mnp中间格式打包成内存池格式: B包
func (m *MnpStream) packKmBufBegin(buf *kmpool.KmBuf, mh *MnpMH) (int, int) {

	b := buf.GetBuf()

	mnptB := b[0:MNPTsize]

	mhB := b[MNPTsize : MNPTsize+MNPMHsize]
	mh.Pack(mhB) // 写MnpMH

	// 写 h264等媒体 data
	dataB := b[MNPTsize+MNPMHsize:]
	pos := 0
	for i := 0; i < len(m.Packs); i++ {
		leftB := dataB[pos:]
		leftLen := len(leftB)

		if m.Packs[i].DataLen <= leftLen {
			kutil.Memcpy(leftB, m.Packs[i].Data, m.Packs[i].DataLen)
			pos += m.Packs[i].DataLen
		} else {
			kutil.Memcpy(leftB, m.Packs[i].Data, leftLen)
			pos += leftLen

			var mnpt MnpT
			mnpt.InitMedia(uint16(pos+MNPTsize+MNPMHsize), MNPbegin)
			mnpt.Pack(mnptB) // 写MnpT头

			buf.Start = 0
			buf.End = pos + MNPTsize + MNPMHsize

			return i, leftLen
		}
	}

	fmt.Println("kmnp.mnp_stream.packKmBufBegin error!")
	kutil.Assert(false)
	return 0, 0
}

// packKmBufNext pack continue,end
// Mnp中间格式打包成内存池格式: C,E包
func (m *MnpStream) packKmBufNext(buf *kmpool.KmBuf, mh *MnpMH, idx, idxPos int) (bool, int, int) {
	b := buf.GetBuf()

	var mnpt MnpT
	mnptB := b[0:MNPTsize]

	dataB := b[MNPTsize:]
	pos := 0
	for i := idx; i < len(m.Packs); i++ {
		leftB := dataB[pos:]
		leftLen := len(leftB)
		leftPack := m.Packs[i].DataLen - idxPos

		if leftPack <= leftLen {
			kutil.Memcpy(leftB, kutil.PointerAdd(m.Packs[i].Data, idxPos), leftPack)
			pos += leftPack
			idxPos = 0
		} else {
			kutil.Memcpy(leftB, kutil.PointerAdd(m.Packs[i].Data, idxPos), leftLen)
			pos += leftLen
			idxPos += leftLen

			mnpt.InitMedia(uint16(pos+MNPTsize), MNPcontinue)
			mnpt.Pack(mnptB)

			buf.Start = 0
			buf.End = MNPTsize + pos

			return false, i, idxPos
		}
	}

	mnpt.InitMedia(uint16(pos+MNPTsize), MNPend)
	mnpt.Pack(mnptB)

	buf.Start = 0
	buf.End = MNPTsize + pos

	return true, 0, 0
}
