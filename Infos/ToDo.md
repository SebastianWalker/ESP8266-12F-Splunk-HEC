+ Rangefinder 
    + add I2C rangefinder to display desk height (sitting/standing...) --> implementet in another [ESP8266 Sensor](https://github.com/SebastianWalker/ESP8266-D1-mini-SPLUNK-HEC)

+ uptime
    + sent uptime in seconds since controller restart

- send meta data 
    - UTC timestamps
    + IP address

- temp sensor DS18B20

+ use config settings instead of hard coded stuff

- implement reset function in code
    - log reset event/reason to splunk
    - reset over web interface button input
    - reset if DHT11 readings are way off.. 

- add a PIR to detect motion in my office (am i home or working inmy company office?)

- add more build pictures and schematics

- add averaging to the distance sensor to avoid false readings (spikes...)

- add feedback leds to show connection status and/or http response code

- timezone change with configmanager is used in setup... right now it needs a restart to take affect...