# Build and library compatibility patches

## Pinned dependencies

| Library | Public upstream revision | Patch |
| --- | --- | --- |
| FreeRTOS-Kernel | V11.3.2, `8a59a4b19bf72977819feddef87d41163ced7004` | `kernel-V11.3.2.patch` |
| FreeRTOS-Plus-TCP | V4.4.2, `b86ad086e9870099bb0ac18a75dcb7f6dd508cc7` | `network-V4.4.2-zds.patch` |

Patches are under `patches/`. Gitlinks point to public upstream revisions;
ZDS changes are distributed explicitly as patches, without requiring extra
forks or unpublished library commits.

## Automatic setup

```sh
git clone https://github.com/notwendig/FreeRTOS-eZ80f91.git
cd FreeRTOS-eZ80f91
python3 setup-dependencies.py
```

The script clones missing libraries, verifies their selected revision and applies
patches. Already applied identical patches are recognized. Repeated invocation
is supported; unrelated local changes prevent automatic version switching.
Back up edits before upgrading an existing dirty library checkout.

Normal submodule initialization is also possible, but does not replace patch setup:

```sh
git submodule update --init -- kernel network
python3 setup-dependencies.py
```

Patched working trees intentionally differ from upstream. Parent submodule
configuration suppresses expected dirty-file status; inspect a dependency itself
with commands such as `git -C kernel status --short`. The unused CP/M 3 submodule
is removed; active CP/M 2.2 integration is under `CPM/`.

## Fresh-clone build errors

Missing `kernel/include/FreeRTOS.h` or
`network/source/portable/BufferManagement/BufferAllocation_2.c` means dependency
setup is incomplete. From this repository, run `python3 setup-dependencies.py`,
then select **Rebuild All** in ZDS. From the enclosing qemuez80f91 workspace,
run `python3 scripts/setup-dependencies.py` instead. The IDE build itself does
not download libraries or apply patches.

A recursive submodule update obtains upstream files but does not apply the
compatibility patches; always finish by running dependency setup.

## Manual setup

From the project directory, using new unchanged library checkouts:

```sh
git clone --branch V11.3.2 https://github.com/FreeRTOS/FreeRTOS-Kernel.git kernel
git -C kernel apply ../patches/kernel-V11.3.2.patch

git clone --branch V4.4.2 https://github.com/FreeRTOS/FreeRTOS-Plus-TCP.git network
git -C network apply ../patches/network-V4.4.2-zds.patch
```

Do not blindly apply patches repeatedly or discard unrelated edits.

## Build with VSCodium or the command line

From the parent `qemuez80f91` checkout, use VSCodium's **FreeRTOS: Build** task
or the Zilog command-line tools under Wine:

```sh
python3 tools/firmware.py build --config Release
python3 tools/firmware.py build --config Debug
python3 tools/flash_firmware.py --config Release --build --stop --run
```

Both configurations start from flash. Their outputs are in
`build/firmware/Release/` (`EZ80F910300ZCOG.*`) and
`build/firmware/Debug/` (`EZ80F910300ZCOGd.*`), separate from this subproject.
The main `.zdsproj` and its selected `.ztgt` remain shared build inputs.
The command-line build does not require generated IDE makefiles or a `.wsp` session.
See the parent checkout's `docs/vscodium.md` for configuration and backup details.

The flash target enables ADL mode, maps external AM29LV008BB flash to
`0x040000–0x13FFFF`, maps CS2 to `0x800000–0xBFFFFF` (including LED I/O),
and places internal SRAM at `0xB7E000–0xB7FFFF`. CS3 stays disabled.

## Build and debug with ZDS II 5.3.5

ZDS II remains supported alongside VSCodium:

1. Open `EZ80F910300ZCOG.zdsproj` in the IDE.
2. Select **Debug** or **Release**; both use `eZ80F91_99C0942_Flash` and flash boot.
3. Run **Rebuild All**. The IDE regenerates its build directories and makefiles.
4. Connect the Ethernet Smart Cable target, load the matching LOD and run/debug.

Debug writes `bin/EZ80F910300ZCOGd.hex`/`.lod`/`.map`, with intermediate files
in `EZ80F910300ZCOG/Debug/`. Release writes `Release/EZ80F910300ZCOG.*`.
These outputs are separate from `build/firmware/`, so both workflows can coexist.
Opening the `.zdsproj` also recreates the local `.wsp` session; the previous
session's window positions and debugger layout were removed during cleanup.


External flash programming uses the explicitly selected `Am29LV008BB`;
autoselection is disabled and the programmer base is `040000` (the CS0
lower address, not its upper address). Check the linker map for RAM overflow
warnings before loading a LOD. An out-of-range RAM vector section can surface
as `ERR_DTLI_INVALID_TOKEN` because the underlying DTL memory write fails.

