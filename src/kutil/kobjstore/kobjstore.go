// kutil.obj 存储

package kobjstore

import "github.com/lishaoliang/klb/src/kutil"

var gStore *Store

// init
func init() {
	// int store(
	gStore = StoreCreate("", "", 12)
}

// DestroyAll DestroyAll
// 销毁所有数据
//func DestroyAll() {
//	gStore.Destroy()
//	gStore = StoreCreate("", "", 12)
//}

// Push push
// 放入数据
func Push(o kutil.Obj) string {
	return gStore.Push(o)
}

// PushByName push by name
// 放入数据
func PushByName(name string, o kutil.Obj) bool {
	return gStore.PushByName(name, o)
}

// Remove remove
// 移出数据, 数据本身并未被释放
func Remove(name string) kutil.Obj {
	return gStore.Remove(name)
}

// Find find
// 查找数据
func Find(name string) kutil.Obj {
	return gStore.Find(name)
}
