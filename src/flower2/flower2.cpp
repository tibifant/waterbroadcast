#include <Arduino.h>
#include <WiFiManager.h>
#include <PubSubClient.h>

const char* mqtt_server = "broker.hivemq.com";
WiFiManager wm;
WiFiClient espClient;
PubSubClient client(espClient);

SemaphoreHandle_t xMutex;

constexpr uint8_t sensorPin = 34;
constexpr uint8_t pumpPin = 19;

bool watered = false;
bool triggerPump = false;

constexpr uint16_t dryThreshold = 200;
constexpr uint16_t wetThreshold = 500;

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived!");
  const uint16_t sensorValue = analogRead(sensorPin);

  if (sensorValue < dryThreshold) {
    if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
      triggerPump = true;
      xSemaphoreGive(xMutex);
    }
  }
}

void connectToWLAN() {  
  if (!wm.autoConnect("AutoConnectAP", "flowerpower"))
    Serial.println("Failed Connection via WiFiManager.");
  else
    Serial.println("Successfully connected via WiFIManager.");
}

void taskPump(void *parameter) {
  while (true) {
    const uint16_t sensorValue = analogRead(sensorPin);

    if (triggerPump) {
      digitalWrite(pumpPin, HIGH);
      
      if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
        Serial.println("Pump was activated");
        watered = true;
        xSemaphoreGive(xMutex);
      }

      vTaskDelay(1000 / portTICK_PERIOD_MS);
      digitalWrite(pumpPin, LOW);
      
      if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
        triggerPump = false;
        xSemaphoreGive(xMutex);
      }
    } else {
      digitalWrite(pumpPin, LOW);
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void taskPumpPublish(void *parameter) {
  while (true) {
    const uint16_t sensorValue = analogRead(sensorPin);

    if (sensorValue > wetThreshold && !watered) {
      client.publish("PumpOn", "true");

      if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
        Serial.println("Publisher was triggered");
        watered = true;
        xSemaphoreGive(xMutex);
      }      
    } else if (sensorValue < dryThreshold) {
      if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
        watered = false;
        xSemaphoreGive(xMutex);
      }
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void clientLoop(void * parameter) {
  while (true) {
    while (!client.connected()) {
      Serial.print("Attempting MQTT connection...");
      if (client.connect("Flower2")) {
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

  xTaskCreate(taskPump, "Pump", 4096, NULL, 1, NULL);
  xTaskCreate(taskPumpPublish,"PumpPublish", 4096, NULL, 1, NULL);
  xTaskCreatePinnedToCore(clientLoop, "Client loop", 8192, NULL, 1, NULL, CONFIG_ARDUINO_RUNNING_CORE);

  Serial.println("Setup complete");
}

void loop(){

}
