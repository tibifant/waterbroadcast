#include <Arduino.h>

int sensorPin = 34;
int pumpPin = 19;
bool watered = false;

int sensorValue = 0;

void taskPump(void *parameter) {
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
  }

void setup() {


  Serial.begin(9600);

  pinMode(sensorPin, INPUT);
  pinMode(pumpPin, OUTPUT);
  digitalWrite(pumpPin, LOW);


  xTaskCreate( taskPump, "Pump", 1024, NULL, 1, NULL) ;  /* Core where the task should run */

  Serial.println("Setup complete");
}


void loop() {


}
