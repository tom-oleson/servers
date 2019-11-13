
/*
 * Copyright (c) 2019, Tom Oleson <tom dot oleson at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * The names of its contributors may NOT be used to endorse or promote
 *     products derived from this software without specific prior written
 *     permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_SHT31.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>
#include <EEPROM.h>

#define ESP32
#define DS18B20

#define JS(s) "" #s ""

#ifdef ESP32
#include <WiFi.h>

char ssid[33] = { "RedLion" };             
char password[33] = { "vaxole63" };

char server_address[20] = { '\0' };
int server_port = -1;

// esp32 mac and hostname
#define DEVICE_PREFIX "ETOK"
uint8_t mac_address[6];
char hostname[33] = { '\0' };

WiFiClient client;

#endif

char sID[20] = "#arduino";

int soil_pin = 0;
Adafruit_SHT31 sht31 = Adafruit_SHT31();
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);

String input;
volatile unsigned long count = 0;

float temperature = 0;
float humidity = 0;
uint16_t lux = 0;
uint16_t bb_lum = 0;
uint16_t ir_lum = 0;;
int soil_moisture = 0;
float ds_temperature = 0;

volatile int interval = 10;   // output interval (seconds)

bool sht31_found = false;
bool tsl_found = false;
volatile bool soil_found = false;

/////////////////// output targets //////////////////////////

bool wifi_out = false;
#define con_print(s) if(wifi_out) client.print(s); \
                        else Serial.print(s)

#define con_println(s) if(wifi_out) client.println(s); \
                        else Serial.println(s)

////////////////////////// WiFi ///////////////////////////////

#ifdef ESP32
void wifi_close_connection() {
    client.stop();
    wifi_out = false; 
}

void init_wifi() {
  
  Serial.print(F("Attempting to connect to "));
  Serial.println(ssid);

  // setup unique hostname for this device
  WiFi.macAddress(mac_address);
  sprintf(hostname,  "ETOK-%02x%02x%02x%02x%02x%02x",
    mac_address[0], mac_address[1], mac_address[2],
    mac_address[3], mac_address[4], mac_address[5]);

  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(hostname);
  
  WiFi.begin(ssid, password);
  //WiFi.setHostname(hostname);

  
  int timeout = 10; // seconds
  while(WiFi.status() != WL_CONNECTED && --timeout > 0) {
    delay(500);
    Serial.print(F("."));
    delay(500);
  }
  Serial.println();
  
  if(WiFi.status() == WL_CONNECTED) {
    Serial.print(F("WiFi connected (IP address:"));
    Serial.print(WiFi.localIP());
    Serial.println(F(")"));
  }
  else {
    Serial.println(F("WiFi not available"));
    return;
  }

  if(server_address[0]) {
    Serial.print(F("Attempting to connect to "));
    Serial.println(server_address);
    delay(500);
    
    if(!client.connect(server_address, server_port)) {
      con_println(F(": connection failed"));
    }
    else { 
      Serial.println(F(": connection successful"));
      wifi_out = true;
      con_println("+hello 'Hello world, from ESP32!");
    }
  }
}
#endif

void init_soil() {
    soil_found = false;
}

////////////////////////// DS18B20 ///////////////////////////////////

#ifdef DS18B20

#include <OneWire.h>              // for digital temp probe
#include <DallasTemperature.h>    // for digitial temp probe

#define ONE_WIRE_BUS    2

DeviceAddress thermometer_address;  // holds 64 bit device address
OneWire one_wire(ONE_WIRE_BUS);        
DallasTemperature temp_sensor(&one_wire);  

bool ds18b20_found = false;

void init_ds18b20() {
  temp_sensor.begin();
  ds18b20_found = temp_sensor.getAddress(thermometer_address, 0);

  if(ds18b20_found) {

    // set resolution bits (9-12)
    temp_sensor.setResolution(thermometer_address, 11);

    // print device address
    Serial.print(F("DS1820B device address:"));
    for (uint8_t i = 0; i < 8; i++) {
      if (thermometer_address[i] < 16) Serial.print(F("0"));
        Serial.print(thermometer_address[i], HEX);
    }        
    
    Serial.println();
  }
  else {
    Serial.println(F("No DS1820B sensor found"));
  }
}
#endif

void init_sht31() {

    sht31_found = sht31.begin(0x44);
    if(!sht31_found) {
      Serial.println(F("No SHT31 sensor found"));
    }  
}

void init_tsl2561() {
  
    tsl_found = tsl.begin();
    if(!tsl_found) {
      Serial.println(F("No TSL2516 sensor found"));
    }
    else {

      // Set the gain or enable auto-gain support

      // No gain: use in bright light
      //tsl.setGain(TSL2561_GAIN_1X);
      
      // 16x gain: use in low light
      //tsl.setGain(TSL2561_GAIN_16X);

      // Auto-gain: auto switch between 1x and 
      tsl.enableAutoRange(true);
      Serial.println(F("TLS Gain: Auto"));

      // Changing integration time gives better sensor
      // resolution (402ms = 16-bit data)

      // fast but low resolution 
      tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS); // fast but low resolution 
      Serial.println(F("TLS Timing: 13 ms"));

      // medium resolution and speed
      // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_101MS);

      // 16-bit data but slowest conversions
      // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);  // 16-bit 

    }
}

void read_eeprom() {
    // if not virgin EEPROM
    if(EEPROM.read(0x00) != 0xff) {
        Serial.println(F("Reading EEPROM"));
        for (int addr = 0; addr < sizeof(sID); addr++) {
            sID[addr] = EEPROM.read(addr);
            if(sID[addr] == '\0') break;
        }
    }
}

void update_eeprom() {
    Serial.println(F("Writing EEPROM"));
    for (int addr = 0; addr < sizeof(sID); addr++) {
      if(EEPROM.read(addr) != sID[addr])
        EEPROM.write(addr, sID[addr]);
      // don't reprogram more than we must!
      if(sID[addr] == '\0') break;
    }
#ifdef ESP32
    EEPROM.commit();
#endif
}

void setup() {
    Serial.begin(9600);
    while(!Serial) { }

#ifndef ESP32
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
#endif

#ifdef ESP32
    EEPROM.begin(1024);
#endif    

    read_eeprom();

#ifdef ESP32
    init_wifi();
#endif

    init_soil();
    init_sht31();
    init_tsl2561();

    #ifdef DS18B20    
    init_ds18b20();
    #endif    
}

void output_field_seperator() {
  con_print(F(","));
}

bool set_server_address_and_port(char *buf, size_t sz) {
   
   // parse out server address and port
    // format of: ip_address:port\0

    char *cp = strchr(buf, ':');
    if(NULL != cp && cp < (buf + sz)) {
      *cp++ = '\0';
      unsigned int port = atoi(cp);
      if(port > 0 && port < 65535) { server_port = port; }
      memcpy(server_address, buf, sizeof(server_address));
      return true;
    }
    return false;
}

void process_config_input(String &str) {

      if(str[0] == 'T') {
        // epoc time
        // T1573615730
        count = str.substring(1).toInt()+3;
      }
      else if(str[0] == 'I') {
        // interval (seconds)
        //I10
        interval = str.substring(1).toInt();
      }
      else if(str[0] == 'N') {
        // name string
        // N+arduino001
        str.substring(1).toCharArray(sID, sizeof(sID));
        update_eeprom();
      }
#ifdef ESP32
      else if(str[0] == 'C') {
        // connect to IP:port
        // C192.168.1.104:54000

        char buf[sizeof server_address];
        memset(buf, 0, sizeof(buf));

        input.substring(1).toCharArray(buf, sizeof(buf));

        if(set_server_address_and_port(buf, sizeof(buf))) {
          if(wifi_out) { 
            wifi_close_connection();
          }
          init_wifi();  
        }
      }
#endif
      else if(input[0] == '+' && input[1] == 'A' && input[2] == '0') {
        // +A0
        soil_found = true;
      }
}


void loop() {

    /////////////////// sensor reads ////////////////

    if(sht31_found) {

      temperature = sht31.readTemperature();
      humidity = sht31.readHumidity();

    }

    if(tsl_found) {

      //If event.light = 0 lux the sensor is probably saturated
      //and no reliable data could be generated!
      sensors_event_t event;
      tsl.getEvent(&event);
      lux = event.light;

      tsl.getLuminosity(&bb_lum, &ir_lum);
    }

    if(soil_found) {
      soil_moisture = map(analogRead(soil_pin), 0, 3505, 100, 0);
    }

#ifdef DS18B20
    if(ds18b20_found) {

      temp_sensor.requestTemperatures();
      ds_temperature = temp_sensor.getTempC(thermometer_address);

    }
#endif
  
    ////////////////// data outputs ///////////////

    if((++count % interval) == 0) {
      
    con_print(sID);

    bool append = false;
    con_print(F(" {"));

    if(count > 0) {
      con_print(F(JS("time")":"));
      con_print(count);
      append = true;
    }

    if(sht31_found) {

      if(append) output_field_seperator();
      
      con_print(F(JS("temp")":"));
      if(isnan(temperature)) { con_print(F(JS("NaN"))); }
      else { con_print(temperature); }

      output_field_seperator();

      Serial.print(F(JS("hum")":"));
      if(isnan(humidity)) { con_print(F(JS("NaN"))); }
      else { con_print(humidity); }
      append = true;
    }

    if(tsl_found) {

      if(append) output_field_seperator();

      con_print(F(JS("bb_lum")":"));
      con_print(bb_lum);

      output_field_seperator();

      con_print(F(JS("ir_lum")":"));
      con_print(ir_lum);

      output_field_seperator();

      con_print(F(JS("lux")":"));
      con_print(lux);
      append = true;
    }

    if(soil_found) {

      if(append) output_field_seperator();

      con_print(F(JS("soil")":"));
      con_print(soil_moisture);
      append = true;
    }

#ifdef DS18B20
    if(ds18b20_found) {
    
      if(append) output_field_seperator();

      con_print(F(JS("ds_temp")":"));
      con_print(ds_temperature);
      append = true;
    }
#endif

    con_println(F("}"));

    } //end interval

    ///////////////////// serial inputs ////////////

    if(Serial.available() > 0) {
      input = Serial.readString();
      input.trim();
      process_config_input(input);
    }

    //////////////////// wifi inputs ///////////////


        
    delay(943);
}
