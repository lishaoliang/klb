package ksimulate

import (
	"github.com/lishaoliang/klb/src/kfile"
	"github.com/lishaoliang/klb/src/knet/kflv/kflvreader"
	"github.com/lishaoliang/klb/src/knet/kmnp"
	"github.com/lishaoliang/klb/src/kutil"
)

type channel struct {
	id uint32

	v1  kfile.Reader
	tV1 int64

	v2  kfile.Reader
	tV2 int64
}

func openChnn(id uint32) *channel {
	var m channel

	m.id = id

	m.v1, _ = kflvreader.Open("./111.flv")
	m.v2, _ = kflvreader.Open("./111.flv")

	//md, d, _ := m.v1.ReadNext()

	//fmt.Println("**", md.Time, md.Dtype, d[0:kutil.Min(32, len(d))])

	return &m
}

func (m *channel) Close() {
	if nil != m.v2 {
		m.v2.Close()
		m.v2 = nil
	}

	if nil != m.v1 {
		m.v1.Close()
		m.v1 = nil
	}
}

func (m *channel) OnTimer(now int64) {
	m.onTimerV1(now)
	m.onTimerV2(now)
}

func (m *channel) onTimerV1(now int64) {
	if nil == m.v1 {
		return
	}

	var md kmnp.MnpMedia
	var d []byte
	var err error

	md, d, err = m.v1.ReadNext()
	if nil != err {
		m.v1.Reset()
		md, d, _ = m.v1.ReadNext()
	}

	if kmnp.MnpDtypeNil != md.Dtype {
		kutil.Assert(0 < len(d))

		md.Chnn = m.id
		md.Sidx = kmnp.MnpSidxV1
		md.Time = now

		pushStream(md, d)
	}

	m.tV1 = now
}

func (m *channel) onTimerV2(now int64) {
	if nil == m.v2 {
		return
	}

	var md kmnp.MnpMedia
	var d []byte
	var err error

	md, d, err = m.v2.ReadNext()
	if nil != err {
		m.v2.Reset()
		md, d, _ = m.v2.ReadNext()
	}

	if kmnp.MnpDtypeNil != md.Dtype {
		kutil.Assert(0 < len(d))

		md.Chnn = m.id
		md.Sidx = kmnp.MnpSidxV2
		md.Time = now

		pushStream(md, d)
	}

	m.tV2 = now
}
