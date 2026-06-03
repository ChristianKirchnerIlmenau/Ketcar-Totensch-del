# KetCar-Totenschaedel Pinbelegung und Testprotokoll

Stand: 03.06.2026

## Ziel
- Minimal robuste Umsetzung mit Arduino Pro Mini 328P, MPU6050 (GY-521), 2x RGB-LED, 2x LiPo parallel.
- Wakeup ueber MPU6050-Interrupt.
- Leuchtdauer 5s, Reset der Laufzeit bei neuer Bewegung.

## Empfohlene Pinbelegung (Arduino Pro Mini 328P)
- VCC 3.3V oder 5V gemaess gewaehlter Variante.
- GND gemeinsam fuer alle Komponenten.
- MPU6050 SDA -> A4.
- MPU6050 SCL -> A5.
- MPU6050 INT -> D2 (INT0, externer Interrupt fuer Wakeup).
- RGB LED 1: Blau -> D9, Rot -> D10, Gruen -> D11.
- RGB LED 2: Blau -> D3, Rot -> D5, Gruen -> D6.

Hinweise:
- Bei diskreten LEDs immer ein Vorwiderstand pro LED-Kanal verwenden.
- Die aktuelle Firmware nutzt PWM-faehige Pins fuer beide RGB-Gruppen.
- Falls 3.3V-Betrieb: Pegel und Board-Takt konsistent halten.
- Falls 5V-Betrieb: MPU6050-Modulversorgung und Pegel des verwendeten Breakouts pruefen.

## Elektrische Kurzcheckliste vor erstem Start
- Polaritaet aller LiPo-Verbindungen pruefen.
- Parallelbetrieb der LiPos nur mit geeigneter Schutz-/Lade-Strategie.
- Keine direkte LED ohne Vorwiderstand.
- Gemeinsame Masse zwischen MCU und Sensor zwingend.

## Verhaltenslogik (Soll)
- Sleep-Grundzustand: MCU in Power-Down.
- Bewegung erzeugt INT -> MCU wacht auf.
- ISR setzt nur motion_flag.
- Main prueft INT_STATUS (Motion-Bit) und setzt active_until = now + 5000ms.
- LEDs aktiv solange now < active_until, mit dynamischem RGB-Breathing.
- Neues Event vor Ablauf setzt active_until erneut.
- Ohne neues Event nach 5s: LEDs AUS, zurueck in Sleep nach Guard-Zeit.

## Kompaktes Bring-up Testprotokoll

### Test 1: Stromloser Verdrahtungscheck
- Ziel: Verkabelungsfehler ausschliessen.
- Schritt:
  - Alle Verbindungen gegen Pinliste pruefen.
  - Durchgang und Kurzschluss gegen GND/VCC messen.
- Soll:
  - Keine Kurzschluesse, Pinzuordnung korrekt.

### Test 2: Sensor-I2C Erkennung
- Ziel: MPU6050 erreichbar.
- Schritt:
  - I2C-Scan laufen lassen.
- Soll:
  - Adresse 0x68 oder 0x69 wird gefunden.

### Test 3: Interrupt-Wakeup Grundfunktion
- Ziel: Wakeup aus Power-Down funktioniert.
- Schritt:
  - MCU in Sleep senden.
  - Bewegung am Sensor ausloesen.
- Soll:
  - MCU wacht reproduzierbar auf.

### Test 4: 5s-Lichtfenster
- Ziel: Basiszeit korrekt.
- Schritt:
  - Ein Trigger ausloesen.
  - Zeit bis Licht AUS messen.
- Soll:
  - 5.0s im erwarteten Toleranzbereich.

### Test 5: Deadline-Reset bei Folgebewegung
- Ziel: Verlangerung funktioniert.
- Schritt:
  - Trigger bei t=0.
  - Zweiter Trigger bei t=3s.
- Soll:
  - Licht bleibt bis etwa t=8s an.

### Test 5b: RGB-Pin-Mapping Selbsttest
- Ziel: Farbzuordnung und Verdrahtung sicher bestaetigen.
- Schritt:
  - Firmware starten, integrierten RGB-Selbsttest abwarten.
  - Beobachten, ob die Kanaele in Reihenfolge D9, D10, D11 (und gespiegelt D3, D5, D6) schalten.
- Soll:
  - D9 und D3 zeigen Blau, D10 und D5 zeigen Rot, D11 und D6 zeigen Gruen.

### Test 6: Ruhefall ohne Bewegung
- Ziel: Rueckkehr in Sleep stabil.
- Schritt:
  - Nach Licht-AUS keine Bewegung.
- Soll:
  - System bleibt im Sleep, keine ungewollten Wakeups.

### Test 7: Fehltrigger-Resistenz
- Ziel: Robustheit bei Vibrationen.
- Schritt:
  - Realistische Fahr-/Rumpelbewegungen testen.
- Soll:
  - Keine Daueraktivierung durch Rauschen.
  - Schwelle/Guard-Zeit nur bei Bedarf nachziehen.

### Test 8: 24h-Dauertest
- Ziel: Langzeitstabilitaet.
- Schritt:
  - Typischer Einsatzmix mit Bewegungsphasen und Standzeiten.
- Soll:
  - Keine Lockups, kein Dauer-Wake-Latch, reproduzierbarer Zustand.

### Test 9: Strommessung und Plausibilitaet
- Ziel: Energiebudget validieren.
- Schritt:
  - I_active und I_sleep messen.
  - Gegen Planung vergleichen.
- Soll:
  - I_active liegt im erwarteten Bereich der realen RGB-Konfiguration.
  - I_sleep grob nahe 3.6mA konservativ oder besser.

## Abnahmeentscheidung
- Projekt freigeben, wenn Tests 1 bis 9 bestanden sind.
- Bei Abweichung zuerst in dieser Reihenfolge korrigieren:
  1) Interrupt-Konfiguration und Quittierung.
  2) Bewegungsschwelle und Guard-Zeit.
  3) LED-Strom und Versorgungspfad.

## Betriebshinweise
- Laufzeitziel mit 2x2000mAh und definiertem Profil: ca. 5.5 Wochen theoretisch, ca. 4.7 Wochen praxisnah.
- Wenn Laufzeit unter Ziel: zuerst Sleep-Strom optimieren, dann LED-Strom reduzieren.
