## Plan: KetCar-Totenschaedel Arduino Motion Light

Dieses Projekt wird als reine Arduino-Offline-Lösung umgesetzt (kein ESP, kein WLAN, kein OTA, kein Home Assistant) mit 2x LiPo 3.7V/2000mAh (1S parallel = 4000mAh), MPU6050-Interrupt-Wakeup, 2 roten 5mm LEDs mit je 120 Ohm und 5s Leuchtdauer pro Bewegungsereignis. Ziel ist maximale Robustheit, minimale Komplexität und hohe Akkulaufzeit.

**Steps**
1. Phase A - Hardware-Basis fixieren
2. Board: Arduino Pro Mini 328P ohne USB-Onboard verwenden.
3. Akku: 2x 3.7V LiPo parallel (gemeinsame Schutzlogik/BMS empfohlen), effektive Kapazität 4000mAh.
4. LED-Pfad: 2x rote LED (ca. 2.0V) mit je 120 Ohm Vorwiderstand, getrennte Widerstände pro LED.
5. Sensor-Pfad: MPU6050 per I2C (SDA/SCL), INT-Pin auf externen Interrupt-Pin der MCU.
6. Versorgung: stabile 3.3V- oder 5V-Strategie gemäß Boardtakt; Spannungsbereich des Sensors beachten. Blocking for all following phases.

7. Phase B - Ereignis- und Laufzeitlogik definieren
8. Bewegungsereignis löst Interrupt aus und setzt nur ein Flag (ISR schlank halten).
9. Hauptloop verarbeitet Flag deterministisch und setzt Laufzeitfenster auf jetzt + 5000ms.
10. Licht EIN solange aktuelle Zeit < Deadline; jedes neue Ereignis überschreibt Deadline erneut auf jetzt + 5000ms.
11. Nach Ablauf: LEDs AUS, Event-Status zurücksetzen, Übergang in Tiefschlaf vorbereiten. Depends on Phase A.

12. Phase C - Schlaf/Wakeup robust machen
13. Tiefschlafmodus des ATmega328P nutzen (Power-Down).
14. Wakeup ausschließlich über externen Interrupt vom MPU6050-INT.
15. Nach Wakeup: Interruptquelle quittieren, kurze Entprell-/Guard-Zeit, dann normale Bewegungsbewertung.
16. Fail-safe: Falls INT dauerhaft anliegt, definierte Recovery-Sequenz (Reinit MPU + erneuter Sleep-Versuch). Depends on Phase B.

17. Phase D - Strombudget gegen Zielprofil validieren
18. Lastprofil fix: Nutzung pro Woche = 2h (Wochenende) + 1h40 (5x20min) = 3h40min aktiv.
19. Aktiver Strom aus Vorgaben: LED gesamt ca. 28.3mA + MCU ca. 5mA + MPU ca. 3.5mA = ca. 36.8mA.
20. Sleep-Annahme (konservativ): ca. 3.6mA Gesamtsystem.
21. Wochengesamtverbrauch: aktiv ca. 135mAh + sleep ca. 592mAh = ca. 727mAh/Woche.
22. Laufzeit mit 4000mAh: ca. 5.5 Wochen theoretisch; mit 15% Praxisabschlag ca. 4.7 Wochen (ca. 33 Tage). Depends on Phase C.

23. Phase E - Verifikation und Abnahme
24. Bench-Test: Wakeup aus Sleep bei Bewegung reproduzierbar.
25. Timer-Test: mehrfaches Triggern innerhalb 5s verlängert Lichtdauer korrekt.
26. Ende-Test: ohne neue Bewegung endet Licht exakt nach 5s und System schläft wieder.
27. Dauer-Test: 24h Standtest auf Fehltrigger/INT-Latch prüfen.
28. Mess-Test: Strom aktiv/sleep gegen Kalkulation dokumentieren und bei Abweichung Schwelle oder LED-Strom nachziehen. Depends on Phase D.

**Relevant files**
- e:/GitHub/ESPHome/Hardware-Specs/Gyroskop-Sensor-GY-521/hardware-info.txt — Sensorbasis und elektrische Eckdaten.

**Verification**
1. Funktion: Bewegung -> Licht an, erneute Bewegung innerhalb 5s -> Deadline reset, ohne Bewegung -> Licht aus + Sleep.
2. Stabilität: kein Hängen in dauerhaftem Wake-Zustand bei INT-Latch-Szenarien.
3. Energie: gemessene Laufzeit liegt im Korridor von ca. 4.5 bis 5.5 Wochen mit 2 Akkus im definierten Nutzungsprofil.
4. Sicherheit: Versorgung und Verkabelung bleiben thermisch unauffällig bei maximaler Helligkeit.

