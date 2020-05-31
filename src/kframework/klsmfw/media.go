package klsmfw

import (
	"github.com/lishaoliang/klb/src/knet/kmnp"
	"github.com/lishaoliang/klb/src/kutil/kbuf"
)

// workerMedia workerMedia
func (m *lsmfw) workerMedia() error {
	defer m.wg.Done()

	for {
		select {
		case md := <-m.ncm.RecvMedia():
			if nil != md.Buf {
				md.Buf.UnrefNext()
			}
		case <-m.ctx.Done():
			return m.ctx.Err() // 超时或强制退出
		}
	}
}

func (m *lsmfw) kbufIsKey(buf kbuf.KBuf) bool {

	d := buf.Data()

	var mnp kmnp.Mnp
	mnp.Unpack(d)

	if kmnp.MnpOptBegin == mnp.Opt || kmnp.MnpOptFull == mnp.Opt {
		d := d[kmnp.MnpHeadSize:]

		var md kmnp.MnpMedia
		md.Unpack(d)

		if kmnp.MnpDtypeH264 == md.Dtype || kmnp.MnpDtypeH265 == md.Dtype {
			if kmnp.MnpVtypeI == md.Vtype {
				return true
			}
		}
	}

	return false
}

// DispatchMedia DispatchMedia
func (m *lsmfw) DispatchMedia(chnn, sidx uint32, buf kbuf.KBuf) bool {
	m.stateMutex.Lock()
	defer m.stateMutex.Unlock()

	//fmt.Println("DispatchMedia:", chnn, sidx)

	for k, v := range m.cstates.mapConn.Map() {
		if status, ok := v.(*connStatus); ok {

			for i := 0; i < len(status.stream); i++ {
				if chnn == status.stream[i].chnn && sidx == status.stream[i].sidx {
					if status.stream[i].needKey && m.kbufIsKey(buf) {
						status.stream[i].needKey = false
					}

					if !status.stream[i].needKey {
						m.ncm.SendMedia(k, buf)
					}
				}
			}
		}
	}

	return true
}
