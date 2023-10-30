/* ============================================
  KEEP THIS INFORMATION IF YOU USE THIS CODE

MIT License

Copyright (c) 2023 dbocktaels

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
  ===============================================*/

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include "LGFX.hpp"
#include "WiFi.h"
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <WiFiUdp.h>
#include <NTPClient.h>


#define LED_PIN 3

// setting PWM properties
const int freq = 5000;
const int ledChannel = 0;
const int resolution = 8;

static const uint32_t screenWidth = 240;
static const uint32_t screenHeight = 240;

const char* ssid = "xxxxxxxxx";
const char* password = "xxxxxxxxxx";

const uint32_t refresh_sec = 3600; // wait for 1 hour

const char * idRTE = "Basic xxxxxxxxx==";

const char* oauthURI   = "https://digital.iservices.rte-france.com/token/oauth/";
const char* tempoURI   = "https://digital.iservices.rte-france.com/open_api/tempo_like_supply_contract/v1/tempo_like_calendars";
const char* sandboxURI = "https://digital.iservices.rte-france.com/open_api/tempo_like_supply_contract/v1/sandbox/tempo_like_calendars";

//String sample_answer = "{"tempo_like_calendars":[{"start_date":"2016-03-12T00:00:00+01:00","end_date":"2016-03-13T00:00:00+01:00","values":{"start_date":"2016-03-12T00:00:00+01:00","end_date":"2016-03-13T00:00:00+01:00","value":"BLUE","updated_date":"2016-03-11T10:20:00+01:00"}}]}"

LGFX lcd;

String token;
int token_expire=0;

String today_date;
String today_color;
String tomorrow_date;
String tomorrow_color;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);


#define LEAP_YEAR(Y)     ( (Y>0) && !(Y%4) && ( (Y%100) || !(Y%400) ) )
static const uint8_t monthDays[]={31,28,31,30,31,30,31,31,30,31,30,31};

// Based on https://github.com/PaulStoffregen/Time/blob/master/Time.cpp
// currently assumes UTC timezone, instead of using this->_timeOffset
String getFormattedDate(unsigned long secs) {
  unsigned long rawTime = secs / 86400L;
  unsigned long days = 0, year = 1970;
  uint8_t month;

  while((days += (LEAP_YEAR(year) ? 366 : 365)) <= rawTime)
    year++;
  rawTime -= days - (LEAP_YEAR(year) ? 366 : 365); // now it is days in this year, starting at 0
  days=0;
  for (month=0; month<12; month++) {
    uint8_t monthLength;
    if (month==1) { // february
      monthLength = LEAP_YEAR(year) ? 29 : 28;
    } else {
      monthLength = monthDays[month];
    }
    if (rawTime < monthLength) break;
    rawTime -= monthLength;
  }
  String monthStr = ++month < 10 ? "0" + String(month) : String(month); // jan is month 1  
  String dayStr = ++rawTime < 10 ? "0" + String(rawTime) : String(rawTime); // day of month  
  return String(year) + "-" + monthStr + "-" + dayStr;
}


// obtention d'une transcription des codes d'erreurs spécifique à l'API RTE ou message général
String errorDescription(int code, HTTPClient& http) {
  switch (code) {
    case 401: return "l'authentification a échouée";
    case 403: return "l'appelant n’est pas habilité à appeler la ressource";
    case 413: return "La taille de la réponse de la requête dépasse 7Mo";
    case 414: return "L'URI transmise par l’appelant dépasse 2048 caractères";
    case 429: return "Le nombre d’appel maximum dans un certain laps de temps est dépassé";
    case 509: return "L'ensemble des requêtes des clients atteint la limite maximale";
    default: return String(code);
    break;
  }
  return http.errorToString(code);
}

// récupération du token d'authentification
bool getAuthToken(){
  bool result=true;
  HTTPClient http;

  //http.begin(client, oauthURI);
  http.begin(oauthURI);

  // Specify content-type header
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.addHeader("Authorization", idRTE);

  // Send HTTP POST request
  int codeReponseHTTP = http.POST(nullptr, 0);

  if (codeReponseHTTP == HTTP_CODE_OK) {
    String oauthPayload = http.getString();
    Serial.println(oauthPayload);
    
    JSONVar json = JSON.parse(oauthPayload);
    if (json.hasOwnProperty("access_token")) {
      String access_token = json["access_token"];
      token = access_token;
      Serial.println(token);
      //Nombre de secondes avant l�expiration de l�access token 
      int expire_date = json["expires_in"];
      //Serial.print("expire date : ");
      //Serial.println(expire_date);

      token_expire = expire_date - 60; // remove 1 min to renew token in advance
    }else{
      result=false;
    }
  }else{
    Serial.println("ERROR "+errorDescription(codeReponseHTTP, http));
    result=false;
  }
  http.end();
  return result;
}

