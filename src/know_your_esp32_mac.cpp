#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  
  // Print MAC address
  Serial.print("ESP32 MAC Address: ");
  Serial.println(WiFi.macAddress());
}

void loop() {
}