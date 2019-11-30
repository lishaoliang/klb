///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   mdc.go
/// @author lishaoliang
/// @brief	mdc
///////////////////////////////////////////////////////////////////////////

package kamdc

import (
	"fmt"
	"net"
)

var gSerMan *serMan

func mdcInit() int {
	//fmt.Println("mdc init")

	gSerMan = serManCreate()

	return 0
}

func mdcQuit() int {
	//fmt.Println("mdc quit")

	gSerMan.Destroy()
	gSerMan = nil

	return 0
}

func mdcStart(name, entry, preload, loader string) int {
	//fmt.Println("mdc start")

	gSerMan.Start(name, entry, preload, loader)

	go listenWorker("tcp", ":8000")
	fmt.Println("mdc listen:", 8000)

	return 0
}

func mdcStop() int {
	//fmt.Println("mdc stop")

	return 0
}

func listenWorker(network, address string) {
	//ln, err := net.Listen("tcp", ":8000")
	ln, err := net.Listen(network, address)
	if err != nil {
		// handle error
	}

	for {
		conn, err := ln.Accept()
		if err != nil {
			// handle error
		}

		/*
			go func(conn net.Conn) {
				csl := kmnp.ConnSerLimitCreate(conn, &gSerMan.rList)
				defer csl.Destroy()

				csl.Worker()
				csl.WaitDone()
			}(conn)
		*/
		gSerMan.Push("mnp", conn)
	}
}
