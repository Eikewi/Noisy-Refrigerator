#include <ArduinoBLE.h>

const int PIR_PIN = 2;

BLEService motionService("180C");
BLEBoolCharacteristic motionChar("2A56", BLERead | BLENotify);

void setup() {
  Serial.begin(9600);
  //while (!Serial) delay(10);

  pinMode(PIR_PIN, INPUT_PULLDOWN);

  if (!BLE.begin()) {
    Serial.println("BLE start failed!");
    while (1);
  }

  BLE.setLocalName("NanoBewegung");
  BLE.setAdvertisedService(motionService);
  motionService.addCharacteristic(motionChar);
  BLE.addService(motionService);

  motionChar.writeValue(false);
  BLE.advertise();

  Serial.println("BLE ready, waiting for connection...");
  Serial.println("Warming up PIR sensor for 30 seconds...");
  delay(30000);
  Serial.println("Ready!");
}

void loop() {
  BLEDevice central = BLE.central();

  if (central) {
    Serial.print("Connected to: ");
    Serial.println(central.address());

    while (central.connected()) {
      int motion = digitalRead(PIR_PIN);

      if (motion == HIGH) {
        motionChar.writeValue(true);
        Serial.println("Motion detected -> sent");
        delay(1000);
      } else {
        motionChar.writeValue(false);
      }
    }

    Serial.println("Connection lost");
  }
}
