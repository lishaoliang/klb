///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/08/24
//
/// @file    kmnp.go
/// @author  lishaoliang
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief	kmnp
///////////////////////////////////////////////////////////////////////////
package kmnp

import (
	"fmt"
	"net"
	"time"
)

// Conn

// ConnCreate create conn
func ConnCreate(netConn net.Conn) *Conn {
	return connCreate(netConn)
}

// Dial client connect "192.168.1.10:8000"
func Dial(address string) (*Conn, error) {
	// conn, err := net.Dial("tcp", "google.com:80")
	netConn, err := net.Dial("tcp", address)
	if err != nil {
		// handle error
		return nil, err
	}

	return connCreate(netConn), nil
}

// DialTimeout client connect "192.168.1.10:8000"
func DialTimeout(address string, timeout time.Duration) (*Conn, error) {
	// conn, err := net.Dial("tcp", "google.com:80")
	// timeout = (time.Duration)(time.Millisecond)*(time.Duration)(6000)
	netConn, err := net.DialTimeout("tcp", address, timeout)
	if err != nil {
		// handle error
		return nil, err
	}

	return connCreate(netConn), nil
}

// 资源受限服务端: 适用于嵌入式服务端

// ConnSerLimitCreate limit server
func ConnSerLimitCreate(netConn net.Conn, r *chan CslDataRead) *ConnSerLimit {
	return connSerLimitCreate(netConn, r)
}

// pack/unpack

// PackTxtS pack text by string
func PackTxtS(sequence, uid uint32, extra, msg string) []byte {
	fmt.Println(sequence, uid, extra, msg)
	return packTxt(MNPsubMedia, sequence, uid, []byte(extra), []byte(msg))
}

// PackTxt pack text by []byte
func PackTxt(sequence, uid uint32, extra, msg []byte) []byte {
	return packTxt(MNPsubMedia, sequence, uid, extra, msg)
}

// GetPackTye get []byte type
func GetPackTye(b []byte) (uint8, error) {
	return getPackTye(b)
}

// UnpackTxtS unpack text
func UnpackTxtS(b []byte) (uint32, uint32, string, string, error) {
	sequence, uid, extra, msg, err := unpackTxt(b)
	return sequence, uid, string(extra), string(msg), err
}

// UnpackTxt unpack text
func UnpackTxt(b []byte) (uint32, uint32, []byte, []byte, error) {
	return unpackTxt(b)
}
