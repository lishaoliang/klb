///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   fb2d.go
/// @author lishaoliang
/// @brief	fb2d interface
///////////////////////////////////////////////////////////////////////////

package kfb2d

import (
	"image"

	"github.com/lishaoliang/klb/src/kimage"
	"github.com/lishaoliang/klb/src/kimage/kdraw"
)

// Fb2d 类似linux fb的2d绘图接口.
// 支持图像;支持绘图原语
type Fb2d interface {
	kimage.Image // 基础图像接口

	kdraw.FbDrawLine // 绘制线段
	kdraw.FbDrawFill // 填充颜色
	kdraw.FbDrawRect // 绘制矩形
	kdraw.FbDrawCopy // 像素拷贝

	GetVersion() string        // 版本信息
	Refresh(r image.Rectangle) // 刷新到屏幕
}

// Fb2dV1 fb 2d
type Fb2dV1 interface {
	Fb2d
	Fb2dV1() string
}

// Fb2dV2 fb 2d
type Fb2dV2 interface {
	Fb2d
	Fb2dV2() string
}

// Fb2dV3 fb 2d
type Fb2dV3 interface {
	Fb2d
	Fb2dV3() string
}
