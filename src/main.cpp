#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "CoCoLabor2";
const char* password = "cocolabor12345";
const char* mqtt_server = "broker.hivemq.com";
WiFiClient espClient;
PubSubClient client(espClient);

int sensorPin = 34;
int pumpPin = 19;
bool watered = false;


int sensorValue = 0;

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  digitalWrite(pumpPin, HIGH);

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


  client.setServer(mqtt_server, 1883);
  client.connect("Emma");
  client.setCallback(callback);

  client.subscribe("PumpOn");
}
/*void taskPump(void *parameter) {
  for (;;) {
    sensorValue = analogRead(sensorPin);

    Serial.print("Soil Moisture Value: ");
    Serial.println(sensorValue);

    if (sensorValue > 500) {
      if (watered == false) {
        digitalWrite(pumpPin, HIGH);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        digitalWrite(pumpPin, LOW);
        watered = true;
      }

    } else {

      digitalWrite(pumpPin, LOW);
    }

    if (sensorValue < 200) {
      watered = false;
    }


  }

    Serial.println("Task running");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }*/

void setup() {
  Serial.begin(9600);

  connectToWLAN();

  pinMode(sensorPin, INPUT);
  pinMode(pumpPin, OUTPUT);


  /*
  digitalWrite(pumpPin, LOW);



  xTaskCreate( taskPump, "Pump", 1024, NULL, 1, NULL) ;  /* Core where the task should run #1#

  Serial.println("Setup complete");
}
*/
}
  void loop(){
  }

