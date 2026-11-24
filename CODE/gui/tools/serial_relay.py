#!/usr/bin/env python3
"""
serial_relay.py - run this ON THE JETSON so several programs can share the board stack's serial port.

Why: only one program can usefully read a serial port. If the Jetson software owns /dev/ttyACM0,
the GUI on your laptop cannot also read it. This relay owns the real port instead and shares it:

    board stack  <--USB-->  serial_relay.py  --+-- TCP 127.0.0.1:5000  read/write   (GUI, through an SSH tunnel)
                                               +-- TCP 127.0.0.1:5001  READ-ONLY    (listen only, input is dropped)
                                               +-- pty /tmp/ttyBOARD   read/write   (point the Jetson software here)

 - everything the board sends is copied to every client
 - bytes from read/write clients are forwarded to the board, bytes from read-only clients are discarded
   (so a read-only connection can never change anything, even if a client misbehaves)
 - the TCP ports only listen on localhost, reach them from your laptop with an SSH tunnel:
        ssh -L 5000:localhost:5000 -L 5001:localhost:5001 user@jetson
   then connect the GUI to tcp://localhost:5001 (read-only) or tcp://localhost:5000
 - the board answers EVERY client's commands to ALL clients, so two programs sending commands at the
   same time see each other's replies. Use the read-only port when the Jetson software is the one in charge.

Usage:   python3 serial_relay.py /dev/ttyACM0 [--rw-port 5000] [--ro-port 5001] [--pty-link /tmp/ttyBOARD]
Needs only Python 3 (standard library), Linux.
"""
import argparse, os, pty, selectors, socket, sys, termios, tty


def open_serial(path):
    fd = os.open(path, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    tty.setraw(fd)
    attrs = termios.tcgetattr(fd)
    attrs[2] |= termios.CLOCAL | termios.CREAD       # modem lines ignored, receiver on
    termios.tcsetattr(fd, termios.TCSANOW, attrs)
    return fd


def listen(port):
    s = socket.socket()
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind(("127.0.0.1", port))
    s.listen(4)
    s.setblocking(False)
    return s


def main():
    ap = argparse.ArgumentParser(description="Share one serial port between the Jetson software and remote tools")
    ap.add_argument("serial", help="real serial port of the board stack, e.g. /dev/ttyACM0")
    ap.add_argument("--rw-port", type=int, default=5000, help="TCP port with read/write access (0 = off)")
    ap.add_argument("--ro-port", type=int, default=5001, help="TCP port with read-only access (0 = off)")
    ap.add_argument("--pty-link", default="", help="create a virtual serial port at this path for the Jetson software")
    a = ap.parse_args()

    sel = selectors.DefaultSelector()
    ser = open_serial(a.serial)
    sel.register(ser, selectors.EVENT_READ, ("serial", None))

    clients = {}                                     # fileobj -> writable (True = read/write client)
    pty_master = None

    for port, writable in ((a.rw_port, True), (a.ro_port, False)):
        if port:
            sel.register(listen(port), selectors.EVENT_READ, ("listen", writable))
            print("listening on 127.0.0.1:%d (%s)" % (port, "read/write" if writable else "READ-ONLY"), flush=True)

    if a.pty_link:
        pty_master, slave = pty.openpty()
        tty.setraw(pty_master)
        if os.path.islink(a.pty_link) or os.path.exists(a.pty_link): os.remove(a.pty_link)
        os.symlink(os.ttyname(slave), a.pty_link)
        os.set_blocking(pty_master, False)
        sel.register(pty_master, selectors.EVENT_READ, ("pty", True))
        print("virtual serial port for the Jetson software: %s" % a.pty_link, flush=True)

    def drop(f):
        try: sel.unregister(f)
        except Exception: pass
        clients.pop(f, None)
        try: f.close()
        except Exception: pass

    def broadcast(data):
        for f in list(clients):
            try: f.sendall(data)
            except OSError: drop(f)
        if pty_master is not None:
            try: os.write(pty_master, data)
            except OSError: pass                    # nobody has the virtual port open yet

    while True:
        for key, _ in sel.select():
            kind, extra = key.data
            if kind == "serial":
                try: data = os.read(ser, 4096)
                except OSError: data = b""
                if not data:
                    print("serial port closed / unplugged, exiting", flush=True)
                    return 1
                broadcast(data)
            elif kind == "listen":
                conn, addr = key.fileobj.accept()
                conn.setblocking(False)
                clients[conn] = extra
                sel.register(conn, selectors.EVENT_READ, ("client", extra))
                print("client connected (%s)" % ("read/write" if extra else "read-only"), flush=True)
            elif kind == "client":
                try: data = key.fileobj.recv(4096)
                except OSError: data = b""
                if not data:
                    drop(key.fileobj); print("client disconnected", flush=True)
                elif extra:
                    os.write(ser, data)             # read/write client: forward to the board
                # read-only client: input is discarded
            elif kind == "pty":
                try: data = os.read(pty_master, 4096)
                except OSError: data = b""
                if data: os.write(ser, data)


if __name__ == "__main__":
    sys.exit(main())
