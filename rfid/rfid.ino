#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <ArduinoJson.h>
#include "Adafruit_Keypad.h"
#include <LiquidCrystal_I2C.h>


unsigned long wakeUpPeriod = 100000;
//LCD variables
int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);  



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
byte rowPins[ROWS] = {16, 10, 9, 0}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {2, 14, 12, 13}; //connect to the column pinouts of the keypad

Adafruit_Keypad customKeypad = Adafruit_Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS);


//WIFI VARIABLES
const char* ssid = "SiAymen";
const char* pwd = "12345678";
const String url = "https://node-js-test-9u9r.onrender.com/";
const String codes[5] = {"1234", "5678", "0000","4567","21944"};
ESP8266WiFiMulti WiFiMulti;

unsigned long lastms=0;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(ssid, pwd);
  customKeypad.begin();
  lcd.init();
  lcd.backlight();
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
//      Serial.println(c);
      int codeIndex = c-'0'-1;
      if(codeIndex < 0 || codeIndex >4){
        return;
      }
      String memberCode = codes[codeIndex];
//      Serial.println(memberCode);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Auth...");
      
      String name = "";
      boolean isPresent = false;
      if (getMemberByCode(client, https, "get-member", memberCode, name, isPresent )) {
        if (name != "") {
//          Serial.print("name : ");
//          Serial.println(name);
//          Serial.print("is Present: ");
//          Serial.println(isPresent);
          if(setPresence(client, https, "set-presence", memberCode, !isPresent)){
            String message = isPresent?"   Good Bye":"    Welcome ";
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(message);
            lcd.setCursor(0,1);
            lcd.print(name);
            delay(2000);
            lcd.clear();
          }
        } else {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Member not found!");
//          Serial.println("Member not found !");
          delay(2000);
          lcd.clear();
        }
      } else {
//        Serial.println("[HTTPS] Unable to connect\n");
      }
      lcd.setCursor(0,0);
      lcd.print("waiting for other code");
//      Serial.println("waiting for other code");
      delay(2000);
    }


    // periodic wake up of the server.
    if((millis()-lastms)>=wakeUpPeriod)
    {
      lastms=millis();
//      Serial.println("waking up the server !");
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
//      Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
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
//        Serial.println((String)responseBody["message"]);
      }
    } else {
//      Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
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
//          Serial.println((String)responseBody["message"]);
        }
      } else {
//        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }
  
      https.end();
    }
}
