#include <Arduino.h>
#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
#include <PubSubClient.h>

#define DEBUG

//define your default values here, if there are different values in config.json, they are overwritten.
//char mqtt_server[40];
char mqtt_server[40] = "192.168.1.249";
char mqtt_port[6] = "1883";
char mqtt_user[20] = "user";
char mqtt_pass[40] = "password";
char mqtt_topic[50] = "stat/smart-doorphone"; // base topic . there will be base-topic/ringing and base-topic/LWT

//  TODO  Dorobit do WiFiManageru cislo telefonu, aby vedel na ktore ma pocuvat
// TODO   dorobit moznost pre Reset nastaveni / Vyvolanie WiFi Manageru manualne tlacidlom


char const * APpassword = "config123";
char const * APname = "Smart-DoorPhone";
const byte interruptPin = D7;
const byte resetPin = D1;
volatile byte interruptCounter = 0;
bool startOfCommand = false;
bool endOfCommand = false;
int ringingAddress = 0;
unsigned long lastTime = 0;
char mqtt_pub_topic[50] ;
char mqtt_lwt_topic[50] ;
bool shouldSaveConfig = false;
String output ;

String ringingTopic = mqtt_topic + String("/ringing");
String lwtTopic = mqtt_topic + String("/LWT");

WiFiClient espClient;
PubSubClient client(espClient);

ICACHE_RAM_ATTR void handleInterrupt() {
  interruptCounter++;
}
 
//callback notifying us of the need to save config
void saveConfigCallback () {
  #ifdef DEBUG
    Serial.println(F("Should save config"));
  #endif
  shouldSaveConfig = true;
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Hold your hats");
   
  //clean FS for testing
 // SPIFFS.format(); 
  
  // Set interrupt pin
  pinMode(interruptPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, CHANGE);

  pinMode(resetPin, INPUT_PULLUP);

  //read configuration from FS json
  #ifdef DEBUG
    Serial.println(F("mounting FS..."));
  #endif
  if (SPIFFS.begin()) {
    #ifdef DEBUG
      Serial.println(F("mounted file system"));
    #endif
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      #ifdef DEBUG
        Serial.println(F("reading config file"));
      #endif
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        #ifdef DEBUG
          Serial.println(F("opened config file"));
        #endif
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        StaticJsonDocument<1024> json;
        auto error = deserializeJson(json, buf.get());
        if (error) {
          Serial.print(F("deserializeJson() failed with code "));
          Serial.println(error.c_str());
          return;
        } else {
          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(mqtt_user, json["mqtt_user"]);
          strcpy(mqtt_pass, json["mqtt_pass"]);
          strcpy(mqtt_topic, json["mqtt_topic"]);
          #ifdef DEBUG
            Serial.println(F("deserializeJson() successful:"));
            serializeJsonPretty(json, Serial);
            Serial.println();
          #endif
        }
        configFile.close();
      } // end if config file
    } // end if config exists
  } else {
    Serial.println(F("failed to mount FS"));
  }
  //end read

  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
  WiFiManagerParameter custom_mqtt_user("user", "mqtt user", mqtt_user, 20);
  WiFiManagerParameter custom_mqtt_pass("pass", "mqtt pass", mqtt_pass, 40);
  WiFiManagerParameter custom_mqtt_topic("topic", "mqtt topic", mqtt_topic, 50);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

