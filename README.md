# KetCar Totenschaedel Firmware Workspace

Stand: 03.06.2026

Dieses Workspace enthaelt die Arduino/PlatformIO-Firmware fuer die bewegungsgetriggerte KetCar-Beleuchtung.

## Enthaltene Funktion

- Arduino-Firmware fuer ATmega328P in src/main.cpp
- MPU6050 Motion-Interrupt als Wakeup-Quelle (INT auf D2)
- Retriggerbares Lichtfenster von 5s pro Bewegungsereignis
- Power-Down Sleep ohne aktives Licht
- 2x RGB-LED Ansteuerung mit PWM-Breathing-Effekt
- Serielle Diagnoseausgaben (Heartbeat, Wake/Sleep, Motion-Events)

## Aktuelle Pinbelegung

MPU6050:
- SDA -> A4
- SCL -> A5
- INT -> D2 (INT0)

RGB LED 1:
- Blau -> D9
- Rot -> D10
- Gruen -> D11

RGB LED 2:
- Blau -> D3
- Rot -> D5
- Gruen -> D6

Hinweis: Die Farbzuordnung D9/D10/D11 wurde per Selbsttest bestaetigt und ist bereits in der Firmware hinterlegt.

## Build und Upload

1. PlatformIO in VS Code installieren.
2. Falls noetig das Ziel-Environment in platformio.ini waehlen.
3. Build ausfuehren:

```bash
pio run
```

4. Flashen:

```bash
pio run -t upload
```

5. Serielle Ausgabe beobachten:

```bash
pio device monitor
```

## Board-Konfiguration

Standard ist pro8 (pro8MHzatmega328) fuer stromsparenden Betrieb.

Alternativ steht pro16 (pro16MHzatmega328) fuer 5V/16MHz Pro Mini zur Verfuegung.