**Decisions**
- In scope: Minimal-Implementierung für robuste Totenschädel-Effektbeleuchtung mit harter Sleep/Wakeup-Mechanik.
- Out of scope: Funkfunktionen, OTA, App-Anbindung, HA-Entitys, ESPHome.
- Festgelegt: 2 Akkus, 120 Ohm, 5s Leuchtdauer, Wochenprofil 1h/WE-Tag + 20min/Wochentag.

**Further Considerations**
1. Wenn reale Laufzeit < 4 Wochen: zuerst LED-Widerstände auf 150 bis 180 Ohm erhöhen, erst danach Logik anpassen.
2. Wenn Fehltrigger häufig: MPU-Interrupt-Schwelle erhöhen und Guard-Zeit moderat verlängern.
3. Wenn Helligkeit zu gering: Pulsbetrieb (z. B. 70-80% duty) als Kompromiss zwischen Sichtbarkeit und Akku.

## Doku-Vorlage zur Ablage im Workspace-Root

Zieldatei 1: KetCar-Totenschaedel-Implementierungsplan.md

Inhalt:
Projektziel
- Aufbau einer robusten, akkubetriebenen Effektbeleuchtung für ein Ketcar.
- Auslösung durch Bewegung via MPU6050.
- Lichtdauer pro Ereignis 5 Sekunden, erneute Bewegung setzt die 5 Sekunden zurück.
- Keine Netzwerkfunktionen.

Festgelegte Hardware
- Controller: Arduino Pro Mini 328P.
- Sensor: MPU6050 (GY-521).
- LEDs: 2x 5mm rot, je 120 Ohm Vorwiderstand.
- Akku: 2x LiPo 3.7V 2000mAh parallel (gesamt 4000mAh).

Implementierungslogik
- Sleep-Grundzustand: MCU im Power-Down.
- Wakeup: externer Interrupt vom MPU6050-INT.
- ISR setzt ausschließlich Bewegungsflag.
- Hauptschleife setzt active_until = now + 5000ms.
- LEDs EIN bis now >= active_until.
- Danach LEDs AUS und Rückkehr in Power-Down.

Robustheitsregeln
- ISR minimal halten (keine I2C-Operation in ISR).
- MPU-Interrupt nach Wakeup sauber quittieren.
- Guard-Zeit gegen Doppelflanken einsetzen.
- Recovery bei dauerhaftem INT-Level vorsehen.

Testkriterien
- Bewegung weckt zuverlässig aus Sleep.
- Zeitfenster verlängert sich bei Folgebewegung reproduzierbar.
- Ohne Folgebewegung endet Licht exakt nach 5s.
- 24h Fehltrigger-Test ohne Lockup.

Zieldatei 2: KetCar-Totenschaedel-Energiebudget.md

Inhalt:
Eingangsdaten
- Akkukapazität: 4000mAh (2x2000mAh parallel).
- Nutzungsprofil pro Woche: 2h Wochenende + 1h40 Wochentage = 3h40 aktiv.
- LED-Strom bei 3.7V und 120 Ohm: ca. 14.17mA pro LED.
- Gesamt-LED-Strom: ca. 28.3mA.
- MCU+Sensor aktiv: ca. 8.5mA.
- Gesamt aktiv: ca. 36.8mA.
- Sleep konservativ: ca. 3.6mA.

Berechnung
- Aktivverbrauch/Woche: 36.8mA x 3.67h = ca. 135mAh.
- Sleepzeit/Woche: 168h - 3.67h = 164.33h.
- Sleepverbrauch/Woche: 3.6mA x 164.33h = ca. 592mAh.
- Gesamt/Woche: ca. 727mAh.
- Laufzeit theoretisch: 4000/727 = ca. 5.5 Wochen.
- Laufzeit praxisnah (15% Reserve): ca. 4.7 Wochen (ca. 33 Tage).

Interpretation
- Haupttreiber des Verbrauchs ist im Wochenmittel der Sleep-Anteil.
- Verbesserungen erfolgen zuerst über Sleep-Stromsenkung und dann LED-Strom.
- Zielkorridor: 4.5 bis 5.5 Wochen unter dem definierten Profil.
