package kflvreader

import "github.com/lishaoliang/klb/src/kfile"

// Open file
func Open(name string) (kfile.Reader, error) {
	var m reader
	err := m.open(name)

	if nil != err {
		return nil, err
	}

	return &m, nil
}
