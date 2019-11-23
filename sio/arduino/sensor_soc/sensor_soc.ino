#define ESP32


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

#define DS18B20
#define VORTEX_WATCH

#define DEVICE_PREFIX "ETOK"
#define JS(s) "" #s ""

struct __eeprom_data {
  char  sID[20] = "#arduino";
  char  ssid[33] =  "" ;
  char  password[33] =  "";
  char  server_address[20] = "";
  int   server_port = -1;
} eeprom_data ;

// mac address for device
// no need to store in eeprom
uint8_t mac[6] = { '\0' };
char mac_address[18] = "00:00:00:00:00:00";
char device_name[20] = "";

int soil_pin = 0;
Adafruit_SHT31 sht31 = Adafruit_SHT31();
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);

String input;
volatile unsigned long count = 0;

#ifdef ESP32
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <WiFiAP.h>

const char *ap_ssid = "ETOK-AP";
const char *ap_password = "administrator";
char hostname[33] = "ETOK-STA";

String result;

WebServer server(80);
WiFiClient client;
bool wifi_out = false;

// ESP32 implementation of this macro is broken, treat as
// normal strings...
#undef F
#define F(s) (s)

#endif

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


/////////////////////// cm_buffer /////////////////////////////
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
    buf[len] = '\0';
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

  inline void append(String &s) {
    append(s.c_str());
  }

  inline void append(const String &s) {
    append(s.c_str());
  }

  inline void append(const __FlashStringHelper *ifsh) {
    append(reinterpret_cast<const char *>(ifsh));
  }
};


////////////////////////// logger ///////////////////////////////

#ifdef ESP32
// provide a buffer we can control since ESP32 API is brain dead in this area!
char tx_buf[1024];
cm_buffer tx_buffer(tx_buf, sizeof(tx_buf));

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
//#define _log_msg(s)  con_print("\""); con_print((s)); con_print("\"")
//#define _log_msg(s) con_print(JS(s)) 
#define _log_msg(s)  con_print('\"'); con_print((s)); con_print('\"')
#define _log_end() con_println(F("}"))

#define log_info(s) { _log_name(); _log_time(); _log_sep(); _log_lvl("info"); _log_msg((s)); _log_end(); }
#define log_warn(s) { _log_name(); _log_time(); _log_sep(); _log_lvl("warn"); _log_msg((s)); _log_end(); }
#define log_error(s) { _log_name(); _log_time(); _log_sep(); _log_lvl("error"); _log_msg((s)); _log_end(); }
#else
#define log_info(s) 
#define log_warn(s)
#define log_warn(s)
#endif  //TOK_LOGGER

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

////////////////////////// VORTEX ///////////////////////////////

#ifdef VORTEX_WATCH
void vortex_watch() {
  
  // time watch
  con_println(F("*T #T"));

  // broadcast watch
  con_println(F("*broadcast #$"));
  
  // notify watch
  // format: *name-notify #$name
  String name = String(eeprom_data.sID+1);
  String s = "*" + name + "-notify #$" + name;
  con_println(s);
  
//  strcpy(buf, "*");
//  strncat(buf, eeprom_data.sID+1, sizeof(eeprom_data.sID)-1);
//  strcat(buf, "-notify #$");
//  strncat(buf, eeprom_data.sID+1, sizeof(eeprom_data.sID)-1);
//  con_println(buf);
}
#endif // #define ESP32


////////////////////////// WiFi/WebServer ///////////////////////////////

#ifdef ESP32

void wifi_close_connection() {
  client.stop();
  wifi_out = false;
}

void check_wifi_connection() {

  //Serial.println("check_wifi_connection");

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
    delay(1000);
  }

  // if we are not connected to the server now, re-initialize
  if(!client.connected()) {
    wifi_close_connection();
    init_wifi();
    return;
  }
}

// collect results of init_wifi
char rs_buf[1048];
cm_buffer rs(rs_buf, sizeof(rs_buf));

#define x_print(ss) Serial.print((ss)); rs.append((ss)); result.concat((ss))
#define x_println(ss)  Serial.println((ss)); rs.append((ss)); rs.append("\n"); result.concat((ss)); result.concat("\n"); rs.clear()

void show_wifi_connected() {
    if (WiFi.status() == WL_CONNECTED) {
      x_print("WiFi connected as local IP address: ");
      x_println(WiFi.localIP().toString()); 
    }
}

