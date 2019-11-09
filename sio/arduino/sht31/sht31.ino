
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
#include <EEPROM.h>

char sID[20] = "#arduino";

Adafruit_SHT31 sht31 = Adafruit_SHT31();

String input;
volatile unsigned long count = 0;

bool sht31_found = false;
float temperature = 0;
float humidity = 0;


void init_sht31() {

    sht31_found = sht31.begin(0x44);
    if(!sht31_found) {
      Serial.println("No SHT31 sensor found");
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
      EEPROM.update(addr, sID[addr]);
      // don't reprogram more than we must!
      if(sID[addr] == '\0') break;
    }
}

void setup() {
    Serial.begin(9600);
    while(!Serial) { }
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    read_eeprom();
    init_sht31();
}

void output_field_seperator() {
  Serial.print(F(","));
}

void loop() {
  
    Serial.print(sID);

    bool append = false;

    ////////////////// serial outputs///////////////
    Serial.print(F(" {"));

    if(count > 0) {
      Serial.print(F("time:"));
      Serial.print(++count);
      append = true;
    }

    if(sht31_found) {

      temperature = sht31.readTemperature();
      humidity = sht31.readHumidity();

      if(append) output_field_seperator();
      
      Serial.print(F("temperature:"));
      if(isnan(temperature)) { Serial.print(F("NaN")); }
      else { Serial.print(temperature); }

      output_field_seperator();

      Serial.print(F("humidity:"));
      if(isnan(humidity)) { Serial.print(F("NaN")); }
      else { Serial.print(humidity); }
      append = true;

    }

    Serial.println(F("}"));

    ///////////////////// serial inputs ////////////
    if(Serial.available() > 0) {
      input = Serial.readString();
      input.trim();
      if(input[0] == 'N') {
        input.substring(1).toCharArray(sID, sizeof(sID));
        update_eeprom();
      }
      if(input[0] == 'T') {
        count = input.substring(1).toInt()+3;
      }
    }
        
    delay(998);
}
