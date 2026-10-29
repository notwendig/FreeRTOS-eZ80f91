# FreeRTOS-eZ80f91

FreeRTOS-Port und Demonstrationsanwendung für den **Zilog eZ80F91** und das
Development Kit EZ80F910300ZCOG. Die Anwendung kombiniert präemptive Tasks,
UART0, Ethernet/DHCP, eine CP/M-2.2-Ramdisk sowie Taster und 5×7-LED-Matrix.

## Voraussetzungen

- ZDS II eZ80Acclaim! **5.3.5**, einschließlich ZTP 2.5.1/RZK-Includes;
- Python 3 und Git für das Bibliothekssetup;
- echtes Development Kit mit passendem Debugger oder
  [qemuez80f91](https://github.com/notwendig/qemuez80f91);
- UART0 mit 115200 Baud, 8N1, RTS/CTS;
- für Netzwerkfunktionen ein erreichbarer DHCP-Server.

## Reproduzierbarer Build

```sh
git clone https://github.com/notwendig/FreeRTOS-eZ80f91.git
cd FreeRTOS-eZ80f91
python3 setup-dependencies.py
```

Das Setup verwendet die **öffentlichen Upstream-Tags** und wendet die
mitgelieferten eZ80-/ZDS-Kompatibilitätspatches an. Dafür sind keine privaten
Bibliotheks-Forks und keine nicht veröffentlichten Commitobjekte erforderlich.
Die gepatchten Bibliotheks-Arbeitsbäume dürfen danach Änderungen anzeigen;
versioniert werden die Original-Basistags und die Patches dieses Projekts.

Danach `EZ80F910300ZCOG.zdsproj` in ZDS öffnen, **Debug** wählen und
**Rebuild All** ausführen. Kernel, Netzwerk und uzlib werden bereits in diesem
Projekt übersetzt; getrennte Bibliotheks-Builds sind nicht erforderlich.
Die ausführliche [Build- und Patchanleitung](BUILD.md) erklärt Versionen,
Pfadkonfiguration und manuelle Patchanwendung.

## Start und Dienste

Die Firmware in das passende RAM-/Flash-Target laden und starten. Die
UART-Konsole meldet Board-/Taskzustand und die per DHCP bezogene IP-Adresse.
Für die Emulation öffnet `./start.sh --kit` im Emulator-Workspace UART und
Kit-Fenster; die CPU wartet anschließend auf den ZDS-Debugger.

| Schnittstelle | Funktion |
| --- | --- |
| UART0 | Startmeldungen, Task- und Netzwerkdiagnose |
| TCP 4040 am Host | Ethernet-Smart-Cable-Debugger des Emulators; keine Gastanwendung |
| TCP 4050 am Gast | CP/M-2.2-Konsole |
| UDP 4060 am Gast | SysInfo-Monitor; ein erstes Datagramm startet die Anzeige |
| Kit | SW1–SW3, Entprellung und 5×7-Matrix mit Laufschrift |

Die Host-IP für den Debugger und die Gast-IP für CP/M/SysInfo sind
verschiedene Adressen. Beim Emulator-Standardstarter liefert nadibox die
DHCP-Lease über `br0`; das Firmwareprojekt selbst richtet keinen DHCP-Server ein.

```sh
nc -u <gast-ip> 4060
nc <gast-ip> 4050
```

Das erste Entpacken der CP/M-Ramdisk kann in der Emulation mehrere Minuten
dauern. `#` zeigt Fortschritt an; danach erscheinen Loader und `a>`.

## Verzeichnisstruktur

| Verzeichnis/Datei | Inhalt |
| --- | --- |
| `EZ80F910300ZCOG/src`, `inc` | Anwendung, Boardstart, Treiber und Konfiguration |
| `EZ80F910300ZCOG.zdsproj` | Hauptprojekt für ZDS II |
| `kernel`, `network`, `uzlib` | Öffentliche Bibliotheksbasen mit lokal angewendeten Patches |
| `patches/` | Versionierte ZDS-/eZ80-Anpassungen |
| `setup-dependencies.py` | Wiederholbares Setup ohne Überschreiben fremder Änderungen |
| `CPM/` | CP/M-2.2-Ramdisk und BIOS-/TRAP-Anbindung |
| `bin/` | Linker-Map und lokale Buildausgaben |
| `doc/` | Vorhandene Hardware-/CP/M-Unterlagen und ursprüngliche Dokumentquellen |

## Enthaltene Korrekturen und Tests

Der aktuelle Stand richtet den EMAC-Empfangsring einschließlich RRP korrekt
ein, verwendet die ISR-API für die Tasterentprellung und prüft beim Entpacken
Ausgabegrenzen, CRC und 32-Bit-Trailerwerte. CP/M-Clients werden vor dem
nächsten `accept()` geschlossen. Die Demo startet mit Rechts-Laufschrift.

Der Emulator-Workspace enthält Tests für C-/Boardstart, Kontextwechsel,
Yield/Delay, Ticküberlauf, Dekompression, Taster und CP/M-Verbindungen.
Ein bestandener Emulatortest ersetzt keinen vollständigen Hardwaretest.
Der funktionale Emulator-PHY kann bei unvollständigen Herstellerregistern
Geschwindigkeit/Duplex irreführend melden; dies misst nicht das Host-Netz.

## Herkunft und Lizenzhinweise

FreeRTOS eZ80F91 Acclaim!-Port: Copyright © 2016–2023 Jürgen Sievers / NadiSoft,
wie in den ursprünglichen Quellköpfen angegeben. Der Port wurde für das
Zilog Development Kit erstellt. Die Lizenztexte und Ausnahmen in den
Port-Quellen und den jeweiligen Bibliotheken bleiben maßgeblich.
ZDS II, Nexus und ZTP werden separat installiert und gehören nicht zum
veröffentlichten Bibliothekssetup.
