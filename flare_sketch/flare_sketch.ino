// Modified by Amadej Tratnik 2021
// Wifi provisioning via BLE
//
// Adapted by Beth Kimber to make a paired set of friendship lights
// When one is activated, the other lights up.
//
// Instructables Internet of Things Class sample code
// Circuit Triggers Internet Action
// A button press is detected and stored in a feed
// An LED is used as confirmation feedback
//
// Modified by Becky Stern 2017
// based on the Adafruit IO Digital Input Example
// Tutorial Link: https://learn.adafruit.com/adafruit-io-basics-digital-input
//
// Adafruit invests time and resources providing this open source code.
// Please support Adafruit and open source hardware by purchasing
// products from Adafruit!
//
// Written by Todd Treece for Adafruit Industries
// Copyright (c) 2016 Adafruit Industries
// Licensed under the MIT license.

/************************ Adafruit IO Configuration *******************************/
#include "secrets.h"

#include <WiFi.h>
#include <AdafruitIO.h>
#include <Preferences.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include "AdafruitIO_WiFi.h"

#define LED_PIN1 13
#define LED_PIN2 12
#define LED_PIN3 27
#define LED_PIN4 33
#define LED_PIN5 15
#define LED_PIN6 32

Preferences preferences;

AdafruitIO_Feed *command = nullptr;
String wifiSSID = "";
String wifiPASS = "";
AdafruitIO_WiFi *io = nullptr;
String deviceRole = "";

long messageReceivedTime = -1000000;

BLECharacteristic *ssidCharacteristic;
BLECharacteristic *passCharacteristic;
BLECharacteristic *roleCharacteristic;
bool SSIDRecevied = false;
bool PASSReceived = false;
bool ROLERecevied = false;

bool connected = false;
int buttonPin;

bool animating = false;
int animationStep = 0;
unsigned long animationStartTime = 0;

class SSIDCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    wifiSSID = pCharacteristic->getValue();
    Serial.println("Received SSID: " + wifiSSID);
    SSIDRecevied = true;
  }
};

class PASSCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    wifiPASS = pCharacteristic->getValue();
    Serial.println("Received PASS: " + wifiPASS);
    PASSReceived = true;
  }
};

class ROLECallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    deviceRole = pCharacteristic->getValue();
    Serial.println("Received ROLE: " + deviceRole);
    ROLERecevied = true;
  }
};

bool connectToWiFi(const char* ssid, const char* pass) {
  WiFi.begin(ssid, pass);
  Serial.print("Connecting to WiFi");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(500);
    Serial.print(".");
  }
  connected = WiFi.status() == WL_CONNECTED;

  if (connected) {
    Serial.println("Wifi connected!");
    digitalWrite(LED_PIN1, HIGH); delay(200);
    digitalWrite(LED_PIN1, LOW);
  } else {
    Serial.println("Could not connect to Wifi");
  }
  return connected;
}

void startBLEProvisioning() {
  BLEDevice::init("FLARE");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *service = pServer->createService("ffb0");

  ssidCharacteristic = service->createCharacteristic("ffb1", BLECharacteristic::PROPERTY_WRITE);
  passCharacteristic = service->createCharacteristic("ffb2", BLECharacteristic::PROPERTY_WRITE);
  roleCharacteristic = service->createCharacteristic("ffb3", BLECharacteristic::PROPERTY_WRITE);

  ssidCharacteristic->setCallbacks(new SSIDCallbacks());
  roleCharacteristic->setCallbacks(new ROLECallbacks());
  passCharacteristic->setCallbacks(new PASSCallbacks());

  service->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->start();

  Serial.println("BLE Provisioning Started. Waiting for credentials...");
}

void saveCredentialsAndRestart() {
  preferences.putString("ssid", wifiSSID);
  preferences.putString("role", deviceRole);
  preferences.putString("pass", wifiPASS);
  Serial.println("Credentials saved! Restarting...");
  delay(500);
  ESP.restart();
}

void startAdafruitIO() {
  io = new AdafruitIO_WiFi(IO_USERNAME, IO_KEY, wifiSSID.c_str(), wifiPASS.c_str());
  command = io->feed("command");
  command->onMessage(handleMessage);
  io->connect(); 
}

void handleMessage(AdafruitIO_Data *data) {
  int incoming = data->toInt();
  Serial.print("Received <- ");
  Serial.println(incoming);

  if (deviceRole.toInt() != incoming) {
    const int ledPins[] = {LED_PIN1, LED_PIN2, LED_PIN3, LED_PIN4, LED_PIN5, LED_PIN6};
    const int numLeds = sizeof(ledPins) / sizeof(ledPins[0]);
    int delayPerLed = 600 / numLeds;

    for (int i = 0; i < numLeds; i++) {
      digitalWrite(ledPins[i], HIGH);
      delay(delayPerLed);
    }
    delay(1000);

    for (int i = 0; i < numLeds; i++) {
      digitalWrite(ledPins[i], LOW);
      delay(delayPerLed);
    }
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN1, OUTPUT);
  pinMode(LED_PIN2, OUTPUT);
  pinMode(LED_PIN3, OUTPUT);
  pinMode(LED_PIN4, OUTPUT);
  pinMode(LED_PIN5, OUTPUT);
  pinMode(LED_PIN6, OUTPUT);

  preferences.begin("wifiCreds", false);
  wifiSSID = preferences.getString("ssid", "");
  wifiPASS = preferences.getString("pass", "");
  deviceRole = preferences.getString("role", "");
  //preferences.clear(); // debug code, to reset the preferences

  buttonPin = (deviceRole == "1") ? 15 : 14; //For this specific case, I put the buttons on the different pins by mistake. You can #define the BUTTON_PIN and delete this code to make it scalable for many devices.
  pinMode(buttonPin, INPUT_PULLUP);

  if (wifiSSID == "") {
    startBLEProvisioning();
  } else if (connectToWiFi(wifiSSID.c_str(), wifiPASS.c_str())) {
    startAdafruitIO();
  } else {
    preferences.clear();
    ESP.restart();     
  }
}

void loop() {
  if (SSIDRecevied && PASSReceived && ROLERecevied) {
    saveCredentialsAndRestart();  
    return;
  }

  if (io != nullptr) io->run();


  static bool buttonPressed = false;
  static int lastButtonState = LOW;
  static unsigned long lastDebounceTime = 0;
  const unsigned long debounceDelay = 50;

  int reading = digitalRead(buttonPin);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading == LOW && !buttonPressed) {
      buttonPressed = true;
      int valueToSend = deviceRole.toInt();
      Serial.print("Sending button -> ");
      Serial.println(valueToSend);
      if (command) command->save(valueToSend);
      digitalWrite(LED_PIN1, HIGH);
      digitalWrite(LED_PIN2, HIGH);
      digitalWrite(LED_PIN3, HIGH);
      digitalWrite(LED_PIN4, HIGH);
      digitalWrite(LED_PIN5, HIGH);
      digitalWrite(LED_PIN6, HIGH);
    }
    if (reading == HIGH && buttonPressed) {
      buttonPressed = false;
      digitalWrite(LED_PIN1, LOW);
      digitalWrite(LED_PIN2, LOW);
      digitalWrite(LED_PIN3, LOW);
      digitalWrite(LED_PIN4, LOW);
      digitalWrite(LED_PIN5, LOW);
      digitalWrite(LED_PIN6, LOW);
    }
  }
  lastButtonState = reading;
}