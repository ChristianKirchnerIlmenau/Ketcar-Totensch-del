#include <Arduino.h>
#include <Wire.h>
#include <avr/sleep.h>

namespace {
constexpr uint8_t kLedD9Pin = 9;
constexpr uint8_t kLedD10Pin = 10;
constexpr uint8_t kLedD11Pin = 11;
constexpr uint8_t kLed2D3Pin = 3;
constexpr uint8_t kLed2D5Pin = 5;
constexpr uint8_t kLed2D6Pin = 6;

// Confirmed by self-test: D9=Blue, D10=Red, D11=Green.
constexpr uint8_t kLedBluePin = kLedD9Pin;
constexpr uint8_t kLedRedPin = kLedD10Pin;
constexpr uint8_t kLedGreenPin = kLedD11Pin;
constexpr uint8_t kLed2BluePin = kLed2D3Pin;
constexpr uint8_t kLed2RedPin = kLed2D5Pin;
constexpr uint8_t kLed2GreenPin = kLed2D6Pin;
constexpr uint8_t kMpuIntPin = 2;  // INT0

constexpr bool kLedActiveHigh = true;
constexpr unsigned long kSerialBaud = 115200;

constexpr uint8_t kMpuAddress = 0x68;
constexpr uint8_t kRegPwrMgmt1 = 0x6B;
constexpr uint8_t kRegConfig = 0x1A;
constexpr uint8_t kRegAccelConfig = 0x1C;
constexpr uint8_t kRegMotionThreshold = 0x1F;
constexpr uint8_t kRegMotionDuration = 0x20;
constexpr uint8_t kRegIntPinCfg = 0x37;
constexpr uint8_t kRegIntEnable = 0x38;
constexpr uint8_t kRegIntStatus = 0x3A;
constexpr uint8_t kRegAccelXoutH = 0x3B;
constexpr uint8_t kMotionIntMask = 0x40;

constexpr uint16_t kLightWindowMs = 5000;
constexpr uint8_t kMotionThreshold = 3;
constexpr uint8_t kMotionDuration = 1;
constexpr uint16_t kWakeGuardMs = 120;
constexpr uint16_t kHeartbeatMs = 2000;
constexpr uint16_t kBreathPeriodSlowMs = 2400;
constexpr uint16_t kBreathPeriodFastMs = 700;
constexpr uint8_t kBreathMin = 4;
constexpr uint8_t kBlueGain = 210;
constexpr uint8_t kRedGain = 255;
constexpr uint8_t kGreenGain = 200;

volatile bool gMotionIrq = false;
volatile uint32_t gIsrCount = 0;
uint32_t gLightUntilMs = 0;
bool gLightsOn = false;
uint32_t gLastHeartbeatMs = 0;
uint8_t gMotionLevel = 0;
uint32_t gSleepEnterCount = 0;
uint32_t gWakeCount = 0;
uint32_t gMotionEventCount = 0;
uint32_t gSpuriousIrqCount = 0;

void writeLedPin(uint8_t pin, bool on) {
  const uint8_t level = (kLedActiveHigh == on) ? HIGH : LOW;
  digitalWrite(pin, level);
}

bool isPwmPin(uint8_t pin) {
  return pin == 3 || pin == 5 || pin == 6 || pin == 9 || pin == 10 || pin == 11;
}

void writeLedPwm(uint8_t pin, uint8_t value) {
  if (isPwmPin(pin)) {
    const uint8_t pwm = kLedActiveHigh ? value : static_cast<uint8_t>(255 - value);
    analogWrite(pin, pwm);
    return;
  }

  // Fallback for non-PWM pins.
  writeLedPin(pin, value > 127);
}

uint8_t triangleWave8(uint32_t timeMs, uint16_t periodMs, uint16_t offsetMs) {
  const uint16_t half = periodMs / 2;
  const uint16_t t = static_cast<uint16_t>((timeMs + offsetMs) % periodMs);

  uint16_t out = 0;
  if (t < half) {
    out = static_cast<uint16_t>((static_cast<uint32_t>(t) * 255U) / half);
  } else {
    out = static_cast<uint16_t>((static_cast<uint32_t>(periodMs - t) * 255U) / half);
  }

  const uint16_t boosted = static_cast<uint16_t>((static_cast<uint32_t>(out) * out) / 255U);
  const uint16_t span = 255 - kBreathMin;
  return static_cast<uint8_t>(kBreathMin + ((static_cast<uint32_t>(boosted) * span) / 255U));
}

uint8_t applyGain(uint8_t value, uint8_t gain) {
  const uint16_t scaled = static_cast<uint16_t>((static_cast<uint16_t>(value) * gain) / 255U);
  return static_cast<uint8_t>(scaled > 255 ? 255 : scaled);
}

void updateBreathingLeds(uint32_t nowMs) {
  const uint16_t period = static_cast<uint16_t>(
      kBreathPeriodSlowMs - ((static_cast<uint32_t>(gMotionLevel) * (kBreathPeriodSlowMs - kBreathPeriodFastMs)) / 255U));
  const uint16_t third = period / 3;
  const uint8_t blue = applyGain(triangleWave8(nowMs, period, 0), kBlueGain);
  const uint8_t red = applyGain(triangleWave8(nowMs, period, third), kRedGain);
  const uint8_t green = applyGain(triangleWave8(nowMs, period, static_cast<uint16_t>(third * 2)), kGreenGain);

  writeLedPwm(kLedBluePin, blue);
  writeLedPwm(kLedRedPin, red);
  writeLedPwm(kLedGreenPin, green);

  // Mirror the same color mix to the second RGB LED.
  writeLedPwm(kLed2BluePin, blue);
  writeLedPwm(kLed2RedPin, red);
  writeLedPwm(kLed2GreenPin, green);
}

void writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(kMpuAddress);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

uint8_t readRegister(uint8_t reg) {
  Wire.beginTransmission(kMpuAddress);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(static_cast<int>(kMpuAddress), 1);
  if (Wire.available() < 1) {
    return 0;
  }
  return Wire.read();
}

bool readAccelerometerRaw(int16_t* x, int16_t* y, int16_t* z) {
  Wire.beginTransmission(kMpuAddress);
  Wire.write(kRegAccelXoutH);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  const int requested = 6;
  if (Wire.requestFrom(static_cast<int>(kMpuAddress), requested) != requested) {
    return false;
  }

  *x = static_cast<int16_t>((Wire.read() << 8) | Wire.read());
  *y = static_cast<int16_t>((Wire.read() << 8) | Wire.read());
  *z = static_cast<int16_t>((Wire.read() << 8) | Wire.read());
  return true;
}

uint8_t estimateMotionLevel() {
  int16_t x = 0;
  int16_t y = 0;
  int16_t z = 0;
  if (!readAccelerometerRaw(&x, &y, &z)) {
    return 96;
  }

  const int32_t dx = abs(static_cast<int32_t>(x));
  const int32_t dy = abs(static_cast<int32_t>(y));
  const int32_t dz = abs(static_cast<int32_t>(z) - 16384);
  const int32_t metric = dx + dy + dz;

  int32_t level = metric / 64;
  if (level < 20) {
    level = 20;
  }
  if (level > 255) {
    level = 255;
  }
  return static_cast<uint8_t>(level);
}

void clearMpuInterrupt() {
  (void)readRegister(kRegIntStatus);
}

void configureMpuMotionInterrupt() {
  writeRegister(kRegPwrMgmt1, 0x00);
  delay(50);
  writeRegister(kRegConfig, 0x03);
  writeRegister(kRegAccelConfig, 0x00);
  writeRegister(kRegMotionThreshold, kMotionThreshold);
  writeRegister(kRegMotionDuration, kMotionDuration);
  // Active-high, latched INT until status read, clear-on-read.
  writeRegister(kRegIntPinCfg, 0x30);
  // Motion detect interrupt only.
  writeRegister(kRegIntEnable, 0x40);
  clearMpuInterrupt();
}

void setLeds(bool on) {
  if (on) {
    updateBreathingLeds(millis());
  } else {
    writeLedPin(kLedBluePin, false);
    writeLedPin(kLedRedPin, false);
    writeLedPin(kLedGreenPin, false);
    writeLedPin(kLed2BluePin, false);
    writeLedPin(kLed2RedPin, false);
    writeLedPin(kLed2GreenPin, false);
  }
  gLightsOn = on;
}

void rgbSelfTest() {
  Serial.println(F("LED self-test start"));

  Serial.println(F("Self-test D9 ON (1s)"));
  writeLedPin(kLedD9Pin, true);
  writeLedPin(kLedD10Pin, false);
  writeLedPin(kLedD11Pin, false);
  writeLedPin(kLed2D3Pin, true);
  writeLedPin(kLed2D5Pin, false);
  writeLedPin(kLed2D6Pin, false);
  delay(1000);

  Serial.println(F("Self-test D10 ON (1s)"));
  writeLedPin(kLedD9Pin, false);
  writeLedPin(kLedD10Pin, true);
  writeLedPin(kLedD11Pin, false);
  writeLedPin(kLed2D3Pin, false);
  writeLedPin(kLed2D5Pin, true);
  writeLedPin(kLed2D6Pin, false);
  delay(1000);

  Serial.println(F("Self-test D11 ON (1s)"));
  writeLedPin(kLedD9Pin, false);
  writeLedPin(kLedD10Pin, false);
  writeLedPin(kLedD11Pin, true);
  writeLedPin(kLed2D3Pin, false);
  writeLedPin(kLed2D5Pin, false);
  writeLedPin(kLed2D6Pin, true);
  delay(1000);

  setLeds(false);
  Serial.println(F("LED self-test done"));
}

void onMotionInterrupt() {
  ++gIsrCount;
  gMotionIrq = true;
}

void enterDeepSleep() {
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();

  noInterrupts();
  const bool pendingMotion = gMotionIrq;
  interrupts();

  if (!pendingMotion) {
    sleep_cpu();
  }

  sleep_disable();
}

void handleMotionEvent() {
  noInterrupts();
  const bool hadIrq = gMotionIrq;
  gMotionIrq = false;
  interrupts();

  if (!hadIrq) {
    return;
  }

  const uint8_t intStatus = readRegister(kRegIntStatus);
  if ((intStatus & kMotionIntMask) == 0) {
    ++gSpuriousIrqCount;
    Serial.print(F("IRQ without motion bit, status=0x"));
    Serial.println(intStatus, HEX);
    Serial.print(F("Spurious IRQ count="));
    Serial.println(gSpuriousIrqCount);
    return;
  }

  const uint8_t newMotion = estimateMotionLevel();
  if (newMotion > gMotionLevel) {
    gMotionLevel = newMotion;
  } else {
    gMotionLevel = static_cast<uint8_t>((static_cast<uint16_t>(gMotionLevel) * 3U + newMotion) / 4U);
  }

  gLightUntilMs = millis() + kLightWindowMs;
  ++gMotionEventCount;
  setLeds(true);
  Serial.print(F("Motion IRQ -> LEDs ON until ms="));
  Serial.println(gLightUntilMs);
  Serial.print(F("Motion level="));
  Serial.println(gMotionLevel);
  Serial.print(F("Motion events="));
  Serial.println(gMotionEventCount);
}
}  // namespace

