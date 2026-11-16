# FreeRTOS-eZ80f91

[English](readme.md) · [Build-Details](BUILD.md)

FreeRTOS-Port und Demoanwendung für das Zilog eZ80F91 und das EZ80F910300ZCOG-Kit. Das Projekt umfasst Tasks, UART0, Ethernet/DHCP, CP/M-Laufwerke, Tasten und eine 5×7-LED-Matrix.

## Reproduzierbarer Einstieg

Benötigt werden ZDS II eZ80Acclaim! 5.3.5 mit ZTP/RZK, Python 3 und Git. Nach dem Klonen zuerst `python3 setup-dependencies.py` ausführen. Das Skript richtet die dokumentierten Upstream-Versionen und Projektpatches ein. Anschließend `EZ80F910300ZCOG.zdsproj` im Debug-Modus mit **Rebuild All** bauen. Einzelheiten und Startbedingungen stehen in der [englischen Anleitung](readme.md).

## Grenzen und Rechte

Netzwerk- und Hardwarefunktionen sind nur in der dokumentierten Testumgebung belegt. Die Abhängigkeiten besitzen eigene Lizenzen; für das Gesamtprojekt fehlt eine eindeutige Lizenzdatei. Vor einer Veröffentlichung von Binärpaketen Rechte und ZDS-Bestandteile prüfen.
