/* =========================================================
   AUTONOMOUS WATER SKIMMER BOAT — ESP32 Firmware
   Obstacle Avoidance + Debris Collection + Water Quality IoT

   Project: Autonomous Adaptive Water Surface Cleaning Robot
   Hardware: ESP32, L298N, HC-SR04, IR sensor, Turbidity x2, pH
   ========================================================= */

#include <WiFi.h>
#include <HTTPClient.h>

// ---------------- WiFi Credentials ----------------
const char* ssid       = "YOUR_WIFI_SSID";
const char* password   = "YOUR_WIFI_PASSWORD";
const char* serverURL  = "http://your-server-or-dashboard-url";

// ---------------- Motor Driver Pins (L298N) ----------------
const int ENA = 14, IN1 = 27, IN2 = 26;   // Left propulsion motor
const int ENB = 25, IN3 = 33, IN4 = 13;   // Right propulsion motor
const int CONV_PWM = 12, CONV_IN1 = 15, CONV_IN2 = 2; // Conveyor belt motor (2nd driver channel)

// ---------------- Ultrasonic Sensor Pins ----------------
const int TRIG_PIN = 5;
const int ECHO_PIN = 18;   // via 1k/2k voltage divider

// ---------------- IR Debris Sensor ----------------
const int IR_PIN = 4;

// ---------------- Water Quality Sensor Pins ----------------
const int TURBIDITY_PRE_PIN  = 34;  // before filter
const int TURBIDITY_POST_PIN = 35;  // after filter
const int PH_PIN             = 39;

// ---------------- Tunable Parameters ----------------
const int OBSTACLE_THRESHOLD_CM   = 30;
const int CRUISE_SPEED            = 180;   // PWM 0-255
const int CONVEYOR_IDLE_SPEED     = 80;
const int CONVEYOR_ACTIVE_SPEED   = 220;
const unsigned long SENSOR_UPLOAD_INTERVAL = 8000;  // ms
const unsigned long DENSITY_WINDOW         = 3000;  // ms rolling window for IR triggers

unsigned long lastUploadTime = 0;
unsigned long densityWindowStart = 0;
int irTriggerCount = 0;

// ---------------------------------------------------------
void setup() {
  Serial.begin(115200);

  pinMode(ENA, OUTPUT); pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT); pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(CONV_PWM, OUTPUT); pinMode(CONV_IN1, OUTPUT); pinMode(CONV_IN2, OUTPUT);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(IR_PIN, INPUT);

  connectWiFi();
  densityWindowStart = millis();
}

// ---------------------------------------------------------
void loop() {
  // --- A) Reactive obstacle avoidance (every cycle) ---
  float distance = getDistanceCM();

  if (distance > 0 && distance < OBSTACLE_THRESHOLD_CM) {
    avoidObstacle();
  } else {
    moveForward(CRUISE_SPEED);
  }

  // --- B) Debris density estimation + adaptive collection ---
  updateDebrisDensity();

  // --- C) Non-blocking periodic sensor upload ---
  if (millis() - lastUploadTime >= SENSOR_UPLOAD_INTERVAL) {
    lastUploadTime = millis();
    logWaterQuality();
  }
}

// ---------------------------------------------------------
// ULTRASONIC DISTANCE MEASUREMENT
float getDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // 30ms timeout
  if (duration == 0) return -1; // no echo = out of range

  return duration * 0.0343 / 2.0; // speed of sound conversion
}

// ---------------------------------------------------------
// DEBRIS DENSITY ESTIMATION (IR trigger frequency, rolling window)
void updateDebrisDensity() {
  if (digitalRead(IR_PIN) == LOW) { // debris detected (active-low sensor)
    irTriggerCount++;
  }

  if (millis() - densityWindowStart >= DENSITY_WINDOW) {
    float density = (float)irTriggerCount / (DENSITY_WINDOW / 1000.0); // triggers/sec

    if (density > 1.5) {
      runConveyor(CONVEYOR_ACTIVE_SPEED); // High density
    } else if (density > 0.5) {
      runConveyor((CONVEYOR_ACTIVE_SPEED + CONVEYOR_IDLE_SPEED) / 2); // Medium
    } else {
      runConveyor(CONVEYOR_IDLE_SPEED); // Low density
    }

    Serial.printf("Debris density: %.2f triggers/sec\n", density);
    irTriggerCount = 0;
    densityWindowStart = millis();
  }
}

