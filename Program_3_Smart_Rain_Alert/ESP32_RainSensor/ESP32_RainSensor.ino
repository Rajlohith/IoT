#include <WiFi.h>
#include <PubSubClient.h>

const char* WIFI_SSID = "Nord 4";
const char* WIFI_PASSWORD = "viaadamo";

const char* TOKEN =
"QNe9ajWLcxaTodvCcBkT";

const char* TB_SERVER =
"";

WiFiClient wifiClient;

PubSubClient client(wifiClient);

#define RAIN_PIN 34

void connectWiFi()
{
  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD);

  while(
    WiFi.status()!=WL_CONNECTED)
  {
    delay(500);
  }
}

void reconnect()
{
  while(!client.connected())
  {
    client.connect(
      "RainMonitor",
      TOKEN,
      NULL);
  }
}

void setup()
{
  Serial.begin(115200);

  connectWiFi();

  client.setServer(
    TB_SERVER,
    1883);
}

void loop()
{
  if(!client.connected())
  {
    reconnect();
  }

  client.loop();

  int rainRaw =
    analogRead(RAIN_PIN);

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

  String payload =
    "{\"rain\":"
    + String(rainPercent)
    + "}";

  client.publish(
    "v1/devices/me/telemetry",
    payload.c_str());

  Serial.println(payload);

  delay(3000);
}