# ESP8266-12F-Splunk-HEC

## The what and why...
### General
* This repository is just for me to learn how to use Github.
* This Readme is just my personal note taking on my endeavour to learn. And so i have a place to review how i did this when i'm old.. :D

### Specific
* I have a few ESP8266-12F..
* I always wanted to use them for their wifi cababilities... didnt have a project/time so far.
* I am implementing Splunk at work. Learning on the go.. why not play with it at home too..
* I can run splunk in a docker on my QNAP NAS.. 

### Scope
* Everything i learn / do to get the ESP8266 to talk to Splunk on my NAS.
* Let's learn...Splunk, Docker, ESP8266, Markdown, ...

## Documentation
* [IoT Framework](https://github.com/maakbaas/esp8266-iot-framework)
* [QNAP Docker Splunk](https://docs.google.com/document/d/1_8vvd1eB1JU5wbU0zWW05CGtKhPdAXr0pFtFbg6_nl4/edit#heading=h.c8ma5kq9d3ms)
  * SPLUNK_START_ARGS	--accept-license --name splunk
  * Docker is running in NAT mode.. that will change all port redirects on restart.. need to adjust the splunk-indexer port in ESP8266 every time

### mixed and mashed the following three informations to get my ESP8266 to talk to splunk
* [ESP8266 Splunk HEC 1](https://maddosaurus.github.io/2018/08/05/esp8266-posting-to-splunk-hec)
* [ESP8266 Splunk HEC 2](https://www.splunk.com/en_us/blog/tips-and-tricks/splunking-sensor-data-with-arduino-and-http-event-collector.html)
* [ESP8266 Splunk HEC 3](https://hackernoon.com/arduino-meet-splunk-81f32e252f9c)

### ressources to read and better understand stuff
* [Splunk HEC Explained](https://medium.com/adarma-tech-blog/splunk-http-event-collectors-explained-2c22e87ab8d2)

### Splunk and Docker
* [splunk.pid unreadable](https://community.splunk.com/t5/Security/splunk-starting-as-root-user-how-to-change-this-one/m-p/305432)
* due to Docker we can not use port 8088 for HEC.. it gets redirected from 32k-something to 8088 on the Docker container
* Docker (at least my container) runs in UTC. 
  * Sounds like best practise... but i am not located in UTC. Splunk HEC will use index time for time stamps, since in the HEC payload (eventdata, metadata) there is no timestamp configured. 
  * Need to figure out how to set the timezone for the source/host/sourcetype of my sensor in props.conf to match my timezone (especially how to do it on Docker...)

### IoT Framework 
* to change the config parameters and rebuild the web pages 
  * NPM needs to be installed 
    * open the project in `platformio cli terminal` and write `npm install`  https://www.npmjs.com/get-npm
    * ?npm install --save-dev webpack? https://webpack.js.org/guides/installation/
  * open the `ESP8266 IoT Framework` in vscode-terminal (right click the project and choose 'open in terminal') and issue a `npm ci` command
  * add a .json file with the config parameters `iotFrameworkConfig.json` in the project root (wanted it in 'src' folder.. but that dind work.. maybe the buildflag was set wrong..)
  * add to platformio.ini: `build_flags = -DCONFIG_PATH=iotFrameworkConfig.json -DREBUILD_HTML -DREBUILD_CONFIG` to trigger the redbuild of the webinterface

### Hardware
* ESP8266 12F
* DHT11 (Signal to D4, Resistor from VCC to signal)
* LED (D8, used for 1s heartbeat)
* LDR (A0 and VCC, Potentiometer to A0 to adjust the LDR reading)
