package ksimulate

import (
	"context"
	"sync"
	"time"
)

type simulate struct {
	ctx    context.Context    // 同步ctx
	cancel context.CancelFunc // 同步cancel
	wg     sync.WaitGroup     // 同步wg

	chnnNum int
	chnns   []*channel
}

func (m *simulate) Init(cfg string) error {

	m.ctx, m.cancel = context.WithCancel(context.Background())

	m.chnnNum = 4

	for i := 0; i < m.chnnNum; i++ {
		chnn := openChnn(uint32(i))
		m.chnns = append(m.chnns, chnn)
	}

	return nil
}

func (m *simulate) Quit() {
	for i := 0; i < m.chnnNum; i++ {
		m.chnns[i].Close()
	}

	m.chnns = nil
	m.chnnNum = 0
}

func (m *simulate) Start() error {
	m.wg.Add(1)
	go m.worker()

	return nil
}

func (m *simulate) Stop() {
	m.cancel()
	m.wg.Wait()
}

func (m *simulate) Command(cmd string, chnn, sidx int64, lparam, wparam string) (int64, error) {
	return 0, nil
}

func (m *simulate) worker() error {
	defer m.wg.Done()

	interval := (time.Duration)(time.Millisecond * 33)
	timer := time.NewTimer(interval)
	defer timer.Stop()

	for {
		select {
		case <-timer.C:
			timer.Reset(interval)

			now := time.Now()
			t := now.Unix()*1000000 + int64(now.Nanosecond()/1000) // 微妙

			for i := 0; i < m.chnnNum; i++ {
				m.chnns[i].OnTimer(t)
			}

		case <-m.ctx.Done():
			return m.ctx.Err() // 超时或强制退出
		}
	}
}
