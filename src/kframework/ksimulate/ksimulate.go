// 模拟硬件设备

package ksimulate

/*
#cgo CFLAGS: -std=c99 -I ${SRCDIR}/../../../inc
#cgo LDFLAGS: -L ${SRCDIR}/../../../lib -lklb_c -lpthread -lm

#include "klb_type.h"
#include "mnp/klb_mnp.h"

klb_mnp_stream_cb g_cb_klua_go_ksimulate_stream_receiver = NULL;

int klua_go_ksimulate_push_stream(uint32_t chnn, uint32_t sidx, uint32_t dtype, int64_t time, uint8_t vtype, void* p_data, int data_len)
{
	if (NULL != g_cb_klua_go_ksimulate_stream_receiver)
	{
		klb_mnp_stream_t mnp = {0};
		mnp.dtype = dtype;
		mnp.vtype = vtype;
		mnp.time = time;

		mnp.pack_num = 1;
		mnp.packs[0].p_data = p_data;
		mnp.packs[0].data_len = data_len;

		return g_cb_klua_go_ksimulate_stream_receiver(chnn, sidx, &mnp);
	}

	return -1;
}

*/
import "C"

import (
	"unsafe"

	"github.com/lishaoliang/klb/src/knet/kmnp"
)

var gSimulate simulate

// init
func init() {

}

// Init Init
func Init(cfg string) error {
	return gSimulate.Init(cfg)
}

// Quit quit
func Quit() {
	gSimulate.Quit()
}

// Start start
func Start() error {
	return gSimulate.Start()
}

// Stop stop
func Stop() {
	gSimulate.Stop()
}

// SetStreamReceiver set stream receiver
func SetStreamReceiver(cb unsafe.Pointer) {
	C.g_cb_klua_go_ksimulate_stream_receiver = (C.klb_mnp_stream_cb)(cb)
}

// Command command
func Command(cmd string, chnn, sidx int64, lparam, wparam string) (int64, error) {
	return gSimulate.Command(cmd, chnn, sidx, lparam, wparam)
}

// pushStream
func pushStream(md kmnp.MnpMedia, b []byte) {

	C.klua_go_ksimulate_push_stream(C.uint32_t(md.Chnn), C.uint32_t(md.Sidx), C.uint32_t(md.Dtype),
		C.int64_t(md.Time), C.uint8_t(md.Vtype), unsafe.Pointer(&b[0]),
		C.int(len(b)))

	//if 0 == md.Chnn {
	//	fmt.Println("**", md.Chnn, md.Sidx, md.Time, md.Size)
	//}
}
