#include <Arduino.h>
#include <WiFiManager.h>
#include <PubSubClient.h>

#if defined(CLIENT_1)
const char* clientName = "Flower1";
const char* pumpPublisherTopic = "activate_pump2";
const char* pumpSubscriberTopic = "activate_pump1";
const char* statusPublisherTopic = "status_flower1";
const char* statusSubscriberTopic = "status_flower2";
#elif defined(CLIENT_2)
const char* clientName = "Flower2";
const char* pumpPublisherTopic = "activate_pump1";
const char* pumpSubscriberTopic = "activate_pump2";
const char* statusPublisherTopic = "status_flower2";
const char* statusSubscriberTopic = "status_flower1";
#else
#error "No Client Name provided."
#endif

const char* mqtt_server = "broker.hivemq.com";
WiFiManager wm;
WiFiClient espClient;
PubSubClient client(espClient);

SemaphoreHandle_t xMutex;

constexpr uint8_t sensorPin = 34;
constexpr uint8_t pumpPin = 19;
constexpr uint8_t ledPin = 26;

bool watered = false;
bool triggerPump = false;

constexpr uint16_t dryThreshold = 200;
constexpr uint16_t wateredThreshold = 500;

enum status_type : uint8_t {
  st_dry,
  st_wet,
  
  st_count
};

status_type lastStatus = st_count;

void callback(char* topic, byte* payload, unsigned int length) {  
  if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
    Serial.print("Message arrived!");
    Serial.println(String(topic));
    xSemaphoreGive(xMutex);
  }

  if (strcmp(topic, pumpSubscriberTopic) == 0) {
    const uint16_t sensorValue = analogRead(sensorPin);

    if (sensorValue < dryThreshold) {
      if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
        triggerPump = true;
        xSemaphoreGive(xMutex);
      }
    }
  } else if (strcmp(topic, statusSubscriberTopic) == 0) {
    if (length >= 1) {
      const status_type status = (status_type)(*payload);

      switch (status) {
        case st_wet: {
          digitalWrite(ledPin, LOW);
          break;
        }
        case st_dry: {
          digitalWrite(ledPin, HIGH);
          break;
        }
        default: {
          if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
            Serial.println("Unkonwn payload status");
            xSemaphoreGive(xMutex);
          }
        }
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

void taskPump(void *parameter) {
  while (true) {
    if (triggerPump) {
      digitalWrite(pumpPin, HIGH);

      if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
        Serial.println("Pump was activated");
        watered = true;
        xSemaphoreGive(xMutex);
      }

      vTaskDelay(2000 / portTICK_PERIOD_MS);
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

void taskPublish(void *parameter) {
  while (true) {
    const uint16_t sensorValue = analogRead(sensorPin);
    status_type currentStatus = st_count;

    if (sensorValue < dryThreshold) {
      currentStatus = st_dry;

      if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
        watered = false;
        xSemaphoreGive(xMutex);
      }
      
    } else {
      currentStatus = st_wet;

      if (sensorValue > wateredThreshold) {
        if (!watered) {
          client.publish(pumpPublisherTopic, "Friend Watered");

          if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
              Serial.println("Watering Publisher was triggered");
            watered = true;
            xSemaphoreGive(xMutex);
          }
        }
      }
    }

    if (currentStatus != lastStatus) {
      const byte payload = currentStatus;
      client.publish(statusPublisherTopic, &payload, sizeof(status_type));
      
      lastStatus = currentStatus;
    }

    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
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
  digitalWrite(ledPin, HIGH);

  connectToWLAN();

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  xTaskCreate(taskPump, "Activate Pump", 4096, NULL, 1, NULL);
  xTaskCreate(taskPublish,"Publish Pump and Status", 4096, NULL, 1, NULL);
  xTaskCreatePinnedToCore(clientLoop, "Client loop", 8192, NULL, 1, NULL, CONFIG_ARDUINO_RUNNING_CORE);

  Serial.println("Setup complete");
}

void loop(){

}
