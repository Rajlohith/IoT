#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// ============================================================================
// WIFI CONFIGURATION
// ============================================================================
const char* ssid = "Nord 4";
const char* password = "viaadamo";

// ============================================================================
// MQTT CONFIGURATION
// ============================================================================
const char* mqtt_server = "10.118.86.186";
const int mqtt_port = 1883;
const char* mqtt_topic = "weather/data";

// ============================================================================
// WIND VANE PINS
// ============================================================================
#define NORTH_PIN 26
#define EAST_PIN  27
#define SOUTH_PIN 25
#define WEST_PIN  33

// ============================================================================
// ANEMOMETER PIN
// ============================================================================
#define WIND_PIN 32

// ============================================================================
// RAIN GAUGE PIN
// ============================================================================
#define RAIN_PIN 35

// ============================================================================
// DHT SENSOR CONFIGURATION
// ============================================================================
#define DHTPIN 13
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

// --------------------------------------------------------------------------
// RAIN GAUGE CALIBRATION VALUES
// --------------------------------------------------------------------------
const float FUNNEL_DIAMETER_CM = 15.2;
const float TIP_VOLUME_ML = 6.0;

// Calculate collector area
const float FUNNEL_RADIUS_CM = FUNNEL_DIAMETER_CM / 2.0000;
const float FUNNEL_AREA_CM2 = 3.14159 * FUNNEL_RADIUS_CM * FUNNEL_RADIUS_CM;

// Calculate rainfall represented by one tip
const float RAIN_TIP = (TIP_VOLUME_ML / FUNNEL_AREA_CM2) * 10.0;

// ============================================================================
// SENSOR CALIBRATION CONSTANTS
// ============================================================================
const float ANEMOMETER_RADIUS_M = 0.02;   // 2 cm radius

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================
volatile long pulseCount = 0;           // Wind sensor pulse counter

unsigned long lastMillis = 0;           // Last sensor update timestamp
unsigned long lastTipTime = 0;          // Last rain tip timestamp

float speedKph = 0;                     // Calculated wind speed
float totalRainfall = 0;                // Total rainfall accumulation

int tipcount = 0;                       // Rain gauge tip counter
bool lastRainState = HIGH;              // Previous rain sensor state

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// ============================================================================
// INTERRUPT SERVICE ROUTINE
// Counts anemometer pulses for wind speed calculation
// ============================================================================
void IRAM_ATTR countPulse() {
  pulseCount++;
}

// ============================================================================
// WIFI CONNECTION
// Connects ESP32 to the configured WiFi network
// ============================================================================
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

// ============================================================================
// MQTT RECONNECTION
// Keeps attempting connection until MQTT broker is available
// ============================================================================
void reconnectMQTT() {

  while (!mqttClient.connected()) {

    Serial.print("Connecting MQTT...");

    String clientId =
      "ESP32Weather-" +
      String(random(0xffff), HEX);

    if (mqttClient.connect(clientId.c_str())) {

      Serial.println("connected");

    } else {

      Serial.print("failed rc=");
      Serial.print(mqttClient.state());
      Serial.println(" retrying...");

      delay(2000);
    }
  }
}

// ============================================================================
// MQTT PUBLISH
// Publishes sensor data payload to MQTT topic
// ============================================================================
bool publishMQTT(const String& payload) {

  if (!mqttClient.connected()) {
    reconnectMQTT();
  }

  mqttClient.loop();

  return mqttClient.publish(
      mqtt_topic,
      payload.c_str()
  );
}

// ============================================================================
// WIND DIRECTION READING
// Converts reed switch states into compass angles
// ============================================================================
String readWindDirection() {

  bool N = digitalRead(NORTH_PIN) == LOW;
  bool E = digitalRead(EAST_PIN) == LOW;
  bool S = digitalRead(SOUTH_PIN) == LOW;
  bool W = digitalRead(WEST_PIN) == LOW;

  int active = N + E + S + W;

  if (active == 0) return "Calm";

  if (active > 2) return "Unknown";

  if (N && !E && !S && !W) return "North";
  if (N && E && !S && !W) return "North-East";
  if (!N && E && !S && !W) return "East";
  if (!N && E && S && !W) return "South-East";
  if (!N && !E && S && !W) return "South";
  if (!N && !E && S && W) return "South-West";
  if (!N && !E && !S && W) return "West";
  if (N && !E && !S && W) return "North-West";

  return "Unknown";
}

