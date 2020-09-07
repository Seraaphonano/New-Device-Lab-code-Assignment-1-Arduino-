//Created by 
//Seraph Jin    - s1032019
//Niels van Ee  - s1037244

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <Wire.h>
#include "paj7620.h"

#include <LOLIN_I2C_MOTOR.h>

#include<ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include<ESP8266WebServer.h>
#include<ESP8266mDNS.h>

#define LISTEN_PORT 80

const char* ssid="ASUS_NDL"; // name of local WiFi network in the NDL
const char* password="RT-AC66U"; // the password of the WiFi network

MDNSResponder mdns;
ESP8266WebServer server(LISTEN_PORT) ;
String webPage="<h1>Candy Lock Changer</h1>";

#define LED_PIN LED_BUILTIN

#define GES_REACTION_TIME 500
#define GES_ENTRY_TIME 800
#define GES_QUIT_TIME 1000
#define I2C_ADDRESS 0x43
#define I2C_ADDRESS2 0x44

#define KEY_A D3
#define KEY_B D4
#define OLED_RESET 1 // GPIO1

char inputs[10] = {'N'};
int inputsSize;

#define PWM_FREQUENCY 1000

Adafruit_SSD1306 display(OLED_RESET);

LOLIN_I2C_MOTOR motor(DEFAULT_I2C_MOTOR_ADDRESS);

int currentPosition;

void updateCode(String s)
{
  inputsSize = s.length();
  for (int i = 0; i < s.length(); i++)
  {
    inputs[i] = s.charAt(i);
    Serial.println("New code uploaded");
  }
}

void setup()
{
  inputs[0] = 'R';
  inputs[1] = 'L';
  inputs[2] = 'U';
  inputs[3] = 'D';
  inputsSize = 4;
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C) ; // initialize with the I2C address
  delay(1000);
  pinMode(KEY_A, INPUT) ;
  pinMode(KEY_B, INPUT) ;

  Serial.begin(115200);
  uint8_t error = paj7620Init();
  if (error)
  {
    Serial.print("INIT ERROR, CODE: ");
    Serial.println(error);
  }

  while (motor.PRODUCT_ID != PRODUCT_ID_I2C_MOTOR)
  {
    motor.getInfo();
  }

  Serial.println(inputs);

  currentPosition = 0;
  display.clearDisplay( ) ;

  webPage+="<form action=\"/code\">";
            webPage+="<input type=\"text\" id=\"input\" name=\"input\">";
            webPage+="<input type=\"submit\" value=\"Submit\"></form>";
  
  WiFi.begin(ssid, password) ; // make the WiFi connection
  Serial.println("Start connecting.") ;
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".") ;
  }
  Serial.print("Connected to ") ;
  Serial.print(ssid) ;
  Serial.print(". IP address: ") ;
  Serial.println(WiFi.localIP( ) ) ;

  if (mdns.begin("esp8266", WiFi.localIP())) {
    Serial.println("MDNS responder started") ;
  }
// make handlers for input from WiFi connection
  server.on("/", [ ](){
  server.send(200 , "text/html", webPage) ;
  });
  server.on("/code", [ ](){
    Serial.println(server.arg(0));
    delay(1000);
    updateCode(server.arg(0));
    server.sendHeader("Location", "/", true) ;
    server.send(302, "text/plain", "");
  });
  server.begin( ) ; // start the server for WiFi input
  Serial.println("HTTP server started") ;
}

void loop()
{
  server.handleClient( ) ;
  uint8_t data = 0, data1 = 0, error;
  error = paj7620ReadReg(I2C_ADDRESS, 1, &data);
  if (!error && currentPosition < inputsSize)
  {
    
    switch (data)
    {
      case GES_RIGHT_FLAG:
        if (inputs[currentPosition] == 'R')
        {
          currentPosition++;
          printOLED("Right");
        }
        else
        {
          currentPosition = 0;
          printOLED("Wrong");
        }
        break;
      case GES_LEFT_FLAG:
        if (inputs[currentPosition] == 'L')
        {
          currentPosition++;
          printOLED("Left");
        }
        else
        {
          currentPosition = 0;
          printOLED("Wrong");
        }
        break;
      case GES_UP_FLAG:
        if (inputs[currentPosition] == 'U')
        {
          currentPosition++;
          printOLED("Up");
        }
        else
        {
          currentPosition = 0;
          printOLED("Wrong");
        }
        break;
      case GES_DOWN_FLAG:
        if (inputs[currentPosition] == 'D')
        {
          currentPosition++;
          printOLED("Down");
        }
        else
        {
          currentPosition = 0;
          printOLED("Wrong");
        }
        break;
    }
  }
  delay(100);
  if (currentPosition == inputsSize)
  {
    printOLED("Candy");
    motor.changeFreq(MOTOR_CH_BOTH, PWM_FREQUENCY);
    motor.changeStatus(MOTOR_CH_A, MOTOR_STATUS_CCW);
    motor.changeDuty(MOTOR_CH_A, 60);
    delay(2000);
    motor.changeStatus(MOTOR_CH_A, MOTOR_STATUS_STANDBY);
    delay(500);
    currentPosition = 0;
  }
  delay(100);
}

void printOLED(String s)
{
  Serial.println(s);
  display.clearDisplay();
  display.setCursor(0 , 0);
  display.setTextColor(WHITE) ;
  display.setTextSize(2);
  display.println(s);
  display.print(currentPosition);
  display.print(" ");
  display.println(inputs[currentPosition]);
  display.display() ;
}
