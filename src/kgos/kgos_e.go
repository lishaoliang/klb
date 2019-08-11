///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/08/11
//
/// @file    kgos_e.go
/// @author  lishaoliang
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief	kgos export
///////////////////////////////////////////////////////////////////////////
package kgos

/*
#include "klua/klua.h"
extern int klua_go_open_kg_os_cgo(lua_State* L);
*/
import "C"
import (
	"bytes"
	"fmt"
	"os/exec"
	"path/filepath"
)

// exprot func

//export klua_go_open_kg_os
func klua_go_open_kg_os(lua *C.lua_State) C.int {
	// export for c load

	ret := C.klua_go_open_kg_os_cgo(lua)

	return ret
}

func command(name string, arg ...string) *exec.Cmd {
	cmd := &exec.Cmd{
		Path: name,
		Args: append([]string{name}, arg...),
	}
	if filepath.Base(name) == name {
		if lp, err := exec.LookPath(name); err != nil {
			//cmd.lookPathErr = err
		} else {
			cmd.Path = lp
		}
	}
	return cmd
}

//export klua_go_kg_os_shell
func klua_go_kg_os_shell(lua *C.lua_State) C.int {

	//param := []string{"-l", "-a"}

	cmd := command("ls", "-la")

	//cmd := exec.Command("ls", "-la")
	//cmd := exec.Command("haha", "-la")
	//cmd.Stdin = strings.NewReader("some input")
	var out bytes.Buffer
	cmd.Stdout = &out
	err := cmd.Run()
	if err != nil {
		//log.Fatal(err)
		fmt.Println(err)
	}
	fmt.Printf("in all caps: %q\n", out.String())

	return 0
}
