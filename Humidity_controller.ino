/*
Powerbank powered Humidity controller based on: Arduino Nano 33 BLE Sense and Grove Water Atomizer
*/

#include <ArduinoBLE.h>
#include <Arduino_HTS221.h>
#include "mbed.h"

#define RED 22    
#define BLUE 24 
#define GREEN 23     
#define LED_PWR 25
#define LED_BUILTIN 13

#define ATOMIZER 10

float old_temp = 0;
float old_hum = 0;

static unsigned int pinNumber = 9;

mbed::PwmOut pin( digitalPinToPinName( pinNumber ) );

BLEService environmentService("181A");  // Standard Environmental Sensing service

BLEIntCharacteristic tempCharacteristic("2A6E",  // Standard 16-bit Temperature characteristic
  BLERead | BLENotify); // Remote clients can read and get updates

BLEUnsignedIntCharacteristic humidCharacteristic("2A6F", // Unsigned 16-bit Humidity characteristic
  BLERead | BLENotify);

void setup() {

  if (!HTS.begin()) {   // Initialize HTS Sensor
    while (1);
  }
  if (!BLE.begin()) {   // Initialize BLE
    while (1);
  }

  // Intitialize the digital Pins as outputs
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);
  pinMode(LED_PWR, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(ATOMIZER, OUTPUT);
  digitalWrite(LED_PWR, LOW);
  digitalWrite(LED_BUILTIN, LOW);

  BLE.setLocalName("Humidity Controller");  // Set name for connection
  BLE.setAdvertisedService(environmentService); // Advertise environment service
  environmentService.addCharacteristic(tempCharacteristic); // Add temperature characteristic
  environmentService.addCharacteristic(humidCharacteristic); // Add humidity chararacteristic
  BLE.addService(environmentService); // Add environment service
  tempCharacteristic.setValue(0); // Set initial temperature value
  humidCharacteristic.setValue(0); // Set initial humidity value

  BLE.advertise();  // Start advertising
}

void loop() {
  // Run Fan to prevent Powerbank shutdown and provide ventilation
  // 180 seconds "pause" + 10 seconds run 
  for(int counter = 1;counter <= 11;counter++) {
  // 0.5 second Fan ON at
  run_fan( 1.0 );
  check_humidity(1);  // 1x500 ms = 500 ms
  // 14 second Fan OFF
  run_fan( 0.0 );
  check_humidity(28); // 28x500ms = 14 sec
  }

  // 10 second Fan ON
  run_fan( 1.0 );
  check_humidity(20); // 20x500ms = 10 sec
  // 14 second Fan OFF
  run_fan( 0.0 );
  check_humidity(28); // 28x500ms = 14 sec
}

void check_humidity(int s){
  for(int counter = 1;counter <= s;counter++) {
    // Read all the sensor values
    int temperature = (int) (HTS.readTemperature() * 100) + (int) (-4.0 * 100); // corrected by -4˚C
    unsigned int humidity = (unsigned int) (HTS.readHumidity() * 100);
    unsigned int humi = (humidity/100);
    // RGB LEDs are active on LOW
    if ( humi > 75 ) {
      digitalWrite(RED, HIGH);
      digitalWrite(GREEN, LOW);
      digitalWrite(BLUE, HIGH);
      digitalWrite(ATOMIZER, LOW);
    }
    if ( 75 >= humi && humi >= 60 ) {
      digitalWrite(RED, LOW);
      digitalWrite(GREEN, LOW);
      digitalWrite(BLUE, HIGH);
      digitalWrite(ATOMIZER, HIGH);
      run_fan( 1.0 );
    }
    if ( humi < 60 ) {
      digitalWrite(RED, LOW);
      digitalWrite(GREEN, HIGH);
      digitalWrite(BLUE, HIGH);
      digitalWrite(ATOMIZER, HIGH);
      run_fan( 1.0 );
    }
    BLEDevice central = BLE.central();  // Wait for a BLE central to connect
    if (central) {
      tempCharacteristic.writeValue(temperature);
      humidCharacteristic.writeValue(humidity);
    }
    delay(100);
    digitalWrite(RED, HIGH);
    digitalWrite(GREEN, HIGH);
    digitalWrite(BLUE, HIGH);
    delay(400);
  }
}

void run_fan(float speed) {
  pin.period( 0.00004 ); // (1 / 0.00004s) = 25kHZ
  pin.write( speed );
}
