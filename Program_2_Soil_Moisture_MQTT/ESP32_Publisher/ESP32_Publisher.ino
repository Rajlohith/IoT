#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "YOUR_WIFI";
const char* password = "YOUR_PASSWORD";

const char* mqtt_server = "192.168.1.100";

WiFiClient espClient;
PubSubClient client(espClient);

#define SOIL_PIN 34

void setupWiFi()
{
  WiFi.begin(ssid,password);

  while(WiFi.status()!=WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi Connected");
}

void reconnect()
{
  while(!client.connected())
  {
    if(client.connect("SoilPublisher"))
    {
      Serial.println("MQTT Connected");
    }
    else
    {
      delay(2000);
    }
  }
}

void setup()
{
  Serial.begin(115200);

  setupWiFi();

  client.setServer(mqtt_server,1883);
}

void loop()
{
  if(!client.connected())
    reconnect();

  client.loop();

  int raw = analogRead(SOIL_PIN);

  int moisture =
  map(raw,4095,1200,0,100);

  moisture = constrain(moisture,0,100);

  String payload = String(moisture);

  client.publish(
      "plant/soil",
      payload.c_str());

  Serial.print("Moisture: ");
  Serial.print(moisture);
  Serial.println("%");

  delay(5000);
}