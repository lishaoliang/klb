package kmnpconn

import (
	"errors"

	"github.com/lishaoliang/klb/src/knet/kmnp"
	"github.com/lishaoliang/klb/src/kutil"
	"github.com/lishaoliang/klb/src/kutil/kbuf"
)

func (m *Conn) readMd(md *kutil.Buf) (bool, error) {
	// 媒体部分不会读满
	pos := md.Pos()
	rn := int(m.readMnp.Size) - kmnp.MnpHeadSize - pos
	spare := md.SpareBy(rn)

	if len(spare) <= 0 {
		// 如果超过预置内存, 则认为溢出
		return false, errors.New("kmnpconn.conn_rmd.readMd,out of memory")
	}

	n, err := m.conn.Read(spare)
	if nil != err {
		return false, err
	}

	if 0 < n {
		md.Add(n)

		if rn <= n {
			return true, nil
		}
	}

	return false, nil
}

func (m *Conn) processMd(md *kutil.Buf) error {

	switch m.readMnp.Opt {
	case kmnp.MnpOptBegin:
		return m.processSrcMnpBegin(md)
	case kmnp.MnpOptContinue:
		return m.processSrcMNPcontinue(md)
	case kmnp.MnpOptEnd:
		return m.processSrcMNPend(md)
	case kmnp.MnpOptFull:
		return m.processSrcMNPfull(md)
	}

	return nil
}

///////////////////////////////////////////////////////////////////////////////

func (m *Conn) processSrcMNPfull(md *kutil.Buf) error {
	d := md.Data()

	var media kmnp.MnpMedia
	media.Unpack(d[:kmnp.MnpMediaSize])

	d = d[kmnp.MnpMediaSize:]

	first := true
	for {
		buf := m.conn.Malloc()
		n := kutil.Min(buf.BufLen()-kmnp.MnpHeadSize-kmnp.MnpMediaSize, len(d))
		kutil.Assert(0 < n)

		b := d[0:n]
		d = d[n:]

		kutil.Assert(0 < len(b))
		if 0 < len(d) {
			if first {
				m.processDstBegin(buf, &media, b)
			} else {
				m.processDstContinue(buf, b)
			}

			m.conn.PushMedia(buf)
		} else {
			if first {
				m.processDstFull(buf, &media, b)
			} else {
				m.processDstEnd(buf, b)
			}

			m.conn.PushMedia(buf)
			break // 此块数据处理完毕
		}

		first = false
	}

	return nil
}

func (m *Conn) processSrcMnpBegin(md *kutil.Buf) error {
	d := md.Data()

	var media kmnp.MnpMedia
	media.Unpack(d[:kmnp.MnpMediaSize])

	d = d[kmnp.MnpMediaSize:]

	first := true
	for {
		buf := m.conn.Malloc()
		n := kutil.Min(buf.BufLen()-kmnp.MnpHeadSize-kmnp.MnpMediaSize, len(d))
		kutil.Assert(0 < n)

		b := d[0:n]
		d = d[n:]

		kutil.Assert(0 < len(b))
		if 0 < len(d) {
			if first {
				m.processDstBegin(buf, &media, b)
			} else {
				m.processDstContinue(buf, b)
			}

			m.conn.PushMedia(buf)
		} else {
			if first {
				m.processDstBegin(buf, &media, b)
			} else {
				m.processDstContinue(buf, b)
			}

			m.conn.PushMedia(buf)
			break // 此块数据处理完毕
		}

		first = false
	}

	return nil
}

func (m *Conn) processSrcMNPcontinue(md *kutil.Buf) error {
	d := md.Data()

	first := true
	for {
		buf := m.conn.Malloc()
		n := kutil.Min(buf.BufLen()-kmnp.MnpHeadSize-kmnp.MnpMediaSize, len(d))
		kutil.Assert(0 < n)

		b := d[0:n]
		d = d[n:]

		kutil.Assert(0 < len(b))
		if 0 < len(d) {
			if first {
				m.processDstContinue(buf, b)
			} else {
				m.processDstContinue(buf, b)
			}

			m.conn.PushMedia(buf)
		} else {
			if first {
				m.processDstContinue(buf, b)
			} else {
				m.processDstContinue(buf, b)
			}

			m.conn.PushMedia(buf)
			break // 此块数据处理完毕
		}

		first = false
	}

	return nil
}

