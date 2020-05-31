package ksmcfw

import (
	"github.com/lishaoliang/klb/src/knet/kmnp"
	"github.com/lishaoliang/klb/src/kutil"
	"github.com/lishaoliang/klb/src/kutil/kbuf"
)

type chnnSidx struct {
	chnn uint32
	sidx uint32
}

// sbind stream bind
// 连接名称与通道/流序号的绑定关系
type sbind struct {
	mapInfo *kutil.MapIface
}

func createSbind() *sbind {
	var m sbind

	m.mapInfo = kutil.NewMapIface("", "", 12)

	return &m
}

func (m *sbind) Destroy() {
	//m.mapInfo.Destroy()
}

func (m *sbind) Bind(name string, chnn, sidx uint32) {
	m.Unbind(name)
	m.mapInfo.PushByName(name, &chnnSidx{chnn, sidx})
}

func (m *sbind) Unbind(name string) {
	m.mapInfo.Remove(name)
}

func (m *sbind) Modify(name string, buf kbuf.KBuf) (ok bool, chnn, sidx uint32) {
	cs := m.find(name)
	if nil == cs {
		return false, 0, 0
	}

	// 遍历数据, 并将其中的通道/流序号修正
	cur := buf
	for nil != cur {
		d := cur.Data()

		var mnp kmnp.Mnp
		mnp.Unpack(d)

		kutil.Assert(kmnp.MnpPackMedia == mnp.Packtype)
		if kmnp.MnpPackMedia == mnp.Packtype &&
			(kmnp.MnpOptBegin == mnp.Opt || kmnp.MnpOptFull == mnp.Opt) {

			// 媒体包
			// 且是开始包或完整包
			var md kmnp.MnpMedia
			md.Unpack(d[kmnp.MnpHeadSize : kmnp.MnpHeadSize+kmnp.MnpMediaSize])

			md.Chnn = cs.chnn
			md.Sidx = cs.sidx

			md.Pack(d[kmnp.MnpHeadSize : kmnp.MnpHeadSize+kmnp.MnpMediaSize])
		}

		cur = cur.Next()
	}

	return true, cs.chnn, cs.sidx
}

func (m *sbind) find(name string) *chnnSidx {
	item := m.mapInfo.Find(name)
	if nil == item {
		return nil
	}

	if cs, ok := item.(*chnnSidx); ok {
		return cs
	}

	kutil.Assert(false)
	return nil
}
