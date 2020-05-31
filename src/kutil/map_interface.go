///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   map_interface.go
/// @author lishaoliang
/// @brief	map interface
///////////////////////////////////////////////////////////////////////////

package kutil

import "fmt"

// MapIface map interface
// 命名对象集合
type MapIface struct {
	r    *Rand
	data map[string]interface{}
	size int64

	preName string
	split   string
	nameLen int
}

// NewMapIface new
// 创建, 名称格式: preName + split + "随机串"
func NewMapIface(preName, split string, nameLen int) *MapIface {
	var m MapIface

	m.r = NewRand()
	m.data = make(map[string]interface{})
	m.size = 0

	m.preName = preName
	m.split = split //":"
	m.nameLen = nameLen

	return &m
}

///////////////////////////////////////////////////////////////////////////////

// Destroy destroy
// 销毁
//func (m *MapIface) Destroy() {
//	//m.r.Destroy()
//
//	m.r = nil
//	m.data = nil
//	m.size = 0
//}

// Map get map
// 获取原map
func (m *MapIface) Map() map[string]interface{} {
	return m.data
}

// Size items size
// 元素数目
func (m *MapIface) Size() int64 {
	return m.size
}

// Push push
// 放入
func (m *MapIface) Push(item interface{}) string {

	for {
		name := m.randName()
		if nil != m.data[name] {
			continue
		}

		m.data[name] = item
		m.size++
		return name
	}
}

// PushByName push by name
// 按名称放入
func (m *MapIface) PushByName(name string, item interface{}) bool {

	if nil != m.data[name] {
		return false
	}

	m.data[name] = item
	return true
}

// Remove remove
// 移除
func (m *MapIface) Remove(name string) interface{} {
	item := m.data[name]

	if nil != item {
		delete(m.data, name)
		m.size--
	}

	return item
}

// Find find
// 获取
func (m *MapIface) Find(name string) interface{} {
	return m.data[name]
}

///////////////////////////////////////////////////////////////////////////////

// randName rand name
// 随机名称: "preName" + "split" + "rand str"
// 例如: "aaa:ABDEFde"
func (m *MapIface) randName() string {
	rname := m.r.Str(m.nameLen)
	return fmt.Sprintf("%s%s%s", m.preName, m.split, rname)
}
