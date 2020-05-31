package kmnpconn

import (
	"errors"

	"github.com/lishaoliang/klb/src/kutil"
)

func (m *Conn) readHead(h *kutil.Buf) (bool, error) {
	spare := h.Spare()
	if len(spare) <= 0 {
		return false, errors.New("kmnpconn.conn_rtxt.readHead,out of memory")
	}

	n, err := m.conn.Read(spare)
	if nil != err {
		return false, err
	}

	if 0 < n {
		h.Add(n)

		if h.IsFull() {
			// 头部接收完毕, 切换状态
			m.readMnp.Unpack(h.Data())
			if !m.readMnp.IsOk() {
				return false, errors.New("kmnpconn.conn_rtxt.readHead,check mnp head")
			}

			return true, nil
		}
	}

	return false, nil
}

func (m *Conn) readTxt(buf *kutil.Buf) (bool, error) {
	spare := buf.Spare()
	if len(spare) <= 0 {
		return false, errors.New("kmnpconn.conn_rtxt.readTxt,out of memory")
	}

	n, err := m.conn.Read(spare)
	if nil != err {
		return false, err
	}

	if 0 < n {
		buf.Add(n)

		if buf.IsFull() {
			return true, nil
		}
	}

	return false, nil
}

func (m *Conn) readBin(buf *kutil.Buf) (bool, error) {
	spare := buf.Spare()
	if len(spare) <= 0 {
		return false, errors.New("kmnpconn.conn_rtxt.readBin,out of memory")
	}

	n, err := m.conn.Read(spare)
	if nil != err {
		return false, err
	}

	if 0 < n {
		buf.Add(n)

		if buf.IsFull() {
			return true, nil
		}
	}

	return false, nil
}
