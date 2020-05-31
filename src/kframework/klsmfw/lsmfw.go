package klsmfw

import (
	"context"
	"net"
	"sync"

	"github.com/lishaoliang/klb/src/knet"
)

// lsmfw lsmfw
type lsmfw struct {
	ctx    context.Context    // 同步ctx
	cancel context.CancelFunc // 同步cancel
	wg     sync.WaitGroup     // 同步wg

	ncm        *knet.Ncm  // 连接管理模块
	nsp        *knet.Nsp  // 监听模块
	env        *luaEnv    // lua环境
	cstates    *connState // 连接状态
	stateMutex sync.Mutex // cstates锁
}

func onNspListenConn(ptr interface{}, conn net.Conn, protocol string, data []byte) int {
	if m, ok := ptr.(*lsmfw); ok {

		m.stateMutex.Lock()
		name := m.cstates.NewConnState(protocol)

		// ncm.Push() 成功, 则conn由ncm托管
		// 失败, 则conn在后续流程关闭
		if m.ncm.Push(name, protocol, conn, data) {
			m.stateMutex.Unlock()

			//fmt.Println("klsmfw.lsmfw.onNspListenConn,New Connect", name, protocol)
			return 0
		}

		m.cstates.RemoveConnState(name)
		m.stateMutex.Unlock()
	}

	//fmt.Println("klsmfw.lsmfw.onNspListenConn", protocol)
	conn.Close()
	return 0
}

// lsmfwCreate lsmfwCreate
func lsmfwCreate() *lsmfw {
	var m lsmfw

	m.ctx, m.cancel = context.WithCancel(context.Background())

	m.ncm = knet.CreateNcm()
	m.nsp = knet.CreateNsp(onNspListenConn, &m)
	m.env = luaEnvCreate()
	m.cstates = createConnState()

	return &m
}

// Destroy Destroy
func (m *lsmfw) Destroy() {
	m.nsp.Destroy() // 先将监听源关闭

	m.Stop()

	m.cstates.Destroy()
	m.env.Destroy()
	m.ncm.Destroy()
}

func (m *lsmfw) Init(cfg string) error {
	return nil
}

func (m *lsmfw) Quit() {

}

func (m *lsmfw) Start(name, entry, preload, loader string) error {

	m.env.Open(name, entry, preload, loader)

	m.wg.Add(1)
	go m.workerMedia()

	m.wg.Add(1)
	go m.workerTextEx()

	return nil
}

func (m *lsmfw) Stop() {
	m.cancel()
	m.wg.Wait()

	m.env.Close()
}

func (m *lsmfw) Listen(name, network, address string) error {
	return m.nsp.Listen(name, network, address)
}

func (m *lsmfw) ListenTLS(name, network, address, certFile, keyFile string) error {
	return m.nsp.ListenTLS(name, network, address, certFile, keyFile)
}

func (m *lsmfw) CloseListen(name string) error {
	return m.nsp.Close(name)
}

// GetLuaEnv GetLuaEnv
func (m *lsmfw) GetLuaEnv() *luaEnv {
	return m.env
}

// SwitchProtocol SwitchProtocol
// 更换协议类型
func (m *lsmfw) SwitchProtocol(name, dstProtocol string, head, body []byte) bool {
	//m.stateMutex.Lock()
	//m.cstates.RemoveConnState(name)
	//m.stateMutex.Unlock()

	b := m.ncm.SwitchProtocol(name, dstProtocol, head, body)
	return b
}

// CloseConn CloseConn
func (m *lsmfw) CloseConn(name, protocol string) {
	m.close(name, protocol)
}

// SendTxt SendTxt
func (m *lsmfw) SendTxt(name, protocol string, sequence, uid uint32, extra, txt []byte) bool {
	return m.ncm.SendTxt(name, sequence, uid, extra, txt)
}

// SendFile SendFile
func (m *lsmfw) SendFile(name, protocol string, sequence, uid uint32, extra, txt, path []byte) bool {
	return m.ncm.SendFile(name, sequence, uid, extra, txt, path)
}

// OpenStream OpenStream
func (m *lsmfw) OpenStream(name, protocol string, chnn, sidx uint32) bool {
	m.stateMutex.Lock()
	defer m.stateMutex.Unlock()

	return m.cstates.OpenStream(name, chnn, sidx)
}

// CloseStream CloseStream
func (m *lsmfw) CloseStream(name, protocol string, chnn, sidx uint32) bool {
	m.stateMutex.Lock()
	defer m.stateMutex.Unlock()

	return m.cstates.CloseStream(name, chnn, sidx)
}

//////////////////////////////////////////////////////////////////////////////////////////////

// close connect
// 关闭连接
func (m *lsmfw) close(name, protocol string) {
	m.stateMutex.Lock()
	m.cstates.RemoveConnState(name)
	m.stateMutex.Unlock()

	m.ncm.Close(name)
}