// ============================================================================
// SETUP
// Initializes sensors, networking, and interrupts
// ============================================================================
void setup() {

  Serial.begin(115200);

  // Wind vane inputs
  pinMode(NORTH_PIN, INPUT_PULLUP);
  pinMode(EAST_PIN, INPUT_PULLUP);
  pinMode(SOUTH_PIN, INPUT_PULLUP);
  pinMode(WEST_PIN, INPUT_PULLUP);

  // Anemometer input
  pinMode(WIND_PIN, INPUT_PULLUP);

  // Rain gauge input
  pinMode(RAIN_PIN, INPUT);

  lastRainState = digitalRead(RAIN_PIN);

  // Initialize DHT sensor
  dht.begin();

  // Start wind pulse counting interrupt
  attachInterrupt(
    digitalPinToInterrupt(WIND_PIN),
    countPulse,
    FALLING
  );

  // Connect to WiFi
  setupWiFi();

  // Configure MQTT broker
  mqttClient.setServer(
    mqtt_server,
    mqtt_port
  );

  lastMillis = millis();
}

// ============================================================================
// MAIN LOOP
// Reads sensors, processes data, and publishes to MQTT
// ============================================================================
void loop() {

  // Ensure MQTT connection is active
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }

  mqttClient.loop();

  // --------------------------------------------------------------------------
  // Rain Gauge Processing
  // --------------------------------------------------------------------------
  bool currentRainState = digitalRead(RAIN_PIN);

  if (lastRainState == LOW && currentRainState == HIGH) {

    // Debounce rain bucket tips
    if (millis() - lastTipTime > 1000) {
      tipcount++;
      lastTipTime = millis();
    }
  }

  lastRainState = currentRainState;

  // Reset rainfall after prolonged inactivity
  if (tipcount > 0 && millis() - lastTipTime > 20000) {
    tipcount = 0;
    totalRainfall = 0;
  }

  // --------------------------------------------------------------------------
  // Sensor Update Every Second
  // --------------------------------------------------------------------------
  if (millis() - lastMillis >= 1000) {

    // Safely copy pulse count from ISR
    noInterrupts();
    long pulses = pulseCount;
    pulseCount = 0;
    interrupts();

    lastMillis = millis();

// ------------------------------------------------------------------------
// Wind Speed Calculation
// 1 pulse = 1 full revolution
// ------------------------------------------------------------------------
float revolutionsPerSecond = pulses;

float circumference =
    2.0 * 3.14159 * ANEMOMETER_RADIUS_M;

float windSpeedMS =
    revolutionsPerSecond * circumference;

speedKph =
    windSpeedMS * 3.6;

// ------------------------------------------------------------------------
// Rainfall Calculation
// ------------------------------------------------------------------------
totalRainfall =
    tipcount * RAIN_TIP;

    // Rain detection status
    bool rainDetected = (millis() - lastTipTime < 20000);

    // Environmental readings
    float temperature = dht.readTemperature();

    float humidity = dht.readHumidity();

    // Wind direction reading
    String windDirection = readWindDirection();

    // ------------------------------------------------------------------------
    // Build JSON Payload
    // ------------------------------------------------------------------------
    String payload = "{";
    payload += "\"temperature\":" + String(temperature) + ",";
    payload += "\"humidity\":" + String(humidity) + ",";
    payload += "\"windSpeed\":" + String(speedKph) + ",";
    payload += "\"windDirection\":\"" + windDirection + "\",";
    payload += "\"rainfall\":" + String(totalRainfall) + ",";
    payload += "\"rainDetected\":" + String(rainDetected ? "true" : "false") + ",";
    payload += "\"pulses\":" + String(pulses) + ",";
    payload += "\"tipCount\":" + String(tipcount);
    payload += "}";

    Serial.println(payload);

    // ------------------------------------------------------------------------
    // Publish Data to MQTT Broker
    // ------------------------------------------------------------------------
    if (publishMQTT(payload)) {
      Serial.println("Data sent to dashboard");
    } else {
      Serial.println("Failed to send");
    }
  }

  delay(100);
}