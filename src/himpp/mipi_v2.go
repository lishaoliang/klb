///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   mipi_v2.go
/// @author lishaoliang
/// @brief	mipi v2
///  usage: go build -tags="himppv2"
///////////////////////////////////////////////////////////////////////////

// +build himppv2

package himpp

/*
#include "himpp/klb_himpp.h"
#include "log/klb_log.h"
#include <string.h>

#include <unistd.h>
#include <fcntl.h>

int klua_go_himpp_set_mipi_attr(int sensor_id, combo_dev_attr_t* p_attr)
{
    // 参考 v2 sample/common/sample_comm_vi.c SAMPLE_COMM_VI_SetMipiAttr

    // mipi reset unrest
    HI_S32 fd = open("/dev/hi_mipi", O_RDWR);
    if (fd < 0)
    {
        return 1;
	}

    if (ioctl(fd, HI_MIPI_SET_DEV_ATTR, p_attr))
    {
        close(fd);
        return 1;
    }

	close(fd);
    return 0;
}

*/
import "C"
import "fmt"

// ComboDevAttr C.combo_dev_attr_t
type ComboDevAttr C.combo_dev_attr_t

// MipiSetAttr set mipi attr
func MipiSetAttr(sensorID int, attr *ComboDevAttr) int {
	ret := C.klua_go_himpp_set_mipi_attr(C.int(sensorID), (*C.combo_dev_attr_t)(attr))

	if 0 == ret {
		fmt.Printf("himpp.mipi_himppv2.MipiSetAttr ok!sensor id=%d\n", sensorID)
	} else {
		fmt.Printf("himpp.mipi_himppv2.MipiSetAttr error!ret=%d,sensor id=%d\n", ret, sensorID)
	}

	return int(ret)
}
