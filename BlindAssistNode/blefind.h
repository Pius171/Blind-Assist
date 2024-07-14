//documentation with docusaraus
#include <stdint.h>
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "FS.h"
#include "jsontonestrings.h"

#define DEBUG 0

#if DEBUG == 0  // chnage to one to enable debugging
#define debug(x) Serial.print(x)
#define debugln(x) Serial.println(x)
#define debugf(x, y) Serial.printf(x, y)

#else
#define debug(x)
#define debugln(x)
#endif


#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"



BLEScan *pBLEScan;
JsonDocument UUIDS;
JsonDocument config;


void Blescan(void *pvParameters);
bool isValidUUID(const String &uuid);
TaskHandle_t Blescan_task_handle;  // You can (don't have to) use this to be able to manipulate a task from somewhere else.

uint32_t scanTime = 1;  // In seconds
//String uuid="";


// fucntion declarations
String readFile(String path);
void save(const char *filename, JsonVariant json);
String uuid = "";
class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String value = pCharacteristic->getValue();
    if (value.length() > 0) {
      String data = "";
      debugln("*********");
      debug("New value: ");
      for (int i = 0; i < value.length(); i++) {
        data += value[i];
        // debug(value[i]);
      }
      // debugln(data);
      // debugln();
      // debugln("*********");



      // Merge the incoming JSON into the existing JSON

      if (data.startsWith("u") || data.startsWith("U")) {
        uuid += "-" + data.substring(1, data.length() - 1);
        // on the sender side the UUID is splited and sent in
        // chunks, the chunks are created by splitting the UUID
        //at its respective hyphens '-', but need to add it back
        // on the receiver end
        if (data.startsWith("U")) {
          debugln(uuid.substring(1));
          UUIDS["target_UUID"] = uuid.substring(1);
          uuid = "";
          serializeJsonPretty(UUIDS, Serial);
          Serial.flush();
          save("/UUIDS.txt", UUIDS);  // save UUIDS to file system
        }
        // Parse the incoming JSON string
        // JsonDocument incomingDoc;  // Adjust the size according to your JSON data
        // deserializeJson(incomingDoc, data.substring(1));
        // // a char is added to the begining of a string
        // //to tell apart when it is a UUID or config

        // for (JsonPair pair : incomingDoc.as<JsonObject>()) {

        //   UUIDS[pair.key()] = pair.value();
        //   debugln(pair.key().c_str());
        //   debugln(pair.value().as<String>());
        // }
        // serializeJsonPretty(UUIDS, Serial);
        // debugln("json deserialized");
        // save("/UUIDS.txt", UUIDS);  // save UUIDS to file system

      } else if (data.startsWith("N")) {  // all notes start with N
        JsonDocument the_tones;
        deserializeJson(the_tones, tones);                                 // load tones into json
        config["tone"] = the_tones[data.substring(0, data.length() - 1)];  // this is to remove a wierd character that arrives at
                                                                           //at the end of the message.
        serializeJsonPretty(config, Serial);
        save("/config.txt", config);
        debugln("config");
      }

      else if (data.startsWith("n")) {
        config["name"] = "Alert Node: " + data.substring(1, data.length() - 1);  // this is to remove a wierd character that arrives at
                                                                                 //at the end of the message.
        debug("device name");
        debugln(config["name"].as<String>());
        save("/config.txt", config);
        debugln("restarting device");
        ESP.restart();
      } else {

        int range = static_cast<int>(data[0]);  // int(buff);
        debugln(data[0]);
        debugln(range);
        config["range"] = range * -1;
        serializeJsonPretty(config, Serial);
        save("/config.txt", config);
        debugln("config");
      }
    }
  }
};

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    //debugf("Advertised Device: %s \n", advertisedDevice.toString().c_str());

    for (JsonPair pair : UUIDS.as<JsonObject>()) {
      const char *key = pair.key().c_str();
      const char *value = pair.value().as<const char *>();  // Adjust the type based on your data
      int maxRange = config["range"].as<int>();
      debug("max_range= ");
      debugln(maxRange);
      debug("rssi= ");
      debugln(advertisedDevice.getRSSI());
      if (advertisedDevice.getServiceUUID().equals(BLEUUID(value)) && advertisedDevice.getRSSI() >= maxRange) {

        ///make
        debugln("he is close by");

        debugln(config["tone"].as<int>());
        tone(9, config["tone"].as<int>());
        delay(200);


      }

      else {
        noTone(9);
      }
    }
  }
};

void inti_fs() {
  if (!LittleFS.begin(true)) {
    debugln("LittleFS Mount Failed");
    return;
  }

  if (LittleFS.exists("/UUIDS.txt")) {
    // if file containing registered
    // UUIDs exist initiate scanning
    String input = readFile("/UUIDS.txt");
    deserializeJson(UUIDS, input);
  } else {
    debugln("No UUIDS stored. Connect to the device");
  }

  if (LittleFS.exists("/config.txt")) {
    // if file containing registered
    // UUIDs exist initiate scanning
    String input = readFile("/config.txt");
    deserializeJson(config, input);
    serializeJsonPretty(config, Serial);
  } else {
    debugln("No config found using default config");
    String input = "{\"tone\": 4978,\n"
                   "\"range\": -62,\n"
                   "\"name\": \"Blind Assist Alert Node\"\n"
                   "}";
    deserializeJson(config, input);
  }
}

void init_blefind() {
  BLEDevice::init(config["name"].as<String>());

  //BLEDevice::init();
  BLEDevice::setMTU(517);
  debugln("Scanning...");

  // scanning
  pBLEScan = BLEDevice::getScan();  // create new scan
  //pBLEScan->
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);  // active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value


  // advertsing
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->setValue("Door is ready");
  //pCharacteristic->

  pService->start();
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  //pAdvertising->setScanFilter(bool scanRequestWhitelistOnly, bool connectWhitelistOnly)

  BLEDevice::startAdvertising();
  debugln("Characteristic defined! Now you can read it in your phone!");
}

void start_scan() {

  BLEScanResults *foundDevices = pBLEScan->start(scanTime, false);

  Serial.print("Devices found: ");
  Serial.println(foundDevices->getCount());
  Serial.println("Scan done!");
  pBLEScan->clearResults();  // delete results fromBLEScan buffer to release memory
                             // delay(2000);
}
String readFile(String path) {
  String DoC = "";
  debug("Reading file: %s\n");
  debugln(path);

  File file = LittleFS.open(path, "r");
  if (!file) {
    debugln("Failed to open file for reading");
    return DoC;
  }

  debug("Read from file: ");
  while (file.available()) {

    DoC += file.readString();
    debug(DoC);
    // delay(500);
  }
  return DoC;
}

void save(const char *filename, JsonVariant json) {


  Serial.printf("Writing file: %s\n", filename);

  File file = LittleFS.open(filename, "w");
  Serial.printf("opening file: %s\n", filename);

  // size_t n = serializeJson(json, file);  // stores the number of characters serialized
  String output;
  serializeJson(json, output);

  if (!file) {
    Serial.printf("Failed to open %s for writing\n",filename);
    return;
  }
  if (file.print(output)) {
    Serial.printf("%s saved\n",filename);
  } else {
    Serial.printf("failed to save %s\n",filename);
  }
  file.close();
}


