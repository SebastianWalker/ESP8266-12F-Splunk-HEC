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

// for the I2C sensors
#include <Wire.h>  
// Stuff for BME280 
#include <Adafruit_BME280.h>
boolean bmeErr = true; // set to false once wire.begin is successfull 
Adafruit_BME280 bme; // use I2C interface
Adafruit_Sensor *bme_temp = bme.getTemperatureSensor();
Adafruit_Sensor *bme_pressure = bme.getPressureSensor();
Adafruit_Sensor *bme_humidity = bme.getHumiditySensor();
// Stuff for MAX44009
#include <Max44009.h>
Max44009 luxSensor(0x4A);
boolean luxErr = true; // set to false once connected in setup()

// LDR input pin (ESP8266 only got one ADC on A0)
#define LDR A0
int LDRvalue = 0;

// HC-SR501 PIR
#define PIR D4  // input pin
#define PIR_LED D5 // output pin for PIR active
static unsigned long pirTrippedTime = 0 ; // last time in millis() the PIR got tripped by motion
boolean pirTripped = false; // is motion detected? true/false

// HC-SR04 ultra sonic distance sensor
#include <Ultrasonic.h>
#define HCSR04_ECHO D6
#define HCSR04_TRIG D7
Ultrasonic HCSR04(HCSR04_TRIG, HCSR04_ECHO);

// Stuff for Splunk
String eventData="";
String hecMessage="";

// variable for timing
static unsigned long msTickSplunk = 0;

String getLocaltime(){
  // getting the time as printout to send t to splunk for maybe indexing 
  time_t now = time(nullptr);
  struct tm * timeinfo;
  char timeStringBuff[50]; //50 chars should be enough
    
  time (&now);
  timeinfo = localtime(&now);
    
  strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%d %H:%M:%S", timeinfo);
  //print like "const char*"
  //Serial.println(timeStringBuff);
  return timeStringBuff;
}
String getUTC(){
  // getting the time as printout to send t to splunk for maybe indexing 
  time_t now = time(nullptr);
  struct tm * timeinfo;
  char timeStringBuff[50]; //50 chars should be enough
    
  time (&now);
  timeinfo = gmtime(&now);
    
  strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%d %H:%M:%S", timeinfo);
  //print like "const char*"
  //Serial.println(timeStringBuff);
  return timeStringBuff;
}
long getEpoch(){
  // getting the time as printout to send t to splunk for maybe indexing 
  time_t now = time(nullptr);
  
  return now;
}

void splunkpost(String PostData){
  hecMessage = "{ \"host\": \"" + String(configManager.data.clientName) + "\", " 
                 "\"sourcetype\": \"" + String(configManager.data.sourcetype) + "\", " 
                 "\"index\": \"" + String(configManager.data.index) + "\", " 
                 "\"time\" : \"" + String(getEpoch()) + "\" , "
                 "\"fields\" : {"
                                "\"IP\" : \"" + String(WiFi.localIP().toString()) + "\" , "
                                "\"UTC\" : \"" + String(getUTC()) + "\" , "
                                "\"Localtime\" : \"" + String(getLocaltime()) + "\" , "
                                "\"interval\" : \"" + String(configManager.data.updateSpeed/1000) + "\" "
                  "}, "
                  "\"event\"  : {" + PostData + "}"
               "}";
  
  
  String payload = hecMessage;

  //Build the request
  WiFiClient client; // just to avoid deprecation error on http.begin(url)
  HTTPClient http;
  String splunkurl="http://"+ String(configManager.data.splunkindexer) +"/services/collector"; //removed :8088 due to DOCKER container port redirect.. port now lives in the splunkindexer variable
  String tokenValue="Splunk " + String(configManager.data.collectorToken);
  
  // fire at will!! 
  http.begin(client, splunkurl); // changed for deprected http.begin(url)
  //http.begin(splunkurl);
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
  // if intervall = 10m and PIR is triggered @2m but then nothing anymore.. we would report nothing..
  pirTripped = digitalRead(PIR);
  digitalWrite(PIR_LED, pirTripped);
  if (pirTripped) pirTrippedTime = millis();
}