bool init_wifi() {

    Serial.println("init_wifi");

    // setup unique hostname for this device
    WiFi.macAddress(mac);
    sprintf(mac_address,  "%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0], mac[1], mac[2],
            mac[3], mac[4], mac[5]);

    // this is a new device...
    if(eeprom_data.sID[0] == '#') {
        snprintf(eeprom_data.sID, sizeof(eeprom_data.sID),
               "+" DEVICE_PREFIX "-%02X%02X%02X%02X%02X%02X",
                mac[0], mac[1], mac[2],
                mac[3], mac[4], mac[5]);
    }

    snprintf(device_name, sizeof(eeprom_data)-1, eeprom_data.sID+1);
    
    show_wifi_connected();

    if((WiFi.status() != WL_CONNECTED) && eeprom_data.ssid[0]) {

      x_print("Attempting to connect to ");
      x_print(eeprom_data.ssid);
    
    WiFi.begin(eeprom_data.ssid, eeprom_data.password, 0, NULL, true);
    WiFi.setHostname(hostname);

    int timeout = 10; // seconds
    while (WiFi.status() != WL_CONNECTED && --timeout > 0) {
      delay(500);
      x_print(".");
      delay(500);
    }
    x_println("");

    if (WiFi.status() == WL_CONNECTED) {
      show_wifi_connected();
    }
    else {
      x_println("Timed out. Check SSID/password.");
      return false;
    }
    WiFi.setHostname(hostname);

  } // !WL_CONNECTED
  
  if (eeprom_data.server_address[0] && eeprom_data.server_port > 0) {
    x_print("Connecting to ");
    x_print(eeprom_data.server_address);
    x_print(":");
    x_println(eeprom_data.server_port);
    x_println("");
        
    if(!client.connect(eeprom_data.server_address, eeprom_data.server_port)) {
      x_println("Connect failed. Check Server IP and Port.");
      return false;
    }

    // missing API in ESP32!  No way to control flush on write!
    //client.setDefaultSync(false);
    x_println("Connection successful");
    wifi_out = true;

    char buf[60];
    snprintf(buf, sizeof(buf), "Hello from: %s", hostname);
    log_info(buf);

#ifdef VORTEX_WATCH
    vortex_watch();
#endif
    return true;
  }

  return false;
}

// safe version of strncpy; always ensures string is null terminated
size_t strlcpy(char * dst, const char * src, size_t max) {
  size_t sz = 0;
  while(sz < max - 1 && src[sz] != '\0'){
    dst[sz] = src[sz];
    sz++;
  }
  dst[sz] = '\0';
  return sz;
}