// Reset Wifi settings for testing
// wifiManager.resetSettings();

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  // change CSS style
  wifiManager.setCustomHeadElement("<style>body{background: #1b262c; color: #58a8de;}button{transition: 0.3s;opacity: 0.8;cursor: pointer;border:0;border-radius:1rem;background-color:#0f4c75;color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%;}button:hover {opacity: 1}button[type=\"submit\"]{margin-top: 15px; margin-bottom: 10px;font-weight: bold;text-transform: capitalize;}input::placeholder{color:#677a7b}input{height: 30px;font-family:verdana; border-radius: 0.3rem; margin-top: 5px;background-color: #223542; color: #b1d0d6;border: 0px;-webkit-box-shadow: 2px 2px 5px 0px rgba(0,0,0,0.45) ;-moz-box-shadow: 2px 2px 5px 0px rgba(0,0,0,0.45);box-shadow: 2px 2px 5px 0px rgba(0,0,0,0.45); }div{color: #3282b8;}div a{text-decoration: none;color: #58a8de;}div[style*=\"text-align:left;\"] {color: #3282b8;}, div[class*=\"c\"]{border: 0px;}a[href*=\"wifi\"]{border: 2px solid #3282b8; text-decoration: none;color: #bbe1fa;padding: 10px 30px 10px 30px;font-family: verdana;font-weight: bolder; transition: 0.3s;border-radius: 5rem;}a[href*=\"wifi\"]:hover{background: #3282b8;color: white;}.l{background: url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACoAAAAqCAYAAADFw8lbAAAABGdBTUEAALGPC/xhBQAAAAFzUkdCAK7OHOkAAAAgY0hSTQAAeiYAAICEAAD6AAAAgOgAAHUwAADqYAAAOpgAABdwnLpRPAAAAAZiS0dEAAAAAAAA+UO7fwAAAAlwSFlzAAAASAAAAEgARslrPgAAAQ9JREFUWMPtlrFKA0EQhr/Z5JIQ8BEUvNYihRB8Bgtb957E0lfwDSzdgI1PYJHaTsEqFgEVOwuJMSZjEzBcDqLhWBJuvmr5h9n92GVgwTAMwzCMNZAYh5zcPO60R42HwqLWj0K297xqj3oM0eY4ccAuoMDXPE4AN61N/uTgYogucBd82go+bQG3/2mMLbo2Jlo2pQyT7z2dq9LN56J6GbL0emNEVekKeryUi/bL2B+26OlNtLKiK4fpNAw6Iprlc1H3euX3LzZGVFQPQM7yuaL3QDTRrXl6E62saJSP8wKHPgw+5+ukVFEV9yHocKkgvPwuZ28gw4L2d4BxczJrjxpFdWrT5DvyZRmGYRhGlfgBOvg+7xfDqxkAAAAldEVYdGRhdGU6Y3JlYXRlADIwMjAtMDMtMDdUMTc6Mjc6MjgrMDA6MDB5NRSiAAAAJXRFWHRkYXRlOm1vZGlmeQAyMDIwLTAzLTA3VDE3OjI3OjI4KzAwOjAwCGisHgAAACh0RVh0c3ZnOmJhc2UtdXJpAGZpbGU6Ly8vdG1wL21hZ2ljay12UjViV2JjaFBVLG4AAAAASUVORK5CYII=') no-repeat left center;background-size: 1.5em;}.q{width:70px}</style>");

  //set static ip
  //  wifiManager.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

  //add all your parameters here
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_pass);
  wifiManager.addParameter(&custom_mqtt_topic);

  //set minimum quality of signal so it ignores AP's under that quality
  //defaults to 8%
  //wifiManager.setMinimumSignalQuality();

  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  //wifiManager.setTimeout(120);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect(APname, APpassword)) {
    Serial.println(F("failed to connect and hit timeout"));
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println(F("connected...yeey :)"));

  //read updated parameters
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(mqtt_user, custom_mqtt_user.getValue());
  strcpy(mqtt_pass, custom_mqtt_pass.getValue());
  strcpy(mqtt_topic, custom_mqtt_topic.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    #ifdef DEBUG
      Serial.println(F("Saving config"));
    #endif
    StaticJsonDocument<1024> json;
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["mqtt_user"] = mqtt_user;
    json["mqtt_pass"] = mqtt_pass;
    json["mqtt_topic"] = mqtt_topic;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println(F("failed to open config file for writing"));
    }

    #ifdef DEBUG
      serializeJson(json, Serial);
      Serial.println();
    #endif
    serializeJson(json, configFile);
    configFile.close();
    //end save
  }

  Serial.println(F("Local IP:"));
  Serial.println(WiFi.localIP());
  
  // MQTT
  strncpy(mqtt_pub_topic, ringingTopic.c_str(), sizeof(mqtt_pub_topic));
  strncpy(mqtt_lwt_topic, lwtTopic.c_str(), sizeof(mqtt_lwt_topic));
  const uint16_t mqtt_port_x = atol( mqtt_port );
  client.setServer(mqtt_server, mqtt_port_x);

} // End Setup

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    // Attempt to connect
    // If you do not want to use a username and password, change next line to
    // if (client.connect("ESP8266Client")) {
    if (client.connect("SmartDoorPhone", mqtt_user, mqtt_pass, mqtt_lwt_topic, 0, 0, "offline")) {
      #ifdef DEBUG  
        Serial.println(F("MQTT connected"));
      #endif
      client.publish(mqtt_lwt_topic,"online");
    } else {
      Serial.print(F("failed, rc="));
      Serial.print(client.state());
      Serial.println(F(" will try again in 3 seconds"));
      // Wait 5 seconds before retrying
      delay(3000);
    } // end if client connect failed
  } // end while ! connected
}  // end reconnect

