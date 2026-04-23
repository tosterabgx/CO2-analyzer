#include "SSD1306Wire.h"
#include "s8_uart.h"

#include "pitches.h"
#include "fonts_rus.h"

#define S8_TX_PIN D1
#define S8_RX_PIN D2

#define BUTTON_PIN D7
#define BEEPER_PIN D8

SoftwareSerial S8_serial(S8_TX_PIN, S8_RX_PIN);
S8_UART sensor_S8(S8_serial);
S8_sensor sensor;

SSD1306Wire display(0x3c, D5, D6);

String status = "";
unsigned long nextBeep = 0;
unsigned long nextUpdate = 0;
int buttonValue = 0;
bool isButtonPressed = false;
bool isDisplayOn = true;

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  display.init();
  display.flipScreenVertically();
  display.setFontTableLookupFunction(utf8_rus);

  display.setFont(ArialRus_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);

  display.clear();
  display.drawString(64, 22, "АНАЛИЗАТОР");
  display.drawString(64, 42, "УРОВНЯ CO2");
  display.display();

  tone(BEEPER_PIN, NOTE_G3, 125);
  delay(125 * 1.1);
  noTone(BEEPER_PIN);

  tone(BEEPER_PIN, NOTE_G4, 210);

  S8_serial.begin(S8_BAUDRATE);

  sensor_S8.get_firmware_version(sensor.firm_version);
  int len = strlen(sensor.firm_version);
  int n_tries = 1;
  while (len == 0 && n_tries < 3) {
    delay(250);
    sensor_S8.get_firmware_version(sensor.firm_version);
    len = strlen(sensor.firm_version);
    n_tries++;
  }

  if (len == 0) {
    display.clear();
    display.setFont(ArialRus_Plain_10);
    display.drawString(64, 32, "Programming...");
    display.display();

    while (1) { delay(1000); }
  }

  sensor.sensor_id = sensor_S8.get_sensor_ID();

  delay(3000);
  sensor_S8.get_co2();

  display.clear();
  display.setFont(ArialRus_Plain_10);
  display.drawString(64, 32, "Загрузка данных...");
  display.display();

  delay(5000);
}

void loop() {
  if (millis() >= nextUpdate) {
    display.clear();

    sensor.co2 = sensor_S8.get_co2();

    if (sensor.co2 > 4000) {
      status = "КРИТИЧЕСКИЙ";
      nextBeep = 0;
    } else if (sensor.co2 > 2000) {
      status = "ОПАСНЫЙ";
      nextBeep = 0;
    } else if (sensor.co2 > 1200) {
      status = "ПРЕВЫШЕН";
      nextBeep = 0;
    } else if (sensor.co2 > 800) {
      status = "В НОРМЕ";
    } else {
      status = "ИДЕАЛЬНЫЙ";
    }

    display.setFont(ArialRus_Plain_10);
    display.drawString(64, 8, "УРОВЕНЬ CO2");

    display.drawString(64, 24, String(sensor.co2) + " ppm");

    if (status != "") {
      display.setFont(ArialRus_Plain_16);
      display.drawString(64, 48, status);
    }

    display.display();

    nextUpdate = millis() + 4000;
  }

  buttonValue = !digitalRead(BUTTON_PIN);

  if (buttonValue == 1 && isButtonPressed == false) {
    if (isDisplayOn == true) display.displayOff();
    else display.displayOn();

    isDisplayOn = !isDisplayOn;
    isButtonPressed = true;
  } else if (buttonValue == 0 && isButtonPressed == true) {
    isButtonPressed = false;
  }

  if (status == "ОПАСНЫЙ" && millis() >= nextBeep) {
    noTone(BEEPER_PIN);
    tone(BEEPER_PIN, NOTE_GS5, 300);

    nextBeep = millis() + 1000;
  } else if (status == "ПРЕВЫШЕН" && millis() >= nextBeep) {
    noTone(BEEPER_PIN);
    tone(BEEPER_PIN, NOTE_GS5, 300);

    nextBeep = millis() + 5000;
  }
}

char utf8_rus(const byte ch) {
  static uint8_t LASTCHAR;

  if ((LASTCHAR == 0) && (ch < 0xC0)) {
    return ch;
  }

  if (LASTCHAR == 0) {
    LASTCHAR = ch;
    return 0;
  }

  uint8_t last = LASTCHAR;
  LASTCHAR = 0;

  switch (last) {
    case 0xD0:
      if (ch == 0x81) return 0xA8;
      if (ch >= 0x90 && ch <= 0xBF) return ch + 0x30;
      break;
    case 0xD1:
      if (ch == 0x91) return 0xB8;
      if (ch >= 0x80 && ch <= 0x8F) return ch + 0x70;
      break;
  }

  return (uint8_t)0;
}
