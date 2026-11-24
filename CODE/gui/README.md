# Battery Tool (Qt GUI)

Desktop GUI for the Battery ORing & Monitoring V2 board stack (RP2350 firmware in `../`).
Qt 6 Widgets + Qt SerialPort, one code base for Linux and Windows (macOS should work too).

## Tabs
| Tab | What it does |
|---|---|
| **Monitor** | Overall status banner, large battery voltage, a battery icon with one fill bar per populated cell (green / yellow / orange / red by the warn / danger / shutdown voltages, 0 % = shutdown voltage, 100 % = 4.20 V) and a thermometer showing the temperature zones. Colors follow the RP2350's *active* settings. If a shutdown delay (`shtdwn_delay`) is set, the banner shows "DANGER - SHUTDOWN IN n s" while the RP2350 counts down; the countdown is tracked entirely by the RP2350 and reported in the S/T frames. When `shtdwn_latch` is on and the RP2350 is latched in shutdown, an "Override shutdown latch" button appears; it lets the RP2350 re-check the battery once (the countdown restarts if conditions are still bad). A cell that differs too much from the others (`cell_diff_check` / `cell_diff_max`) is reported as "CELL VOLTAGE IMBALANCE". |
| **Simulate** | Measured vs. what the system is using for every cell, the battery voltage and the temperature. Tick **Override** and move the slider to feed a simulated value to the RP2350 (shown purple on the Monitor page). Needs **Debug** or **Silent** mode. |
| **Settings (EEPROM)** | Every setting with active value, editable pending value, default and limits. **Load settings from EEPROM** and **Save settings to EEPROM** buttons at the top. Rows with unsaved edits are highlighted. |
| **Terminal** | Everything sent and received, colour coded. Type commands in the box at the bottom (Up/Down = history, Tab/auto-complete). Pause, clear, save log, hide S/T frames, timestamps. |
| **Serial** | Baud, data bits, parity, stop bits, flow control, DTR/RTS, line ending, auto-reconnect, keep-alive, reply timeout (remembered between runs) plus the RP2350's own stream period and measurement interval. |

The toolbar has the port box (type any port/path if it is not listed), Connect/Disconnect, the three modes
(Normal / Debug / Silent) and **Read now**.

## Build

Requirements: CMake >= 3.16, a C++17 compiler, Qt 6.2+ with the *Widgets*, *Serial Port* and *SVG* modules.

### Linux (Debian / Ubuntu)
```
sudo apt install build-essential cmake ninja-build qt6-base-dev qt6-serialport-dev qt6-svg-dev libgl1-mesa-dev
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/BatteryTool
```
To open `/dev/ttyACM0` without sudo: `sudo usermod -aG dialout $USER`, then log out and back in.

### Windows
Install Qt 6 with the online installer (select a kit such as *MSVC 2022 64-bit* or *MinGW*, and under
*Additional Libraries* tick **Qt Serial Port**). Then either open `CMakeLists.txt` in Qt Creator and press Run, or from a
Qt command prompt:
```
cmake -S . -B build -DCMAKE_PREFIX_PATH=C:\Qt\6.x.x\msvc2022_64 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
The build runs `windeployqt` automatically, so the exe in `build\Release` (or `build`) runs on its own.
The board shows up as a "COMx" port (USB Serial Device).

## Trying it without hardware (Linux / macOS)
```
python3 tools/fake_rp2350.py --link /tmp/ttyFAKE0
```
then type `/tmp/ttyFAKE0` in the GUI's Port box and press Connect. The script behaves like the firmware
(modes, settings, EEPROM, overrides, 30 s override timeout).

## Graphics and icon
`resources/mil_logo_{light,dark}.svg` and `resources/radm_sprite_{light,dark}.svg` are built into the program (via `resources/resources.qrc`), so the exe
needs no extra files. They are drawn in the top-right corner of the window, and `mil_logo_light.svg` is also the window / taskbar icon.
On Windows the icon of the `.exe` file itself comes from `resources/mil_logo.ico` (referenced by `resources/app_icon.rc`);
if you change the SVG, regenerate that .ico (PNG sizes 16-256 packed into one .ico file).

## Notes
* The board only sends data while DTR is asserted, so keep **Assert DTR** ticked on the Serial tab.
* In Debug / Silent mode the GUI sends a `ping` every few seconds so the RP2350 does not clear simulated values (it does so after 30 s of silence).
* Stream period and measurement interval are not stored in the EEPROM; they reset when the RP2350 reboots.

## Font

The GUI uses Cascadia Mono. The variable-weight font and the static Bold (used only
if your Qt version does not expose the variable font's Bold) are embedded from
`resources/fonts/` through `resources/resources.qrc`, so nothing has to be installed.

## Plots tab

Records the values the GUI receives and plots them over time (the RP2350 itself keeps
no history). Press **Start recording** to begin adding frames to the plots; press it
again to pause (a pause is drawn as a gap). **Clear plotted data** deletes everything,
**Save as CSV...** exports the recording, and **Time shown** limits the x axis to the
last 30 s ... 1 h. There is one plot per value (cells 1, 2, 3, 5, battery voltage,
temperature) plus a large overlay plot; tick the values to show in the overlay. In the
overlay every trace is scaled to its own min / max, and the legend lists the real
value and range of each trace. Hover over any plot to read the values at that time.
Up to 200 000 samples are kept (the oldest are dropped after that). In Silent mode
only frames you request (Read now / `telemetry`) are recorded.


## Connecting to a board stack on the vehicle (over SSH)

The GUI can connect over the network as well as to a serial port: type `tcp://host:port` in the Port box
(for example `tcp://localhost:5000`). Tick **Read-only** to listen without ever sending anything (no settings,
simulation, mode changes, polling or latch override). Read-only shows the Monitor and Plots tabs from the status
lines the RP2350 sends by itself in Normal mode.

Only one program can use a serial port, so run `tools/serial_relay.py` **on the Jetson**. It owns the real port and shares it:

```
python3 serial_relay.py /dev/ttyACM0 --pty-link /tmp/ttyBOARD
```

| Where | Access |
|---|---|
| TCP `127.0.0.1:5000` | read / write (GUI) |
| TCP `127.0.0.1:5001` | read-only (the relay discards anything a client sends) |
| `/tmp/ttyBOARD` (virtual serial port) | read / write - point the Jetson software at this instead of `/dev/ttyACM0` |

On your laptop open a tunnel, then connect the GUI to `tcp://localhost:5001` (read-only) or `tcp://localhost:5000`:

```
ssh -L 5000:localhost:5000 -L 5001:localhost:5001 user@jetson
```

The board's replies go to every client, so two programs sending commands at the same time see each other's replies.
If the SSH tunnel drops the GUI reconnects by itself (when Auto reconnect is on).
