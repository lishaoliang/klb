// 媒体容器格式读取

package kfile

import "github.com/lishaoliang/klb/src/knet/kmnp"

// Reader interface
type Reader interface {
	Close() error

	Reset() error
	ReadNext() (kmnp.MnpMedia, []byte, error)
}
