package ksmcfw

import (
	"context"
	"sync"

	"github.com/lishaoliang/klb/src/kframework/klsmfw"
	"github.com/lishaoliang/klb/src/knet"
	"github.com/lishaoliang/klb/src/knet/kmnp"
	"github.com/lishaoliang/klb/src/kutil/kpool"
)

// smcfw smcfw
type smcfw struct {
	ctx    context.Context    // 同步ctx
	cancel context.CancelFunc // 同步cancel
	wg     sync.WaitGroup     // 同步wg

	ncm       *knet.Ncm
	env       *luaEnv
	bind      *sbind
	bindMutex sync.Mutex // bind锁
}

// smcfwCreate smcfwCreate
func smcfwCreate() *smcfw {
	var m smcfw

	m.ctx, m.cancel = context.WithCancel(context.Background())

	m.ncm = knet.CreateNcm()
	m.env = luaEnvCreate()
	m.bind = createSbind()

	return &m
}

// Destroy Destroy
func (m *smcfw) Destroy() {
	m.Stop()

	m.env.Destroy()
	m.ncm.Destroy()
	m.bind.Destroy()
}

func (m *smcfw) Init(cfg string) error {
	return nil
}

func (m *smcfw) Quit() {

}

func (m *smcfw) Start(name, entry, preload, loader string) error {

	m.env.Open(name, entry, preload, loader)

	m.wg.Add(1)
	go m.workerMedia()

	m.wg.Add(1)
	go m.workerTextEx()

	return nil
}

func (m *smcfw) Stop() {
	m.cancel()
	m.wg.Wait()

	m.env.Close()
}

// GetLuaEnv GetLuaEnv
func (m *smcfw) GetLuaEnv() *luaEnv {
	return m.env
}

// Connect connect
func (m *smcfw) Dial(name, protocol, network, address string) {
	m.ncm.Dial(name, protocol, network, address)
}

// SendTxt SendTxt
func (m *smcfw) SendTxt(name, protocol string, sequence, uid uint32, extra, txt []byte) bool {
	return m.ncm.SendTxt(name, sequence, uid, extra, txt)
}

// Bind Bind
func (m *smcfw) Bind(name string, chnn, sidx uint32) {
	m.bindMutex.Lock()
	defer m.bindMutex.Unlock()

	m.bind.Bind(name, chnn, sidx)
}

// Unbind Unbind
func (m *smcfw) Unbind(name string) {
	m.bindMutex.Lock()
	defer m.bindMutex.Unlock()

	m.bind.Unbind(name)
}

func (m *smcfw) StreamCb(chnn, sidx uint32, mnps kmnp.MnpStream) {
	buf, err := mnps.PackKBuf(kpool.Default(), chnn, sidx)

	if nil != err {
		return
	}

	klsmfw.PushMedia(chnn, sidx, buf)
	buf.UnrefNext()
}
