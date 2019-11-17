/*
   Copyright (c) 2019, Tom Oleson <tom dot oleson at gmail dot com>
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

 *   * Redistributions of source code must retain the above copyright notice,
       this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
 *   * The names of its contributors may NOT be used to endorse or promote
       products derived from this software without specific prior written
       permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
*/

#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_SHT31.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>
#include <EEPROM.h>

#define ESP32
#define DS18B20

#define DEVICE_PREFIX "eTOK"
#define JS(s) "" #s ""

struct __eeprom_data {
  char  sID[20] = "#arduino";
  char  ssid[33] =  "" ;
  char  password[33] =  "";
  char  server_address[20] = "";
  int   server_port = -1;
} eeprom_data ;

// hostname and mac address for device
// no need to store in eeprom
uint8_t mac_address[6] = { '\0' };
char hostname[33] = { '\0' };

#ifdef ESP32
#include <WiFi.h>
WiFiClient client;
#endif

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

#ifdef ESP32
struct cm_buffer {

  size_t len;             /* bytes written to buffer */
  size_t sz;              /* buffer size */
  char *buf = nullptr;    /* buffer address */

  cm_buffer(char *_buf, size_t _sz): buf(_buf), sz(_sz) {
    clear();
  }

  inline size_t available() {
    return sz - len;
  }
  inline size_t size() {
    return sz;
  }
  inline size_t length() {
    return len;
  }

  inline void clear() {
    len = 0;
    buf[0] = '\0';
  }

  inline void append(char ch) {
    if (len < sz) {
      buf[len++] = ch;
      buf[len] = '\0';
    }
  }

  inline void append(const char *s, size_t s_sz) {

    size_t n = ( s_sz < available() ? s_sz : available() - 1);
    memcpy(&buf[len], s, n);
    len += n;
    buf[len] = '\0';

  }

  // append C-string to buffer
  inline void append(const char *s) {
    append(s, strlen(s));
  }

  inline void append(int i) {
    size_t n = available();
    int ss = snprintf(&buf[len], n, "%d", i);
    len += (ss >= n ? n : ss );
  }

  inline void append(unsigned int i) {
    size_t n = available();
    int ss = snprintf(&buf[len], n, "%u", i);
    len += (ss >= n ? n : ss );
  }

  inline void append(long l) {
    size_t n = available();
    int ss = snprintf(&buf[len], n, "%ld", l);
    len += (ss >= n ? n : ss );
  }

  inline void append(unsigned long l) {
    size_t n = available();
    int ss = snprintf(&buf[len], n, "%lu", l);
    len += (ss >= n ? n : ss );
  }

  inline void append(double f) {
    size_t n = available();
    int ss = snprintf(&buf[len], n, "%f", f);
    len += (ss >= n ? n : ss);
  }

  inline void append(const __FlashStringHelper *ifsh) {
    append(reinterpret_cast<const char *>(ifsh));
  }
};

// provide a buffer we can control since ESP32 API is brain dead in this area!
char tx_buf[1024];
cm_buffer tx_buffer(tx_buf, sizeof(tx_buf));

bool wifi_out = false;
#undef F
#define F(s) (s)

#endif  //ESP32

#ifdef ESP32
inline void con_flush() {
  if(wifi_out) {
    client.print(tx_buf);
    client.flush();
    tx_buffer.clear();
  } 
  else {
    Serial.flush(); 
  }
}
#define con_print(s)  if(wifi_out) tx_buffer.append((s)); else Serial.print((s))
#define con_println(s)  if(wifi_out) { tx_buffer.append((s)); tx_buffer.append("\r\n"); con_flush(); }\
  else Serial.println((s))
#else
#define con_flush() Serial.flush()
#define con_print(s) Serial.print((s))
#define con_println(s) Serial.println((s))
#endif //ESP32

#define TOK_LOGGER
#ifdef TOK_LOGGER
#define _log_flush() con_flush()
#define _log_name() con_print(eeprom_data.sID); con_print(F(" {"))
#define _log_sep() con_print(F(","))
#define _log_time() con_print(JS("time")":"); con_print(count)
#define _log_lvl(s) con_print(F(JS(s)":"))
#define _log_msg(s)  con_print("\""); con_print((s)); con_print("\"")
#define _log_end() con_println(F("}"))

