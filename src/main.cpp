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

// Stuff for the DHT sensor
  #include "DHT.h"
  #define DHTPIN 2 
  #define DHTTYPE DHT11  
  DHT dht(DHTPIN, DHTTYPE);

// LDR input pin (ESP8266 only got one ADC on A0)
#define LDR A0
int LDRvalue = 0;

// Stuff for Splunk
String eventData="";

// variable for timing
static unsigned long msTickSplunk = 0;
static unsigned long msTick = 0;

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


void setup()
{
    Serial.begin(115200);

    LittleFS.begin();
    GUI.begin();
    configManager.begin();
    WiFiManager.begin(configManager.data.projectName);

    //Set the timezone to UTC
    timeSync.begin();

    //Wait for connection
    timeSync.waitForSyncResult(10000);

    if (timeSync.isSynced())
    {
        time_t now = time(nullptr);
        Serial.print(PSTR("Current UTC: "));
        Serial.print(asctime(localtime(&now)));
    }
    else 
    {
        Serial.println("Timeout while receiving the time");
    }

    pinMode(D8, OUTPUT);
    digitalWrite(D8, HIGH);
    pinMode(LDR, INPUT);


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

}

void loop()
{
    //software interrupts
    WiFiManager.loop();
    updater.loop();    

    // toggle LED every second
    digitalWrite(D8, (millis() / 1000) % 2); // doesnt even need a timer ... can be placed just inside the loop

    // if (millis() - msTick > 1000) {
    //   msTick = millis();
    //   sensors_event_t temp_event, pressure_event, humidity_event;
    //   bme_temp->getEvent(&temp_event);
    //   bme_pressure->getEvent(&pressure_event);
    //   bme_humidity->getEvent(&humidity_event);
      
    //   Serial.print(F("Temperature = "));
    //   Serial.print(temp_event.temperature);
    //   Serial.println(" *C");

    //   Serial.print(F("Humidity = "));
    //   Serial.print(humidity_event.relative_humidity);
    //   Serial.println(" %");

    //   Serial.print(F("Pressure = "));
    //   Serial.print(pressure_event.pressure);
    //   Serial.println(" hPa");
    //   }

    if (millis() - msTickSplunk > configManager.data.updateSpeed){
    //if (millis() - msTickSplunk > 5000){
      msTickSplunk = millis();

      // Wait and print the time
      time_t now = time(nullptr);
      Serial.print(PSTR("Current UTC: "));
      Serial.print(asctime(localtime(&now)));   

      // Read the light intensity
      LDRvalue=0; // reset
      for(int i=0; i<4; i++){
        LDRvalue = LDRvalue + analogRead(LDR); 
        delay(10);
      }
      LDRvalue = map(LDRvalue / 4, 0,1024,0,100);
      
    // DHT11 
      // Reading temperature or humidity takes about 250 milliseconds!
      // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
      float h = dht.readHumidity();
      // Read temperature as Celsius (the default)
      float t = dht.readTemperature();
      // Read temperature as Fahrenheit (isFahrenheit = true)
      float f = dht.readTemperature(true);

      // Check if any reads failed and exit early (to try again).
      if (isnan(h) || isnan(t) || isnan(f)) {
        Serial.println(F("Failed to read from DHT sensor!"));
        //delay(2000);
        //return;
      }

      // Compute heat index in Fahrenheit (the default)
      //float hif = dht.computeHeatIndex(f, h);
      // Compute heat index in Celsius (isFahreheit = false)
      float hic = dht.computeHeatIndex(t, h, false);

      // Serial.print(F("Humidity: ")); Serial.print(h);
      // Serial.print(F("%  Temperature: ")); Serial.print(t); Serial.print(F("°C "));
      // Serial.print(f); Serial.print(F("°F  Heat index: ")); Serial.print(hic); Serial.print(F("°C ")); Serial.print(hif); Serial.print(F("°F "));
      // Serial.print(F("Light: ")); Serial.print(LDRvalue); Serial.println(F("i"));
      // Serial.println();
    // DHT11 END

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
                                  "\"temp\" : \"" + t + "\" , "
                                  "\"heatindex\" : \"" + hic + "\" , "
                                  "\"humidity\": \"" + h + "\" , "
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