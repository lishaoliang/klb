///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   socket.go
/// @author lishaoliang
/// @brief	socket
///////////////////////////////////////////////////////////////////////////

package kutil

import (
	"net"
	"reflect"
)

// SocketFD get socket fd by net.Conn
// 获取net.Conn的系统文件句柄
func SocketFD(conn net.Conn) int {
	socketConn := reflect.Indirect(reflect.ValueOf(conn)).FieldByName("conn")
	fdVal := socketConn.FieldByName("fd")
	pfdVal := reflect.Indirect(fdVal).FieldByName("pfd")
	return int(pfdVal.FieldByName("Sysfd").Int())
}