// Software restart 
void(* resetFunc) (void) = 0; //declare reset function @ address 0
void forceRestart(){
  // save false to config data.. else it would keep restarting...
  configManager.data.forceRestart = false;
  configManager.save();
  splunkpost("\"status\" : \"INFO\", \"msg\" : \"Executing Order 66\""); 
  resetFunc();
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
    //Serial.println("Timeout while receiving the time");
    splunkpost("\"status\" : \"ERROR\", \"msg\" : \"No time sync available.\""); 
  }

  // set input/output pins
  pinMode(D8, OUTPUT); // heartbeat LED
  digitalWrite(D8, HIGH);
  pinMode(D5, OUTPUT); // PIR active LED
  digitalWrite(D5, LOW);

  pinMode(LDR, INPUT);
  pinMode(PIR, INPUT);


  // BME280
  if (!bme.begin(0x76)){
    splunkpost("\"status\" : \"ERROR\", \"msg\" : \"BME280 not found.\""); 
    // set an error LED high.. 
  }
  else{
    bmeErr = false;
    //bme_temp->printSensorDetails(); bme_pressure->printSensorDetails(); bme_humidity->printSensorDetails();   
  }
  // BME280 END

  // MAX44009 lux sensor
  if (!luxSensor.isConnected()){
    splunkpost("\"status\" : \"ERROR\", \"msg\" : \"MAX44009 not found.\""); 
    // set error LED high..
  }else{
    luxSensor.setAutomaticMode();
    luxErr = false;
  }
  // MAX44009 lux sensor END

  splunkpost("\"status\" : \"INFO\", \"msg\" : \"restarted\""); 
}

void loop()
{
  //software interrupts
  WiFiManager.loop();
  updater.loop();    

  // restart over web interface...
  if(configManager.data.forceRestart){forceRestart();}

  // toggle LED every second
  digitalWrite(D8, (millis() / 1000) % 2); // doesnt even need a timer ... can be placed just inside the loop

  // read PIR state
  checkPir();

  if (millis() - msTickSplunk > configManager.data.updateSpeed){
    msTickSplunk = millis();

    // Read the light intensity
    LDRvalue=0; // reset
    for(int i=0; i<4; i++){
      LDRvalue = LDRvalue + analogRead(LDR); 
      delay(10);
    }
    LDRvalue = map(LDRvalue / 4, 0,1024,0,100);
  

    // PIR
    // maybe put it outside the loop and trigger splunking it on change
    //checkPir();
    // if last trigger time is inside the last update intervall then log motion as true, else the checkPIR function has reset the pirTripped to 0 anyways
    if (pirTrippedTime >= msTickSplunk-configManager.data.updateSpeed){pirTripped = 1;}

    // BME280
    sensors_event_t temp_event, pressure_event, humidity_event;
    bme_temp->getEvent(&temp_event);
    bme_pressure->getEvent(&pressure_event);
    bme_humidity->getEvent(&humidity_event);
    // BME280 END

    // only report these sensors if they are up and running...
    String BME280_data = bmeErr ? "" :  "\"BME280_Temp\": \"" + String(temp_event.temperature) + "\" , "
                                        "\"BME280_Pressure\": \"" + String(pressure_event.pressure) + "\" , "
                                        "\"BME280_Humidity\": \"" + String(humidity_event.relative_humidity) + "\" , ";

    String MAX44009_data = luxErr ? "" :  "\"MAX44009_lux\": \"" + String(luxSensor.getLux()) + "\" , "
                                          "\"MAX44009_IntegrationTime\": \"" + String(luxSensor.getIntegrationTime()/1000.0) + "\" , ";

    //"\"BME280_Temp\": \"" + String(temp_event.temperature) + "\" , "
    // "\"BME280_Pressure\": \"" + String(pressure_event.pressure) + "\" , "
    // "\"BME280_Humidity\": \"" + String(humidity_event.relative_humidity) + "\" , "
    // "\"MAX44009_lux\": \"" + String(luxSensor.getLux()) + "\" , "
    // "\"MAX44009_IntegrationTime\": \"" + String(luxSensor.getIntegrationTime()/1000.0) + "\" , "
    eventData =   "\"PIR_State\": \"" + String(pirTripped) + "\" , "
                  "\"lightIndex\": \"" + String(LDRvalue) + "\" , "
                  + BME280_data 
                  + MAX44009_data + 
                  "\"HCSR04_Distance\": \"" + String(HCSR04.read()) + "\" , "
                  "\"uptime\": \"" + String(millis()/1000) + "\" ";

    //send off the data
    splunkpost(eventData);
  }

}