func (m *Conn) processSrcMNPend(md *kutil.Buf) error {
	d := md.Data()

	first := true
	for {
		buf := m.conn.Malloc()
		n := kutil.Min(buf.BufLen()-kmnp.MnpHeadSize-kmnp.MnpMediaSize, len(d))
		kutil.Assert(0 < n)

		b := d[0:n]
		d = d[n:]

		kutil.Assert(0 < len(b))
		if 0 < len(d) {
			if first {
				m.processDstContinue(buf, b)
			} else {
				m.processDstContinue(buf, b)
			}

			m.conn.PushMedia(buf)
		} else {
			if first {
				m.processDstEnd(buf, b)
			} else {
				m.processDstEnd(buf, b)
			}

			m.conn.PushMedia(buf)
			break // 此块数据处理完毕
		}

		first = false
	}

	return nil
}

///////////////////////////////////////////////////////////////////////////////

func (m *Conn) processDstFull(dst kbuf.KBuf, media *kmnp.MnpMedia, d []byte) {
	wb := dst.Buf()

	kutil.Assert(0 < len(d))
	kutil.Assert(len(d)+kmnp.MnpHeadSize+kmnp.MnpMediaSize <= len(wb))
	kutil.Assert(len(wb) <= mnpPackMax)

	var mnp kmnp.Mnp
	mnp.Default()
	mnp.Packtype = kmnp.MnpPackMedia
	mnp.Opt = kmnp.MnpOptFull
	mnp.Size = uint16(kmnp.MnpHeadSize + kmnp.MnpMediaSize + len(d))

	mnp.Pack(wb)
	media.Pack(wb[kmnp.MnpHeadSize : kmnp.MnpHeadSize+kmnp.MnpMediaSize])
	copy(wb[kmnp.MnpHeadSize+kmnp.MnpMediaSize:], d)

	dst.SetPos(0, int(mnp.Size))
}

func (m *Conn) processDstBegin(dst kbuf.KBuf, media *kmnp.MnpMedia, d []byte) {
	wb := dst.Buf()

	kutil.Assert(0 < len(d))
	kutil.Assert(len(d)+kmnp.MnpHeadSize+kmnp.MnpMediaSize <= len(wb))
	kutil.Assert(len(wb) <= mnpPackMax)

	var mnp kmnp.Mnp
	mnp.Default()
	mnp.Packtype = kmnp.MnpPackMedia
	mnp.Opt = kmnp.MnpOptBegin
	mnp.Size = uint16(kmnp.MnpHeadSize + kmnp.MnpMediaSize + len(d))

	mnp.Pack(wb)
	media.Pack(wb[kmnp.MnpHeadSize : kmnp.MnpHeadSize+kmnp.MnpMediaSize])
	copy(wb[kmnp.MnpHeadSize+kmnp.MnpMediaSize:], d)

	dst.SetPos(0, int(mnp.Size))
}

func (m *Conn) processDstContinue(dst kbuf.KBuf, d []byte) {
	wb := dst.Buf()

	kutil.Assert(0 < len(d))
	kutil.Assert(len(d)+kmnp.MnpHeadSize <= len(wb))
	kutil.Assert(len(wb) <= mnpPackMax)

	var mnp kmnp.Mnp
	mnp.Default()
	mnp.Packtype = kmnp.MnpPackMedia
	mnp.Opt = kmnp.MnpOptContinue
	mnp.Size = uint16(kmnp.MnpHeadSize + len(d))

	mnp.Pack(wb)
	copy(wb[kmnp.MnpHeadSize:], d)

	dst.SetPos(0, int(mnp.Size))
}

func (m *Conn) processDstEnd(dst kbuf.KBuf, d []byte) {
	wb := dst.Buf()

	kutil.Assert(0 < len(d))
	kutil.Assert(len(d)+kmnp.MnpHeadSize <= len(wb))
	kutil.Assert(len(wb) <= mnpPackMax)

	var mnp kmnp.Mnp
	mnp.Default()
	mnp.Packtype = kmnp.MnpPackMedia
	mnp.Opt = kmnp.MnpOptEnd
	mnp.Size = uint16(kmnp.MnpHeadSize + len(d))

	mnp.Pack(wb)
	copy(wb[kmnp.MnpHeadSize:], d)

	dst.SetPos(0, int(mnp.Size))
}
