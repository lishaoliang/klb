///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   serve.go
/// @author lishaoliang
/// @brief	serve
///////////////////////////////////////////////////////////////////////////

package klisten

import (
	"errors"
	"sync"

	"github.com/lishaoliang/klb/src/kutil"
)

// serve serve
type serve struct {
	lns *kutil.MapIface

	mutex sync.Mutex
}

func serveInit(m *serve) {
	m.lns = kutil.MapIfaceCreate("", "", 0)
}

func serveQuit(m *serve) {

}

func (m *serve) push(name string, l *listen) error {
	m.mutex.Lock()
	defer m.mutex.Unlock()

	if m.lns.PushByName(name, l) {
		l.start()
		return nil
	}

	return errors.New("")
}

func (m *serve) close(name string) error {
	m.mutex.Lock()
	defer m.mutex.Unlock()

	iter := m.lns.Remove(name)
	if nil != iter {
		l, ok := iter.(*listen)
		if ok {
			l.destroy()

			return nil
		}

		kutil.Assert(false)
		return errors.New("")
	}

	return errors.New("")
}
