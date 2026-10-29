# Build und Bibliothekspatches

## Unterstützter Stand

| Bibliothek | Öffentliche Basis | Anzuwendende Patches in Reihenfolge |
| --- | --- | --- |
| FreeRTOS-Kernel | `V10.6.1` | `kernel-V10.6.1.patch` |
| FreeRTOS-Plus-TCP | `V3.1.0` | `network-V3.1.0.patch`, `network-zds-tick-print.patch` |
| uzlib | `v2.9.5` | `uzlib-v2.9.5.patch`, `uzlib-zds-inline.patch`, `uzlib-zds-checksum.patch` |

Alle Patches liegen unter `patches/`. Die Gitlinks verweisen auf öffentliche
Basistags; die eZ80-Anpassungen werden ausdrücklich als Patches veröffentlicht.
Es werden keine zusätzlichen Forks benötigt.

## Automatisches Setup

```sh
git clone https://github.com/notwendig/FreeRTOS-eZ80f91.git
cd FreeRTOS-eZ80f91
python3 setup-dependencies.py
```

Das Skript klont fehlende Bibliotheken, prüft ihre Basistags und wendet die
Patches in der angegebenen Reihenfolge an. Bereits identisch angewendete
Patches werden erkannt; ein weiterer Aufruf ist zulässig. Fremde lokale
Änderungen verhindern einen automatischen Versionswechsel.

Eine normale Submodulinitialisierung ist ebenfalls möglich, ersetzt das
Patchsetup jedoch nicht:

```sh
git submodule update --init -- kernel network uzlib
python3 setup-dependencies.py
```

Nach dem Setup sind die Bibliotheks-Arbeitsbäume absichtlich gegenüber den
Upstream-Tags geändert. Die Submodulkonfiguration unterdrückt diese erwarteten
Arbeitsbaumänderungen in der Statusübersicht des Elternprojekts. Für die
Bibliothek selbst gilt weiterhin etwa `git -C kernel status --short`.
Die früheren lokalen Bibliothekscommits werden nicht als Downloadvoraussetzung
verwendet. Das ungenutzte, nicht konfigurierte CP/M-3-Submodul wurde entfernt;
der aktive CP/M-2.2-Pfad liegt unter `CPM/`.

## Manuell anwenden

Aus dem Projektverzeichnis, für neue, unveränderte Bibliothekscheckouts:

```sh
git clone --branch V10.6.1 https://github.com/FreeRTOS/FreeRTOS-Kernel.git kernel
git -C kernel apply ../patches/kernel-V10.6.1.patch

git clone --branch V3.1.0 https://github.com/FreeRTOS/FreeRTOS-Plus-TCP.git network
git -C network apply ../patches/network-V3.1.0.patch
git -C network apply ../patches/network-zds-tick-print.patch

git clone --branch v2.9.5 https://github.com/pfalcon/uzlib.git uzlib
git -C uzlib apply ../patches/uzlib-v2.9.5.patch
git -C uzlib apply ../patches/uzlib-zds-inline.patch
git -C uzlib apply ../patches/uzlib-zds-checksum.patch
```

Bei bestehenden, bearbeiteten Checkouts das automatische Setup verwenden
oder Änderungen zuerst separat sichern. Patches nicht blind mehrfach anwenden.

## ZDS II 5.3.5

1. `EZ80F910300ZCOG.zdsproj` öffnen.
2. Konfiguration **Debug** auswählen.
3. **Rebuild All** ausführen und das passende RAM-Target laden.

Das Hauptprojekt enthält die erforderlichen Kernel-, TCP/IP- und uzlib-Quellen.
ZDS und ZTP werden unter `C:\Zilog\ZDSII_eZ80Acclaim!_5.3.5` erwartet.
Bei anderer Installation den zusätzlichen Compiler-Includepfad
`ZTP2.5.1\RZK\Inc` anpassen; dort liegen unter anderem `_ez80.h`,
`AMD79C874_phy.h` und die I/O-Typen.

Nach Änderungen an uzlib-Headern vollständig neu bauen. Die generierten
Make-Regeln bilden nicht jede Headerabhängigkeit ab. Buildprodukte liegen
unter `EZ80F910300ZCOG/Debug/` und `bin/`; `.hex`, `.lod` und `.wsp` sind
ignoriert. Auch die generierte Debug-Map wird nicht versioniert; die
Live-Tests verwenden die zum aktuell gebauten und geladenen Image passende Map.

## Inhalt der Anpassungen

- Kernel: eZ80-Kontextwechsel, ZDS-ABI und Heap-/Schedulerkompatibilität.
- TCP/IP: ZDS-Kompatibilität und korrekte Ausgabe der 24-Bit-Ticks nach
  expliziter Konvertierung für `%lu`.
- uzlib: normale statische Hilfsfunktionen statt nicht unterstütztem
  C99-`inline`; explizite `uint32_t`-Werte für Gzip-CRC und Trailer.
- Anwendung: EMAC-Grenzen TLBP/BP/RHBP und RRP=BP, vorzeichenbehaftete
  Socketfehler, Client-Abbau vor erneutem `accept()` und ISR-konformer
  Entprelltimerstart mit `xTimerStartFromISR`.
- CP/M-Wrapper: `TINF_DONE` als Erfolg, Ausgabegrenzen und CRC auch bei exakt
  gefülltem Zielpuffer; kein Start mit einem ungültigen Ramdiskzeiger.

CP/M verwendet reservierte `CB 30`–`CB 35` als BIOS-Traps. Der Emulator muss
sie am F91-TRAP-Vektor im ADL-Supervisor behandeln und mit `RET.L` in den
Z80-Modus zurückkehren. Ein Emulator, der bei jedem illegalen Opcode lediglich
abbricht, unterstützt diesen CP/M-Port nicht.

## Prüfen

Im begleitenden Emulator-Workspace:

```sh
python3 tests/test_freertos_unzip.py
python3 tests/test_freertos_buttons.py /tmp/ez80f91-1000/4040.qmp
python3 tests/test_freertos_cpm.py <gast-ip>
```

Der erste Test ist ein Hosttest mit den tatsächlichen Wrapper-/uzlib-Quellen.
Die beiden anderen benötigen laufende Firmware und deren passende Mapdatei.
Weitere CPU-/Kernel-/Netzwerkprüfungen beschreibt der
[Testkatalog](https://github.com/notwendig/qemuez80f91/blob/main/docs/testing.md).
