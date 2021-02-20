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

// Stuff for the DHT sensor
  #include "DHT.h"
  #define DHTPIN 2 
  #define DHTTYPE DHT11  
  DHT dht(DHTPIN, DHTTYPE);

// LDR input pin (ESP8266 only got one ADC on A0)
#define LDR A0
int LDRvalue = 0;

// Stuff for Splunk
// splunk settings and http collector token
//String collectorToken = "13b98a55-2060-4754-b639-072af70ae770";
//String splunkindexer = "192.168.1.121:32773";
String eventData="";
//you need a different client per board
String clientName ="HQ";

// variables for timing
static unsigned long msTickLED = 0;
static unsigned long msTickSplunk = 0;

void splunkpost(String collectorToken,String PostData, String splunkindexer)
{
  // recieved the token, post data clienthost and the splunk indexer  
  //String payload = "{ \"host\" : \"" + Host +"\", \"sourcetype\" : \"http_test\", \"index\" : \"main\", \"event\": {" + PostData + "}}";
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
  Serial.println(payload);
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
}

void loop()
{
    //software interrupts
    WiFiManager.loop();
    updater.loop();    

    // toggle LED every second
    if (millis() - msTickLED > 1000) {
        msTickLED = millis();
        digitalWrite(D8, !digitalRead(D8));
        //digitalWrite(LED_BUILTIN, (millis() / 1000) % 2); // doesnt even need a timer ... can be placed just inside the loop
    }
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
      return;
    }

    // Compute heat index in Fahrenheit (the default)
    float hif = dht.computeHeatIndex(f, h);
    // Compute heat index in Celsius (isFahreheit = false)
    float hic = dht.computeHeatIndex(t, h, false);

    Serial.print(F("Humidity: "));
    Serial.print(h);
    Serial.print(F("%  Temperature: "));
    Serial.print(t);
    Serial.print(F("°C "));
    Serial.print(f);
    Serial.print(F("°F  Heat index: "));
    Serial.print(hic);
    Serial.print(F("°C "));
    Serial.print(hif);
    Serial.print(F("°F "));
    Serial.print(F("Light: "));
    Serial.print(LDRvalue);
    Serial.println(F("i"));
    Serial.println();

    // SPLUNK NOW
    // build the event data, telemtry and metrics type of data goes below
    clientName = configManager.data.clientName;
    eventData = "{ \"host\" : \"" + clientName + "\", \"sourcetype\" : \"diySensor\", \"index\" : \"esp8266hec\", " 
                "\"event\" :  {\"temp\" : \"" + t + "\" , "
                "\"heatindex\" : \"" + hic + "\" , "
                "\"humidity\": \"" + h + "\" , "
                "\"lightIndex\": \"" + LDRvalue + "\" , "
                "\"uptime\": \"" + millis()/1000 + "\" "
                "}}";

    //Serial.println(eventData);
    //send off the data
    //splunkpost(collectorToken, eventData, clientName, splunkindexer); 
    splunkpost(configManager.data.collectorToken, eventData, configManager.data.splunkindexer); 
  }
}