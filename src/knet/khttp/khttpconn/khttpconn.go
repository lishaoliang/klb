package khttpconn

import (
	"net"

	"github.com/lishaoliang/klb/src/knet/kconn"
	"github.com/lishaoliang/klb/src/kutil/kpool"
)

// CreateServe CreateServe
func CreateServe(conn net.Conn, data []byte, pool kpool.KPool, rOut *chan kconn.ConnRead, rMdOut *chan kconn.ConnRead) kconn.KConn {

	var m Conn
	kconn.InitConn(&m.conn, kconn.SERVE, conn, data, pool, rOut, rMdOut)

	return &m
}

// CreateClient CreateClient
func CreateClient(conn net.Conn, data []byte, pool kpool.KPool, rOut *chan kconn.ConnRead, rMdOut *chan kconn.ConnRead) kconn.KConn {
	var m Conn

	kconn.InitConn(&m.conn, kconn.CLIENT, conn, data, pool, rOut, rMdOut)

	return &m
}
