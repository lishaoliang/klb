package kwsflv

import (
	"net"

	"github.com/lishaoliang/klb/src/knet/kconn"
	"github.com/lishaoliang/klb/src/kutil"
	"github.com/lishaoliang/klb/src/kutil/kbuf"
	"github.com/lishaoliang/klb/src/kutil/kpool"
)

var gRand *kutil.Rand

// init
func init() {
	gRand = kutil.NewRand()
}

func getRandMask() []byte {
	return gRand.StrB(4)
}

// CreateServe CreateServe
func CreateServe(conn net.Conn, data []byte, pool kpool.KPool, rOut *chan kconn.ConnRead, rMdOut *chan kconn.ConnRead) kconn.KConn {
	var m Conn
	m.mdWrite = kbuf.New(64 * 1024)
	kconn.InitConn(&m.conn, kconn.SERVE, conn, data, pool, rOut, rMdOut)

	return &m
}

// CreateClient CreateClient
func CreateClient(conn net.Conn, data []byte, pool kpool.KPool, rOut *chan kconn.ConnRead, rMdOut *chan kconn.ConnRead) kconn.KConn {
	var m Conn
	m.mdWrite = kbuf.New(64 * 1024)
	kconn.InitConn(&m.conn, kconn.CLIENT, conn, data, pool, rOut, rMdOut)

	return &m
}
