//go:build !linux
// +build !linux

package tproxy

import (
	"errors"
	"github.com/tobyxdd/hysteria/pkg/core"
	"net"
	"time"
)

var ErrTimeout = errors.New("inactivity timeout")

type UDPTProxy struct{}

func NewUDPTProxy(hyClient *core.Client, listen string, timeout time.Duration,
	connFunc func(addr, reqAddr net.Addr), errorFunc func(addr, reqAddr net.Addr, err error)) (*UDPTProxy, error) {
	return nil, errors.New("not supported on the current system")
}

func (r *UDPTProxy) ListenAndServe() error {
	return nil
}
