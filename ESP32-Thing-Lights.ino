/*
  Lights whith PIR sensor. Lights can be turned on and off normally
  from the power switch. The touch sensor will set the automatic
  proximity system on or off.
  When the system is on, the lights will turn on for about 1 min until
  it detects no precense.
  
  v1.2     2017-06-20
  Added server to activate or deactivate system
  
  v1.1     2017-06-20
  Post to alarm URL if it is activated
  Avoid interrupts while changing state
  
  v1.0     2017-06-20


  
  Alfredo Rius
  alfredo.rius@gmail.com
*/

#include <WiFi.h>
#include <HTTPClient.h>

#define ALARM_URL "http://192.168.0.175/io/1/"

#define TOUCH_PIN 4
#define PIR 13
#define RELAY 12

// States
#define STAND_BY   0
#define SETTING_UP 1
#define POST       2
#define POSTING    3

#define TOUCH_SENSITIVITY 40

#define LIGHT_PERIOD 2400 // X*50 milliseconds   2 min
#define RELAY_FILTER 2000 //milliseconds
#define ALARM_DELAY 2400 // X*50 milliseconds    2 min


const char* ssid     = "******";
const char* password = "******";

int httpCode;

WiFiServer server(80);

int server_status = 0;

uint8_t state = 0;
// 0 - Stand by
// 1 - Setting up
// 2 - Posting

uint8_t touch_filter;
long d;
long alarm_delay = 0;



void alarm(){
  HTTPClient http;
  state = POSTING;
  
  if(WiFi.status() == WL_CONNECTED){
    
    //Serial.println("Alarm!!!");
    http.begin(ALARM_URL);
    
    //http.addHeader ("Authorization", AUTH_TOKEN);

    httpCode = -1;
    httpCode = http.GET();
    
    if(httpCode > 0) {
      if(httpCode == HTTP_CODE_OK){
        String payload = http.getString();
        //Serial.println();
        //Serial.println(payload);
        //Serial.println();
      }else{
        String payload = http.getString();
        //Serial.println();
        //Serial.println(payload);
        //Serial.println();
      }
    }else{
      //Serial.println("Server Error: ");
    }
    httpCode = 0; //Allow new post
  }else{
    //Serial.println("No Wifi");
  }
  
  state = STAND_BY;
}


void setup() {
  pinMode(LED_BUILTIN,OUTPUT);
  pinMode(RELAY,OUTPUT);
  pinMode(0,INPUT);
  pinMode(PIR,INPUT_PULLUP);
  //Serial.begin(115200);
  WiFi.begin(ssid, password);


  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    digitalWrite(LED_BUILTIN,!digitalRead(LED_BUILTIN));
    //Serial.print(".");
  }
  
  digitalWrite(LED_BUILTIN,LOW);

  
  //Serial.println("Server Begin");
  //Serial.println(WiFi.localIP());
  server.begin();

  attachInterrupt(digitalPinToInterrupt(PIR),pir,FALLING);
}

void loop() {
  WiFiClient client = server.available();   // listen for incoming clients


  if(1){
    if (client) {                             // if you get a client,

      noInterrupts();
      
      //Serial.println("New Client.");           // print a message out the serial port
      String currentLine = "";                // make a String to hold incoming data from the client
      while (client.connected()) {            // loop while the client's connected
        if (client.available()) {             // if there's bytes to read from the client,
          char c = client.read();             // read a byte, then
          //Serial.write(c);                    // print it out the serial monitor
          if (c == '\n') {                    // if the byte is a newline character
  
            // if the current line is blank, you got two newline characters in a row.
            // that's the end of the client HTTP request, so send a response:
            if (currentLine.length() == 0) {
              // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
              // and a content-type so the client knows what's coming, then a blank line:
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println();

              // the content of the HTTP response follows the header:
              client.print("Click <a href=\"/1\">here</a> to turn system on.<br>");
              client.print("Click <a href=\"/0\">here</a> to turn system off.<br>");

              // The HTTP response ends with another blank line:
              client.println();
              // break out of the while loop:
              break;
            } else {    // if you got a newline, then clear currentLine:
              currentLine = "";
            }
          } else if (c != '\r') {  // if you got anything else but a carriage return character,
            currentLine += c;      // add it to the end of the currentLine
          }

          // Check to see if the client request was "GET /1" or "GET /0":
          if (currentLine.endsWith("GET /1")) {
            digitalWrite(LED_BUILTIN, HIGH);               // GET /1 turns the LED on
            if(!digitalRead(RELAY)){
              d = LIGHT_PERIOD;
            }
          }
          if (currentLine.endsWith("GET /0")) {
            digitalWrite(LED_BUILTIN, LOW);                // GET /0 turns the LED off
            d = 0;
          }
        }
      }
      // close the connection:
      client.stop();
      //Serial.println("Client Disconnected.");

      interrupts();
      
    }
  }

  
  if(touchRead(TOUCH_PIN)<TOUCH_SENSITIVITY){
    touch_filter = 0;
    while(touchRead(TOUCH_PIN)<TOUCH_SENSITIVITY){touch_filter++; delay(10);}
    if(touch_filter>10){
      
      noInterrupts();
      
      alarm_delay = ALARM_DELAY;
      
      digitalWrite(LED_BUILTIN,!digitalRead(LED_BUILTIN));
      if(!digitalRead(LED_BUILTIN)){
        digitalWrite(RELAY,HIGH);
        delay(300);
        digitalWrite(RELAY,LOW);
      }else{
        digitalWrite(RELAY,HIGH);
        delay(300);
        digitalWrite(RELAY,LOW);
        delay(300);
        digitalWrite(RELAY,HIGH);
        delay(300);
        digitalWrite(RELAY,LOW);
        d = LIGHT_PERIOD;
      }

      interrupts();
      
      while(touchRead(TOUCH_PIN)<TOUCH_SENSITIVITY) delay(10);
    }
  }
  if(digitalRead(LED_BUILTIN)){
    if(d>1){
      digitalWrite(RELAY,LOW);
      d--;
    }else if(d==1){
      digitalWrite(RELAY,HIGH);
      delay(RELAY_FILTER);
      d--;
    }
  }else{
    d = 0;
  }





  if(alarm_delay)
    alarm_delay--;
  
  if(state == POST)
      alarm();




  
  delay(50);
}

void pir(){
  
  //Serial.println("PIR!");
  
  if(d != 1 && digitalRead(LED_BUILTIN)){
    digitalWrite(RELAY,LOW);
    d = LIGHT_PERIOD;
  }
  
  if(!alarm_delay){
    if(state==STAND_BY)
      state = POST; 
  }
  alarm_delay = ALARM_DELAY;
}
