# Build and library compatibility patches

## Pinned dependencies

| Library | Public upstream revision | Patch |
| --- | --- | --- |
| FreeRTOS-Kernel | V11.3.2, `8a59a4b19bf72977819feddef87d41163ced7004` | `kernel-V11.3.2.patch` |
| FreeRTOS-Plus-TCP | V4.4.2, `b86ad086e9870099bb0ac18a75dcb7f6dd508cc7` | `network-V4.4.2-zds.patch` |
| uzlib | master, `6d60d651a4499a64f2e5b21b4cc08d98cb84b5c1` | `uzlib-6d60d65-zds.patch` |

Patches are under `patches/`. Gitlinks point to public upstream revisions;
ZDS changes are distributed explicitly as patches, without requiring extra
forks or unpublished library commits. uzlib has no release tag newer than
v2.9.5; the pinned master commit is dated 15 November 2021.

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
git submodule update --init -- kernel network uzlib
python3 setup-dependencies.py
```

Patched working trees intentionally differ from upstream. Parent submodule
configuration suppresses expected dirty-file status; inspect a dependency itself
with commands such as `git -C kernel status --short`. The unused CP/M 3 submodule
is removed; active CP/M 2.2 integration is under `CPM/`.

## Fresh-clone build errors

Missing `kernel/include/FreeRTOS.h`, `uzlib/src/adler32.c` or
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

git clone https://github.com/pfalcon/uzlib.git uzlib
git -C uzlib checkout --detach 6d60d651a4499a64f2e5b21b4cc08d98cb84b5c1
git -C uzlib apply ../patches/uzlib-6d60d65-zds.patch
```

Do not blindly apply patches repeatedly or discard unrelated edits.

## ZDS II 5.3.5

1. Open `EZ80F910300ZCOG.zdsproj`.
2. Select **Debug**.
3. Run **Rebuild All**, then load the matching RAM target.

The main project includes required kernel, IPv4/routing, TCP/IP and uzlib
sources. The default installation is `C:\Zilog\ZDSII_eZ80Acclaim!_5.3.5`.
For another installation, update the additional include path
`ZTP2.5.1\RZK\Inc`, which supplies `_ez80.h`, `AMD79C874_phy.h` and I/O types.

Always rebuild fully after changing uzlib headers: generated make dependencies
may be incomplete. Outputs reside in `EZ80F910300ZCOG/Debug/` and `bin/`.
HEX, LOD, workspace and generated map files are ignored. Live tests must use
the map matching the firmware actually loaded.

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

## uzlib and CP/M

The old large compressor refactoring is already present upstream. The small
current patch retains C90 declaration order, ordinary static functions instead
of unsupported inline functions, 32-bit CRC/trailers and output-bound checks.
The wrapper treats `TINF_DONE` as success and validates CRC even when output
exactly fills its destination. It does not start CP/M with an invalid disk pointer.

CP/M uses reserved `CB 30`–`CB 35` BIOS traps. The emulator must handle them
through the F91 ADL supervisor TRAP and `RET.L` back to Z80 mode. Merely aborting
on every illegal opcode does not support this port.

Other application corrections include EMAC TLBP/BP/RHBP/RRP setup, signed socket
errors, closing clients before accept and `xTimerStartFromISR` for debounce timers.

## Tests

From the companion emulator workspace:

```sh
python3 tests/test_freertos_unzip.py
python3 tests/test_freertos_buttons.py /tmp/ez80f91-1000/4040.qmp
python3 tests/test_freertos_cpm.py <guest-ip>
```

The first is a host test of actual wrapper/uzlib sources. The others require
running firmware and its matching map/IP. See the
[test catalog](https://github.com/notwendig/qemuez80f91/blob/main/docs/testing.md).

For a complete freshly built board image:

```sh
python3 scripts/ihex_to_bin.py subprojects/FreeRTOS-eZ80f91/bin/EZ80F910300ZCOGd.hex build/freertos-current.bin
python3 tests/test_freertos_network.py build/ez80/qemu-system-ez80 build/freertos-current.bin
```

This isolated test needs neither bridge nor external DHCP. It checks DHCP,
ARP, ICMP Echo, TCP-console Uptime data and full-width IPv4 logs.

Checks executed on 30 October 2026 include all five kernel scenarios with reset,
the complete ZDS build, isolated networking, CP/M CRC/truncation/output-bound
checks, uzlib upstream corpus/malformed inputs, compressor round-trip and
fresh-clone/repeated dependency setup. Physical hardware remains unvalidated.
