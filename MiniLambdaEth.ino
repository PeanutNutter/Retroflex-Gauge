#include <Arduino.h>
#include <TM1637Display.h>

#define CLK 2
#define DIO 3

const int PIN_RED   = 4;
const int PIN_GREEN = 5;
const int PIN_BLUE  = 6;

const int PIN_BTN     = 7;   // Mode cycler
const int PIN_ETHANOL = 8;   // GM flex sensor input

TM1637Display display(CLK, DIO);

// AFR smoothing
float smoothAFR = 0;
const float afrAlpha = 0.85;

// Mode 0–3
int mode = 0;
unsigned long lastButton = 0;
const unsigned long debounceDelay = 300;

// "E" symbol
const uint8_t SEG_LETTER_E = 0b01111001;

// Manual ethanol override
bool manualETH = true;
float manualEthValue = 0;

// Read GM ethanol sensor (50–150 Hz → 0–100%)
float ReadETH() {
  unsigned long highTime = pulseIn(PIN_ETHANOL, HIGH, 25000);
  unsigned long lowTime  = pulseIn(PIN_ETHANOL, LOW, 25000);

  if (highTime == 0 || lowTime == 0) return 0;

  float period = highTime + lowTime;
  float freq = 1000000.0 / period;

  float ethanol = (freq - 50.0);
  if (ethanol < 0) ethanol = 0;
  if (ethanol > 100) ethanol = 100;

  return ethanol;
}

// serial input for ethanol%
void checkSerialInput() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    // Return to sensor mode
    if (input.equalsIgnoreCase("S")) {
      manualETH = false;
      Serial.println("Sensor mode enabled.");
      return;
    }

    // Manual override: E##
    if (input.startsWith("E") || input.startsWith("e")) {
      String num = input.substring(1);
      int val = num.toInt();

      if (val >= 0 && val <= 100) {
        manualETH = true;
        manualEthValue = val;
        Serial.print("Manual ethanol override: ");
        Serial.print(val);
        Serial.println("%");
      } else {
        Serial.println("Invalid ethanol value. Use E0–E100.");
      }
    }
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println("Type E## to set ethanol %, or S to return to sensor mode.");

  display.setBrightness(3);

  pinMode(PIN_RED,   OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_BLUE,  OUTPUT);

  pinMode(PIN_BTN, INPUT_PULLUP);
  pinMode(PIN_ETHANOL, INPUT);
}

void loop() {

  // Check for serial override commands
  checkSerialInput();

  // Mode cycling
  if (millis() - lastButton > debounceDelay) {
    if (digitalRead(PIN_BTN) == LOW) {
      mode++;
      if (mode > 3) mode = 0;
      lastButton = millis();
    }
  }

  // Wideband input
  int raw = analogRead(A1);
  float afrRaw = 10.0 + (raw / 1023.0) * 10.0;

  // Smooth AFR
  smoothAFR = smoothAFR + afrAlpha * (afrRaw - smoothAFR);

  // Lambda
  float lambda = smoothAFR / 14.7;

  // Ethanol %
  float ethanol;
  if (manualETH) ethanol = manualEthValue;
  else ethanol = ReadETH();

  // Stoich AFR for blend
  float stoichAFR = 14.7 - (14.7 - 9.8) * (ethanol / 100.0);

  // Ethanol based AFR
  float afrCorrected = lambda * stoichAFR;

  // Display buffer
  uint8_t seg[4] = {0,0,0,0};

  if (mode == 0) {
    // MODE 0: 14.7 stoich AFR
    int afr10 = (int)(smoothAFR * 10 + 0.5);
    if (afr10 > 999) afr10 = 999;

    seg[0] = display.encodeDigit(afr10 / 100);
    seg[1] = display.encodeDigit((afr10 / 10) % 10) | 0x80;
    seg[2] = display.encodeDigit(afr10 % 10);
    seg[3] = 0x00;
  }

  else if (mode == 1) {
    // MODE 1: Ethanol corrected AFR
    int afr10 = (int)(afrCorrected * 10 + 0.5);
    if (afr10 > 999) afr10 = 999;

    seg[0] = display.encodeDigit(afr10 / 100);
    seg[1] = display.encodeDigit((afr10 / 10) % 10) | 0x80;
    seg[2] = display.encodeDigit(afr10 % 10);
    seg[3] = 0x00;
  }

  else if (mode == 2) {
    // MODE 2: Lambda
    int lam100 = (int)(lambda * 100 + 0.5);
    if (lam100 > 999) lam100 = 999;

    seg[0] = 0x00;
    seg[1] = display.encodeDigit(lam100 / 100) | 0x80;
    seg[2] = display.encodeDigit((lam100 / 10) % 10);
    seg[3] = display.encodeDigit(lam100 % 10);
  }

  else if (mode == 3) {
    // MODE 3: Ethanol %
    int e = (int)(ethanol + 0.5);
    if (e > 100) e = 100;

    seg[0] = SEG_LETTER_E;
    seg[1] = (e >= 100) ? display.encodeDigit(1) : 0x00;
    seg[2] = display.encodeDigit((e / 10) % 10);
    seg[3] = display.encodeDigit(e % 10);
  }

  display.setSegments(seg);

  // LED AFR indicator
  float voltage = raw * (5.0 / 1023.0);

  analogWrite(PIN_RED,   0);
  analogWrite(PIN_GREEN, 0);
  analogWrite(PIN_BLUE,  0);

  if (voltage <= 1.875) {
    analogWrite(PIN_GREEN, 50);
  }
  else if (voltage <= 3) {
    analogWrite(PIN_RED, 255);
    analogWrite(PIN_GREEN, 25);
  }
  else {
    analogWrite(PIN_RED, 255);
  }

  delay(50);
}
