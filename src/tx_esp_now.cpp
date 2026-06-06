#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "EspNowRcLink/Transmitter.h" 


#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);


// Pins

#define vrx1 34
#define vry1 35
#define vrx2 32
#define vry2 33
#define sw1 27
#define sw2 16  
#define whiteled 13
//#define redled 17
#define blueled 14
#define tx_batt_pin 36 


EspNowRcLink::Transmitter tx;


// Logic & Math Constants
const float tx_batt_ratio = 2.0;   // 10k/10k local divider
const float tx_batt_cal = 1.02; 
const float ema_alpha = 0.05;


//float smoothedTXV = 0.0;
float rawTXV = 0.0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastSendTime = 0;


bool armstate = false;


int simpleChannel(int raw) {
  return map(raw, 0, 4095, 1000, 2000);
}

// TX Battery

void processMath() {
  float tPinV = (analogRead(tx_batt_pin) * 3.3) / 4095.0;
  rawTXV = tPinV * tx_batt_ratio * tx_batt_cal;
  //if (smoothedTXV < 1.0) smoothedTXV = rawTXV;
  //else smoothedTXV = (ema_alpha * rawTXV) + ((1.0 - ema_alpha) * smoothedTXV);
}


void updateOLED() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  processMath();

  display.setCursor(0,0);
  display.print(armstate ? "ARMED" : "DISARMED");

  display.setCursor(80, 0);
  display.print("TX: ");
  display.print(rawTXV, 2);
  display.print("V");

  display.setCursor(20, 25);
  display.print("SENDING PACKETS");

  display.setTextSize(2);
  display.setCursor(35,45);
  display.print("HELIX");

  display.display();

}

void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  //Wire.setClock(400000); 
  
  pinMode(tx_batt_pin, INPUT);
  pinMode(sw1, INPUT_PULLUP);
  pinMode(sw2, INPUT_PULLUP);
  pinMode(whiteled, OUTPUT); 
  //pinMode(redled, OUTPUT); 
  pinMode(blueled, OUTPUT);


  // Initialize ESP-NOW Transmitter. 
  if(tx.begin(true)){digitalWrite(blueled, HIGH);}
  else{digitalWrite(blueled, LOW);}
}


void loop() {

  tx.update();
  
  //digitalWrite(redled, LOW);



  // 2. Send data at a stable 50Hz (every 20ms)
  if (millis() - lastSendTime > 20) {
    
    // Read & Map Joysticks
    uint16_t throttle = 3000 - simpleChannel(analogRead(vry1));
    uint16_t roll     = simpleChannel(analogRead(vrx2));
    uint16_t pitch    = 3000 - simpleChannel(analogRead(vry2));
    uint16_t yaw      = simpleChannel(analogRead(vrx1));


    // Switches
    armstate = (digitalRead(sw1) == LOW);
    digitalWrite(whiteled, armstate);
    uint16_t aux1 = armstate ? 2000 : 1000;
    uint16_t aux2 = (digitalRead(sw2) == LOW) ? 2000 : 1000;


    // Load channels into the library buffer (AETR mapped 0-3)
    tx.setChannel(0, roll);
    tx.setChannel(1, pitch);
    tx.setChannel(2, throttle);
    tx.setChannel(3, yaw);
    tx.setChannel(4, aux1); // Betaflight Arming switch
    tx.setChannel(5, aux2); // Extra switch


    // Transmit the packet
    tx.commit();
    lastSendTime = millis();
  }


  // 3. Update UI
  if (millis() - lastDisplayUpdate > 100) {
    updateOLED();
    lastDisplayUpdate = millis();
  }
}