void loop() {
  //Serial.println(interruptPin ? "HIGH" : "LOW") ;
  if(interruptCounter>0){  // We have interrupt, do something
      unsigned long now = micros();
      uint32_t timeDifference = now - lastTime ;
      lastTime = now;
      interruptCounter--;

      if( timeDifference >= 150 * 1000  && timeDifference <= 250 * 1000  ) {  // 200 ms
        // We have starting or ending bit
        if(startOfCommand == false ) {  // Start command
          startOfCommand = true;
          endOfCommand = false;
          ringingAddress = 0;
          output = "\n \r<Start cmd>";
        } else {                        // End command
          endOfCommand = true;
          output += "\n \r<End cmd>";
        }
        output += "t=" + String(timeDifference) + "us";
      }
    
    if( timeDifference >= 20 && timeDifference <= 28 && startOfCommand == true ) {  // 24 us
      // We are in command mode, start counting pulses
      ringingAddress++;
      output += "\n \r<Ring" + String(ringingAddress) + ">";
      output += "t=" + String(timeDifference) + "us";
    }

  }

  // Timeouts
  unsigned long now = micros();
  uint32_t timeDifference = now - lastTime ;
  if( timeDifference >= 100 * 1000  && startOfCommand == true && ringingAddress > 0) {  // 100 ms
    // there were no pulses, trigger timeout
    endOfCommand = true;
    startOfCommand = false;
    output += "\n \r<Timeout1>";
    output += "t=" + String(timeDifference) + "us";

    // Prepare a JSON payload string
  //  String payload = "{";
  //  payload += "\"eCO2\":"; payload += avgeCO2; payload += ",";
  //  payload += "\"TVOC\":"; payload += avgTVOC; payload += ",";
  //  payload += "\"temperature\":"; payload += temp;
  //  payload += "}";

    String payload = String(ringingAddress);
    
    // Convert payload
    char attributes[100];
    payload.toCharArray( attributes, 100 );
    // Send MQTT message
    #ifdef DEBUG
      Serial.print("Sending MQTT message: ");
      Serial.println(payload);
    #endif
    
    client.publish(mqtt_pub_topic, attributes , false);

    Serial.print("There is call to phone : ");
    Serial.println(ringingAddress);
    #ifdef DEBUG
      Serial.print(output);
      Serial.println();
    #endif
    output = "";
  }  // end timeout of dialing

  if( timeDifference >= 1000 * 1000 && startOfCommand == true ) {   // Timeoute for inactivity 1 s
    startOfCommand = false;
    endOfCommand = true;
    ringingAddress = 0;
    #ifdef DEBUG
      Serial.print(output);
      Serial.println();
    #endif
    #ifdef DEBUG
      Serial.println("Timeout for inactivity");  
    #endif
  }  // end timeout for inactivity

  if(interruptCounter == 0 &&  timeDifference >= 500 * 1000){  // Do not handle wifi and MQTT if we are dealing with interrupts

    // Check WIFI connection
    if (!client.connected()) {
      reconnect();
    }
    // check for MQTT messages
    client.loop();  
  }

  if(digitalRead(resetPin) == LOW && interruptCounter == 0 ) {   // Reset WIFI settings
    WiFiManager wifiManager;
    wifiManager.resetSettings();
    ESP.restart();
  }
} // end LOOP