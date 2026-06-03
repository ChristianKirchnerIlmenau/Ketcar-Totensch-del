# KetCar-Totenschaedel ChatHistorie

## Metadaten
- Datumskontext: 03.06.2026
- Zielprojekt: Akku-Effektbeleuchtung fuer Ketcar, ausgeloest durch Bewegung
- Endentscheidung: Arduino-only, kein ESP, kein WLAN, kein OTA, kein Home Assistant

## Ausgangsidee
- Bewegungsgetriggerte Effektbeleuchtung fuer Ketcar.
- Sensor: MPU6050/GY-521.
- Verhalten: Bewegung startet Licht-Timer, neue Bewegung setzt Timer zurueck.
- Urspruengliche Laufzeitidee: ca. 30 Sekunden.
- Wunsch nach DeepSleep wegen Akkubetrieb.

## Erste technische Klaerung
- ESPHome-Workspace wurde auf wiederverwendbare Muster geprueft.
- DeepSleep- und Timer-Muster wurden im Bestand gefunden.
- MPU6050-Hardwaredaten waren vorhanden, aber keine fertige MPU6050-Implementierung im Repo.
- Erkenntnis: Technisch machbar, aber zunaechst als Architekturfrage klaeren.

## Richtungswechsel durch Nutzerentscheidung
- Nutzer schliesst ESP fuer dieses Vorhaben explizit aus.
- Keine Einbindung in Home Assistant.
- Kein OTA und kein WLAN.
- Prioritaeten: so einfach wie moeglich, so akkuschonend wie moeglich, so robust wie moeglich.

## Machbarkeitsbewertung Arduino + MPU6050
- Wakeup via externem Interrupt vom MPU6050 ist auf Arduino grundsaetzlich moeglich.
- Kritischer Faktor ist nicht die Logik, sondern der reale Ruhestrom der Gesamtplattform.
- Boardentscheidung: Pro Mini 328P als geeignete Basis (kein USB-Onboard).

## Festgelegte Hardware- und Betriebsparameter
- Board: Arduino Pro Mini 328P.
- Akku: 3.7V LiPo, 2000mAh pro Zelle.
- Varianten diskutiert: 1, 2 oder 4 Zellen.
- Festgelegt: 2 Zellen parallel, also 4000mAh Gesamt.
- LEDs: aktuell 2x RGB-LED ueber 6 PWM-Ausgaenge in der Firmware.
- Frueheres Konzept 2x rot mit 120 Ohm bleibt als Vergleichsbasis fuer Energiebudget dokumentiert.
- Leuchtdauer final: 5 Sekunden je Trigger.

## Lastprofil (final)
- Wochenendtage: 1 Stunde pro Tag.
- Wochentage: 20 Minuten pro Tag.
- Aktivzeit pro Woche: 2h + 1h40 = 3h40 (3.67h).

## Strom- und Laufzeitrechnung (finale Basis)
- LED-Strom pro LED bei 3.7V und 120 Ohm:
  - I = (3.7V - 2.0V) / 120 Ohm = 14.17mA.
- LED-Gesamtstrom: 28.3mA.
- MCU + MPU aktiv angenommen: 8.5mA.
- Gesamtstrom aktiv: 36.8mA.
- Sleep (konservativ) angenommen: 3.6mA.

Berechnung pro Woche:
- Aktivverbrauch: 36.8mA * 3.67h = ca. 135mAh.
- Sleepzeit: 168h - 3.67h = 164.33h.
- Sleepverbrauch: 3.6mA * 164.33h = ca. 592mAh.
- Gesamtverbrauch: ca. 727mAh/Woche.

Laufzeit mit 2x2000mAh (4000mAh):
- Theoretisch: 4000 / 727 = ca. 5.5 Wochen.
- Praxisnah mit 15 Prozent Reserve: ca. 4.7 Wochen (ca. 33 Tage).

## Implementierungsentscheidung (final)
- Architektur: eventgetrieben, Wakeup durch MPU6050-INT, MCU ansonsten Power-Down.
- ISR bleibt minimal und setzt nur ein Bewegungsflag.
- Hauptlogik verwaltet Deadline:
  - active_until = now + 5000ms.
  - LEDs aktiv solange now < active_until.
  - neues Ereignis setzt Deadline erneut.
- Nach Ablauf: LEDs aus, Interruptquelle quittieren, zurueck in Sleep.
- Erweiterung: Motion-Level wird aus ACCEL-Rohdaten geschaetzt und steuert die Geschwindigkeit des RGB-Breathing.
- Erweiterung: Heartbeat-, Wake-, ISR- und Spurious-IRQ-Zaehlung per Serial-Log.

## Abnahme- und Qualitaetskriterien
- Wakeup aus Sleep bei Bewegung reproduzierbar.
- 5s-Fenster verlaengert sich bei Folgetrigger reproduzierbar.
- Ohne Folgebewegung schaltet Licht exakt aus und System schlaeft wieder.
- 24h-Standtest ohne Lockups/Fehlzustand.
- Strommessung aktiv/sleep gegen Planung dokumentiert.

## Ergebnisstand
- Planung und Doku fuer Arduino-only sind konsolidiert.
- Pinbelegung und Bring-up/Testablauf sind als separate Datei gepflegt.
- Firmware-Stand beinhaltet MPU-Motion-Interrupt, Sleep/Wakeup und RGB-Breathing mit 5s retriggerbarem Fenster.
- Noch offen: reale Strommesswerte (active/sleep) als Messprotokoll zur Budget-Validierung ergaenzen.