const char *config_page = R"(
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<meta http-equiv="refresh" content="120">
<title>ETOK-AP</title>
<style>
html {
  -webkit-text-size-adjust: 100%;
  -moz-text-size-adjust: 100%;
  -ms-text-size-adjust: 100%;
}
body { background-color:black; color: white; font-family: Arial, Helvetica, sans-serif; }<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
html {
  -webkit-text-size-adjust: 100%;
  -moz-text-size-adjust: 100%;
  -ms-text-size-adjust: 100%;
}
body { background-color:black; color: white; font-family: Arial, Helvetica, sans-serif; }
h1 { width: 100%; background-color: #007777; color:white; font-size:130%; }
input, textarea { height: 100%; width:100%; background-color: #353535;  color: white; font-size:100%; padding: 5px; }
td { color: white; font-size:100%; }

table { width: 100%; text-align: center;}

.container{ height:100%; }

.panel { padding-top: 5%; border: 1px solid gray; }
.input_panel { border: 1px solid gray;  }
.result_panel { height: 40%; border: 1px solid gray;  }

.input_name { height: 10%; padding-top: 5%;}
.text_area { font-size:110%; border: none; /*border-collapse: collapse;*/}

.input_button { height: 65%; width: 100%; background-color:#007777; margin-top: 5%; }

.result_panel, .panel, .input_panel { background-color: #353535; }
.divider { height: 20px; }
table {
  table-layout: fixed;
}

</style>
</head>
<body>
<h1>ETOK-AP  ({{device_name}})</h1>

<div class="container">

<div class="input_panel">
<form action='/config' method='POST'>
<table >
<tbody">
<tr >
<td class="input_name">Network (SSID)</td>
<td class="input_name">Network Password</td>
</tr>
<tr>
<td><input type='text' name='SSID' placeholder='ssid' value='{{ssid}}'></td>
<td><input type='password' name='PASSWORD' placeholder='password' value='{{ssid_password}}'></td>
</tr>
<tr>
<td class="input_name">Server IP</td>
<td class="input_name">Server Port</td>
</tr>
<tr>
<td><input type='text' name='SERVER_IP' placeholder='server_ip' value='{{server_ip}}'></td>
<td><input type='text' name='SERVER_PORT' placeholder='server_port' value='54000' value='{{server_port}}'></td>
</tr>
<tr>
<td> <input class="input_button" type='submit' name='CLEAR' value='Clear'></td>
<td> <input class="input_button" type='submit' name='UPDATE' value='Update'></td>
</tr>
</tbody>
</table>
</form>
</div>

<div class="divider"></div>

<div class="result_panel">
<textarea class="text_area">
{{result}}    
</textarea>
</div>

</div>

</body>
</html>
)";

// safe version of strncpy; always ensures string is null terminated
size_t _strlcpy(char * dst, const char * src, size_t max) {
  size_t sz = 0;
  while(sz < max - 1 && src[sz] != '\0'){
    dst[sz] = src[sz];
    sz++;
  }
  dst[sz] = '\0';
  return sz;
}

void handleConfig() {
  Serial.println("handleConfig");

  if(server.method() == HTTP_POST) {
    Serial.println("HTTP_POST");

    if(server.hasArg("CLEAR")) {
      result = "";
      rs.clear();
      eeprom_data.ssid[0] = '\0';
      eeprom_data.password[0] = '\0';
      eeprom_data.server_address[0] = '\0';
      eeprom_data.server_port = 54000;
    }
    else if(server.hasArg("UPDATE")) {

      result = "";
      rs.clear();

      __eeprom_data save_eeprom;
      memcpy(&save_eeprom, &eeprom_data, sizeof(save_eeprom));

      // get form data
      String _ssid = server.arg("SSID");
      String _ssid_password = server.arg("PASSWORD");
      String _server_ip = server.arg("SERVER_IP");
      String _server_port = server.arg("SERVER_PORT");

      bool do_update = true;

      if(_ssid == "") { result += "Missing Network (SSID)\n"; do_update = false; }
      if(_ssid_password == "") { result += "Missing Network Password\n"; do_update = false; }
      if(_server_ip == "") { result += "Missing Server IP\n"; do_update = false; }

      if(do_update) {
        // copy form data into our data structure
        _strlcpy(eeprom_data.ssid, _ssid.c_str(), sizeof(eeprom_data.ssid));
        _strlcpy(eeprom_data.password, _ssid_password.c_str(), sizeof(eeprom_data.password));
        _strlcpy(eeprom_data.server_address, _server_ip.c_str(), sizeof(eeprom_data.server_address));
        eeprom_data.server_port = atoi(_server_port.c_str());

        if(init_wifi()) {
          // successful, update the EEPROM
          update_eeprom();
        }
        else {
          // error, restore previous data
          memcpy(&eeprom_data, &save_eeprom, sizeof(eeprom_data));
          result = "There was a connection problem. Please check your settings.";
        }
      }
    }
    // redirect/load the page
    server.sendHeader("Location", "/config");
    server.sendHeader("Cache-Control", "no-cache");
    server.send(301);
    return;
  }
  else if(server.method() == HTTP_GET) {
    Serial.println("HTTP_GET");

    // get the template
    String content = String(config_page);

    content.replace("{{device_name}}", String(device_name));
    content.replace("{{ssid}}", String(eeprom_data.ssid));
    content.replace("{{ssid_password}}", String(eeprom_data.password));
    content.replace("{{server_ip}}", String(eeprom_data.server_address));
    content.replace("{{server_port}}", String(eeprom_data.server_port));
    content.replace("{{result}}", result);
    
    server.send(200, "text/html", content);
  }
}

void handleRoot() {
  Serial.println("handleRoot");
  server.sendHeader("Location", "/config");
  server.sendHeader("Cache-Control", "no-cache");
  server.send(301);
}

void wifi_ap_mode_setup() {

  Serial.println();
  x_println("Configuring Access Point...");

  WiFi.disconnect();
  WiFi.setHostname(ap_ssid);
  WiFi.softAPsetHostname(ap_ssid);
  WiFi.mode(WIFI_MODE_APSTA);
  WiFi.softAP(ap_ssid, ap_password);    
  
  IPAddress myIP = WiFi.softAPIP();
  x_print("AP IP address: ");
  x_println(myIP.toString());
  
  /// setup web server end points here ///
  server.on("/", handleRoot);
  server.on("/config", handleConfig);

  server.begin();

  x_println("HTTP server started");
}

#endif // #define ESP32

///////////////////////////// SOIL ///////////////////////////////////

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

////////////////////////// SHT31 ///////////////////////////////////

void init_sht31() {

  sht31_found = sht31.begin(0x44);
  if (!sht31_found) {
    //Serial.println(F("No SHT31 sensor found"));
    log_info(F("No SHT31 sensor found"));
  }
}

////////////////////////// TSL2561 /////////////////////////////////

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
  wifi_ap_mode_setup();
  init_wifi();
#endif

  init_soil();
  init_sht31();
  init_tsl2561();

#ifdef DS18B20
  init_ds18b20();
#endif

  char buf[60];
  
#ifdef VORTEX_WATCH
  vortex_watch();
#endif

  snprintf(buf, sizeof(buf), "hello from: %s", eeprom_data.sID+1);
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

void normalize_input(String &s) {

  //Serial.println(input);
  
  // format:  $:value (from notify)
  if(s.length() > 2 && s[0] == '$' && s[1] == ':') {
    String val = s.substring(2);
    // input remains the same T000000 stays T000000
    s = val;
    return;
  }
   
  // format:  key:value
  int colon_index = s.indexOf(':');
  if (colon_index > 0 && s.length() > colon_index + 1) {
    String key = s.substring(0, colon_index);
    String val = s.substring(colon_index + 1);
    // format: $name-notify:value
    if(key[0] == '$') {
        String name = key.substring(1);
        if(strcmp(name.c_str(), eeprom_data.sID+1) == 0) {
          s = val;      
        }
        else {
          // notify is NOT for this device, ignore the input!
          s = "";
        }
    }
    else {
      // format for config input:  T:000000  becomes T000000
      s = key + val;      
    }
  }
}
  
void evaluate(String &str, bool safe_source) {

  normalize_input(str);

  // ignore empty string
  if(!(str.length() > 0)) return;
  
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
    
    #ifdef VORTEX_WATCH
    vortex_watch();
    #endif
  }
#ifdef VORTEX_WATCH
  else if(str[0] == 'V') {
    char buf[60];

    // send built-in watch config
    vortex_watch();

    // send watch config in this request (if any)
    // format: V@name #tag
    // format: V*name #tag
    if(str.length() > 4 && str[1] == '*' || str[1] == '@') {
      str.substring(1).toCharArray(buf, sizeof(buf));
      con_println(buf);
    }
  }
#endif
  
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

// split input string on '\n' because client.readStringUntil('\n') is
// brain dead...
void process_config_input(String &s, bool safe_source) {

#ifdef ESP32
    char my_buf[1024] = {'\0' };
#else
    char my_buf[80] = {'\0' };
#endif    
    char *bp = my_buf;
    const char *cp = s.c_str();
    int index = 0;
    for(char *sp = bp; index < sizeof(my_buf); cp++ ) {
        if(*cp == '\0' || *cp == '\n') {
            String ns = String(sp);
            evaluate(ns, safe_source);
            sp = bp + 1;
        }
        if(*cp == '\0') break;
        *bp++ = *cp;
        index++;
    }
}

void loop() {

  // check the access point for a client
  server.handleClient();
  WiFiClient ap_client = server.client();
  if(ap_client.connected()) {
    // we are currently in AP mode
    return;
  }
  
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
    process_config_input(input, true);
  }

  //////////////////// wifi inputs ///////////////
#ifdef ESP32
  if (client.connected() && client.available() > 0) {
    input = client.readString();
    input.trim();
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
        con_print(F("NaN"));
      }
      else {
        con_print(temperature);
      }

      output_field_seperator();

      con_print(F(JS("hum")":"));
      if (isnan(humidity)) {
        con_print(F("NaN"));
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
} // end loop
