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

// I2C sensors
#include <Wire.h>  
// BME280 
#include <Adafruit_BME280.h>
boolean bmeErr = true; // set to false once wire.begin is successfull 
Adafruit_BME280 bme; // use I2C interface
Adafruit_Sensor *bme_temp = bme.getTemperatureSensor();
Adafruit_Sensor *bme_pressure = bme.getPressureSensor();
Adafruit_Sensor *bme_humidity = bme.getHumiditySensor();
// MAX44009
#include <Max44009.h>
Max44009 luxSensor(0x4A);
boolean luxErr = true; // set to false once connected in setup()

// LDR input pin (ESP8266 only got one ADC on A0)
#define LDR A0
int LDRvalue = 0;

// HC-SR501 PIR
#define PIR D5  // input pin

static unsigned long pirTrippedTime = 0 ; // last time in millis() the PIR got tripped by motion
boolean pirTripped = false; // is motion detected? true/false

// HC-SR04 ultra sonic distance sensor
#include <Ultrasonic.h>
#define HCSR04_ECHO D6
#define HCSR04_TRIG D7
Ultrasonic HCSR04(HCSR04_TRIG, HCSR04_ECHO);

// LEDs
#define Heartbeat_LED D4 // D4 = GPIO2 = onchip LED
#define Splunking_LED D8 // output pin for measurement and splunking indicator

// Stuff for Splunk
String eventData="";
String hecMessage="";

// variable for timing
static unsigned long msTickSplunk = 0;

/*
 * Returns a string of Localtime
 * in format "%Y-%m-%d %H:%M:%S"
 */
String getLocaltime(){
  time_t now = time(nullptr);
  struct tm * timeinfo;
  char timeStringBuff[20];
    
  time (&now);
  timeinfo = localtime(&now);
    
  strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%d %H:%M:%S", timeinfo);
  return timeStringBuff;
}

/*
 * Returns a string of time in UTC
 * in format "%Y-%m-%d %H:%M:%S"
 */
String getUTC(){
  time_t now = time(nullptr);
  struct tm * timeinfo;
  char timeStringBuff[20];
    
  time (&now);
  timeinfo = gmtime(&now);
    
  strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%d %H:%M:%S", timeinfo);
  return timeStringBuff;
}

/*
 * Returns epoch time as long
 */
long getEpoch(){
  time_t now = time(nullptr);
  return now;
}


/*
 * Post event data to splunk http event collector
 * 
 * PostData: 
 * a string of json formatted key:value pairs 
 * just everything between the curly braces of the event node
 */
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

/*
 * Post multiple event data to splunk http event collector
 * 
 * PostData: 
 * a string of json formatted event-nodes with key:value pairs 
 * event:{key:value},event:{key:value, key:value}
 */
void splunkMultiPost(String events){
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
                  + events + 
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
  // if the splunk send intervall is high.. this only sends the state just before the measurement..
  // if intervall = 10m and PIR is triggered @2m but then nothing anymore.. we would report nothing..
  pirTripped = digitalRead(PIR);
  //digitalWrite(PIR_LED, pirTripped);
  if (pirTripped) pirTrippedTime = millis();
}

// Software restart 
void(* resetFunc) (void) = 0; //declare reset function @ address 0
/*
 * Triggers a software restart
 * resets the flag in config manager
 * send INFO msg to splunk
 */
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
    Serial.print(PSTR("Local time: ")); Serial.println(asctime(localtime(&now)));
    Serial.print(PSTR("UTC: ")); Serial.println(asctime(gmtime(&now)));
  }
  else 
  {
    splunkpost("\"status\" : \"ERROR\", \"msg\" : \"No time sync available.\""); 
    // set an error LED high..
  }

  // set input/output pins
  pinMode(Heartbeat_LED, OUTPUT); // heartbeat LED
  digitalWrite(Heartbeat_LED, HIGH);
  //pinMode(PIR_LED, OUTPUT); // PIR active LED
  //digitalWrite(PIR_LED, LOW);
  pinMode(Splunking_LED, OUTPUT); // PIR active LED
  digitalWrite(Splunking_LED, LOW);
 

  pinMode(LDR, INPUT);
  pinMode(PIR, INPUT);


  // BME280
  if (!bme.begin(0x76)){
    splunkpost("\"status\" : \"ERROR\", \"msg\" : \"BME280 not found.\""); 
    // set an error LED high.. 
  }
  else{
    bmeErr = false; 
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
  digitalWrite(Heartbeat_LED, (millis() / 1000) % 2); // doesnt even need a timer ... can be placed just inside the loop

  // read PIR state
  checkPir();

  if (millis() - msTickSplunk > configManager.data.updateSpeed){
    msTickSplunk = millis();

    digitalWrite(Splunking_LED, HIGH);

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
    //if(!bmeErr){
      sensors_event_t temp_event, pressure_event, humidity_event;
      bme_temp->getEvent(&temp_event);
      bme_pressure->getEvent(&pressure_event);
      bme_humidity->getEvent(&humidity_event);
    //}
    // BME280 END

    // only report these sensors if they are up and running...
    String BME280_data = bmeErr ? "" :  "\"BME280_Temp\": \"" + String(temp_event.temperature) + "\" , "
                                        "\"BME280_Pressure\": \"" + String(pressure_event.pressure) + "\" , "
                                        "\"BME280_Humidity\": \"" + String(humidity_event.relative_humidity) + "\" , ";

    String MAX44009_data = luxErr ? "" :  "\"MAX44009_lux\": \"" + String(luxSensor.getLux()) + "\" , "
                                          "\"MAX44009_IntegrationTime\": \"" + String(luxSensor.getIntegrationTime()/1000.0) + "\" , ";

    // build the event data string
    eventData =   "\"PIR_State\": \"" + String(pirTripped) + "\" , "
                  "\"lightIndex\": \"" + String(LDRvalue) + "\" , "
                  + BME280_data 
                  + MAX44009_data + 
                  "\"HCSR04_Distance\": \"" + String(HCSR04.read()) + "\" , "
                  "\"uptime\": \"" + String(millis()/1000) + "\" ";

    //send off the data
    splunkpost(eventData);

    // this is not how event stacking is working.. needs a fixfix
    eventData = "\"event\"  : {" 
                  "\"lightIndex\": \"" + String(LDRvalue) + "\" , "
                  "\"uptime\": \"" + String(millis()/1000) + "\" " + 
                  "},"
                  "\"event\"  : {" 
                  "\"sensor\": \"" + String("HC-SR501") + "\" , "
                  "\"PIR_State\": \"" + String(pirTripped) + "\" "
                  "},"
                  "\"event\"  : {" 
                  "\"sensor\": \"" + String("HC-SR04") + "\" , "
                  "\"distance\": \"" + String(HCSR04.read()) + "\" "
                  "},"
                  "\"event\"  : {" 
                  "\"sensor\": \"" + String("BME280") + "\" , "
                  + BME280_data +
                  "},"
                  "\"event\"  : {" 
                  "\"sensor\": \"" + String("MAX44009") + "\" , "
                  + MAX44009_data +
                  "}";

    //splunkMultiPost(eventData);
    digitalWrite(Splunking_LED, LOW);
  }
}