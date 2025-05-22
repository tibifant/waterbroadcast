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

void taskPumpPublish(void *parameter) {
  for (;;) {
    sensorValue = analogRead(sensorPin);

    Serial.print("Soil Moisture Value: ");
    Serial.println(sensorValue);

    if (sensorValue > 500) {
      if (watered == false) {

        client.publish("PumpOn","sensorValue");
        Serial.println("SensorValue: " + String(sensorValue));
        Serial.println("Pump is ON");
        watered = true;
      }
    }

    if (sensorValue < 200) {
      watered = false;
    }
  }

  Serial.println("Task running");
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}

void clientLoop(void * parameter) {
  for (;;)  {
    while (!client.connected()) {
      Serial.print("Attempting MQTT connection...");
      if (client.connect("Emma")) {
        Serial.println("connected");
        client.subscribe("PumpOn2");
      } else {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
      }
    }

    client.loop();
    vTaskDelay(100 / portTICK_PERIOD_MS);
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

  xTaskCreate( taskPump, "Pump", 2048, NULL, 1, NULL);
  xTaskCreate(taskPumpPublish,"PumpPublish", 2048, NULL, 1, NULL);
  xTaskCreatePinnedToCore(clientLoop, "Client loop",8192,NULL,1,NULL, CONFIG_ARDUINO_RUNNING_CORE);

  Serial.println("Setup complete");
}
void loop(){

}

