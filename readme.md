# FreeRTOS-eZ80f91

FreeRTOS port and demonstration application for the **Zilog eZ80F91** and
EZ80F910300ZCOG development kit. It combines preemptive tasks, UART0,
Ethernet/DHCP, a CP/M 2.2 RAM disk, buttons and a scrolling 5×7 LED matrix.

## Requirements

- ZDS II eZ80Acclaim! **5.3.5**, including ZTP 2.5.1/RZK headers.
- Python 3 and Git for dependency setup.
- A development kit/debugger or [qemuez80f91](https://github.com/notwendig/qemuez80f91).
- UART0 at 115200 baud, 8N1, RTS/CTS.
- An accessible DHCP server for ordinary network operation.

## Reproducible build

```sh
git clone https://github.com/notwendig/FreeRTOS-eZ80f91.git
cd FreeRTOS-eZ80f91
python3 setup-dependencies.py
```

Setup uses public upstream revisions plus supplied ZDS/eZ80 patches:
FreeRTOS-Kernel **V11.3.2**, Plus-TCP **V4.4.2**, uzlib **6d60d65**.
No private library fork or unpublished commit is required. Library working
trees intentionally contain applied changes; this project tracks upstream
revisions and patches.

Open `EZ80F910300ZCOG.zdsproj`, select **Debug**, then **Rebuild All**.
The main project already compiles kernel, networking and uzlib; separate library
builds are unnecessary. [Build instructions](BUILD.md) cover pinned commits,
paths, manual setup and compatibility details.

## Start and services

Load firmware into the matching RAM/flash target and run. UART reports board,
task and DHCP state. In the emulator workspace, `./start.sh --kit` opens UART
and kit windows, initially waiting for the ZDS debugger.

| Interface | Function |
| --- | --- |
| UART0 | Startup, task and network diagnostics |
| Host TCP 4040 | Emulator Ethernet Smart Cable debugger |
| Guest TCP 4040 | Firmware diagnostic console, independent of the host debugger |
| Guest TCP 4050 | CP/M 2.2 console |
| Guest UDP 4060 | SysInfo monitor; the first datagram starts output |
| Kit | SW1–3, debouncing and scrolling matrix |

Host debugger and guest application addresses are different. The standard
emulator launcher uses `br0`; nadibox is the DHCP server on the development LAN,
not a firmware requirement. Another router or DHCP server can supply the lease.
The firmware does not create a DHCP server.

**Using another network:** connect the host bridge to that LAN. To change the
bridge name or QEMU bridge-helper path for `start.sh`, update the bridge checks,
`-nic` options and `start_after_bridge` call in the emulator's
[`tools/start.py`](https://github.com/notwendig/qemuez80f91/blob/main/tools/start.py).
Authorize the selected bridge in the helper's allowlist, normally
`/etc/qemu/bridge.conf`. See
[network setup](https://github.com/notwendig/qemuez80f91/blob/main/docs/getting-started.md#using-another-network).
Use the resulting guest DHCP address for application connections and the host
address in ZDS debugger settings.

```sh
nc -u <guest-ip> 4060
nc <guest-ip> 4050
```

Initial disk decompression may take minutes in emulation. `#` shows progress,
followed by loader messages and `a>`.

## Layout

| Path | Contents |
| --- | --- |
| `EZ80F910300ZCOG/src`, `inc` | Application, startup, drivers and configuration |
| `EZ80F910300ZCOG.zdsproj` | Main ZDS project |
| `kernel`, `network`, `uzlib` | Public library revisions with applied patches |
| `patches/` | Tracked compatibility changes |
| `setup-dependencies.py` | Repeatable setup that refuses unrelated dirty version changes |
| `CPM/` | CP/M disk and BIOS/TRAP integration |
| `bin/` | Generated images and matching linker maps |
| `doc/` | Existing hardware/CP/M references and original document sources |

## Fixes and tests

The EMAC driver initializes ring boundaries/RRP and registers a TCP/IP 4 interface.
Button debounce uses the ISR timer API. Disk decompression validates bounds,
CRC and full-width trailers; clients are closed before the next accept.
The demonstration scrolls to the right. IPv4 logs use 32-bit arguments rather
than truncating addresses to eZ80's 24-bit `unsigned int`.

The emulator workspace provides startup, switching, blocking, rollover,
decompression, buttons, CP/M and isolated DHCP/ARP/ICMP/TCP tests. These do not
replace physical-hardware validation. The functional PHY may show misleading
speed/duplex until vendor registers are completed; it does not measure host-link speed.

## Origins and notices

Original port notices identify Copyright © 2016–2023 Jürgen Sievers / NadiSoft.
Source-file license texts/exceptions and dependency licenses remain authoritative.
This README does not replace them. ZDS, Nexus and ZTP are separately installed
vendor tools, not included in dependency setup.
