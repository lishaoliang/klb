///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   himpp.go
/// @author lishaoliang
/// @brief	himpp
///  usage: go build -tags="hi3531dv100 himppv3 nvp6134c"
///  usage: go build -tags="hi3516ev300 himppv2 imx274"
///  hi35xx: http://www.hisilicon.com/en/Products/ProductList/Camera
///   dev type: dvr, ipc, nvr, etc.
///   product: hi3516ev300, hi3531dv100, etc.
///   MPP version: himppv1, himppv2, himppv3, etc.
///   AD: nvp6134c, nvp6144, etc.
///   ISP sensor: imx274, ov4689, etc.
///////////////////////////////////////////////////////////////////////////

package himpp

/*
#include "himpp/klb_himpp.h"
*/
import "C"

import "fmt"

// Test test
func Test() {

	ver, _ := SysGetVersion()
	fmt.Println(ver)
}
