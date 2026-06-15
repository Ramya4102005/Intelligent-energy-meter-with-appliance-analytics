#include <EmonLib.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

EnergyMonitor emon1, emon2, emonV;
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Pins
#define FAN_RELAY 5
#define LIGHT_RELAY 4
#define BUZZER 6
#define FAN_BUTTON 7
#define LIGHT_BUTTON 8

#define CURRENT_CAL 60.6
#define VOLT_CAL 234.0

bool fanState = false;
bool lightState = false;

bool lastFanBtn = HIGH;
bool lastLightBtn = HIGH;

unsigned long lastUpdate = 0;
unsigned long buzzerTimer = 0;

bool overload = false;

void setup() {
  Serial.begin(9600);

  lcd.init();
  lcd.backlight();

  pinMode(FAN_RELAY, OUTPUT);
  pinMode(LIGHT_RELAY, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  pinMode(FAN_BUTTON, INPUT_PULLUP);
  pinMode(LIGHT_BUTTON, INPUT_PULLUP);

  digitalWrite(FAN_RELAY, HIGH);
  digitalWrite(LIGHT_RELAY, HIGH);
  digitalWrite(BUZZER, LOW);

  emon1.current(A0, CURRENT_CAL);
  emon2.current(A1, CURRENT_CAL);
  emonV.voltage(A2, VOLT_CAL, 1.7);

  lcd.print("System Ready");
  delay(1000);
  lcd.clear();
}

void loop() {

  // -------- BUTTON --------
  bool fanBtn = digitalRead(FAN_BUTTON);
  bool lightBtn = digitalRead(LIGHT_BUTTON);

  if (fanBtn == LOW && lastFanBtn == HIGH) {
    fanState = !fanState;
    digitalWrite(FAN_RELAY, fanState ? LOW : HIGH);
  }

  if (lightBtn == LOW && lastLightBtn == HIGH) {
    lightState = !lightState;
    digitalWrite(LIGHT_RELAY, lightState ? LOW : HIGH);
  }

  lastFanBtn = fanBtn;
  lastLightBtn = lightBtn;

  // -------- SENSOR EVERY 1 SEC --------
  if (millis() - lastUpdate >= 1000) {
    lastUpdate = millis();

    double fanI = emon1.calcIrms(500);
    double lightI = emon2.calcIrms(500);

    emonV.calcVI(10, 1000);
    double voltage = emonV.Vrms;

    // -------- OVERLOAD --------
    if (lightI > 1.50 && !overload) {
      overload = true;

      digitalWrite(LIGHT_RELAY, HIGH); // OFF
      digitalWrite(BUZZER, HIGH);

      buzzerTimer = millis();
    }

    // buzzer OFF after 3 sec
    if (overload && millis() - buzzerTimer >= 3000) {
      digitalWrite(BUZZER, LOW);
    }

    // -------- LCD --------
    lcd.clear();
    if (overload) {
      lcd.setCursor(0, 0);
      lcd.print("!!! OVERLOAD !!!");
      lcd.setCursor(0, 1);
      lcd.print("Light OFF");
    } else {
      lcd.setCursor(0, 0);
      lcd.print("V:");
      lcd.print(voltage, 1);

      lcd.setCursor(0, 1);
      lcd.print("F:");
      lcd.print(fanI, 2);
      lcd.print(" L:");
      lcd.print(lightI, 2);
    }

    // -------- SERIAL SEND --------
    Serial.print("V:");
    Serial.print(voltage);
    Serial.print(",F:");
    Serial.print(fanI);
    Serial.print(",L:");
    Serial.print(lightI);
    Serial.print(",S:");
    Serial.println(overload ? "OVERLOAD" : "NORMAL");
  }
}