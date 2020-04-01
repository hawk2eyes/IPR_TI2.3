//Librarys
#include <SPI.h>
#include <WiFiNINA.h>
#include <ArduinoHttpClient.h>
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

//Pin definitions
#define TFT_CS   A3
#define TFT_DC   0
#define TFT_MOSI 8
#define TFT_CLK  9
#define TFT_MISO 10
#define TFT_LED  A2  

#define HAVE_TOUCHPAD
#define TOUCH_CS A4
#define TOUCH_IRQ 1

#define BEEPER 2

//Touchscreen
#define MINPRESSURE 10
#define TS_MINX 370
#define TS_MINY 470
#define TS_MAXX 3700
#define TS_MAXY 3600

//Wifi
char ssid[] = "ssid";       //your network SSID (name)
char pass[] = "pass";       //your network password
const int kNetworkTimeout = 30*1000;
const int kNetworkDelay = 2000; 

int status = WL_IDLE_STATUS;
int infected=0;
int recovered=0;
int deaths=0;

WiFiSSLClient client;
HttpClient http(client,"www.worldometers.info", 443); 

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

void setup() 
{
  Serial.begin(9600);
  while (!Serial) 
  {
    ; // wait for port to connect.
  }

  //Init GPIOs
  pinMode(TFT_LED, OUTPUT);

  Serial.println("Init TFT ...");
  tft.begin();          
  tft.setRotation(3);             //Landscape mode  
  tft.fillScreen(ILI9341_BLACK);  //Clear screen 

  tft.setCursor(70,110);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.print("Connecting...");
  digitalWrite(TFT_LED, LOW);


  //WiFi module connection:
  if (WiFi.status() == WL_NO_MODULE) 
  {
    Serial.println("Communication with WiFi module failed!");
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) 
  {
    Serial.println("Please upgrade the firmware");
  }

  while (status != WL_CONNECTED) 
  {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    //Connect to network.
    status = WiFi.begin(ssid, pass);

    delay(10000);
  }
  Serial.println("Connected to wifi");

}

void loop() 
{
 check_country("China");
 delay(2000);
 check_country("Italy");
 delay(2000); 
 check_country("Germany");
 delay(2000); 
 check_country("Spain");
 delay(2000); 
 check_country("Austria");
 delay(2000); 
 check_country("Switzerland");
 delay(2000); 
}


void draw_country_screen(String sCountry)
{
  tft.fillScreen(ILI9341_BLACK); //Clear screen

  //Headline
  tft.setCursor(10,10);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(3);
  tft.print(sCountry + ":");

  //Infected
  tft.setCursor(10,70);
  tft.setTextColor(ILI9341_RED);
  tft.print("Infected:");
  tft.setCursor(200,70);
  tft.print(infected);

  //Recovered
  tft.setCursor(10,130);
  tft.setTextColor(ILI9341_GREEN);
  tft.print("Recovered:");
  tft.setCursor(200,130);
  tft.print(recovered);

  //Deaths
  tft.setCursor(10,190);
  tft.setTextColor(ILI9341_LIGHTGREY);
  tft.print("Deaths:");
  tft.setCursor(200,190);
  tft.print(deaths);      
}

void check_country(String sCountry) 
{
  int err =0;
  int readcounter = 0;
  int read_value_step = 0;
  String s1 = "";
  String s2 = "";
  
  err = http.get("/coronavirus/country/" + sCountry +"/");
  if (err == 0)
  {
    Serial.println("startedRequest ok");

    err = http.responseStatusCode();
    if (err >= 0)
    {
      Serial.print("Got status code: ");
      Serial.println(err); 
      
      //Print out response
      Serial.print("Request data for ");
      Serial.println(sCountry);
    
      unsigned long timeoutStart = millis();
      char c;

      while ( (http.connected() || http.available()) &&
             (!http.endOfBodyReached()) &&
             ((millis() - timeoutStart) < kNetworkTimeout) )
      {
          if (http.available())
          {
              c = http.read();
              s2 = s2 + c;
              if (readcounter < 255) 
              {
                readcounter++;
              } else 
              {
                readcounter = 0;
                String tempString = "";
                tempString.concat(s1);
                tempString.concat(s2);
                //Check infected first 
                if (read_value_step == 0) 
                {                               
                  int place = tempString.indexOf("Coronavirus Cases:");
                  if ((place != -1) && (place < 350)) 
                  { 
                    read_value_step = 1;
                    s2 = tempString.substring(place + 15);
                    tempString = s2.substring(s2.indexOf("#aaa") + 6);
                    s1 = tempString.substring(0, (tempString.indexOf("</")));
                    s1.remove(s1.indexOf(","),1);  
                    Serial.print("Coronavirus Cases: ");
                    Serial.println(s1);
                    infected = s1.toInt();
                  }
                }
                //Check deaths               
                if (read_value_step == 1) 
                {
                  int place = tempString.indexOf("Deaths:");
                  if ((place != -1) && (place < 350)) 
                  { 
                    read_value_step = 2;
                    s2 = tempString.substring(place + 15);
                    tempString = s2.substring(s2.indexOf("<span>") + 6);
                    s1 = tempString.substring(0, (tempString.indexOf("</")));
                    s1.remove(s1.indexOf(","),1);  
                    Serial.print("Deaths: ");
                    Serial.println(s1);
                    deaths = s1.toInt();
                  }
                }                
                //Check recovered               
                if (read_value_step == 2) 
                {
                  int place = tempString.indexOf("Recovered:");
                  if ((place != -1) && (place < 350)) 
                  {                   
                    s2 = tempString.substring(place + 15);
                    tempString = s2.substring(s2.indexOf("<span>") + 6);
                    s1 = tempString.substring(0, (tempString.indexOf("</")));
                    s1.remove(s1.indexOf(","),1);  
                    Serial.print("Recovered: ");
                    Serial.println(s1);
                    recovered = s1.toInt();
                    draw_country_screen(sCountry);
                    http.stop();
                    return;
                  }
                }                
                s1 = s2;
                s2 = ""; 
              }              
              //Reset the timeout counter
              timeoutStart = millis();
          }
          else
          {
              delay(kNetworkDelay);
          }
      }
    }
    else
    {
      Serial.print("Getting response failed: ");
      Serial.println(err);
    }
  }
  else
  {
    Serial.print("Connect failed: ");
    Serial.println(err);
  }
  http.stop();
}

void printWiFiStatus() {
  //Print the connected SSID
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  //Print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  //Print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
