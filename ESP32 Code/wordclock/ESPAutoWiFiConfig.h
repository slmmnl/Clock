#ifndef ESP_AUTO_WIFI_CONFIG_H
#define ESP_AUTO_WIFI_CONFIG_H
// for Stream
#include <Arduino.h>  

/**
   ESPAutoWiFiConfig.h
   by Matthew Ford,  2022/04/07
   See the tutorial at https://www.forward.com.au/pfod/ESPAutoWiFiCofig/index.html

   This file needs the SafeString library.  Install from the Arduino library manager.
   
   (c)2022 Forward Computing and Control Pty. Ltd.
   NSW, Australia  www.forward.com.au
   This code may be freely used for both private and commerical use.
   Provide this copyright is maintained.
*/
// 
/**
    call this from near the top of setup() with
  if (ESPAutoWiFiConfigSetup(13,true,0)) {
      return;
   }
   // rest of setup
*/
// if ESPAutoWiFiConfigSetup() returns true the in AP mode waiting for connection to set WiFi SSID/pw/ip settings
// ledPin is the output that drives the indicator led, highForLedOn is true if +volts turns led on, else false if 0V out turns led on
// EEPROM_offset 0 unless you are using EEPROM in your code in which case pass in the size of the EEPROM your code uses
// and ESPAutoWiFiConfig will EEPROM addresses after that.
bool ESPAutoWiFiConfigSetup(int ledPin, bool highForLedOn, size_t EEPROM_offset);

/**
    call this from near the top of loop() with
  void loop() {
   if (ESPAutoWiFiConfigLooptoConfigLoop()) {
      return;
   }
   // rest of loop
*/
// if ESPAutoWiFiConfigLoop() returns true the in AP mode processing setting WiFi SSID/pw/ip settings
bool ESPAutoWiFiConfigLoop();


// call this to enable debug out for the ESPAutoWiFiConfig code
void setESPAutoWiFiConfigDebugOut(Stream &out);

#endif
