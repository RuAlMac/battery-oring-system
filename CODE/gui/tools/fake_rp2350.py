#!/usr/bin/env python3
"""
Fake battery board for trying the GUI without hardware (Linux / macOS only).

Creates a virtual serial port and speaks the same line protocol as the
RP2350 firmware (communication/serial_interface.c): modes normal/debug/silent,
settings list/get/set/save/reload/..., overrides, S/T frames, EVT lines.

    python3 fake_rp2350.py --link /tmp/ttyFAKE0

then, in the GUI toolbar, type  /tmp/ttyFAKE0  into the Port box and press
Connect (the port box accepts any path). Without --link the script prints the
/dev/pts/N name to type instead.
"""
import argparse, math, os, pty, select, sys, time, tty

SETTINGS = [
    # name, units, default, lo, hi
    ("warn_cell_voltage",   "V",  3.50, 2.0, 4.5),
    ("danger_cell_voltage", "V",  3.30, 2.0, 4.5),
    ("shtdwn_cell_voltage", "V",  3.10, 2.0, 4.5),
    ("warn_batt_voltage",   "V", 14.00, 6.0, 20.0),
    ("danger_batt_voltage", "V", 13.20, 6.0, 20.0),
    ("shtdwn_batt_voltage", "V", 12.40, 6.0, 20.0),
    ("warn_temp",           "C", 45.0, -20.0, 120.0),
    ("danger_temp",         "C", 55.0, -20.0, 120.0),
    ("shtdwn_temp",         "C", 65.0, -20.0, 120.0),
    ("warn_buzzer_beep_speed",   "ms", 400, 20, 5000),
    ("danger_buzzer_beep_speed", "ms", 120, 20, 5000),
    ("warn_led_interval",   "ms", 1000, 50, 5000),
    ("danger_led_interval", "ms", 250, 50, 5000),
    ("shtdwn_delay",        "s",  0, 0, 50),
    ("shtdwn_latch",        "bool", 0, 0, 1),
    ("cell_diff_check",     "bool", 1, 0, 1),
    ("cell_diff_max",       "mV", 100, 1, 1000),
]
ORDER_VOLT = [("shtdwn_cell_voltage", "danger_cell_voltage", "warn_cell_voltage"),
              ("shtdwn_batt_voltage", "danger_batt_voltage", "warn_batt_voltage")]
ORDER_TEMP = [("warn_temp", "danger_temp", "shtdwn_temp")]
UNPOPULATED = 3

