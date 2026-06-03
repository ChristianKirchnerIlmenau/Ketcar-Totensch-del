# KetCar-Totenschaedel Implementierungsplan

Stand: 03.06.2026

## Ziel

Robuste, rein lokale Arduino-Loesung fuer bewegungsgetriggerte Effektbeleuchtung mit MPU6050-Wakeup und 5s Leuchtzeitfenster pro Ereignis.

## Aktueller Umsetzungsstand

- Firmware laeuft auf Arduino Pro Mini (ATmega328P).
- MPU6050 ist per I2C angebunden und als Motion-Interruptquelle konfiguriert.
- Wakeup erfolgt ueber externen Interrupt auf D2 (RISING).
- Lichtfenster ist retriggerbar: jedes gueltige Motion-Ereignis setzt Deadline auf now + 5000ms.
- Ohne aktives Licht geht die MCU in Power-Down Sleep.
- LEDs laufen als 2x RGB-Gruppe mit PWM-Breathing-Effekt statt statischem EIN/AUS.
- Serielle Diagnosen sind aktiv (Heartbeat, Wake-Zaehler, ISR-Zaehler, Motion-Level).

## Festgelegte Hardware

- Controller: Arduino Pro Mini 328P
- Sensor: MPU6050 (GY-521)
- LED-Ausgaenge: D9/D10/D11 und D3/D5/D6 (2x RGB)
- Akkuziel: 2x LiPo 3.7V 2000mAh parallel (4000mAh)

## Laufzeit- und Ereignislogik

1. ISR bleibt schlank und setzt nur ein IRQ-Flag.
2. Hauptschleife liest INT-Status vom MPU6050 und verwirft ungueltige IRQs.
3. Bei gueltigem Motion-Bit wird ein Motion-Level aus den Beschleunigungsdaten geschaetzt.
4. active_until wird auf now + 5000ms gesetzt.
5. Solange Zeitfenster aktiv ist, werden die LEDs dynamisch aktualisiert.
6. Nach Ablauf wird Licht abgeschaltet und Sleep vorbereitet.

## Stabilitaetsregeln

- Keine I2C-Operationen in der ISR.
- Latch-Interrupt wird ueber INT_STATUS-Read quittiert.
- Wake-Guard-Zeit nach Sleep-Rueckkehr ist aktiv.
- Spurious IRQs werden gezaehlt und ueber Serial ausgegeben.

## Offene Punkte

1. Reale Strommessung fuer active und sleep als Messprotokoll nachziehen.
2. Motion-Schwelle und Duration unter realen Vibrationen feinjustieren.
3. Optionalen Recovery-Pfad fuer dauerhaft anliegenden INT testen und dokumentieren.

## Abnahmekriterien

1. Wakeup aus Power-Down funktioniert reproduzierbar ueber Bewegung.
2. 5s-Lichtfenster verlaengert sich bei Folgeereignissen korrekt.
3. Ohne Folgeereignis endet Lichtfenster stabil, danach Sleep.
4. Keine Daueraktivierung bei Fehltriggern im 24h-Dauertest.
