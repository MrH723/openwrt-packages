package wechat

import (
	"encoding/binary"
	"github.com/tobyxdd/hysteria/pkg/obfs"
	"math/rand"
	"net"
	"os"
	"sync"
	"syscall"
	"time"
)

const udpBufferSize = 65535

type ObfsWeChatUDPConn struct {
	orig *net.UDPConn
	obfs obfs.Obfuscator

	readBuf    []byte
	readMutex  sync.Mutex
	writeBuf   []byte
	writeMutex sync.Mutex
	sn         uint32
}

func NewObfsWeChatUDPConn(orig *net.UDPConn, obfs obfs.Obfuscator) *ObfsWeChatUDPConn {
	return &ObfsWeChatUDPConn{
		orig:     orig,
		obfs:     obfs,
		readBuf:  make([]byte, udpBufferSize),
		writeBuf: make([]byte, udpBufferSize),
		sn:       rand.Uint32() & 0xFFFF,
	}
}

func (c *ObfsWeChatUDPConn) ReadFrom(p []byte) (int, net.Addr, error) {
	for {
		c.readMutex.Lock()
		n, addr, err := c.orig.ReadFrom(c.readBuf)
		if n <= 13 {
			c.readMutex.Unlock()
			return 0, addr, err
		}
		newN := c.obfs.Deobfuscate(c.readBuf[13:n], p)
		c.readMutex.Unlock()
		if newN > 0 {
			// Valid packet
			return newN, addr, err
		} else if err != nil {
			// Not valid and orig.ReadFrom had some error
			return 0, addr, err
		}
	}
}

func (c *ObfsWeChatUDPConn) WriteTo(p []byte, addr net.Addr) (n int, err error) {
	c.writeMutex.Lock()
	c.writeBuf[0] = 0xa1
	c.writeBuf[1] = 0x08
	binary.BigEndian.PutUint32(c.writeBuf[2:], c.sn)
	c.sn++
	c.writeBuf[6] = 0x00
	c.writeBuf[7] = 0x10
	c.writeBuf[8] = 0x11
	c.writeBuf[9] = 0x18
	c.writeBuf[10] = 0x30
	c.writeBuf[11] = 0x22
	c.writeBuf[12] = 0x30
	bn := c.obfs.Obfuscate(p, c.writeBuf[13:])
	_, err = c.orig.WriteTo(c.writeBuf[:13+bn], addr)
	c.writeMutex.Unlock()
	if err != nil {
		return 0, err
	} else {
		return len(p), nil
	}
}

func (c *ObfsWeChatUDPConn) Close() error {
	return c.orig.Close()
}

func (c *ObfsWeChatUDPConn) LocalAddr() net.Addr {
	return c.orig.LocalAddr()
}

func (c *ObfsWeChatUDPConn) SetDeadline(t time.Time) error {
	return c.orig.SetDeadline(t)
}

func (c *ObfsWeChatUDPConn) SetReadDeadline(t time.Time) error {
	return c.orig.SetReadDeadline(t)
}

func (c *ObfsWeChatUDPConn) SetWriteDeadline(t time.Time) error {
	return c.orig.SetWriteDeadline(t)
}

func (c *ObfsWeChatUDPConn) SetReadBuffer(bytes int) error {
	return c.orig.SetReadBuffer(bytes)
}

func (c *ObfsWeChatUDPConn) SetWriteBuffer(bytes int) error {
	return c.orig.SetWriteBuffer(bytes)
}

func (c *ObfsWeChatUDPConn) SyscallConn() (syscall.RawConn, error) {
	return c.orig.SyscallConn()
}

func (c *ObfsWeChatUDPConn) File() (f *os.File, err error) {
	return c.orig.File()
}
