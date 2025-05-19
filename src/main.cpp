#include <Arduino.h>


moistureSensor pin(18);
LED pin(26)

void setup() {
// write your initialization code here

    Serial.begin(9600);

pinMode(LED, OUTPUT);
pinMode(moistureSensor, INPUT);

digitalWrite(LED, LOW);

}

void loop() {
// write your code here
}