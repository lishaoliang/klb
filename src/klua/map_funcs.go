///////////////////////////////////////////////////////////////////////////
//	Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file	map_funcs.go
/// @author	lishaoliang
/// @brief	map funcs
///////////////////////////////////////////////////////////////////////////

package klua

import (
	"sync"
)

// mapFuncs map func
type mapFuncs struct {
	preloads      map[string]LuaCFunction // func
	mutexPreloads sync.Mutex              // 锁(preloads)

	loaders      map[string]LuaCFunction // func
	mutexLoaders sync.Mutex              // 锁(loaders)
}

// 模块唯一
var gMapFuncs mapFuncs

///////////////////////////////////////////////////////////////////////////////

func mapFuncsInit() {
	gMapFuncs.preloads = make(map[string]LuaCFunction)
	gMapFuncs.loaders = make(map[string]LuaCFunction)
}

// Destroy
func (m *mapFuncs) Destroy() {
	m.preloads = nil
	m.loaders = nil
}

// PushPreload
func (m *mapFuncs) PushPreload(name string, cbPreLoad LuaCFunction) bool {
	m.mutexPreloads.Lock()
	defer m.mutexPreloads.Unlock()

	if nil == m.preloads[name] {
		m.preloads[name] = cbPreLoad
		return true
	}

	return false
}

// FindPreload
func (m *mapFuncs) FindPreload(name string) LuaCFunction {
	m.mutexPreloads.Lock()
	defer m.mutexPreloads.Unlock()

	return m.preloads[name]
}

// PushLoader
func (m *mapFuncs) PushLoader(name string, cbLoader LuaCFunction) bool {
	m.mutexLoaders.Lock()
	defer m.mutexLoaders.Unlock()

	// 使用 ""名称 用于表示从文件中加载, 而不是使用加载函数
	if "" == name {
		return false
	}

	if nil == m.preloads[name] {
		m.loaders[name] = cbLoader
		return true
	}

	return false
}

// FindLoader
func (m *mapFuncs) FindLoader(name string) LuaCFunction {
	m.mutexLoaders.Lock()
	defer m.mutexLoaders.Unlock()

	if "" == name {
		return nil
	}

	return m.loaders[name]
}