- Internal flash: `0x000000–0x03FFFF` for program code and ordinary constants.
- CP/M disks: separate writable A/B/C/D/I/J images behind a WD2797A
  controller. Sector data moves through I/O ports; no CS3 disk RAM is used.
- Application data, stack and first heap: external RAM `0xC00000–0xC3FFFF`.
  CP/M reserves `0xB80000–0xB8FFFF`; the additional heap remains at
  `0xB90000–0xBFFFFF`.

`FLASH_BOOT` distinguishes cold reset from the CP/M BIOS trap at the shared
reset vector, preserving the task registers for BIOS calls. A debugger-provided
stack is supported before Timer 0 starts; board and port initialization enable
the mixed ADL mode required by FreeRTOS and CP/M. CP/M starts directly from
raw CS3 disks without decompression or disk-sized heap allocation. The BIOS uses
IBM 3740 geometry (boot tracks only on A) and 4mb-hd geometry for I/J.

From the parent qemuez80f91 repository, rebuild the system tracks and images:

```sh
python3 tools/cpm/build_system.py
python3 tools/cpm/build_disks.py
```

Start from the parent repository with:

```sh
qemuez80f91 --kit --run --wd2797a 0x1000 --dev '/path/to/disks'
```

The directory contains raw A.img/B.img/etc. (plain A/B names also work).
The first four controller ports are Status/Command, Track, Sector and Data.
Board extensions provide drive selection, DRQ/INTRQ/READY and the high track
byte for I/J. Firmware discovers the configured base through I/O FF00–FF07,
so changing the base does not require rebuilding. QEMU saves modified files
on orderly exit. Without --dev, the starter preserves individual per-debug-port
images and migrates the prior combined image on first use.

CS3 is disabled; existing CS1/CS2, application RAM and LED decoding remain
unchanged. CP/M boots by reading A: track 0, sector 1 into its existing RAM.
There is no embedded disk, decompression, large disk allocation or memory bank.
Controller details and limitations are documented in the parent repository's
`docs/wd2797a.md`.

The DHCP hostname hook reads QEMU's read-only `HOST` descriptor at external
I/O FF08–FF4B. The default is `ez80<debugport>`, or use launcher option
`--hostname <name>`. Rebuild and load firmware once after this update;
subsequent hostname changes require only a QEMU restart. On physical hardware,
without that descriptor, the compiled-in `ez80` hostname remains the fallback.
The TCP/IP compatibility patch also splits the DHCP hostname option pointer
calculation into separate steps: ZDS 5.3.5 otherwise miscompiles the nested
unsigned addition/subtraction and writes the option outside the options area.

The SysInfo monitor (`nc -u ez804040 4060`, then Enter) continues receiving
UDP datagrams between display updates. A new sender IP/port takes over the
display and receives a fresh banner, so reopening netcat works without a guest
restart. The monitor uses the timed receive in place of its previous 200-tick
delays and retains its active address when no datagram arrives.

Close the ZDS project **before** pulling a project configuration update, then
reopen it and run **Rebuild All**. An already-open IDE can save its cached old
settings over the updated `.zdsproj` file; reopening after that save cannot
recover the update. If this has already happened, back up local project changes,
restore the updated project file from Git while it is closed, and reopen it.
Do not repair generated `.mak` files: ZDS replaces them on the next build.

Missing headers only in Release indicate outdated configuration/include paths,
rather than missing libraries when Debug already builds. The Release settings
must include `EZ80F910300ZCOG\inc`, `kernel\include`,
`network\source\include` and the installed RZK headers.

 The default installation is `C:\Zilog\ZDSII_eZ80Acclaim!_5.3.5`.
For another installation, update the additional include path
`ZTP2.5.1\RZK\Inc`, which supplies `_ez80.h`, `AMD79C874_phy.h` and I/O types.

Always rebuild fully after changing library headers: generated make dependencies
may be incomplete. Outputs reside in `EZ80F910300ZCOG/Debug/` and `bin/`.
HEX, LOD, workspace and generated map files are ignored. Live tests must use
the map matching the firmware actually loaded.

## Firmware command console

Both configurations start the FreeRTOS+CLI UDP server on guest port **4030**
after the network comes up. Connect to the guest DHCP hostname/address, for example:

```sh
nc -u ez804040 4030
```

Enter `help` followed by Enter. Available commands include `ip-config`,
`net-stats`, `ping`, `date`, `time`, `dump`, `ihex`, `echo-parameters` and
`echo-3-parameters`. Commands are newline-terminated and limited to 59 characters.
The server accepts CRLF, backspace and fragmented lines, rejects oversized lines,
and discards unfinished input when a different client attaches.

