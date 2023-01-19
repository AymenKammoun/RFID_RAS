#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <ArduinoJson.h>


//const uint8_t fingerprint[20] = {0x40, 0xaf, 0x00, 0x6b, 0xec, 0x90, 0x22, 0x41, 0x8e, 0xa3, 0xad, 0xfa, 0x1a, 0xe8, 0x25, 0x41, 0x1d, 0x1a, 0x54, 0xb3};
const char* ssid = "SiAymen";
const char* pwd = "12345678";
const String url = "https://node-js-test-9u9r.onrender.com/";

ESP8266WiFiMulti WiFiMulti;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(ssid, pwd);
}

void loop() {
  if ((WiFiMulti.run() == WL_CONNECTED)) {

    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
    client->setInsecure();
    HTTPClient https;
    String name="";
    boolean isPresent = false;

    String memberCode = "1234";
    
    if(getMemberByCode(client, https, "get-present", memberCode, name, isPresent )){
      Serial.println(name);
      
      
    }else{
      Serial.print("[HTTPS] Unable to connect\n");

    }
    Serial.println("Wait 3s before next round...");
    delay(3000);
  }

  

  
}






boolean getMemberByCode(std::unique_ptr<BearSSL::WiFiClientSecure>& client, HTTPClient& https,
                          String route, String code, String& name, boolean& isPresent){
  if (https.begin(*client, url+route)) {  
      int httpCode = https.GET();
      if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = https.getString();
          StaticJsonDocument<200> doc;
          DeserializationError error = deserializeJson(doc, payload);
          name = (String)doc["data"]["name"];
          isPresent = (boolean)doc["data"]["is_present"];

             
        }
      } else {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }

      https.end();
      return true;
    } 
    return false;
}
