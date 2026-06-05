#include <WiFi.h>
#include <PubSubClient.h>

// ============================================================================
// WIFI CONFIGURATION
// ============================================================================
const char* WIFI_SSID = "Nord 4";
const char* WIFI_PASSWORD = "viaadamo";

// ============================================================================
// THINGSBOARD MQTT CONFIGURATION
// ============================================================================
const char* TOKEN =
"QNe9ajWLcxaTodvCcBkT";

const char* TB_SERVER =
"";

// ============================================================================
// RAIN SENSOR PIN
// ============================================================================
#define RAIN_PIN 34

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================
WiFiClient wifiClient;

PubSubClient client(wifiClient);

// ============================================================================
// WIFI CONNECTION
// Connects ESP32 to the configured WiFi network
// ============================================================================
void connectWiFi()
{
  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD);

  while (
    WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }
}

// ============================================================================
// MQTT RECONNECTION
// Keeps attempting connection until ThingsBoard becomes available
// ============================================================================
void reconnect()
{
  while (!client.connected())
  {
    client.connect(
      "RainMonitor",
      TOKEN,
      NULL);
  }
}

// ============================================================================
// SETUP
// Initializes serial communication, WiFi, and MQTT client
// ============================================================================
void setup()
{
  Serial.begin(115200);

  // Connect to WiFi
  connectWiFi();

  // Configure ThingsBoard MQTT server
  client.setServer(
    TB_SERVER,
    1883);
}

// ============================================================================
// MAIN LOOP
// Reads rain sensor data and publishes telemetry to ThingsBoard
// ============================================================================
void loop()
{
  // Ensure MQTT connection is active
  if (!client.connected())
  {
    reconnect();
  }

  client.loop();

  // --------------------------------------------------------------------------
  // Rain Sensor Reading
  // --------------------------------------------------------------------------
  int rainRaw =
    analogRead(RAIN_PIN);

  // --------------------------------------------------------------------------
  // Convert ADC Reading to Rain Percentage
  // 4095 = Dry
  // 0    = Fully Wet
  // --------------------------------------------------------------------------
  int rainPercent =
    map(
      rainRaw,
      4095,
      0,
      0,
      100);

  rainPercent =
    constrain(
      rainPercent,
      0,
      100);

  // --------------------------------------------------------------------------
  // Build JSON Payload
  // --------------------------------------------------------------------------
  String payload =
    "{\"rain\":"
    + String(rainPercent)
    + "}";

  // --------------------------------------------------------------------------
  // Publish Data to ThingsBoard
  // --------------------------------------------------------------------------
  client.publish(
    "v1/devices/me/telemetry",
    payload.c_str());

  // Debug Output
  Serial.println(payload);

  // Update every 3 seconds
  delay(3000);
}