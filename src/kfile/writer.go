// 媒体容器格式写入

package kfile

import "github.com/lishaoliang/klb/src/knet/kmnp"

// Writer interface
type Writer interface {
	Close() error

	Write(md kmnp.MnpMedia, b []byte) error
}
