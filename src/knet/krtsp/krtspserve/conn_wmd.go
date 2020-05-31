package krtspserve

import (
	"github.com/lishaoliang/klb/src/knet/kconn"
	"github.com/lishaoliang/klb/src/knet/kmnp"
	"github.com/lishaoliang/klb/src/kutil"
)

func (m *Conn) writeMedia(data *kconn.ConnWrite) error {
	cur := data.Buf
	for nil != cur {
		d := cur.Data()
		var mnp kmnp.Mnp
		mnp.Unpack(d)

		d = d[kmnp.MnpHeadSize:]
		switch mnp.Opt {
		case kmnp.MnpOptFull:
			if err := m.writeOptFull(&mnp, d); nil != err {
				return err
			}
		case kmnp.MnpOptBegin:
			if err := m.writeOptBegin(&mnp, d); nil != err {
				return err
			}
		case kmnp.MnpOptContinue:
			if err := m.writeOptContinue(&mnp, d); nil != err {
				return err
			}
		case kmnp.MnpOptEnd:
			if err := m.writeOptEnd(&mnp, d); nil != err {
				return err
			}
		default:
			kutil.Assert(false)
		}

		cur = cur.Next()
	}

	return nil
}

func (m *Conn) writeOptFull(mnp *kmnp.Mnp, d []byte) error {
	var md kmnp.MnpMedia
	md.Unpack(d)

	d = d[kmnp.MnpMediaSize:]

	return nil
}

func (m *Conn) writeOptBegin(mnp *kmnp.Mnp, d []byte) error {
	var md kmnp.MnpMedia
	md.Unpack(d)

	d = d[kmnp.MnpMediaSize:]

	return nil
}

func (m *Conn) writeOptContinue(mnp *kmnp.Mnp, d []byte) error {
	return nil
}

func (m *Conn) writeOptEnd(mnp *kmnp.Mnp, d []byte) error {
	return nil
}
