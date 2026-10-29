# Build mit ZDS II 5.3.5

Die öffentlichen Basisversionen sind FreeRTOS-Kernel V10.6.1,
FreeRTOS-Plus-TCP V3.1.0 und uzlib v2.9.5. Die ursprünglich eingetragenen
Submodul-Commits enthalten lokale Änderungen und sind von den öffentlichen
Upstreams nicht abrufbar. Deshalb ohne rekursive Submodule klonen und danach:

```sh
python3 setup-dependencies.py
```

Das Skript richtet diese drei Tags ein und wendet die mitgelieferten
ZDS/eZ80-Patches sowie `patches/uzlib-zds-inline.patch` an. Es ist wiederholbar
und überschreibt keine lokalen Änderungen beim Wechsel einer vorhandenen
Version. Das nicht konfigurierte CP/M-3-Submodul wird für den CP/M-2.2-Debugbuild
nicht benötigt und nicht abgerufen.

In ZDS II `EZ80F910300ZCOG.zdsproj` öffnen, Konfiguration **Debug** auswählen
und **Rebuild All** ausführen. Dieses Projekt enthält bereits die benötigten
Kernel-, TCP/IP- und uzlib-Quellen; separate Bibliotheks-Builds sind dafür
nicht erforderlich. ZDS II und ZTP werden unter
`C:\Zilog\ZDSII_eZ80Acclaim!_5.3.5` erwartet. Bei einer anderen Installation
muss der zusätzliche RZK-Include-Pfad im Compiler-Setup angepasst werden.

Build-Korrekturen: gültiger ZTP/RZK-Include-Pfad für `_ez80.h`,
`AMD79C874_phy.h` und die I/O-Typen sowie normale statische uzlib-Hilfsfunktionen
statt des vom ZDS-Compiler nicht unterstützten C99-`inline`.

## Ethernet-Laufzeitkorrektur

Der Empfangsring wird mit TLBP/BP/RHBP in der richtigen Reihenfolge und
RRP=BP initialisiert. Der Resetwert RRP=0 liegt sonst außerhalb eines
Empfangsrings mit nichtnull Startadresse und verhindert eingehende Pakete.
DHCP-Tick-Debugausgaben konvertieren 24-Bit-Ticks vor `%lu` explizit in
`unsigned long`; das zusätzliche Patch wird vom Setup angewendet.

## CP/M-Ramdisk auf TCP 4050

Gzip-CRC und Trailerwerte verwenden explizit `uint32_t`, da eZ80-`int` nur
24 Bit hat. `TINF_DONE` ist der erfolgreiche Abschluss, nicht ein Fehler.
Der Entpacker trennt Fortschrittsanzeige vom Eingabecallback, kontrolliert
Ausgabegrenzen und prüft den CRC auch bei exakt gefülltem Zielpuffer.
Bei einem Ladefehler wird der CP/M-Task beendet, bevor ein NULL-Zeiger
verwendet werden kann. Der Checksum-Patch wird vom Setup mit angewendet.

Nach Änderungen an uzlib-Headern immer vollständig neu bauen, da die
generierten Make-Regeln keine vollständigen Header-Abhängigkeiten enthalten.

CP/M verwendet reservierte `CB 30`–`CB 35` als BIOS-Traps. Der Emulator
muss diese am globalen TRAP-Vektor im ADL-Supervisor behandeln und mit
`RET.L` in den Z80-Modus zurückkehren; ein Abbruch bei illegalen Opcodes
unterstützt diesen Port nicht. Die CPU-Regression dafür liegt in
`tests/test_illegal_traps.py` im Emulatorprojekt.

Das erste Entpacken kann in der Emulation mehrere Minuten dauern. Die
`#`-Zeichen melden Fortschritt. Danach erscheinen der CP/M-Loader und `a>`.

Die Telnet-Schleife behandelt Empfangsfehler mit einem vorzeichenbehafteten
Rückgabewert und schließt jeden angenommenen Client vor dem nächsten
`accept()`. Andernfalls blockiert der alte Socket den einzigen Backlog-Platz.
Live-Test nach Start des Targets:

```sh
python3 tests/test_freertos_cpm.py 192.168.77.26
```

Die Kit-Taster starten ihren Entprelltimer mit `xTimerStartFromISR`, da
der Aufruf aus dem GPIO-Interrupt erfolgt. Die normale Task-API kann dort
einen unzulässigen Schedulerwechsel auslösen.
