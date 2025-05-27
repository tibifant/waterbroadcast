#include <Arduino.h>
//#include <WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>

//const char* ssid = "CoCoLabor2";
//const char* password = "cocolabor12345";
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

  if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
    triggerPump = true;
    xSemaphoreGive(xMutex);
  }
}

//void connectToWLAN() {
//  Serial.println();
//  Serial.println();
//  Serial.print("Connecting to ");
//  Serial.println(ssid);
//
//  WiFi.begin(ssid, password);
//
//  while (WiFi.status() != WL_CONNECTED) {
//    delay(500);
//    Serial.print(".");
//  }
//
//  Serial.println("");
//  Serial.println("WiFi connected");
//  Serial.println("IP address:");
//  Serial.println(WiFi.localIP());
//}

void taskPump(void *parameter) {
  for (;;) {
    const uint16_t sensorValue = analogRead(sensorPin);

    if (triggerPump && sensorValue < dryThreshold) {
      digitalWrite(pumpPin, HIGH);
      Serial.println("Pump was activated with sensorvalue = " + sensorValue);
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
  for (;;) {
    const uint16_t sensorValue = analogRead(sensorPin);

    if (sensorValue > wetThreshold && !watered) {
      client.publish("PumpOn2", "Sensor value > 500");
      Serial.println("Publisher was triggered with sensorvalue = " + sensorValue);
      watered = true;
    } else if (sensorValue < dryThreshold) {
      watered = false;
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void clientLoop(void * parameter) {
  for (;;)  {
    while (!client.connected()) {
      Serial.print("Attempting MQTT connection...");
      if (client.connect("Malin")) {
        Serial.println("connected");
        client.subscribe("PumpOn");
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

  if (!wm.autoConnect("AutoConnectAP", "flowerpower"))
    Serial.println("Failed Connection via WiFiManager.");
  else
    Serial.println("Successfully connected via WiFIManager.");

  //connectToWLAN();

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  xTaskCreate(taskPump, "Pump", 2048, NULL, 1, NULL);
  xTaskCreate(taskPumpPublish,"PumpPublish", 4096, NULL, 1, NULL);
  xTaskCreatePinnedToCore(clientLoop, "Client loop",8192,NULL,1,NULL, CONFIG_ARDUINO_RUNNING_CORE);

  Serial.println("Setup complete");
}

void loop(){

}