This command console is separate from the TCP diagnostic output on guest port
4040, CP/M on TCP 4050, the UDP SysInfo display on 4060 and the host ZDS debugger.
`FreeRTOS_CLI.c`/`.h` are vendored unchanged from the MIT-licensed official
[FreeRTOS 202212.01 release](https://github.com/FreeRTOS/FreeRTOS/tree/202212.01/FreeRTOS-Plus/Source/FreeRTOS-Plus-CLI).
The project includes the CLI core, existing application commands and the updated
UDP transport directly, so no additional library setup is required.

Both configurations currently use `eZ80F91_99C0942_Flash`. Rebuild after changing the target because
startup memory configuration comes from linker-generated target symbols.

## FreeRTOS 11 port

The port remains 24-bit ADL with `TickType_t=uint24_t`. Legacy
`configUSE_16_BIT_TICKS` retains the small event-group configuration; it does
not change the port typedef to 16 bits. The rollover test crosses `0xffffff`.

Static idle/timer memory callbacks use `configSTACK_DEPTH_TYPE *` and return
stack-element counts rather than bytes. Assembly context switching and vectors
are unchanged. The kernel patch exports the heap header size, removes discarded
volatile expressions unsupported by ZDS and makes the static timer queue size
compatible with the compiler.

`pdTICKS_TO_MS` uses quotient/remainder 32-bit arithmetic instead of scalar
`uint64_t`, which ZDS does not support. Local integer headers supply the
additional standard limits required by the new stack.

## TCP/IP 4 and EMAC

The driver registers a `NetworkInterface_t`, assigns incoming buffers to an
interface/endpoint and reports link changes through that interface. The board
retains a single IPv4 interface with IPv6 disabled. Unused DHCP application
hooks are explicitly disabled.

Pointer validation/buffer allocation use the port's byte alignment instead of
assuming `sizeof(size_t)` is a power of two: eZ80 pointers occupy three bytes.
Bitfields are cast to `unsigned` before switch statements for ZDS compatibility.
Allocation sizes and Ethernet-header pointers are explicitly initialized so ZDS
can establish their validity without three possibly-uninitialized warnings.

IPv4 diagnostics use `%lxip` and `unsigned long`. Upstream `%xip` with an
`unsigned int` cast lost the upper byte on eZ80; signed formatting then printed
`255.168.77.26` instead of `192.168.77.26`. The network test checks complete
DHCP/ARP addresses as well as traffic.

## CP/M drive storage

CP/M uses raw writable disk images or Unix directory drives through QEMU's
native I/O ports. No decompression library is required. The legacy
`CPM/CPM22_A.c` compressed fixture is retained only as input to the companion
emulator's Python disk-image builder; it is not compiled into the firmware.
`CPM/Makefile` builds and verifies an uncompressed `A.DSK`.

Other application corrections include EMAC TLBP/BP/RHBP/RRP setup, signed socket
errors, closing clients before accept and `xTimerStartFromISR` for debounce timers.

## Tests

From the companion emulator workspace:

```sh
python3 tests/test_freertos_buttons.py /tmp/ez80f91-1000/4040.qmp
python3 tests/test_freertos_cpm.py <guest-ip>
```

These tests require running firmware and its matching map/IP. See the
[test catalog](https://github.com/notwendig/qemuez80f91/blob/main/docs/testing.md).

For a complete freshly built board image:

```sh
python3 tools/firmware.py build --config Release
python3 scripts/ihex_to_bin.py build/firmware/Release/EZ80F910300ZCOG.hex build/freertos-current.bin
python3 tests/test_freertos_network.py build/ez80/qemu-system-ez80 build/freertos-current.bin
```

This isolated test needs neither bridge nor external DHCP. It checks DHCP,
ARP, ICMP Echo, TCP-console Uptime data and full-width IPv4 logs.

Checks executed on 30 October 2026 include all five kernel scenarios with reset,
the complete ZDS build, isolated networking, CP/M CRC/truncation/output-bound
checks, uzlib upstream corpus/malformed inputs, compressor round-trip and
fresh-clone/repeated dependency setup. Physical hardware remains unvalidated.


## CAMEL24 in this workspace

`EZ80F910300ZCOG.zdsproj` includes the CAMEL24 kernel sources from the sibling
`../CAMELF91_24/asm` checkout. `CAMEL_FREERTOS` selects its task entry and private
memory; `FLASH_BOOT` is enabled for both Debug and Release. Built-in dictionary
words are linked into Flash. Runtime definitions use `B80000–B8DFFF`; the static
FreeRTOS task stack occupies `B8E000–B8EBFF`. The remaining reserved bank contains
Forth return/leave stacks and temporary buffers. It is excluded from heap_5.

`camel_task.c` starts the interpreter at priority 2. The port console adapter
waits with `vTaskDelay(1)` rather than reinitializing the board or scheduler.
The standalone Kit ROM continues to build with `CAMEL_FREERTOS` undefined.
