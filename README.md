# ESP8266-12F-Splunk-HEC
![First Try](Infos/Pictures/Breadboard%20Picture.png?raw=true "Prototype")

## The what and why...
### General
* This repository is just for me to learn how to use Github.
* This readme is just my personal note taking on my endeavour to learn. And so i have a place to review how i did this when i'm old.. :D

### Specific
* I have a few ESP8266-12F (Node MCU V3) and also a few Wemos D1 mini.
* I always wanted to use them for their wifi cababilities... didnt have a project/time so far.
* I am implementing Splunk at work. Learning on the go.. why not play with it at home too..
* I can run splunk in a docker on my QNAP NAS.. (not the best idea for me at least..)
* I have a spare mini PC (ASUS PN60) to run ubuntu and splunk.
* I want to use Visual-Studio-Code and the PlatformIO plugin.

### Scope
* Everything i learn / do to get the ESP8266 to talk to Splunk ~~on my NAS~~ on a dedicated computer.
* Let's learn...Splunk, ~~Docker~~, ESP8266, Markdown, GitHub, Linux...

## Documentation
* [ESP8266 IoT Framework](https://github.com/maakbaas/esp8266-iot-framework)
* [HEC Error Codes](https://docs.splunk.com/Documentation/Splunk/8.1.2/Data/TroubleshootHTTPEventCollector)
* [QNAP Docker Splunk](https://docs.google.com/document/d/1_8vvd1eB1JU5wbU0zWW05CGtKhPdAXr0pFtFbg6_nl4/edit#heading=h.c8ma5kq9d3ms)
  * SPLUNK_START_ARGS	--accept-license --name splunk
  * Docker is running in NAT mode.. that will change all port redirects on restart.. need to adjust the splunk-indexer port in ESP8266 every time

### Learnings?
* Difference in sending all sensor data in one eventvs sending in batches (eg each sensor type as batch)?
  * NoBatch: i just named the sensor like BME280_temp..
  * Batch: just name the measurement Temperatur and have a sensor type as key:value
    * nope that didnt work out as expected.. sensor type didnt get in the index for each sensor.. jsut for the last in the batch
    * need to read the batching docu again maybe.. yeah. I tried to just send the event data. But it needs all the other information aswell.. also its stacked and so there is no `,`separator between the events curly braces.
* Event index vs Metric index
  * metric is faster for metrics ;) .. so they say
* Light sensors.. i wanted to substitute the LDR with a I2C Llux sensor (MAX44009).. but instead i keep both. This way i can run scatter plots with both and see how good my cheap LDR holds up against a 4€ sensor :D
* after splunkpost there is a `{"text":"Success","code":0}` which is followed by a a 'HTTP200', but when `Enable indexer acknnowledgement` is checked.. it returns `{"text":"Data channel is missing","code":10}`followed by a 'HTTP400' --> question is.. why (see HEC error coeds in Docu section) is splunk returning this and how can i use it :D

### mixed and mashed the following three informations to get my ESP8266 to talk to splunk
* [ESP8266 Splunk HEC 1](https://maddosaurus.github.io/2018/08/05/esp8266-posting-to-splunk-hec)
* [ESP8266 Splunk HEC 2](https://www.splunk.com/en_us/blog/tips-and-tricks/splunking-sensor-data-with-arduino-and-http-event-collector.html)
* [ESP8266 Splunk HEC 3](https://hackernoon.com/arduino-meet-splunk-81f32e252f9c)

### ressources to read and better understand stuff
* [Splunk HEC Explained](https://medium.com/adarma-tech-blog/splunk-http-event-collectors-explained-2c22e87ab8d2)
* [ESP8266 Pinouts](https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/)
* [Adafruit BME280 Documentation](https://learn.adafruit.com/adafruit-bme280-humidity-barometric-pressure-temperature-sensor-breakout/downloads)
* [MAX44009 Abmient Light Sensor](https://datasheets.maximintegrated.com/en/ds/MAX44009.pdf)
* [How much Lux?](https://en.wikipedia.org/wiki/Lux)
* [Format time with strftime c++](http://www.cplusplus.com/reference/ctime/strftime/)
* [TimeZone inputs for ConfigManager](https://github.com/esp8266/Arduino/blob/master/cores/esp8266/TZ.h) -> use the string between `PSTR("` and `")` without the double qoutes e.g. CET-1CEST,M3.5.0,M10.5.0/3

### Splunk and Docker
* me from the future speaking... nonono dont deal with docker...get a real pc :D
* [splunk.pid unreadable](https://community.splunk.com/t5/Security/splunk-starting-as-root-user-how-to-change-this-one/m-p/305432)
* due to Docker we can not use port 8088 for HEC.. it gets redirected from 32k-something to 8088 on the Docker container
* Docker (at least my container) runs in UTC. 
  * Sounds like best practise... but i am not located in UTC. Splunk HEC will use index time for time stamps, since in the HEC payload (eventdata, metadata) there is no timestamp configured. 
  * Need to figure out how to set the timezone for the source/host/sourcetype of my sensor in props.conf to match my timezone (especially how to do it on Docker...).. everything i tried didnt work out as planned.. 

### Splunk on a ubuntu 20 box
* [Downlaod Splunk](https://www.splunk.com/en_us/download/)
* [Starting Splunk for the first time](https://docs.splunk.com/Documentation/Splunk/8.0.3/Installation/StartSplunkforthefirsttime)
* install Splunk.. lazy by double clicking the .deb package in the downloads folder
* set splunk home directory `export SPLUNK_HOME=/opt/splunk`
* Warning: cannot create "/opt/splunk/etc/licenses/download-trial"
  * seb@BlackboxDEV:/opt/splunk/bin$ sudo ./splunk enable boot-start -user splunk --accept-license [This Link](https://community.splunk.com/t5/Archive/Splunk-reports-first-time-run-failed/m-p/464516)
* seb@BlackboxDEV:/opt/splunk/bin$ sudo ./splunk start
* Splunk is running on localhost:8000
* lets try restart then.. works.
* .. what else to do witha fresh splunk install?

#### Migrate from Docker to ubuntu 20 box
* stopped the container
* copied the esp8266hec index files from `lib/splunk/` to the new destination
* copied the indexes.conf (just the content for the esp8266hec index) to the new destination
* since i copied the indexes.conf file with my linux user.. need to set the splunk user as owner `sudo chown splunk:splunk indexes.conf`
* the esp8266hec index folder and .dat is not that easy to change owner.. on terminal i get permission denied to `cd /opt/splunk/var/lib/splunk`.. but if i do `sudo su` it is allowed :D then just `chown -R splunk:splunk esp8266hec` and `chown splunk:splunk esp8266hec.dat`
* use `ls -l`to list the files and folders with owners 
* the permissions dont look the same on the new copied index files...`sudu su` .. `chmod 600 esp8266hec.dat` .. `chmod -R 700 esp8266hec`
* success. the index is listed in splunk web. searching works too.
* now just setup the HEC on the new instance and reconfigure the sensor box. DON'T FORGET TO DISABLE SSL IN HEC GLOBAL SETTINGS! (trust me.. i nearly started troubleshooting this again...)
* get the dashboard over.. well i just copy and paste the source code...
* whole migration with lots of google-fu and writing this text took a bit over one hour... and i have little skills in linux or splunk :D

### IoT Framework 
* to change the config parameters and rebuild the web pages 
  * NPM needs to be installed (i guess u need to [install NPM](https://www.npmjs.com/get-npm) / node.js frist)
   * open the `ESP8266 IoT Framework lib folder` (.pio/lipdeps/ESP8266 IoT Framework -> right click `open in integrated Terminal`) with vscode-terminal ()
   * in the new terminal window type `npm ci`
    * ~~open the project in `platformio cli terminal` and write `npm init` [Link](https://stackoverflow.com/questions/50895493/solving-the-npm-warn-saveerror-enoent-no-such-file-or-directory-open-users)~~
    * ~~use npm init -y to not be asked for any details~~
    * ~~`npm install`  [install NPM](https://www.npmjs.com/get-npm)~~
    * ~~?npm install --save-dev webpack? [Link](https://webpack.js.org/guides/installation/)~~
  * add a .json file with the config parameters `iotFrameworkConfig.json` in the project root (wanted it in 'src' folder.. but that dind work.. maybe the buildflag was set wrong..)
  * add to platformio.ini: `build_flags = -DCONFIG_PATH=iotFrameworkConfig.json -DREBUILD_HTML -DREBUILD_CONFIG` to trigger the redbuild of the webinterface

### Hardware
* ESP8266 12F
* ~~DHT11 (temp, humidity)~~ dropped due to unreliable readings
* BME280 on I2C (temp, pressure, humidity)
* LED (1s heartbeat)
* LED (PIR activity)
* LED (Splunking signal)
* LDR (cheap light intensity sensor)
* MAX44009 on I2C (ambient light sensor measures LUX.. as addition/improvement over the LDR solution)
* HC-SR04 (ultra sonic distance sensor)
* HC-SR501 (PIR sensor)

### Problems
#### ESP8266 IoT Framework 
* After first changing the config parameters i struggeled to get it working for a 2nd project. But that was a case of RTFM. I tried to install npm and all that stuff in the project folder and not in the ESP8266 IoT Framework library folder. Anyways it is expalined in the manual.. just gotta read it..
#### DHT11
* This sensor (maybe just mine) is totally unrelaiable. Sometimes after restart it just reported numbers so far off the scale.. 150% humidity, -12°C temperature.. or returning NaN then a normal reading then off scale reading and so on...
#### Timezone and Docker
* The Docker container with splunk is running in UTC. And no matter what i set as my Timezone in Splunk web UI.. i only get my charts reporting in UTC. 
* Tried to set the TZ attribute in props.conf. No luck. Not for source, sourcetype or host.
* Well.. just giving up on Docker running Splunk on my NAS.. Getting Splunk on a dedicated machine and see how that works out.
#### Indextime vs Sensortime
* After i send the timestamp from the sensor (which is NTP synced) to splunk i noticed a time difference. The splunk HEC is using indextime since within the HEC json i do not specify the time in epoch (yet). Appearently it takes 15s to index or the docker container is not in sync..
* The time difference seems to getting worse over time... **insert picture**
* checking the time online and the `date`in docker over terminal.. the docker container is 18s in the future right now. That fits the 18s time difference between index and sensor time..
* Soooo...this is a docker problem... not a splunk one, nor a slow NAS :D
* [to fix it in the docker container](https://askubuntu.com/questions/342854/what-is-the-command-line-statement-for-changing-the-system-clock) (at least untill restart) i used `sudo date new_date_time_string` (format: MMDDhhmmyyyy.ss) MM=Month, DD=Day, hh=Hour, mm=Minute, yyyy=Year, ss=Second
#### Docker container restart
* After restart of the container the inputs didnt show up in splunk anymore. After debugging the Arduinos and any possible code change.. figured out it is Splunk. The HEC global setting for SSL was enabled again.. guess this setting doesnt persit a restart of the container. After disableing SSL.. the inputs were fine again.
#### Serial gibber gabber after reboot
* seems like the PIR output connected to input pin D4 is not a good idea. D4 is HIGH at boot and boot fails if pulled low (see ESP8266 pinout link).
