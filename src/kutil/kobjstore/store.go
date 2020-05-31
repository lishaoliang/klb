package kobjstore

import (
	"sync"

	"github.com/lishaoliang/klb/src/kutil"
)

// Store store
// 存储
type Store struct {
	mapData *kutil.MapIface // 存储
	mutex   sync.Mutex      // 锁
}

// StoreCreate StoreCreate
func StoreCreate(preName, split string, nameLen int) *Store {
	var m Store
	m.mapData = kutil.NewMapIface(preName, split, nameLen)

	return &m
}

// Destroy Destroy
func (m *Store) Destroy() {
	// 先销毁所有数据
	datas := m.mapData.Map()
	for _, v := range datas {
		if o, ok := v.(kutil.Obj); ok {
			o.Destroy()
		}
	}

	m.mapData.Destroy()
}

// Push push
// 放入数据
func (m *Store) Push(o kutil.Obj) string {
	m.mutex.Lock()
	defer m.mutex.Unlock()

	return m.mapData.Push(o)
}

// PushByName push by name
// 放入数据
func (m *Store) PushByName(name string, o kutil.Obj) bool {
	m.mutex.Lock()
	defer m.mutex.Unlock()

	return m.mapData.PushByName(name, o)
}

// Remove remove
// 移除数据
func (m *Store) Remove(name string) kutil.Obj {
	m.mutex.Lock()
	defer m.mutex.Unlock()

	item := m.mapData.Remove(name)

	if o, ok := item.(kutil.Obj); ok {
		return o
	}

	kutil.Assert(false)
	return nil
}

// Find find
// 查找数据
func (m *Store) Find(name string) kutil.Obj {
	m.mutex.Lock()
	defer m.mutex.Unlock()

	item := m.mapData.Find(name)
	if nil == item {
		return nil
	}

	if o, ok := item.(kutil.Obj); ok {
		return o
	}

	kutil.Assert(false)
	return nil
}
