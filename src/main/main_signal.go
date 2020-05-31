// +build !windows

package main

import (
	"os"
	"os/signal"
	"syscall"
)

func signalNotify(c chan<- os.Signal) {
	signal.Notify(c, os.Interrupt, os.Kill, syscall.SIGINT, syscall.SIGTERM, syscall.SIGTSTP)
}
