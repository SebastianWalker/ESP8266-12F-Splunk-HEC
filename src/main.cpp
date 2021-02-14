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


    pinMode(LED_BUILTIN, OUTPUT);
}

void loop()
{
    //software interrupts
    WiFiManager.loop();
    updater.loop();    

    // Wait five seconds and print the time
    delay(5000);
    digitalWrite(LED_BUILTIN, LOW);

    time_t now = time(nullptr);
    Serial.print(PSTR("Current UTC: "));
    Serial.print(asctime(localtime(&now)));   

    delay(5000);
    digitalWrite(LED_BUILTIN, HIGH);


    

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
  Serial.println(F("°F"));

}