class Board:
    def __init__(self):
        self.eeprom = {s[0]: s[2] for s in SETTINGS}
        self.eeprom_valid = True
        self.active = dict(self.eeprom)
        self.pending = dict(self.eeprom)
        self.mode = "normal"
        self.buzzers = True
        self.check_ms = 250
        self.normal_check_ms = 250
        self.stream_ms = 1000
        self.ovr = {}                      # key -> value ; keys cell0..4, batt, temp
        self.last_rx = time.time()
        self.seq = 0
        self.t0 = time.time()
        self.cd_end = None
        self.shut = False
        self.latched = False
        self.imb = False
        self.next_stream = 0.0
        self.next_measure = 0.0
        self.real = self.measure()
        self.connected_dtr = True

    def ms(self): return int((time.time() - self.t0) * 1000)

    def measure(self):
        t = time.time() - self.t0
        cells = [3.85 + 0.03 * math.sin(t / 3 + i) for i in range(5)]
        cells[UNPOPULATED] = 0.0
        batt = sum(cells)
        temp = 31.0 + 2.0 * math.sin(t / 7)
        return {"cells": cells, "batt": batt, "temp": temp}

    def effective(self):
        cells = list(self.real["cells"])
        for i in range(5):
            if f"cell{i}" in self.ovr: cells[i] = self.ovr[f"cell{i}"]
        batt = self.ovr.get("batt", self.real["batt"])
        temp = self.ovr.get("temp", self.real["temp"])
        return cells, batt, temp

    @staticmethod
    def lvl_low(v, w, d, s): return 3 if v <= s else 2 if v <= d else 1 if v <= w else 0
    @staticmethod
    def lvl_high(v, w, d, s): return 3 if v >= s else 2 if v >= d else 1 if v >= w else 0

    def levels(self):
        """raw levels with the shutdown delay applied (shutdown shows as danger while counting down)"""
        lvl, cl, bl, tl = self.raw_levels()
        if self.latched and self.active["shtdwn_latch"] < 0.5: self.latched = False
        if self.latched:
            self.cd_end = None; self.shut = True
            return 3, cl, bl, tl
        if lvl != 3:
            self.cd_end = None; self.shut = False
            return lvl, cl, bl, tl
        if not self.shut:
            delay = self.active["shtdwn_delay"]
            now = time.time()
            if delay <= 0: self.shut = True; self.cd_end = None
            else:
                if self.cd_end is None: self.cd_end = now + delay
                if now >= self.cd_end: self.shut = True; self.cd_end = None
        if self.shut and self.active["shtdwn_latch"] >= 0.5: self.latched = True
        return (3 if self.shut else 2), cl, bl, tl

    def cd(self):
        self.levels()
        if self.cd_end is None: return 0, 0
        return 1, max(0, int((self.cd_end - time.time()) * 1000))

    def flags(self):
        self.levels()
        return (1 if self.latched else 0), (1 if self.imb else 0)

    def raw_levels(self):
        cells, batt, temp = self.effective()
        a = self.active
        lowest = min(c for i, c in enumerate(cells) if i != UNPOPULATED)
        cl = self.lvl_low(lowest, a["warn_cell_voltage"], a["danger_cell_voltage"], a["shtdwn_cell_voltage"])
        bl = self.lvl_low(batt, a["warn_batt_voltage"], a["danger_batt_voltage"], a["shtdwn_batt_voltage"])
        tl = self.lvl_high(temp, a["warn_temp"], a["danger_temp"], a["shtdwn_temp"])
        self.imb = False
        if a["cell_diff_check"] >= 0.5:
            pop = [c for i, c in enumerate(cells) if i != UNPOPULATED]
            for c in pop:
                others = (sum(pop) - c) / (len(pop) - 1)
                if abs(c - others) * 1000 > a["cell_diff_max"]: self.imb = True
        if self.imb: cl = 3
        return max(cl, bl, tl), cl, bl, tl

    def mask(self):
        m = 0
        for i in range(5):
            if f"cell{i}" in self.ovr: m |= 1 << i
        if "batt" in self.ovr: m |= 1 << 5
        if "temp" in self.ovr: m |= 1 << 6
        return m

    def s_frame(self):
        cells, batt, temp = self.effective()
        lvl = self.levels()[0]
        now = self.ms()
        cda, cdm = self.cd()
        lat, imb = self.flags()
        return "S,%d,%d,%d,%.3f,%.2f,%s,%d,%d,%d,%d,%d" % (now, 0, lvl, batt, temp,
               ",".join("%.3f" % c for c in cells), self.mask(), cda, cdm, lat, imb)

    def t_frame(self):
        cells, batt, temp = self.effective()
        lvl, cl, bl, tl = self.levels()
        rc = self.real["cells"]
        unsaved = 1 if self.pending != self.active else 0
        cda, cdm = self.cd()
        return "T,%d,%d,%d,%d,%d,%d,%d,%d,%d,%.3f,%s,%.2f,%.3f,%s,%.2f,%d,%d,%d,%d" % (
            self.ms(), 0, self.seq, lvl, cl, bl, tl, self.mask(), unsaved,
            batt, ",".join("%.3f" % c for c in cells), temp,
            self.real["batt"], ",".join("%.3f" % c for c in rc), self.real["temp"], cda, cdm, *self.flags())

    def fmt(self, name, v):
        units = next(s[1] for s in SETTINGS if s[0] == name)
        return "%.0f" % v if units in ("ms", "Hz", "s", "mV", "bool") else "%.3f" % v

    def valid_order(self, d):
        for lo, mid, hi in ORDER_VOLT:
            if not (d[lo] < d[mid] < d[hi]): return False
        for lo, mid, hi in ORDER_TEMP:
            if not (d[lo] < d[mid] < d[hi]): return False
        return True

    def set_mode(self, m, out=None):
        if m == self.mode: return
        if m == "debug":
            if self.mode == "normal":
                self.normal_check_ms = self.check_ms
                self.check_ms = 50
            self.stream_ms = 100
        elif m == "silent":
            self.stream_ms = 0
        else:
            self.ovr.clear()
            self.buzzers = True
            self.check_ms = self.normal_check_ms
            self.stream_ms = 1000
        self.mode = m
        self.next_stream = 0

    def handle(self, line):
        """returns list of output lines"""
        self.last_rx = time.time()
        w = line.lower().split()
        if not w: return []
        if len(w) > 6: return ["ERR bad_args too many words (max 6)"]
        c, a = w[0], w[1:]
        o = []
        if c == "help":
            o += ["# help ...", "OK proto=3"]
        elif c == "ping": o.append("OK t_ms=%d" % self.ms())
        elif c == "ver": o.append("OK fw=0.1 proto=3")
        elif c == "info":
            unsaved = 1 if self.pending != self.active else 0
            o.append("OK mode=%s uptime_ms=%d check_ms=%d stream_ms=%d ovr=0x%02x buzzers=%d unsaved=%d samples=%d have_data=1 level=%s fw=0.1 proto=3" % (
                self.mode, self.ms(), self.check_ms, self.stream_ms, self.mask(), 1 if self.buzzers else 0, unsaved, self.seq,
                ["good", "warning", "danger", "shutdown"][self.levels()[0]]))
        elif c == "unlatch":
            self.levels()
            if not self.latched: o.append("ERR state system is not latched in shutdown")
            else:
                self.latched = False; self.shut = False; self.cd_end = None
                o.append("OK level=%d latched=%d" % (self.levels()[0], 1 if self.latched else 0))
        elif c == "buzzer":
            if not a: o.append("OK buzzers=%d" % (1 if self.buzzers else 0))
            elif a == ["on"]: self.buzzers = True; o.append("OK buzzers=1")
            elif a == ["off"]:
                if self.mode == "normal": o.append("ERR mode buzzers can only be muted in debug or silent mode (send: mode debug)")
                else: self.buzzers = False; o.append("OK buzzers=0")
            else: o.append("ERR bad_args usage: buzzer [on|off]")
        elif c == "mode":
            if not a: o.append("OK mode=%s" % self.mode)
            elif a[0] == "normal" and len(a) == 1:
                self.set_mode("normal"); o.append("OK mode=normal stream_ms=%d" % self.stream_ms)
            elif a[0] == "silent" and len(a) == 1:
                self.set_mode("silent"); o.append("OK mode=silent stream_ms=0 ovr=0x%02x" % self.mask())
            elif a[0] == "debug" and len(a) <= 2:
                p = 100
                if len(a) == 2:
                    if not a[1].isdigit(): return ["ERR bad_args stream_ms must be a whole number"]
                    p = int(a[1])
                    if not (p == 0 or 20 <= p <= 10000): return ["ERR range stream_ms must be 0 or 20-10000"]
                self.set_mode("debug"); self.stream_ms = p
                o.append("OK mode=debug stream_ms=%d check_ms=%d" % (p, self.check_ms))
            else: o.append("ERR bad_args usage: mode [normal|silent|debug [stream_ms]]")
        elif c == "status": o.append(self.s_frame())
        elif c == "telemetry": o.append(self.t_frame())
        elif c == "list":
            for i, (n, u, d, lo, hi) in enumerate(SETTINGS):
                o.append("SET idx=%d name=%s units=%s active=%s pending=%s default=%s min=%s max=%s" % (
                    i, n, u, self.fmt(n, self.active[n]), self.fmt(n, self.pending[n]),
                    self.fmt(n, d), self.fmt(n, lo), self.fmt(n, hi)))
            o.append("OK count=%d unsaved=%d" % (len(SETTINGS), 1 if self.pending != self.active else 0))
        elif c == "get":
            if len(a) != 1: return ["ERR bad_args usage: get <name>"]
            if a[0] not in self.active: return ["ERR bad_id unknown setting '%s' (try: list)" % a[0]]
            u = next(s[1] for s in SETTINGS if s[0] == a[0])
            o.append("OK name=%s active=%s pending=%s units=%s" % (a[0], self.fmt(a[0], self.active[a[0]]), self.fmt(a[0], self.pending[a[0]]), u))
        elif c == "set":
            if len(a) != 2: return ["ERR bad_args usage: set <name> <value>"]
            if a[0] not in self.active: return ["ERR bad_id unknown setting '%s' (try: list)" % a[0]]
            u = next(x[1] for x in SETTINGS if x[0] == a[0])
            word = {"on": 1.0, "true": 1.0, "off": 0.0, "false": 0.0}.get(a[1]) if u == "bool" else None
            try: v = word if word is not None else float(a[1])
            except ValueError: return ["ERR bad_args '%s' is not a number" % a[1]]
            s = next(s for s in SETTINGS if s[0] == a[0])
            if not (s[3] <= v <= s[4]): return ["ERR range %s must be %s to %s %s" % (a[0], self.fmt(a[0], s[3]), self.fmt(a[0], s[4]), s[1])]
            self.pending[a[0]] = v
            o.append("OK name=%s pending=%s unsaved=%d" % (a[0], self.fmt(a[0], v), 1 if self.pending != self.active else 0))
        elif c == "validate":
            o.append("OK valid" if self.valid_order(self.pending) else "ERR order thresholds must satisfy: shutdown < danger < warn (voltages), warn < danger < shutdown (temps)")
        elif c == "save":
            if not self.valid_order(self.pending):
                o.append("ERR order not saved: thresholds must satisfy shutdown < danger < warn (voltages), warn < danger < shutdown (temps)")
            else:
                self.eeprom = dict(self.pending); self.active = dict(self.pending); o.append("OK saved")
        elif c == "discard": self.pending = dict(self.active); o.append("OK unsaved=0")
        elif c == "defaults":
            self.pending = {s[0]: s[2] for s in SETTINGS}
            o.append("OK unsaved=%d" % (1 if self.pending != self.active else 0))
        elif c == "reload":
            self.active = dict(self.eeprom); self.pending = dict(self.eeprom); o.append("OK reloaded")
        elif c == "rate":
            if not a: o.append("OK check_ms=%d" % self.check_ms)
            elif len(a) == 1 and a[0].isdigit(): self.check_ms = max(50, int(a[0])); o.append("OK check_ms=%d" % self.check_ms)
            else: o.append("ERR bad_args usage: rate [ms]")
        elif c == "stream":
            if self.mode == "silent": return ["ERR mode silent mode sends nothing automatically (use: mode normal|debug)"]
            if not a: o.append("OK stream_ms=%d" % self.stream_ms)
            elif len(a) == 1 and a[0].isdigit() and (int(a[0]) == 0 or 20 <= int(a[0]) <= 10000):
                self.stream_ms = int(a[0]); self.next_stream = 0; o.append("OK stream_ms=%d" % self.stream_ms)
            else: o.append("ERR range stream_ms must be 0 or 20-10000")
        elif c == "ovr":
            if self.mode == "normal": return ["ERR mode overrides need debug or silent mode (send: mode debug)"]
            def show():
                t = "mask=0x%02x" % self.mask()
                for i in range(5):
                    if f"cell{i}" in self.ovr: t += " cell%d=%.3f" % (i + 1, self.ovr[f"cell{i}"])
                if "batt" in self.ovr: t += " batt=%.3f" % self.ovr["batt"]
                if "temp" in self.ovr: t += " temp=%.2f" % self.ovr["temp"]
                return "OK " + t
            if not a: o.append(show())
            elif a[0] == "clear":
                if len(a) == 1 or a[1] == "all": self.ovr.clear()
                elif a[1] in ("batt", "temp"): self.ovr.pop(a[1], None)
                elif a[1] == "cell" and len(a) == 3 and a[2].isdigit() and 1 <= int(a[2]) <= 5: self.ovr.pop("cell%d" % (int(a[2]) - 1), None)
                else: return ["ERR bad_args usage: ovr clear [cell <n>|batt|temp|all]"]
                o.append(show())
            elif a[0] == "cell":
                try: n = int(a[1]); v = float(a[2])
                except Exception: return ["ERR bad_args usage: ovr cell <1-5> <volts>"]
                if not 1 <= n <= 5: return ["ERR bad_args usage: ovr cell <1-5> <volts>"]
                if n - 1 == UNPOPULATED: return ["ERR bad_args cell %d is not populated on this battery" % n]
                if not 0 <= v <= 6: return ["ERR range cell voltage must be 0.0 to 6.0 V"]
                self.ovr["cell%d" % (n - 1)] = v; self.real = self.real; o.append(show())
            elif a[0] in ("batt", "temp"):
                try: v = float(a[1])
                except Exception: return ["ERR bad_args usage: ovr %s <value>" % a[0]]
                lo, hi = (0, 30) if a[0] == "batt" else (-60, 150)
                if not lo <= v <= hi: return ["ERR range value must be %d to %d" % (lo, hi)]
                self.ovr[a[0]] = v; o.append(show())
            else: o.append("ERR bad_args usage: ovr [cell <n> <V>|batt <V>|temp <C>|clear [...]]")
        else:
            o.append("ERR unknown_cmd '%s' (try: help)" % c)
        return o

    def tick(self):
        """periodic work; returns list of output lines"""
        o = []
        now = time.time()
        if self.ovr and now - self.last_rx > 30:
            self.ovr.clear(); o.append("EVT ovr_cleared reason=timeout")
        if now >= self.next_measure:
            self.real = self.measure(); self.seq += 1
            self.next_measure = now + self.check_ms / 1000.0
        if self.stream_ms and now >= self.next_stream and self.mode != "silent":
            self.next_stream = now + self.stream_ms / 1000.0
            o.append(self.t_frame() if self.mode == "debug" else self.s_frame())
        return o

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--link", help="create this symlink to the virtual port (e.g. /tmp/ttyFAKE0)")
    args = ap.parse_args()

    master, slave = pty.openpty()
    tty.setraw(master)
    name = os.ttyname(slave)
    if args.link:
        try: os.unlink(args.link)
        except FileNotFoundError: pass
        os.symlink(name, args.link)
    print("Fake RP2350 on", name, ("(link %s)" % args.link) if args.link else "", flush=True)
    print("# serial interface ready, proto 3, mode normal (send: help)")

    board, buf = Board(), b""
    os.write(master, b"# serial interface ready, proto 3, mode normal (send: help)\n")
    while True:
        r, _, _ = select.select([master], [], [], 0.01)
        if r:
            try: data = os.read(master, 4096)
            except OSError: time.sleep(0.05); continue
            buf += data
            while b"\n" in buf or b"\r" in buf:
                idx = min(i for i in (buf.find(b"\n"), buf.find(b"\r")) if i >= 0)
                line, buf = buf[:idx].decode(errors="replace"), buf[idx + 1:]
                for out in board.handle(line): os.write(master, (out + "\n").encode())
        for out in board.tick(): os.write(master, (out + "\n").encode())

if __name__ == "__main__":
    main()
