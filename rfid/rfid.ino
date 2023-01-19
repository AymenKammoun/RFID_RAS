#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <ArduinoJson.h>
#include "Adafruit_Keypad.h"

unsigned long wakeUpPeriod = 100000;

//KEY PAD VARIABLES
const byte ROWS = 4; // rows
const byte COLS = 4; // columns
//define the symbols on the buttons of the keypads
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {16, 5, 4, 0}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {2, 14, 12, 13}; //connect to the column pinouts of the keypad

Adafruit_Keypad customKeypad = Adafruit_Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS);


//WIFI VARIABLES
const char* ssid = "SiAymen";
const char* pwd = "12345678";
const String url = "https://node-js-test-9u9r.onrender.com/";
const String codes[4] = {"1234", "5678", "0000"};
ESP8266WiFiMulti WiFiMulti;

unsigned long lastms=0;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(ssid, pwd);
  customKeypad.begin();
}

void loop() {
  char c = 0;
  customKeypad.tick();
  while (customKeypad.available()) {
    keypadEvent e = customKeypad.read();
    if (e.bit.EVENT == KEY_JUST_PRESSED){
      c = (char)e.bit.KEY;
    }
  }
  if ((WiFiMulti.run() == WL_CONNECTED)) {
    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
    client->setInsecure();
    HTTPClient https;
    if(c){
      int codeIndex = c-'0'-1;
      String memberCode = codes[codeIndex];
      Serial.println(memberCode);
      
      String name = "";
      boolean isPresent = false;
      if (getMemberByCode(client, https, "get-member", memberCode, name, isPresent )) {
        if (name != "") {
          Serial.print("name : ");
          Serial.println(name);
          Serial.print("is Present: ");
          Serial.println(isPresent);
          setPresence(client, https, "set-presence", memberCode, !isPresent);
        } else {
          Serial.println("Member not found !");
        }
      } else {
        Serial.println("[HTTPS] Unable to connect\n");
      }
      delay(2000);
      Serial.println("waiting for other code");
    }


    // periodic wake up of the server.
    if((millis()-lastms)>=wakeUpPeriod)
    {
      lastms=millis();
      Serial.println("waking up the server !");
      periodicGet(client, https, "get-present");
    }
  }
  delay(10);
}






boolean getMemberByCode(std::unique_ptr<BearSSL::WiFiClientSecure>& client, HTTPClient & https,
                        String route, String code, String & name, boolean & isPresent) {
  if (https.begin(*client, url + route)) {
    https.addHeader("Content-Type", "application/json");
    String requestBody = "{\"code\":\"" + code + "\"}";
    int httpCode = https.POST(requestBody);
    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String response = https.getString();
        StaticJsonDocument<200> responseBody;
        DeserializationError error = deserializeJson(responseBody, response);
        if (responseBody["success"]) {
          name = (String)responseBody["data"]["name"];
          isPresent = (boolean)responseBody["data"]["is_present"];
        } else {
          name = "";
        }
      }
    } else {
      Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
    }

    https.end();
    return true;
  }
  return false;
}

boolean setPresence(std::unique_ptr<BearSSL::WiFiClientSecure>& client, HTTPClient & https,
                    String route, String code, boolean newPresence) {
  if (https.begin(*client, url + route)) {
    https.addHeader("Content-Type", "application/json");
    String requestBody = "{\"code\":\"" + code + "\", \"is_present\":" + newPresence + "}";
    int httpCode = https.POST(requestBody);
    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String response = https.getString();
        StaticJsonDocument<200> responseBody;
        DeserializationError error = deserializeJson(responseBody, response);
        Serial.println((String)responseBody["message"]);
      }
    } else {
      Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
    }

    https.end();
    return true;
  }
  return false;
}

void periodicGet(std::unique_ptr<BearSSL::WiFiClientSecure>& client, HTTPClient & https,
                    String route) {
    if(https.begin(*client, url + route)){
      int httpCode = https.GET();
      if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String response = https.getString();
          StaticJsonDocument<200> responseBody;
          DeserializationError error = deserializeJson(responseBody, response);
          Serial.println((String)responseBody["message"]);
        }
      } else {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }
  
      https.end();
    }
}
