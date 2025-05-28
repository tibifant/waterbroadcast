#include <Arduino.h>
#include <WiFiManager.h>
#include <PubSubClient.h>

const char* mqtt_server = "broker.hivemq.com";
WiFiManager wm;
WiFiClient espClient;
PubSubClient client(espClient);

const char* clientName = "Flower2";
const char* pumpPublisherTopic = "PumpOn";
const char* pumpSubscriberTopic = "PumpOn1";
const char* statusPublisherTopic = "status_flower2";
const char* statusSubscriberTopic = "status_flower1";

SemaphoreHandle_t xMutex;

constexpr uint8_t sensorPin = 34;
constexpr uint8_t pumpPin = 19;
constexpr uint8_t ledPin = 26;

bool watered = false;
bool triggerPump = false;

constexpr uint16_t dryThreshold = 200;
constexpr uint16_t wetThreshold = 500;

enum status_type : uint8_t {
  st_dry,
  st_wet,
  
  st_count
};

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived!");

  if (!strcmp(topic, pumpSubscriberTopic)) {
    const uint16_t sensorValue = analogRead(sensorPin);

    if (sensorValue < dryThreshold) {
      if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
        triggerPump = true;
        xSemaphoreGive(xMutex);
      }
    }
  } else if (!strcmp(topic, statusSubscriberTopic)) {
    switch (*payload) {
      case st_wet: {
        manageLed(false);
        break;
      }
      case st_dry: {
        manageLed(true);
        break;
      }
      default: {
        Serial.println("Unkonwn payload status");
      }
    }
  }
}

void connectToWLAN() {  
  if (!wm.autoConnect("AutoConnectAP", "flowerpower"))
    Serial.println("Failed Connection via WiFiManager.");
  else
    Serial.println("Successfully connected via WiFIManager.");
}

void taskPump() {
  while (true) {
    if (triggerPump) {
      digitalWrite(pumpPin, HIGH);
      
      if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
        Serial.println("Pump was activated");
        watered = true;
        xSemaphoreGive(xMutex);
      }

      vTaskDelay(5000 / portTICK_PERIOD_MS);
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

    if (sensorValue > wetThreshold) {
      const byte payload = (byte)(st_wet);
      client.publish(statusPublisherTopic, &payload, sizeof(status_type));

      if (!watered) {
        client.publish(pumpPublisherTopic, "true");

        if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
          Serial.println("Publisher was triggered");
          watered = true;
          xSemaphoreGive(xMutex);
        } 
      }     
    } else if (sensorValue < dryThreshold) {
      if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
        watered = false;
        xSemaphoreGive(xMutex);
      }
      
      const byte payload = (byte)(st_dry);
      client.publish(statusPublisherTopic, &payload, sizeof(status_type));
    }

    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}

void manageLed(bool on) {
  if (on)
    digitalWrite(ledPin, HIGH);
  else
    digitalWrite(ledPin, LOW);

}

void clientLoop(void * parameter) {
  while (true) {
    while (!client.connected()) {
      Serial.print("Attempting MQTT connection...");
      if (client.connect(clientName)) {
        Serial.println("connected");
        client.subscribe(pumpSubscriberTopic);
        client.subscribe(statusSubscriberTopic);
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
  pinMode(ledPin, OUTPUT);
  analogWrite(ledPin, LOW);

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
