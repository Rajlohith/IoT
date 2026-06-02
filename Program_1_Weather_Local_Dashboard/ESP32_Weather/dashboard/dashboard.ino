#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>

// ---------------- WIFI ----------------
const char* ssid = "Nord 4";
const char* password = "viaadamo";

// ---------------- LOCAL DASHBOARD ----------------
const char* serverUrl = "http://10.118.86.186:5000/api/data";

// ---------------- WIND VANE ----------------
#define NORTH_PIN 26
#define EAST_PIN  27
#define SOUTH_PIN 25
#define WEST_PIN  33

// ---------------- ANEMOMETER ----------------
#define WIND_PIN 32

// ---------------- RAIN GAUGE ----------------
#define RAIN_PIN 35

// ---------------- DHT ----------------
#define DHTPIN 13
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

// ---------------- CONSTANTS ----------------
const float RAIN_TIP = 0.1983;
const float CALIBRATION_FACTOR = 1.2;

// ---------------- VARIABLES ----------------
volatile long pulseCount = 0;

unsigned long lastMillis = 0;
unsigned long lastTipTime = 0;

float speedKph = 0;
float totalRainfall = 0;

int tipcount = 0;
bool lastRainState = HIGH;

WiFiClient wifiClient;

// ---------------- INTERRUPT ----------------
void IRAM_ATTR countPulse() {
  pulseCount++;
}

// ---------------- WIFI ----------------
void setupWiFi() {

  Serial.print("Connecting to WiFi");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi Connected");
  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.localIP());
}

// ---------------- HTTP POST ----------------
bool sendToDashboard(const String& payload) {

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected");
    return false;
  }

  HTTPClient http;

  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/json");

  int httpCode = http.POST(payload);

  Serial.print("HTTP Code: ");
  Serial.println(httpCode);

  if (httpCode > 0) {
    String response = http.getString();
    Serial.println(response);
  }

  http.end();

  return (httpCode >= 200 && httpCode < 300);
}

// ---------------- WIND DIRECTION ----------------
int readWindDirection() {

  bool N = digitalRead(NORTH_PIN) == LOW;
  bool E = digitalRead(EAST_PIN) == LOW;
  bool S = digitalRead(SOUTH_PIN) == LOW;
  bool W = digitalRead(WEST_PIN) == LOW;

  int active = N + E + S + W;

  if (active > 2) return 0;

  if (N && !E && !S && !W) return 0;
  if (N && E && !S && !W) return 45;
  if (!N && E && !S && !W) return 90;
  if (!N && E && S && !W) return 135;
  if (!N && !E && S && !W) return 180;
  if (!N && !E && S && W) return 225;
  if (!N && !E && !S && W) return 270;
  if (N && !E && !S && W) return 315;

  return 0;
}

// ---------------- SETUP ----------------
void setup() {

  Serial.begin(115200);

  pinMode(NORTH_PIN, INPUT_PULLUP);
  pinMode(EAST_PIN, INPUT_PULLUP);
  pinMode(SOUTH_PIN, INPUT_PULLUP);
  pinMode(WEST_PIN, INPUT_PULLUP);

  pinMode(WIND_PIN, INPUT_PULLUP);

  pinMode(RAIN_PIN, INPUT);

  lastRainState = digitalRead(RAIN_PIN);

  dht.begin();

  attachInterrupt(
    digitalPinToInterrupt(WIND_PIN),
    countPulse,
    FALLING
  );

  setupWiFi();

  lastMillis = millis();
}

// ---------------- LOOP ----------------
void loop() {

  bool currentRainState = digitalRead(RAIN_PIN);

  if (lastRainState == LOW && currentRainState == HIGH) {

    if (millis() - lastTipTime > 1000) {
      tipcount++;
      lastTipTime = millis();
    }
  }

  lastRainState = currentRainState;

  if (tipcount > 0 && millis() - lastTipTime > 20000) {
    tipcount = 0;
    totalRainfall = 0;
  }

  if (millis() - lastMillis >= 1000) {

    noInterrupts();
    long pulses = pulseCount;
    pulseCount = 0;
    interrupts();

    lastMillis = millis();

    speedKph = pulses * CALIBRATION_FACTOR;

    totalRainfall = tipcount * RAIN_TIP;

    bool rainDetected =
      (millis() - lastTipTime < 20000);

    float temperature =
      dht.readTemperature();

    float humidity =
      dht.readHumidity();

    int directionAngle =
      readWindDirection();

    String payload = "{";
    payload += "\"temperature\":" + String(temperature) + ",";
    payload += "\"humidity\":" + String(humidity) + ",";
    payload += "\"windSpeed\":" + String(speedKph) + ",";
    payload += "\"windDirection\":" + String(directionAngle) + ",";
    payload += "\"rainfall\":" + String(totalRainfall) + ",";
    payload += "\"rainDetected\":" + String(rainDetected ? "true" : "false") + ",";
    payload += "\"pulses\":" + String(pulses) + ",";
    payload += "\"tipCount\":" + String(tipcount);
    payload += "}";

    Serial.println(payload);

    if (sendToDashboard(payload)) {
      Serial.println("Data sent to dashboard");
    } else {
      Serial.println("Failed to send");
    }
  }

  delay(100);
}
