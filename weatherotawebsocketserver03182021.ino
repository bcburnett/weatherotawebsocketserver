#include <Arduino.h>
#include "bcbaws.h"
#include "bcbsdcard.h"
#include "bcbbmx.h"
#include "time.h"
#include "WiFiCred1.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <BMx280I2C.h>
#include "State.h"

#define ARDUINO_RUNNING_CORE 1

// internal rtc variables
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -18000;
const int daylightOffset_sec = 3600;

bool wifiavail = false;

// localtime from internal rtc
String printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return ("");
  }
  return (asctime(&timeinfo));
}

String printLocalHour() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return ("");
  }
  return (String(asctime(&timeinfo)).substring(11, 16) + " ");
}

// define functions
void UpdateClients(void *pvParameters); // maintains the websocket display
void UpdateDatabase(void *pvParameters); // maintains the websocket display
void MeasureData(void *pvParameters); // maintains the websocket display
void initWiFi();
void initTime();

void setup() {
  // start the serial interface
  Serial.begin(115200);
  delay(500);
  Wire.begin();
  initBmx280();
  initWiFi();
  initTime();
  initWebServer();
  initWebSocket();
  initSDCard();
  checkForIndex();
  state.reload = true;

  xTaskCreatePinnedToCore(UpdateClients, "updateClients" // A name just for humans
                          ,
                          4096 // This stack size can be checked & adjusted by
                          // reading the Stack Highwater
                          ,
                          NULL, 2 // Priority, with 3 (configMAX_PRIORITIES - 1)
                          // being the highest, and 0 being the lowest.
                          ,
                          NULL, ARDUINO_RUNNING_CORE);

  xTaskCreatePinnedToCore(UpdateDatabase, "updateDatabase" // A name just for humans
                          ,
                          4096 // This stack size can be checked & adjusted by
                          // reading the Stack Highwater
                          ,
                          NULL, 2 // Priority, with 3 (configMAX_PRIORITIES - 1)
                          // being the highest, and 0 being the lowest.
                          ,
                          NULL, ARDUINO_RUNNING_CORE);
                          
  xTaskCreatePinnedToCore(MeasureData, "measureData" // A name just for humans
                          ,
                          4096 // This stack size can be checked & adjusted by
                          // reading the Stack Highwater
                          ,
                          NULL, 2 // Priority, with 3 (configMAX_PRIORITIES - 1)
                          // being the highest, and 0 being the lowest.
                          ,
                          NULL, ARDUINO_RUNNING_CORE);

  ArduinoOTA
  .onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS
    //SPIFFS.end();
 // Disable client connections    
    ws.enable(false);

    // Advertise connected clients what's going on
    ws.textAll("OTA Update Started");

    // Close them
    ws.closeAll();
    state.ota = true;

    
    Serial.println("Start updating " + type);
  })
  .onEnd([]() {
    Serial.println("\nEnd");
  })
  .onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  })
  .onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
      Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR)
      Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR)
      Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR)
      Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR)
      Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  ArduinoOTA.handle(); // check if an update is available
  if (WiFi.status() != WL_CONNECTED) // check if wifi is still connected, if not, reconnect
    initWiFi();
  vTaskDelay(60);
}

void UpdateClients(void *pvParameters) { // handle websocket and oled displays
  (void)pvParameters;
  for (;;) {
    if(!state.ota) {notifyInitialClients(getJson(true));} // send state to the client as a json string
    vTaskDelay(30000);
  }
}

void UpdateDatabase(void *pvParameters) { // handle websocket and oled displays
  (void)pvParameters;
  for (;;) {
    if(!state.ota) {updateDB();} // send state to the mysql database
    vTaskDelay(60000);
  }
}

void MeasureData(void *pvParameters) { // handle websocket and oled displays
  (void)pvParameters;
  for (;;) {
    if(!state.ota) {doSensorMeasurement();} // send state to the mysql database
    vTaskDelay(30000);
  }
}

void initWiFi() {
  Serial.println("connecting to wifi");
  // connect to wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
  Serial.println("connecting to wifi failed");
  delay(500);
  initWiFi();
  }
  Serial.println("wifi connected");
  wifiavail = true;
}

void initTime() {
  // set the clock from ntp server
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();
}


//void initBmx280() {
//  if (!bmx280.begin())
//  {
//    Serial.println("begin() failed. check your BMx280 Interface and I2C Address.");
//    delay(1000);
//    initBmx280();
//  }
//
//  if (bmx280.isBME280())
//    Serial.println("sensor is a BME280");
//  else
//    Serial.println("sensor is a BMP280");
//
//  //reset sensor to default parameters.
//  bmx280.resetToDefaults();
//
//  //by default sensing is disabled and must be enabled by setting a non-zero
//  //oversampling setting.
//  //set an oversampling setting for pressure and temperature measurements.
//  bmx280.writeOversamplingPressure(BMx280MI::OSRS_P_x16);
//  bmx280.writeOversamplingTemperature(BMx280MI::OSRS_T_x16);
//
//  //if sensor is a BME280, set an oversampling setting for humidity measurements.
//  if (bmx280.isBME280())
//    bmx280.writeOversamplingHumidity(BMx280MI::OSRS_H_x16);
//}
//
//void doSensorMeasurement() {
//  //start a measurement
//  if (!bmx280.measure())
//    return ;
//
//  do
//  {
//    delay(100);
//  } while (!bmx280.hasValue());
//
//  state.pressure = bmx280.getPressure() * 0.0002953;
//  state.pressure64 = bmx280.getPressure64() * 0.0002953;
//  state.temp = bmx280.getTemperature() * 1.8 + 32;
//
//  if (bmx280.isBME280())
//    state.humidity = bmx280.getHumidity();
//
//}
