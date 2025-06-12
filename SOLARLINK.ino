#include <ESP32Servo.h>
#include "DHT.h"
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

// Blynk Configurations
#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPL3_g1wns1p"
#define BLYNK_TEMPLATE_NAME "SUN TRACKING"
#define BLYNK_AUTH_TOKEN "UKPUoMc7Fmyok6eDs3B2aj5a7JIiciNu"
char ssid[] = "Pixel 7";
char pass[] = "qwertyuiop1";

// Pin Definitions
#define DHTPIN 25
#define DHTTYPE DHT11
#define LDR1 32
#define LDR2 33
#define servopin 14
#define Analog_channel_pin 26

// Variables
Servo servo1;
int initial_servopos = 90;
int currentServoAngle = 90;
int error = 5;  // Sensitivity threshold for LDR sensors
int ADC_VALUE = 0;
float voltage_value = 0.0;
float t, h;  // Temperature and humidity values
bool manualControl = false;  // Controlled via V5 switch

// DHT Sensor
DHT dht(DHTPIN, DHTTYPE);

// Voltage reader
void sendVoltage() {
  ADC_VALUE = analogRead(Analog_channel_pin);
  voltage_value = (ADC_VALUE * 3.3) / 4095.0;
  Blynk.virtualWrite(V4, voltage_value);  // V4: Voltage
}

// Manual control switch handler (V5)
BLYNK_WRITE(V5) {
  manualControl = param.asInt();  // 0 = Auto, 1 = Manual
}

// Manual angle input (V0)
BLYNK_WRITE(V0) {
  if (manualControl) {
    int angle = param.asInt();
    angle = constrain(angle, 0, 180);
    currentServoAngle = angle;
    servo1.write(currentServoAngle);
  }
}

void setup() {
  Serial.begin(115200);

  // WiFi setup
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi Connected.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Blynk setup
  Serial.println("Connecting to Blynk...");
  Blynk.config(BLYNK_AUTH_TOKEN, "blynk.cloud", 80);
  if (Blynk.connect(5000)) {
    Serial.println("✅ Blynk Connected!");
  } else {
    Serial.println("❌ Blynk Connection Failed!");
  }

  // Sensor and actuator setup
  servo1.attach(servopin);
  servo1.write(initial_servopos);
  pinMode(LDR1, INPUT);
  pinMode(LDR2, INPUT);
  dht.begin();

  delay(2000); // sensor warm-up
}

void loop() {
  Blynk.run();

  // Read DHT11 and send to Blynk
  h = dht.readHumidity();
  t = dht.readTemperature();
  if (!isnan(h) && !isnan(t)) {
    Blynk.virtualWrite(V2, h);  // V2: Humidity
    Blynk.virtualWrite(V3, t);  // V3: Temperature
  }

  // Voltage monitor
  sendVoltage();

  // Read LDRs
  int R1 = analogRead(LDR1);
  int R2 = analogRead(LDR2);
  int diff = abs(R1 - R2);

  Serial.print("LDR1: ");
  Serial.print(R1);
  Serial.print("  LDR2: ");
  Serial.print(R2);
  Serial.print("  Diff: ");
  Serial.println(diff);

  // 🔧 Auto tracking mode logic
  if (!manualControl && diff > error) {
    if (R1 > R2) {
      currentServoAngle = max(0, currentServoAngle - 2);  // 🔧 faster step
    } else if (R1 < R2) {
      currentServoAngle = min(180, currentServoAngle + 2);  // 🔧 faster step
    }
    servo1.write(currentServoAngle);
    Serial.print("Auto Servo Angle: ");
    Serial.println(currentServoAngle);
  }

  // Always send current servo angle to Blynk (V1)
  Blynk.virtualWrite(V1, currentServoAngle);  // V1: Current angle

  delay(500);  // 🔧 faster refresh rate
}
