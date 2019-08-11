///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/08/10
//
/// @file    worker.go
/// @author  lishaoliang
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief	工作线程
///////////////////////////////////////////////////////////////////////////
package klua

import (
	"context"
	"sync"
)

func worker(ctx context.Context, wg *sync.WaitGroup, myCtx *Ctx, eo *envOne, loader string, bDoFile bool) error {
	defer eo.Destroy()
	defer wg.Done()

	env := eo.GetEnv()

	if bDoFile {
		env.DoFile(loader)
	} else {
		env.DoLibrary(loader)
	}

	if !env.HasKgo() {
		// lua 如果没有"kgo"函数, 则可以直接退出
		return ctx.Err()
	}

	for {
		select {
		case msg := <-eo.msgs:
			//fmt.Println(msg)
			env.CallKgo(msg.msg, msg.lparam, msg.wparam, msg.ptr) // post消息
			//default:
			//	env.CallKgo()
			//	return ctx.Err()
		case <-eo.closes:
			return ctx.Err() // 关闭退出
		case <-ctx.Done():
			return ctx.Err() // 超时或强制退出
		}
	}
}