// ---------------------------------------------------------
// MOTOR CONTROL FUNCTIONS — Propulsion
void moveForward(int speed) {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  analogWrite(ENA, speed);
  analogWrite(ENB, speed);
}

void moveBackward(int speed) {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  analogWrite(ENA, speed);
  analogWrite(ENB, speed);
}

void turnRight(int speed) {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
  analogWrite(ENA, speed);
  analogWrite(ENB, speed);
}

void stopMotors() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

// MOTOR CONTROL FUNCTIONS — Conveyor belt
void runConveyor(int speed) {
  digitalWrite(CONV_IN1, HIGH);
  digitalWrite(CONV_IN2, LOW);
  analogWrite(CONV_PWM, speed);
}

// ---------------------------------------------------------
// OBSTACLE AVOIDANCE MANEUVER
void avoidObstacle() {
  stopMotors();
  delay(200);
  moveBackward(CRUISE_SPEED);
  delay(500);
  turnRight(CRUISE_SPEED);
  delay(400);
  stopMotors();
}

// ---------------------------------------------------------
// WATER QUALITY READ + WQI CLASSIFICATION + UPLOAD
void logWaterQuality() {
  int turbidityPreRaw  = analogRead(TURBIDITY_PRE_PIN);
  int turbidityPostRaw = analogRead(TURBIDITY_POST_PIN);
  int phRaw = analogRead(PH_PIN);

  // TODO: Replace with calibrated conversion formulas (see calibration notes in /docs)
  float turbidityPreNTU  = map(turbidityPreRaw, 0, 4095, 0, 3000);
  float turbidityPostNTU = map(turbidityPostRaw, 0, 4095, 0, 3000);
  float phValue = (phRaw / 4095.0) * 14.0;

  float turbidityReductionPct = 0;
  if (turbidityPreNTU > 0) {
    turbidityReductionPct = ((turbidityPreNTU - turbidityPostNTU) / turbidityPreNTU) * 100.0;
  }

  String wqiStatus = classifyWQI(turbidityPostNTU, phValue);

  Serial.printf("Turbidity Pre: %.1f NTU | Post: %.1f NTU | Reduction: %.1f%% | pH: %.2f | WQI: %s\n",
                turbidityPreNTU, turbidityPostNTU, turbidityReductionPct, phValue, wqiStatus.c_str());

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(serverURL) +
                 "?turbidity_pre=" + turbidityPreNTU +
                 "&turbidity_post=" + turbidityPostNTU +
                 "&ph=" + phValue +
                 "&wqi_status=" + wqiStatus;
    http.begin(url);
    int httpCode = http.GET();
    Serial.printf("Upload status: %d\n", httpCode);
    http.end();
  } else {
    Serial.println("WiFi disconnected — skipping upload");
  }
}

// ---------------------------------------------------------
// WATER QUALITY INDEX (WQI) CLASSIFICATION
// Simple weighted threshold classification against reference bands
String classifyWQI(float turbidityNTU, float ph) {
  int turbidityScore, phScore;

  if (turbidityNTU < 5) turbidityScore = 3;        // Good
  else if (turbidityNTU < 25) turbidityScore = 2;  // Moderate
  else turbidityScore = 1;                          // Poor

  if (ph >= 6.5 && ph <= 8.5) phScore = 3;          // Good
  else if ((ph >= 6 && ph < 6.5) || (ph > 8.5 && ph <= 9)) phScore = 2; // Moderate
  else phScore = 1;                                  // Poor

  float avgScore = (turbidityScore + phScore) / 2.0;

  if (avgScore >= 2.5) return "Safe";
  else if (avgScore >= 1.5) return "Moderate";
  else return "Polluted";
}

// ---------------------------------------------------------
void connectWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 15000) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected! IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nWiFi connection failed — continuing offline (local nav still active)");
  }
}
