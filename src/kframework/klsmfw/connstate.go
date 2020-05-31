package klsmfw

import (
	"github.com/lishaoliang/klb/src/kutil"
)

type chnnSidx struct {
	chnn    uint32 // 通道
	sidx    uint32 // 流序号
	needKey bool   // 需要关键帧
}

type connStatus struct {
	protocol string
	stream   []chnnSidx
}

// connState connect state
type connState struct {
	mapConn *kutil.MapIface
}

func createConnState() *connState {
	var m connState

	m.mapConn = kutil.NewMapIface("", "", 12)
	return &m
}

func (m *connState) Destroy() {
	//m.mapConn.Destroy()
}

func (m *connState) NewConnState(protocol string) string {
	var status connStatus
	status.protocol = protocol

	name := m.mapConn.Push(&status)
	return name
}

func (m *connState) RemoveConnState(name string) {
	m.mapConn.Remove(name)
}

func (m *connState) Find(name string) *connStatus {
	item := m.mapConn.Find(name)
	if nil == item {
		return nil
	}

	if status, ok := item.(*connStatus); ok {
		return status
	}

	kutil.Assert(false)
	return nil
}

func (m *connState) OpenStream(name string, chnn, sidx uint32) bool {
	status := m.Find(name)
	if nil == status {
		return false
	}

	for i := 0; i < len(status.stream); i++ {
		if chnn == status.stream[i].chnn && sidx == status.stream[i].sidx {
			return true
		}
	}

	status.stream = append(status.stream, chnnSidx{chnn, sidx, true})

	return true
}

func (m *connState) CloseStream(name string, chnn, sidx uint32) bool {
	status := m.Find(name)
	if nil == status {
		return false
	}

	return true
}