#define log_info(s) { _log_name(); _log_time(); _log_sep(); _log_lvl("info"); _log_msg((s)); _log_end(); }
#define log_warn(s) { _log_name(); _log_time(); _log_sep(); _log_lvl("warn"); _log_msg((s)); _log_end(); }
#define log_error(s) { _log_name(); _log_time(); _log_sep(); _log_lvl("error"); _log_msg((s)); _log_end(); }
#else
#define log_info(s) 
#define log_warn(s)
#define log_warn(s)

#endif

////////////////////////// EEPROM ///////////////////////////////

void read_eeprom() {
  // if not virgin EEPROM
  if (EEPROM.read(0x00) != 0xff) {
    Serial.println(F("Reading EEPROM"));
    char *sp = (char *) &eeprom_data;
    for (int addr = 0; addr < sizeof(eeprom_data); addr++) {
      sp[addr] = EEPROM.read(addr);
    }
  }
}

void update_eeprom() {
  Serial.println(F("Writing EEPROM"));
  char *sp = (char *) &eeprom_data;
  for (int addr = 0; addr < sizeof(eeprom_data); addr++) {
    // don't reprogram more than we must!
    if (EEPROM.read(addr) != sp[addr])
      EEPROM.write(addr, sp[addr]);
  }
#ifdef ESP32
  EEPROM.commit();
#endif
}

////////////////////////// WiFi ///////////////////////////////

#ifdef ESP32
void wifi_close_connection() {
  client.stop();
  wifi_out = false;
}

void check_wifi_connection() {

  // if no wifi, re-initialize
  if (WiFi.status() != WL_CONNECTED) {
    wifi_close_connection();
    init_wifi();
    return;
  }

  // ESP32 API fails to expose the reconnect method in WiFiClient! WTF?!!
  // if we lost server connection, attempt reconnect
  for(int timeout = 6; !client.connected() && timeout > 0; timeout--) {
    if(client.connect(eeprom_data.server_address, eeprom_data.server_port)) return;
    delay(10000);
  }

  // if we are not connected to the server now, re-initialize
  if(!client.connected()) {
    wifi_close_connection();
    init_wifi();
    return;
  }
}

