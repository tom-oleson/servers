
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

#include <EEPROM.h>

char sID[20] = "#arduino";

String name;
unsigned long count = 0;

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
  pinMode(LED_BUILTIN, OUTPUT);
  read_eeprom();
}
void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.print(sID);
  Serial.print(F(" {count:"));
  Serial.print(++count);
  Serial.println(F("}"));
  if(Serial.available() > 0) {
    name = Serial.readString();
    name.trim();
    name.toCharArray(sID, sizeof(sID));
    update_eeprom();
  }
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
}
