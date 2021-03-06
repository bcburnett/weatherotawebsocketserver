#include <SPI.h>
#include "FS.h"
#include "SD.h"
#include "ArduinoJson.h"
#include "State.h"

#ifndef BCBSD
#define BCBSD

//default html page if not in SPIFFS

// beginning of default web page
const char htmlCode[]PROGMEM = R"rawliteral(
 <!DOCTYPE HTML>
<html>

<head>
  <! define meta data>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <! define the style CSS of your page>
      <style>
        html {
          font-family: Arial;
          display: inline-block;
          text-align: center;
        }

        h1 {
          font-size: 2.9rem;
        }

        h2 {
          font-size: 2.1rem;
        }

        p {
          font-size: 1.9rem;
        }

        body {
          margin:auto;
          padding-bottom: 30px;
          text-align: center;
        }

      </style>
</head>

<body>

  <div style="width: 75%; margin:auto; text-align:center;"></div>
  <h2>ESP32 Weather Station <span id="time"></span></h2>


    <div id="openweathermap-widget-11" style="width:fit-content; margin: auto; text-align:center;"></div>
    <script src='http://openweathermap.org/themes/openweathermap/assets/vendor/owm/js/d3.min.js'></script>
    <script>window.myWidgetParam ? window.myWidgetParam : window.myWidgetParam = [];
      window.myWidgetParam.push({ id: 11, cityid: '4839420', appid: '2e47deaedc70f784b3aabcab50a2e950', units: 'imperial', containerid: 'openweathermap-widget-11', });
      (function () {
        var script = document.createElement('script');
        script.async = true;
        script.charset = "utf-8";
        script.src = "http://openweathermap.org/themes/openweathermap/assets/vendor/owm/js/weather-widget-generator.js";
        var s = document.getElementsByTagName('script')[0];
        s.parentNode.insertBefore(script, s);
      })();</script>

    <hr>
    <p>Temperature: <span id="temperature"></span></p>
    <p>Barometric Pressure: <span id="pressure"></span></p>
    <p>Humidity: <span id="humidity"></span></p>
    <p>Dew Point: <span id="dewpoint"></span></p>

    <input type='file' name='upload' id='upload' value=''><span id="percent"></span>
  </div>
  <script>
    var gateway = `ws://${window.location.hostname}/ws`;
    var websocket;
    var state = {};
    var json = {};

    window.addEventListener('load', onLoad);

    function initWebSocket() {
      console.log('Trying to open a WebSocket connection...');
      websocket = new WebSocket(gateway);
      websocket.onopen = onOpen;
      websocket.onclose = onClose;
      websocket.onmessage = onMessage;
    }

    function onOpen(event) {
      console.log('Connection opened');
    }

    function onClose(event) {
      console.log('Connection closed');
      setTimeout(initWebSocket, 2000);
    }

    function toFixed(num, fixed) {
      var re = new RegExp('^-?\\d+(?:\.\\d{0,' + (fixed || -1) + '})?');
      return num.toString().match(re)[0];
    }

    function onMessage(event) {
      json = JSON.parse(event.data);
      console.log(json);
      document.getElementById("temperature").innerHTML = toFixed(json.temperature, 2);
      document.getElementById("pressure").innerHTML = toFixed(json.pressure, 2);
      document.getElementById("humidity").innerHTML = toFixed(json.humidity, 2);
      document.getElementById("dewpoint").innerHTML = toFixed(json.dew, 2);
      if (json.reload) location.reload();
    }

    // on page load
    function onLoad(event) {
      initWebSocket();
      document.getElementById("upload").addEventListener("change", (e) => processFile(e));
    }

    function processFile(e) {
      const reader = new FileReader();
      reader.readAsText(e.path[0].files[0]);
      var myfilename = e.path[0].files[0].name;

      reader.onload = (e) => {
        var myfile = e.target.result;
        var myarray = myfile.split('');
        var mylength = myfile.length;
        i = 0;
        k = 1;
        var sendArray = [];
        sendArray[0] = ("upld:" + myfilename);

        function getChunk() {
          var sendstring = '';
          for (var j = 0; j < 512; j++) {
            if (i < mylength) {
              sendstring += myarray[i];
            }
            i++
          }
          return sendstring;
        }

        function loopThroughSplittedText(splittedText) {
          splittedText.forEach(function (text, i) {
            setTimeout(function () {
              sendMessage(text, i);
            }, i * 500)
          })
        }

        while (i < mylength) {

          sendArray[k] = ("file:" + getChunk());
          k++;
        }
        sendArray[k] = ("comp");
        sendstring = '';
        loopThroughSplittedText(sendArray);
      };
    }

    function sendMessage(data, index) {
      if (index) var percent = index / k * 100;
      if (data.substring(0, 4) == 'file') document.getElementById('percent').innerHTML = percent + ' %';
      if (data.substring(0, 4) == 'comp') {
        document.getElementById('percent').innerHTML = "completed";
        setTimeout(() => {
          sendMessage("reload");
        }, 2000);
      }
      websocket.send(data);
    }
      function doTime() {
        document.getElementById('time').innerHTML = new Date().toLocaleString();
      }
      setInterval(doTime, 1000);

  </script>
</body>

</html>
)rawliteral";

// end of default web page

// variables
xSemaphoreHandle semFile = xSemaphoreCreateMutex(); // file operation lock
void initSDCard() {
  Serial.print("Initializing SD card...");

  if (!SD.begin(5)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");

}

void appendFile(fs::FS &fs, const char * path, const char * message) {

  if (xSemaphoreTake(semFile, 500)) {
    File file = fs.open(path, FILE_APPEND);

    if (!file) {
      return;
    }
    file.print(message);
    file.close();

    xSemaphoreGive(semFile);
  }
}

void writeFile(fs::FS &fs, const char * path, const char * message) {

  if (xSemaphoreTake(semFile, 500)) {
    File file = fs.open(path, FILE_WRITE);

    if (!file) {
      return;
    }
    file.print(message);
    file.close();

    xSemaphoreGive(semFile);
  }
}
String readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\r\n", path);

    File file = fs.open(path);
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
        return "";
    }
    String result;
    Serial.println("- read from file:");
    while(file.available()){
        result += String(char(file.read()));
    }
    file.close();
    return result;
}

void renameFile(fs::FS &fs, const char * path1, const char * path2) {
  if (xSemaphoreTake(semFile, 500)) {
    fs.rename(path1, path2);
    xSemaphoreGive(semFile);
  }
}

void deleteFile(fs::FS &fs, const char * path) {
  if (xSemaphoreTake(semFile, 500)) {
    fs.remove(path);
    xSemaphoreGive(semFile);
  }
}

// write the default index.htm to SPIFFS
//  check if index exists and only update if it doesn't
void checkForIndex(){
  if(SD.exists("/data.json")) {
    state.json = readFile(SD, "/data.json");
    Serial.println(state.json);
  }
  if(SD.exists("/index.htm")) return; // 
  deleteFile(SD,"/index.htm");
  delay(500);
  File file = SD.open("/index.htm", FILE_WRITE);
    file.print(htmlCode);
    file.close();
    }

#endif
