#include <Arduino.h>

int sensorPin = 34;
int pumpPin = 19;


int sensorValue = 0;

void setup() {

  Serial.begin(9600);

  pinMode(sensorPin, INPUT);
  pinMode(pumpPin, OUTPUT);
  digitalWrite(LED, LOW);

xtaskCreatePinnedToCore(
    taskPump,   /* Function to implement the task */
    "TaskPump", /* Name of the task */
    10000,      /* Stack size in bytes */
    NULL,       /* Task input parameter */
    1,          /* Priority of the task */
    NULL,       /* Task handle. */
    0);         /* Core where the task should run */

  Serial.println("Setup complete");
}

void taskPump(void *parameter) {


  for (;;) {
    sensorValue = analogRead(sensorPin);

    Serial.print("Soil Moisture Value: ");
    Serial.println(sensorValue);

    if (sensorValue > 500) {

      digitalWrite(pumpPin, HIGH)
      vTaskDelay(100000 / portTICK_PERIOD_MS);
      digitalWrite(pumpPin, LOW)

    } else {

      digitalWrite(pumpPin, LOW);
    }


  }

    Serial.println("Task running");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void loop() {


}