bool getTempoToday(){
  bool result = true;
  int splitT=0;

  timeClient.update();
  unsigned long epochNow = timeClient.getEpochTime();
  unsigned long epochYesterday = timeClient.getEpochTime() - 86400L;
  unsigned long epochTomorrow = timeClient.getEpochTime() + 86400L;
  unsigned long epochTomorrowEnd = timeClient.getEpochTime() + 2*86400L;

  today_date = getFormattedDate(epochNow); 
  tomorrow_date = getFormattedDate(epochTomorrow);
  String tomorrowEnd_date = getFormattedDate(epochTomorrowEnd);
  String yesterday_date = getFormattedDate(epochYesterday); 

  Serial.println("GET tempo calendars : " + today_date + " -> " + tomorrowEnd_date);
  
  ///open_api/tempo_like_supply_contract/v1/tempo_like_calendars?start_date=2015-06-08T00:00:00%2B02:00&end_date=2015-06-11T00:00:00%2B02:00&fallback_status=true
  String url = tempoURI;
  url += "?start_date=" + yesterday_date + "T00:00:00+01:00";
  url += "&end_date=" + tomorrowEnd_date + "T00:00:00+01:00";;
  url += "&fallback_status=true";


  HTTPClient http;

  http.begin(url);

  String autorization = "Bearer " + token ;
  http.addHeader("Authorization", autorization);

  int codeReponseHTTP = http.GET();
  if (codeReponseHTTP == HTTP_CODE_OK) {
    String tempoStr = http.getString();
    Serial.println(tempoStr);

    JSONVar json = JSON.parse(tempoStr);
    if (json.hasOwnProperty("tempo_like_calendars")) {

      JSONVar values = json["tempo_like_calendars"]["values"];
      

      tomorrow_color="BLACK";
      today_color="BLACK";


      for (uint8_t i=0; i<values.length();i++){
        String date0 = values[i]["start_date"];
        String color0 = values[i]["value"];
        // check which value is first
        splitT = date0.indexOf("T");
        date0 = date0.substring(0, splitT);

        if (date0.equals(today_date)){
          today_color=color0;
        }else if(date0.equals(tomorrow_date)){
          tomorrow_color=color0;
        }

      }

      Serial.print("today : " + today_date);
      Serial.println(" color : " + today_color);
      Serial.print("tomorrow : " + tomorrow_date);
      Serial.println(" color : " + tomorrow_color);

    }else{
      Serial.println("no tempo_like_calendars in json");
    }

  }else{
    result = false;
    Serial.println("ERROR "+errorDescription(codeReponseHTTP, http));
  }
  http.end();
  return result;
}



void wifi_connect(){
  WiFi.mode(WIFI_MODE_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting");
  while(WiFi.status() != WL_CONNECTED){
      Serial.print(".");
      lcd.print(".");
      delay(100);
  }
  Serial.print("\nESP32 IP on the WiFi network: ");
  Serial.println(WiFi.localIP());
}



void updateScreen(){
  int bgcolor = TFT_BLACK;
  int nextcolor = TFT_BLACK;
  int textcolor = TFT_WHITE;
  int textnextcolor  = TFT_WHITE;
  int circlecolor  = TFT_WHITE;

  if (today_color.equals("BLUE")){
    bgcolor = TFT_BLUE;
  }else if (today_color.equals("RED")){
    bgcolor = TFT_RED;
  }else if (today_color.equals("WHITE")){
    bgcolor = TFT_WHITE;
    textcolor = TFT_BLACK;
  }

  if (tomorrow_color.equals("BLUE")){
    nextcolor = TFT_BLUE;
  }else if (tomorrow_color.equals("RED")){
    nextcolor = TFT_RED;
  }else if (tomorrow_color.equals("WHITE")){
    nextcolor = TFT_WHITE;
    textnextcolor = TFT_BLACK;
  }

  if ((bgcolor==TFT_WHITE)&&(nextcolor==TFT_WHITE)){
    circlecolor=TFT_BLACK;
  }

  lcd.fillScreen(bgcolor);

  lcd.fillCircle( 195, 142, 40, nextcolor);  
  lcd.drawCircle( 195, 142, 40, circlecolor); 

	lcd.setTextColor(textcolor,bgcolor);	
  lcd.setTextSize(2);
  lcd.setCursor(25, 90);
	lcd.println(F("Aujourd'hui"));
  
  lcd.setCursor(60, 50);
  lcd.setTextSize(2);
	lcd.println(today_date.c_str());

  lcd.setTextColor(textnextcolor,nextcolor);	
  lcd.setTextSize(2);
  lcd.setCursor(162, 131);
	lcd.println(F("demain"));
}

void initScreen(){
  lcd.fillScreen(TFT_BLACK);
	lcd.setCursor(90, 110);
	lcd.setTextColor(TFT_WHITE,TFT_BLACK);	
  lcd.setTextSize(2);
	lcd.println("Init");
}

void setup(){

  Serial.begin(115200);
  delay(200);
  Serial.println("Application RTE Tempo LCD");
  
  //LCD screen 
  lcd.init();
  lcd.initDMA();
  lcd.startWrite();

  lcd.setColorDepth(16);  // RGB565

  initScreen();

  // Backlight
  ledcSetup(ledChannel, freq, resolution);
  ledcAttachPin(LED_PIN, ledChannel);
  ledcWrite(ledChannel, 48);


  wifi_connect();
  // Initialize a NTPClient to get time
  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  timeClient.setTimeOffset(3600);  // GMT +1 = 3600

  while(!timeClient.update()) {
    timeClient.forceUpdate();
  }
}

void loop() {
//Il est conseillé de faire un appel par jour à ce service vers 10h40 et de ne pas dépasser une période de 366 jours par appel.

  if (token_expire <= 0){
    // get a new token
    getAuthToken();
  }
  getTempoToday();
  // this part is just for testing the screen config and overwrite colors
  if (Serial.available()){
    String teststr = Serial.readString();
    Serial.println(teststr);
    JSONVar json = JSON.parse(teststr);
    if (json.hasOwnProperty("today_date")){
      String str = json["today_date"];
      today_date = str;
    }
    if (json.hasOwnProperty("today_color")){
      String str =  json["today_color"];
      today_color = str;
    }
    if (json.hasOwnProperty("tomorrow_color")){
      String str = json["tomorrow_color"];
      tomorrow_color = str;
    }
  }
  updateScreen();
  delay(refresh_sec*1000);       // waits for a minute
  token_expire-=refresh_sec;   //remove 1 minute from counter
  //  Serial.println(token_expire);
}
