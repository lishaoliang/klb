///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   stream.go
/// @author lishaoliang
/// @brief	stream
///////////////////////////////////////////////////////////////////////////

package kgstream

import (
	"context"
	"sync"
	"time"

	"github.com/lishaoliang/klb/src/knet/kmnp"
)

type stream struct {
	ctx    context.Context    // 同步ctx
	cancel context.CancelFunc // 同步cancel
	wg     sync.WaitGroup     // 同步wg

	v1 *fh264
	v2 *fh264
}

func streamCreate(path1, path2 string) *stream {
	var s stream
	s.ctx, s.cancel = context.WithCancel(context.Background())

	s.v1, _ = fh264Load(path1)
	s.v2, _ = fh264Load(path2)

	return &s
}

///////////////////////////////////////////////////////////////////////////////

// Destroy destroy
func (s *stream) Destroy() {
	s.Stop()

	s.v1.Unload()
	s.v2.Unload()
}

// Start start
func (s *stream) Start() {
	s.wg.Add(1)
	go s.worker()
}

// Stop stop
func (s *stream) Stop() {
	s.cancel()
	s.wg.Wait()
}

///////////////////////////////////////////////////////////////////////////////

func (s *stream) streamV1(t int64) {
	if nil == s.v1 {
		return
	}

	frame, err := s.v1.GetNext()
	if nil == err {
		//fmt.Println("v1:", len(sps), len(pps), len(img), t)
		cbStreamH264(t, 0, kmnp.MNPsidxV1, frame)
	}
}

func (s *stream) streamV2(t int64) {
	if nil == s.v2 {
		return
	}

	frame, err := s.v2.GetNext()
	if nil == err {
		//fmt.Println("v2:", len(sps), len(pps), len(img), t)
		cbStreamH264(t, 0, kmnp.MNPsidxV2, frame)
	}
}

func (s *stream) worker() error {
	defer s.wg.Done()

	interval := (time.Duration)(time.Millisecond) * (time.Duration)(33)
	timer := time.NewTimer(interval)
	defer timer.Stop()

	t := int64(0)

	for {
		select {
		case <-timer.C:
			s.streamV1(t)
			s.streamV2(t)

			t += 33333
			timer.Reset(interval)
		case <-s.ctx.Done():
			return s.ctx.Err() // 超时或强制退出
		}
	}
}
