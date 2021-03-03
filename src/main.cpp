#include <Arduino.h>

// Stuff for the WiFi manager 
#include "LittleFS.h"
#include "WiFiManager.h"
#include "webServer.h"
#include "updater.h"
#include "fetch.h"
#include "configManager.h"
#include "timeSync.h"
#include <TZ.h>
#include "ESP8266HTTPClient.h"

// Stuff for BME280 
#include <Wire.h>  
#include <Adafruit_BME280.h>
Adafruit_BME280 bme; // use I2C interface
Adafruit_Sensor *bme_temp = bme.getTemperatureSensor();
Adafruit_Sensor *bme_pressure = bme.getPressureSensor();
Adafruit_Sensor *bme_humidity = bme.getHumiditySensor();


// LDR input pin (ESP8266 only got one ADC on A0)
#define LDR A0
int LDRvalue = 0;

// PIR input
#define PIR D4  // input pin
static unsigned long pirTrippedTime = 0 ; // last time in millis() the PIR got tripped by motion
boolean pirTripped = false; // is motion detected? true/false


// Stuff for Splunk
String eventData="";

// variable for timing
static unsigned long msTickSplunk = 0;

void splunkpost(String collectorToken,String PostData, String splunkindexer)
{
  String payload = PostData;

  //Build the request
  HTTPClient http;
  String splunkurl="http://"+ splunkindexer +"/services/collector"; //removed :8088 due to DOCKER container port redirect.. port now lives in the splunkindexer variable
  String tokenValue="Splunk " + collectorToken;
  
  // fire at will!! 
  http.begin(splunkurl);
  http.addHeader("Content-Type", "application/json");
  //Serial.println(tokenValue);
  http.addHeader("Authorization", tokenValue);

  Serial.print("splunking: ");
  Serial.print(payload);

  String contentlength = String(payload.length());
  http.addHeader("Content-Length", contentlength );
  http.POST(payload);
  http.writeToStream(&Serial);
  Serial.println();
  http.end();
}


void checkPir(){ 
  // function need to get smarter
  // see the transition from HIGH/LOW LOW/HIGH
  // get the time in UTC instead of millis()
  // if hte splunk send intervall is high.. this only sends the state just before the measurement..
  // if intervall = 10m and PIR is triggered @2m but then nothong anymore.. we would report nothing..
  pirTripped = digitalRead(PIR);
  if (pirTripped) pirTrippedTime = millis();
}

void setup()
{
  Serial.begin(115200);

  LittleFS.begin();
  GUI.begin();
  configManager.begin();
  WiFiManager.begin(configManager.data.projectName);

  //Set the timezone
  timeSync.begin(configManager.data.sensorTimezone);


  //Wait for connection
  timeSync.waitForSyncResult(10000);

  if (timeSync.isSynced())
  {
    time_t now = time(nullptr);
    Serial.print(PSTR("Current local time: "));
    Serial.print(asctime(localtime(&now)));
    Serial.print(" - UTC: ");
    Serial.print(asctime(gmtime(&now)));

    struct tm * timeinfo;
    char timeStringBuff[50]; //50 chars should be enough
        
    time (&now);
    timeinfo = localtime(&now);
        
    strftime(timeStringBuff, sizeof(timeStringBuff), "%Y.%m.%d %H:%M:%S", timeinfo);
    //print like "const char*"
    Serial.println(timeStringBuff);


  }
  else 
  {
    Serial.println("Timeout while receiving the time");
  }

  pinMode(D8, OUTPUT);
  digitalWrite(D8, HIGH);
  pinMode(LDR, INPUT);
  pinMode(PIR, INPUT);


  // BME280
  if (!bme.begin(0x76)) {
    Serial.println(F("Could not find a valid BME280 sensor, check wiring!"));
    while (1) delay(10);
  }
  
  bme_temp->printSensorDetails();
  bme_pressure->printSensorDetails();
  bme_humidity->printSensorDetails();
  // BME280 END


  // sending restart event to splunk
  eventData = "{"
              "\"host\": \"" + String(configManager.data.clientName) + "\", \"sourcetype\": \"diySensor\", \"index\": \"esp8266hec\", " 
              "\"fields\" : {\"IP\" : \"" + String(WiFi.localIP().toString()) + "\" }, "
              "\"event\"  : {\"status\" : \"" + "restarted" + "\"}"
              "}";
  splunkpost(configManager.data.collectorToken, eventData, configManager.data.splunkindexer); 
  Serial.println(eventData);

}

void loop()
{
  //software interrupts
  WiFiManager.loop();
  updater.loop();    

  // toggle LED every second
  digitalWrite(D8, (millis() / 1000) % 2); // doesnt even need a timer ... can be placed just inside the loop

  if (millis() - msTickSplunk > configManager.data.updateSpeed){
    msTickSplunk = millis();

    // Wait and print the time
    time_t now = time(nullptr);
    Serial.print(PSTR("Current local time: "));
    Serial.print(asctime(localtime(&now)));   
    Serial.print(" - UTC: ");
    Serial.println(asctime(gmtime(&now)));

    // getting the time as printout to send t to splunk for maybe indexing 
            struct tm * timeinfo;
            char timeStringBuff[50]; //50 chars should be enough
             
            time (&now);
            timeinfo = localtime(&now);
              
            strftime(timeStringBuff, sizeof(timeStringBuff), "%Y.%m.%d %H:%M:%S", timeinfo);
            //print like "const char*"
            Serial.println(timeStringBuff);

    // Read the light intensity
    LDRvalue=0; // reset
    for(int i=0; i<4; i++){
      LDRvalue = LDRvalue + analogRead(LDR); 
      delay(10);
    }
    LDRvalue = map(LDRvalue / 4, 0,1024,0,100);
      
    // PIR
    // maybe put it outside the loop and trigger splunking it on change
    checkPir();

    // BME280
    sensors_event_t temp_event, pressure_event, humidity_event;
    bme_temp->getEvent(&temp_event);
    bme_pressure->getEvent(&pressure_event);
    bme_humidity->getEvent(&humidity_event);
    //Serial.print(temp_event.temperature);
    //Serial.print(humidity_event.relative_humidity);
    //Serial.print(pressure_event.pressure);
    // BME280 END

    // SPLUNK NOW
    eventData = "{ \"host\": \"" + String(configManager.data.clientName) + "\", \"sourcetype\": \"diySensor\", \"index\": \"esp8266hec\", " 
                  "\"fields\" : {"
                                "\"IP\" : \"" + String(WiFi.localIP().toString()) + "\" , "
                                "\"interval\" : \"" + String(configManager.data.updateSpeed/1000) + "\" "
                  "}, "
                  "\"event\"  : {"
                                "\"PIR_State\": \"" + pirTripped + "\" , "
                                "\"lightIndex\": \"" + LDRvalue + "\" , "
                                "\"BME280_Temp\": \"" + temp_event.temperature + "\" , "
                                "\"BME280_Pressure\": \"" + pressure_event.pressure + "\" , "
                                "\"BME280_Humidity\": \"" + humidity_event.relative_humidity + "\" , "
                                "\"uptime\": \"" + millis()/1000 + "\" "
                  "}"
                "}";
    //send off the data
    splunkpost(configManager.data.collectorToken, eventData, configManager.data.splunkindexer); 
  }
}