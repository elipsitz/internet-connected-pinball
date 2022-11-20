#include <Arduino.h>

static int i = 0;

void setup() {
  Serial.begin();
}

void loop() {
  i++;
  Serial.printf("hello, world: %d\n", i);
  delay(1000);
}