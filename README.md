# KetCar Totenschaedel Firmware Workspace

This workspace contains a minimal Arduino/PlatformIO setup for the KetCar motion light project.

## What is included

- Arduino firmware for ATmega328P (`src/main.cpp`)
- MPU6050 interrupt wake-up handling (INT pin on D2)
- Retriggerable light window (`5s` on each motion event)
- Deep sleep between events

## Default pin mapping

- `MPU6050 SDA` -> `A4`
- `MPU6050 SCL` -> `A5`
- `MPU6050 INT` -> `D2`
- `LED left` -> `D9` (with `120 ohm` resistor)
- `LED right` -> `D10` (with `120 ohm` resistor)

## Build and flash

1. Install PlatformIO in VS Code.
2. Select the environment in `platformio.ini` if needed.
3. Build:

```bash
pio run
```

4. Upload:

```bash
pio run -t upload
```

## Board configuration

The default setup targets `pro8MHzatmega328` for low-power operation.

If you run a `5V / 16MHz` Pro Mini, switch to the `pro16` environment in `platformio.ini`.