void init_wifi() {

retry:
  if(WiFi.status() != WL_CONNECTED && eeprom_data.ssid[0]) {
    Serial.print(F("Attempting to connect to "));
    Serial.print(eeprom_data.ssid);

    // setup unique hostname for this device
    WiFi.macAddress(mac_address);
    sprintf(hostname,  DEVICE_PREFIX "-%02x%02x%02x%02x%02x%02x",
            mac_address[0], mac_address[1], mac_address[2],
            mac_address[3], mac_address[4], mac_address[5]);

    WiFi.disconnect();
    WiFi.setHostname(hostname);
    WiFi.mode(WIFI_STA);
    WiFi.begin(eeprom_data.ssid, eeprom_data.password, 0, NULL, true);
    WiFi.setHostname(hostname);

    int timeout = 120; // seconds
    while (WiFi.status() != WL_CONNECTED && --timeout > 0) {
      delay(500);
      Serial.print(F("."));
      delay(500);
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      Serial.print(F("WiFi connected as local IP address: "));
      Serial.println(WiFi.localIP());
    }
    else {
      Serial.println(F("Timed out. WiFi not available. Restarting..."));
      //goto retry;
      ESP.restart();
    }

    WiFi.setHostname(hostname);

  }
  if (eeprom_data.server_address[0] && eeprom_data.server_port > 0) {
    Serial.print(F("Connecting to "));
    Serial.println(eeprom_data.server_address);
    delay(1000);

    if (!client.connect(eeprom_data.server_address, eeprom_data.server_port)) {
      Serial.println(F("Connection failed"));
    }
    else {
      // missing API in ESP32!  No way to control flush on write!
      //client.setDefaultSync(false);
      Serial.println(F("Connection successful"));
      wifi_out = true;

      char buf[80];
      //snprintf(buf, sizeof(buf), "+hello 'Hello from %s!'", hostname);
      //con_println(buf);
      snprintf(buf, sizeof(buf), "Hello from eTOK: %s", hostname);
      log_info(buf);
      
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

  if (ds18b20_found) {

    // set resolution bits (9-12)
    temp_sensor.setResolution(thermometer_address, 11);

    // print device address
    //Serial.print(F("DS1820B device address:"));
    //for (uint8_t i = 0; i < 8; i++) {
      //if (thermometer_address[i] < 16) Serial.print(F("0"));
      //Serial.print(thermometer_address[i], HEX);
    //}
    //Serial.println();
    
    char buf[45];
    snprintf(buf, sizeof(buf), "DS1820B device address: %02x%02x%02x%02x%02x%02x%02x%02x",
          thermometer_address[0], thermometer_address[1], thermometer_address[2], thermometer_address[3],
          thermometer_address[4], thermometer_address[5], thermometer_address[6], thermometer_address[7]);
    log_info(buf);
  }
  else {
    //Serial.println(F("No DS1820B sensor found"));
    log_info(F("No DS1820B sensor found"));
  }
}
#endif

void init_sht31() {

  sht31_found = sht31.begin(0x44);
  if (!sht31_found) {
    //Serial.println(F("No SHT31 sensor found"));
    log_info(F("No SHT31 sensor found"));
  }
}

void init_tsl2561() {

  tsl_found = tsl.begin();
  if (!tsl_found) {
    //Serial.println(F("No TSL2516 sensor found"));
    log_info(F("No TSL2516 sensor found"));
  }
  else {

    // Set the gain or enable auto-gain support

    // No gain: use in bright light
    //tsl.setGain(TSL2561_GAIN_1X);

    // 16x gain: use in low light
    //tsl.setGain(TSL2561_GAIN_16X);

    // Auto-gain: auto switch between 1x and
    tsl.enableAutoRange(true);
    //Serial.println(F("TLS Gain: Auto"));
    log_info(F("TLS Gain: Auto"));

    // Changing integration time gives better sensor
    // resolution (402ms = 16-bit data)

    // fast but low resolution
    tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS); // fast but low resolution
    //Serial.println(F("TLS Timing: 13 ms"));
    log_info(F("TLS Timing: 13 ms"));

    // medium resolution and speed
    // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_101MS);

    // 16-bit data but slowest conversions
    // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);  // 16-bit

  }
}
//////////////////////////////////// setup ////////////////////////////////////

void setup() {
  Serial.begin(9600);
  while (!Serial) { }
  delay(500);

#ifndef ESP32
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
#endif

#ifdef ESP32
  EEPROM.begin(1024);
#endif

  con_println(F(DEVICE_PREFIX " build: " __DATE__ " " __TIME__));

  // useful for taking it back to "new"
  //memset(&eeprom_data, 0, sizeof(eeprom_data));
  //update_eeprom();

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

#define VORTEX_TIME_WATCH
#ifdef VORTEX_TIME_WATCH
  con_println(F("*T #T"));
#endif

  char buf[40];
  snprintf(buf, sizeof(buf), "hello from eTOK: %s", eeprom_data.sID+1);
  log_info(buf);

  
}

void output_field_seperator() {
  con_print(F(","));
}

/////////////////////////////////// config API ///////////////////////////////////

bool set_server_address_and_port(char *buf, size_t sz) {

  // parse out server address and port
  // format of: ip_address:port\0

  char *cp = strchr(buf, ':');
  if (NULL != cp && cp < (buf + sz)) {
    *cp++ = '\0';
    unsigned int port = atoi(cp);
    if (port > 0 && port < 65535) {
      eeprom_data.server_port = port;
    }
    memcpy(eeprom_data.server_address, buf, sizeof(eeprom_data.server_address));
    update_eeprom();
    return true;
  }
  return false;
}

bool set_ssid_and_password(char *buf, size_t sz) {

  // parse out server address and port
  // format of: ssid:password\0

  char *cp = strchr(buf, ':');
  if (NULL != cp && cp < (buf + sz)) {
    *cp++ = '\0';
    memcpy(eeprom_data.password, cp, sizeof(eeprom_data.password));
    memcpy(eeprom_data.ssid, buf, sizeof(eeprom_data.ssid));
    update_eeprom();
    return true;
  }
  return false;
}

void normalize_input() {
  // format:  key:value
  int colon_index = input.indexOf(':');
  if (colon_index > 0 && input.length() > colon_index + 1) {
    String key = input.substring(0, colon_index);
    String val = input.substring(colon_index + 1);
    // format for config input:  T:000000  becomes T000000
    input = key + val;
  }
}

void process_config_input(String &str, bool safe_source) {

  if (str[0] == 'T') {
    // epoch time
    // T1573615730
    count = str.substring(1).toInt();
    log_info(str.c_str());
  }
  else if (str[0] == 'I') {
    // interval (seconds)
    //I10
    interval = str.substring(1).toInt();
    log_info(str.c_str());
  }
  else if (str[0] == 'N') {
    // name string
    // N+arduino001
    str.substring(1).toCharArray(eeprom_data.sID, sizeof(eeprom_data.sID));
    update_eeprom();
    log_info(str.c_str());
  }
#ifdef ESP32
  else if (str[0] == 'C' && safe_source) {
    // connect to IP:port
    // C192.168.1.104:54000

    char buf[sizeof eeprom_data.server_address + 6];
    memset(buf, 0, sizeof(buf));

    str.substring(1).toCharArray(buf, sizeof(buf));

    if (set_server_address_and_port(buf, sizeof(buf))) {
      log_info(str.c_str());
      if (wifi_out) {
        wifi_close_connection();
      }
      init_wifi();
    }
  }
  else if (str[0] == 'W' && safe_source) {
    // connect to SSID:password
    // WSSID:password

    char buf[sizeof eeprom_data.ssid + sizeof eeprom_data.password];
    memset(buf, 0, sizeof(buf));

    str.substring(1).toCharArray(buf, sizeof(buf));

    if (set_ssid_and_password(buf, sizeof(buf))) {
      // note: do NOT send SSID and password out in the clear, ever!!
      log_info(F("SSID:password set"));
      if (wifi_out) {
        wifi_close_connection();
      }
      init_wifi();
    }

  }
#endif
  else if (str[0] == '+' && str[1] == 'A' && str[2] == '0') {
    // +A0
    soil_found = true;
    log_info(str.c_str());
  }
}

void loop() {
  ////////////////////// clock /////////////////////

  static unsigned long clock = 0;
  unsigned long now = count;
  while(now == count) {
    if((millis() - clock) >= 1000) {
      clock = millis();
      //Serial.print(millis()); Serial.println(" ms");
      count++;
  }
  
  ///////////////////// serial inputs ////////////

  if (Serial.available() > 0) {
    input = Serial.readString();
    input.trim();
    normalize_input();
    process_config_input(input, true);
  }

  //////////////////// wifi inputs ///////////////
#ifdef ESP32
  if (client.connected() && client.available() > 0) {
    input = client.readString();
    input.trim();
    normalize_input();
    process_config_input(input, false);
  }
#endif
    
  } // end clock

  /////////////////// sensor reads ////////////////

  if((count % 2) == 0) {
    if (sht31_found) {

      temperature = sht31.readTemperature();
      humidity = sht31.readHumidity();

    }

    if (tsl_found) {

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
  }
  ////////////////// data outputs ///////////////

  if ((count % interval) == 0) {

#ifdef ESP32
    if (wifi_out) check_wifi_connection();
#endif

    con_print(eeprom_data.sID);

    bool append = false;
    con_print(F(" {"));

    if (count > 0) {
      con_print(F(JS("time")":"));
      con_print(count);
      append = true;
    }

    if (sht31_found) {

      if (append) output_field_seperator();

      con_print(F(JS("temp")":"));
      if (isnan(temperature)) {
        con_print(F(JS("NaN")));
      }
      else {
        con_print(temperature);
      }

      output_field_seperator();

      con_print(F(JS("hum")":"));
      if (isnan(humidity)) {
        con_print(F(JS("NaN")));
      }
      else {
        con_print(humidity);
      }
      append = true;
    }

    if (tsl_found) {

      if (append) output_field_seperator();

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

    if (soil_found) {

      if (append) output_field_seperator();

      con_print(F(JS("soil")":"));
      con_print(soil_moisture);
      append = true;
    }

#ifdef DS18B20
    if (ds18b20_found) {

      if (append) output_field_seperator();

      con_print(F(JS("ds_temp")":"));
      con_print(ds_temperature);
      append = true;
    }
#endif

    con_println(F("}"));
    con_flush();

  } //end interval


}
