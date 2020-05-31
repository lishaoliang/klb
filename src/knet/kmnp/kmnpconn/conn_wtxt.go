package kmnpconn

import (
	"bytes"

	"github.com/lishaoliang/klb/src/knet/kconn"
	"github.com/lishaoliang/klb/src/knet/kmnp"
	"github.com/lishaoliang/klb/src/kutil"
)

const mnpPackMax = 32 * 1024
const mnpPackTxtMax = mnpPackMax - kmnp.MnpHeadSize - kmnp.MnpTextSize
const mnpPackBinMax = mnpPackMax - kmnp.MnpHeadSize - kmnp.MnpBinSize

// writeData writeData
func (m *Conn) writeData(data *kconn.ConnWrite) error {
	switch data.Packtype {
	case kconn.DataHeart:
		return m.writeHeart()
	case kconn.DataTxt:
		return m.writeTxt(data)
	case kconn.DataBin:
		return m.writeBin(data)
	case kconn.DataMedia:
		return m.writeMedia(data)
	default:
		// 其他不处理
	}

	return nil
}

// writeHeart writeHeart
func (m *Conn) writeHeart() error {
	var mnp kmnp.Mnp
	mnp.Default()

	return m.conn.Write(mnp.Pack(nil))
}

// 暂简单处理 data.Extra + data.Data分包
func (m *Conn) writeTxt(data *kconn.ConnWrite) error {
	sequence := data.Sequence
	uid := data.UID
	head := data.Extra
	body := data.Data

	d := bytes.Join([][]byte{head, body}, nil)
	first := true

	for {
		n := kutil.Min(mnpPackTxtMax, len(d))
		b := d[:n]
		d = d[n:]

		end := true
		if 0 < len(d) {
			end = false
		}

		if first {
			var mnp kmnp.Mnp
			mnp.Default()
			mnp.Packtype = kmnp.MnpPackText
			mnp.Size = uint16(kmnp.MnpHeadSize + kmnp.MnpTextSize + n)
			mnp.Opt = kmnp.MnpOptFull

			if !end {
				mnp.Opt = kmnp.MnpOptBegin
			}

			var txt kmnp.MnpText
			txt.Default()
			txt.Size = uint32(kmnp.MnpTextSize + len(head) + len(body))
			txt.Extra = uint32(len(head))
			txt.Sequence = sequence
			txt.UID = uid

			if err := m.conn.Write(mnp.Pack(nil)); nil != err {
				return err
			}

			if err := m.conn.Write(txt.Pack(nil)); nil != err {
				return err
			}

			if err := m.conn.Write(b); nil != err {
				return err
			}
		} else {
			var mnp kmnp.Mnp
			mnp.Default()
			mnp.Packtype = kmnp.MnpPackText
			mnp.Size = uint16(kmnp.MnpHeadSize + n)
			mnp.Opt = kmnp.MnpOptContinue

			if end {
				mnp.Opt = kmnp.MnpOptEnd
			}

			if err := m.conn.Write(mnp.Pack(nil)); nil != err {
				return err
			}

			if err := m.conn.Write(b); nil != err {
				return err
			}
		}

		if end {
			break
		}

		first = false
	}

	return nil
}

func (m *Conn) writeBin(data *kconn.ConnWrite) error {
	sequence := data.Sequence
	uid := data.UID
	head := data.Extra
	body := data.Data

	d := bytes.Join([][]byte{head, body}, nil)
	first := true

	for {
		n := kutil.Min(mnpPackBinMax, len(d))
		b := d[:n]
		d = d[n:]

		end := true
		if 0 < len(d) {
			end = false
		}

		if first {
			var mnp kmnp.Mnp
			mnp.Default()
			mnp.Packtype = kmnp.MnpPackBin
			mnp.Size = uint16(kmnp.MnpHeadSize + kmnp.MnpBinSize + n)
			mnp.Opt = kmnp.MnpOptFull

			if !end {
				mnp.Opt = kmnp.MnpOptBegin
			}

			var bin kmnp.MnpBin
			bin.Default()
			bin.Size = uint32(kmnp.MnpBinSize + len(head) + len(body))
			bin.Extra = uint32(len(head))
			bin.Sequence = sequence
			bin.UID = uid

			if err := m.conn.Write(mnp.Pack(nil)); nil != err {
				return err
			}

			if err := m.conn.Write(bin.Pack(nil)); nil != err {
				return err
			}

			if err := m.conn.Write(b); nil != err {
				return err
			}
		} else {
			var mnp kmnp.Mnp
			mnp.Default()
			mnp.Packtype = kmnp.MnpPackBin
			mnp.Size = uint16(kmnp.MnpHeadSize + n)
			mnp.Opt = kmnp.MnpOptContinue

			if end {
				mnp.Opt = kmnp.MnpOptEnd
			}

			if err := m.conn.Write(mnp.Pack(nil)); nil != err {
				return err
			}

			if err := m.conn.Write(b); nil != err {
				return err
			}
		}

		if end {
			break
		}

		first = false
	}

	return nil
}

func (m *Conn) writeMedia(data *kconn.ConnWrite) error {
	cur := data.Buf
	for nil != cur {
		d := cur.Data()
		kutil.Assert(len(d) <= mnpPackMax)

		if err := m.conn.Write(d); nil != err {
			return err
		}

		cur = cur.Next()
	}

	return nil
}