void setup() {
  Serial.begin(kSerialBaud);
  delay(50);
  Serial.println(F("Booting KetCar motion light"));

  pinMode(kLedD9Pin, OUTPUT);
  pinMode(kLedD10Pin, OUTPUT);
  pinMode(kLedD11Pin, OUTPUT);
  pinMode(kLed2D3Pin, OUTPUT);
  pinMode(kLed2D5Pin, OUTPUT);
  pinMode(kLed2D6Pin, OUTPUT);
  pinMode(kMpuIntPin, INPUT);
  setLeds(false);

  Wire.begin();
  configureMpuMotionInterrupt();
  Serial.println(F("MPU6050 configured"));

  attachInterrupt(digitalPinToInterrupt(kMpuIntPin), onMotionInterrupt, RISING);
  Serial.println(F("Interrupt attached on D2 (RISING)"));
  Serial.println(F("LED2 uses full PWM pins D3/D5/D6"));
  rgbSelfTest();
}

void loop() {
  const uint32_t now = millis();

  if (static_cast<int32_t>(now - gLastHeartbeatMs) >= 0 &&
      (now - gLastHeartbeatMs) >= kHeartbeatMs) {
    gLastHeartbeatMs = now;
    Serial.print(F("HB int="));
    Serial.print(digitalRead(kMpuIntPin));
    Serial.print(F(" lights="));
    Serial.print(gLightsOn ? F("ON") : F("OFF"));
    Serial.print(F(" motionLvl="));
    Serial.print(gMotionLevel);
    Serial.print(F(" isr="));
    Serial.print(gIsrCount);
    Serial.print(F(" wake="));
    Serial.println(gWakeCount);
  }

  if (gMotionLevel > 3) {
    gMotionLevel = static_cast<uint8_t>(gMotionLevel - 3);
  } else {
    gMotionLevel = 0;
  }

  handleMotionEvent();

  if (gLightsOn) {
    updateBreathingLeds(now);
    if (static_cast<int32_t>(millis() - gLightUntilMs) >= 0) {
      setLeds(false);
      Serial.println(F("LED window expired -> LEDs OFF"));
    }
    return;
  }

  ++gSleepEnterCount;
  Serial.print(F("Entering deep sleep #"));
  Serial.println(gSleepEnterCount);
  enterDeepSleep();
  ++gWakeCount;
  Serial.print(F("Woke from sleep #"));
  Serial.println(gWakeCount);
  delay(kWakeGuardMs);
}
