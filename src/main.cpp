#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "CoCoLabor2";
const char* password = "cocolabor12345";
const char* mqtt_server = "broker.hivemq.com";
WiFiClient espClient;
PubSubClient client(espClient);

SemaphoreHandle_t xMutex;

int sensorPin = 34;
int pumpPin = 19;
bool watered = false;
bool triggerPump = false;


int sensorValue = 0;

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived on topic: ");

  if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
    triggerPump = true;
    xSemaphoreGive(xMutex);
  }
}

void connectToWLAN() {
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address:");
  Serial.println(WiFi.localIP());
}

void taskPump(void *parameter) {
  for (;;) {
    sensorValue = analogRead(sensorPin);

    if (triggerPump) {
          digitalWrite(pumpPin, HIGH);
          vTaskDelay(1000 / portTICK_PERIOD_MS);
          digitalWrite(pumpPin, LOW);

        Serial.println("Pump triggerd");
        if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
          triggerPump = false;
          xSemaphoreGive(xMutex);
        }
      } else {

        digitalWrite(pumpPin, LOW);
      }
  }
}

void reconnect() {
  if (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("Emma")) {
      Serial.println("connected");
      client.subscribe("PumpOn");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(9600);
  xMutex = xSemaphoreCreateMutex();

  pinMode(sensorPin, INPUT);
  pinMode(pumpPin, OUTPUT);
  digitalWrite(pumpPin, LOW);

  connectToWLAN();

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  //client.connect("Emma");
  //client.subscribe("PumpOn");

  xTaskCreate( taskPump, "Pump", 1024, NULL, 1, NULL);  // Core where the task should run #1#

  Serial.println("Setup complete");
}
void loop(){
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}

