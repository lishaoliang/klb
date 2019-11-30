///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   klisten.go
/// @author lishaoliang
/// @brief	k listen
///  端口复用: 同一个端口支持多种连接协议
///  1. 监听模块监听端口得到连接
///  2. 从连接中读取最多4K的数据
///  3. 分析数据并确定属于什么协议
///  4. 将连接交给对应处理模块
///////////////////////////////////////////////////////////////////////////

package klisten

import (
	"crypto/tls"
	"net"
)

var gServe serve

// init
func init() {
	serveInit(&gServe)
}

// Listen listen
func Listen(name, network, address string) error {
	ln, err := net.Listen(network, address)
	if nil != err {
		return err
	}

	l := newListen(ln)
	err = gServe.push(name, l)

	if nil != err {
		l.destroy()
	}

	return err
}

// ListenTLS listen tls
func ListenTLS(name, network, address string, config *tls.Config) error {
	ln, err := tls.Listen(network, address, config)
	if nil != err {
		return err
	}

	l := newListen(ln)
	err = gServe.push(name, l)

	if nil != err {
		l.destroy()
	}

	return err
}

// Close close listen
func Close(name string) error {
	return gServe.close(name)
}
