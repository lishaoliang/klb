///////////////////////////////////////////////////////////////////////////
//	Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file	mnp_stream.go
/// @author	lishaoliang
/// @brief	mnp stream
///////////////////////////////////////////////////////////////////////////

package kmnp

import (
	"errors"
	"fmt"
	"unsafe"

	"github.com/lishaoliang/klb/src/kutil"
	"github.com/lishaoliang/klb/src/kutil/kbuf"
)

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
	Dtype uint32 // 媒体数据类型: klb_mnp_dtype_e
	Vtype uint8  // 视频(video type): klb_mnp_vtype_e
	Time  int64  // 时间戳

	Packs []MnpPack // 数据
	Size  int       // 总大小: 附加数据 + 媒体数据
}

///////////////////////////////////////////////////////////////////////////////

// PackKBuf PackKBuf
func (m *MnpStream) PackKBuf(malloc kbuf.Malloc, chnn, sidx uint32) (kbuf.KBuf, error) {
	return m.packKBuf(malloc, chnn, sidx)
}

// PackByte pack
// 打包数据成[]byte
//func (m *MnpStream) PackByte(chnn, sidx uint32) ([]byte, error) {
//	return nil, errors.New("kmnp.mnp_stream.PackByte error!to do")
//}

///////////////////////////////////////////////////////////////////////////////

// 64K - 8
// mnp.Size为: [0,65535], 这里在末尾空闲几个字节
// const mnpSize64KLess = 1024*64 - 8

// packKBuf pack
// Mnp中间格式打包成内存池格式
// 如果内存池内存不足, 则返回nil, error
func (m *MnpStream) packKBuf(malloc kbuf.Malloc, chnn, sidx uint32) (kbuf.KBuf, error) {
	buf := malloc.Malloc()

	var md MnpMedia
	md.Size = uint32(MnpMediaSize + m.Size)
	md.Dtype = uint32(m.Dtype)
	md.Chnn = chnn
	md.Sidx = sidx
	md.Time = m.Time
	md.Vtype = uint8(m.Vtype)

	// 分包: B[C]E
	// fmt.Println("kmnp.mnp_stream.packKBuf,m.size:", m.size)
	cur := buf
	end := false
	idx := 0
	pos := 0

	if nil == buf {
		goto ErrMalloc
	}

	if MnpHeadSize+MnpMediaSize+m.Size <= buf.BufLen() {
		return m.packKBufFull(buf, &md) // 完整包: F
	}

	if nil == buf {
		goto ErrMalloc
	}

	idx, pos = m.packKBufBegin(buf, &md)

	for {
		next := malloc.Malloc()
		if nil == next {
			goto ErrMalloc
		}

		cur.SetNext(next)
		next.SetPrev(cur)
		cur = next

		end, idx, pos = m.packKBufNext(next, &md, idx, pos)

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

// packKBufFull pack full
// Mnp中间格式打包成内存池格式: F包
func (m *MnpStream) packKBufFull(buf kbuf.KBuf, md *MnpMedia) (kbuf.KBuf, error) {

	b := buf.Buf()

	mnpB := b[0:MnpHeadSize]

	mdB := b[MnpHeadSize : MnpHeadSize+MnpMediaSize]
	md.Pack(mdB)

	dataB := b[MnpHeadSize+MnpMediaSize:]
	pos := 0
	for i := 0; i < len(m.Packs); i++ {
		kutil.Memcpy(dataB[pos:], m.Packs[i].Data, m.Packs[i].DataLen)
		pos += m.Packs[i].DataLen
	}

	var mnp Mnp
	mnp.Default()
	mnp.Size = uint16(MnpHeadSize + MnpMediaSize + pos)
	mnp.Opt = MnpOptFull
	mnp.Packtype = MnpPackMedia

	mnp.Pack(mnpB)

	buf.SetPos(0, MnpHeadSize+MnpMediaSize+pos)

	return buf, nil
}

// packKBufBegin pack Begin
// Mnp中间格式打包成内存池格式: B包
func (m *MnpStream) packKBufBegin(buf kbuf.KBuf, md *MnpMedia) (int, int) {

	b := buf.Buf()

	mnpB := b[0:MnpHeadSize]

	mhB := b[MnpHeadSize : MnpHeadSize+MnpMediaSize]
	md.Pack(mhB) // 写MnpMH

	// 写 h264等媒体 data
	dataB := b[MnpHeadSize+MnpMediaSize:]
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

			var mnp Mnp
			mnp.Default()

			mnp.Size = uint16(MnpHeadSize + MnpMediaSize + pos)
			mnp.Opt = MnpOptBegin
			mnp.Packtype = MnpPackMedia

			mnp.Pack(mnpB) // 写Mnp头

			buf.SetPos(0, MnpHeadSize+MnpMediaSize+pos)

			return i, leftLen
		}
	}

	fmt.Println("kmnp.mnp_stream.packKmBufBegin error!")
	kutil.Assert(false)
	return 0, 0
}

// packKBufNext pack continue,end
// Mnp中间格式打包成内存池格式: C,E包
func (m *MnpStream) packKBufNext(buf kbuf.KBuf, md *MnpMedia, idx, idxPos int) (bool, int, int) {
	b := buf.Buf()

	var mnp Mnp
	mnp.Default()

	mnpB := b[0:MnpHeadSize]

	dataB := b[MnpHeadSize:]
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

			mnp.Size = uint16(MnpHeadSize + pos)
			mnp.Opt = MnpOptContinue
			mnp.Packtype = MnpPackMedia

			mnp.Pack(mnpB)

			buf.SetPos(0, MnpHeadSize+pos)

			return false, i, idxPos
		}
	}

	mnp.Size = uint16(MnpHeadSize + pos)
	mnp.Opt = MnpOptEnd
	mnp.Packtype = MnpPackMedia

	mnp.Pack(mnpB)

	buf.SetPos(0, MnpHeadSize+pos)

	return true, 0, 0
}
