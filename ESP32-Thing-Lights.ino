/*
  Lights with PIR sensor. Lights can be turned on and off normally
  from the power switch. The touch sensor will set the automatic
  proximity system on or off.
  When the system is on, the lights will turn on for about 1 min until
  it detects no precense.
  The system can be armed or disarmed through the network.
  It can also be updated if it is connected to the internet


  Urls:
  /system/1      Turns on motion detection system
  /system/0      Turns off motion detection system
  /lights/1      Turn on lights
  /lights/0      Turn off lights
  /update        Firmware update
  

  v1.7   2017-12-17
  Fixed Reconnection issues
  
  v1.6   2017-11-17
  System ON by default

  v1.5   2017-06-23
  Remaped relay pin

  v1.4.2   2017-06-23
  Ignore wifi if unable to connect, try again after some time
  
  v1.4.1   2017-06-23
  Removed touch button
  
  v1.4     2017-06-22
  Added url to controll lights
  
  v1.3     2017-06-20
  Added firmware update
  
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
#include <Update.h>
#include <HTTPClient.h>

#define FIRMWARE "v1.7"
#define ALARM_URL "http://192.168.0.175/io/1/" // Send something to the server if the system is activated

#define PIR 13 // PIR sensor
#define RELAY 25 // Logic is inverted so if nothing works, lights can work as usual
#define BUTTON 0 // Update with on-board button

// States
#define STAND_BY   0
#define SETTING_UP 1
#define POST       2
#define POSTING    3

#define TOUCH_SENSITIVITY 70

#define LIGHT_PERIOD  2400   // X*50 milliseconds    2 min
#define RELAY_FILTER  2000   // milliseconds
#define ALARM_DELAY  12000   // X*50 milliseconds    10 min
#define WIFI_COUNTER 12000   // X*50 milliseconds    10 min
#define WIFI_COUNTER_CONNECT 300// X*50 30 seconds

#define SSID "Eileen"
#define PASS "You-are-far-too-young-and-clever"




// OTA Bucket Config
String host = "eileen.behuns.com"; // Host => bucket-name.s3.region.amazonaws.com
int port = 80; // Non https. For HTTPS 443. As of today, HTTPS doesn't work.
String bin = "/ESP32-Thing-Lights.ino.bin"; // bin file name with a slash in front.
uint8_t ota = 0; // Update flag

// Variables to validate
// response from S3
int contentLength = 0;
bool isValidContentType = false;


int httpCode;

WiFiServer server(80);
WiFiClient client;

int server_status = 0;

uint8_t state = 0;
// 0 - Stand by
// 1 - Setting up
// 2 - Posting


uint8_t touch_filter;
long d;
long alarm_delay = 0;
long wifi_counter = WIFI_COUNTER_CONNECT;

// Utility to extract header value from headers
String getHeaderValue(String header, String headerName) {
  return header.substring(strlen(headerName.c_str()));
}

// OTA Logic 
void execOTA() {
  detachInterrupt(digitalPinToInterrupt(PIR));
  noInterrupts();
  Serial.println("Connecting to: " + String(host));
  // Connect to S3
  if (client.connect(host.c_str(), port)) {
    // Connection Succeed.
    // Fecthing the bin
    Serial.println("Fetching Bin: " + String(bin));

    // Get the contents of the bin file
    client.print(String("GET ") + bin + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Cache-Control: no-cache\r\n" +
                 "Connection: close\r\n\r\n");

    // Check what is being sent
    //    Serial.print(String("GET ") + bin + " HTTP/1.1\r\n" +
    //                 "Host: " + host + "\r\n" +
    //                 "Cache-Control: no-cache\r\n" +
    //                 "Connection: close\r\n\r\n");

    delay(100);
    // Once the response is available,
    // check stuff

    /*
       Response Structure
        HTTP/1.1 200 OK
        x-amz-id-2: NVKxnU1aIQMmpGKhSwpCBh8y2JPbak18QLIfE+OiUDOos+7UftZKjtCFqrwsGOZRN5Zee0jpTd0=
        x-amz-request-id: 2D56B47560B764EC
        Date: Wed, 14 Jun 2017 03:33:59 GMT
        Last-Modified: Fri, 02 Jun 2017 14:50:11 GMT
        ETag: "d2afebbaaebc38cd669ce36727152af9"
        Accept-Ranges: bytes
        Content-Type: application/octet-stream
        Content-Length: 357280
        Server: AmazonS3
                                   
        {{BIN FILE CONTENTS}}

    */
    while (client.available()) {
      // read line till /n
      String line = client.readStringUntil('\n');
      // remove space, to check if the line is end of headers
      line.trim();

      // if the the line is empty,
      // this is end of headers
      // break the while and feed the
      // remaining `client` to the
      // Update.writeStream();
      if (!line.length()) {
        //headers ended
        break; // and get the OTA started
      }

      // Check if the HTTP Response is 200
      // else break and Exit Update
      if (line.startsWith("HTTP/1.1")) {
        if (line.indexOf("200") < 0) {
          Serial.println("Got a non 200 status code from server. Exiting OTA Update.");
          break;
        }
      }

      // extract headers here
      // Start with content length
      if (line.startsWith("Content-Length: ")) {
        contentLength = atoi((getHeaderValue(line, "Content-Length: ")).c_str());
        Serial.println("Got " + String(contentLength) + " bytes from server");
      }

      // Next, the content type
      if (line.startsWith("Content-Type: ")) {
        String contentType = getHeaderValue(line, "Content-Type: ");
        Serial.println("Got " + contentType + " payload.");
        if (contentType == "application/octet-stream") {
          isValidContentType = true;
        }
      }
    }
  } else {
    // Connect to S3 failed
    // May be try?
    // Probably a choppy network?
    Serial.println("Connection to " + String(host) + " failed. Please check your setup");
    // retry??
    // execOTA();
  }

  // Check what is the contentLength and if content type is `application/octet-stream`
  Serial.println("contentLength : " + String(contentLength) + ", isValidContentType : " + String(isValidContentType));

  // check contentLength and content type
  if (contentLength && isValidContentType) {
    // Check if there is enough to OTA Update
    bool canBegin = Update.begin(contentLength);

    // If yes, begin
    if (canBegin) {
      Serial.println("Begin OTA. This may take 2 - 5 mins to complete. Things might be quite for a while.. Patience!");
      // No activity would appear on the Serial monitor
      // So be patient. This may take 2 - 5mins to complete
      size_t written = Update.writeStream(client);

      if (written == contentLength) {
        Serial.println("Written : " + String(written) + " successfully");
      } else {
        Serial.println("Written only : " + String(written) + "/" + String(contentLength) + ". Retry?" );
        // retry??
        // execOTA();
      }

      if (Update.end()) {
        Serial.println("OTA done!");
        if (Update.isFinished()) {
          Serial.println("Update successfully completed. Rebooting.");
          ESP.restart();
        } else {
          Serial.println("Update not finished? Something went wrong!");
        }
      } else {
        Serial.println("Error Occurred. Error #: " + String(Update.getError()));
      }
    } else {
      // not enough space to begin OTA
      // Understand the partitions and
      // space availability
      Serial.println("Not enough space to begin OTA");
      client.flush();
    }
  } else {
    Serial.println("There was no content in the response");
    client.flush();
  }
}


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

uint8_t wifiConnect(){
  Serial.println("Connecting...");
  if(WiFi.status() != WL_CONNECTED){
    WiFi.begin(SSID,PASS);
    delay(500);
    
    wifi_counter = WIFI_COUNTER_CONNECT;
    
    while(WiFi.status() != WL_CONNECTED && wifi_counter>0) {
      delay(50);
      digitalWrite(LED_BUILTIN,!digitalRead(LED_BUILTIN));
      wifi_counter--;
    }
    if(!wifi_counter){
      return 0; //Unable to connect
    }
  }
  if(WiFi.status() == WL_CONNECTED){
    Serial.println(WiFi.localIP());
    server.begin();
  }
  return 1; //Connected
}

void setup() {
  pinMode(LED_BUILTIN,OUTPUT);
  pinMode(RELAY,OUTPUT);
  pinMode(BUTTON,INPUT);
  pinMode(PIR,INPUT_PULLUP);
  Serial.begin(115200);

  wifiConnect();
  
  digitalWrite(LED_BUILTIN,HIGH); // Start system ON
  pir(); // Activate system

  Serial.print("Firmware: ");
  Serial.println(FIRMWARE);

  attachInterrupt(digitalPinToInterrupt(PIR),pir,FALLING);
}

void loop() {
  client = server.available();   // listen for incoming clients


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
            
            client.print("Firware: ");
            client.print(FIRMWARE);
            client.print("<br><br>System: ");
            client.print(digitalRead(LED_BUILTIN));
            if(digitalRead(LED_BUILTIN))
              client.print(" <a href=\"/system/0\">off</a><br>");
            else
              client.print(" <a href=\"/system/1\">on</a><br>");
            client.print("<br>Lights: ");
            client.print(!digitalRead(RELAY));
            if(digitalRead(RELAY))
              client.print(" <a href=\"/lights/1\">on</a>");
            else
              client.print(" <a href=\"/lights/0\">off</a>");
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
        if (currentLine.endsWith("GET /system/1")) {
          digitalWrite(LED_BUILTIN, HIGH);               // GET /1 turns the LED on
          if(!digitalRead(RELAY)){
            d = LIGHT_PERIOD;
          }
        }
        else if (currentLine.endsWith("GET /system/0")) {
          digitalWrite(LED_BUILTIN, LOW);                // GET /0 turns the LED off
          d = 0;
        }
        else if (currentLine.endsWith("GET /lights/1")) {
          digitalWrite(RELAY, LOW);
          d = LIGHT_PERIOD;
        }
        else if (currentLine.endsWith("GET /lights/0")) {
          digitalWrite(RELAY, HIGH);
          d = 0;
        }
        else if (currentLine.endsWith("GET /update")) {
          ota = 1;
        }
        else{
          
        }
      }
    }
    // close the connection:
    client.stop();
    //Serial.println("Client Disconnected.");
     interrupts();
  }

  if(!digitalRead(BUTTON))
    ota = 1;

  if(ota){
    digitalWrite(RELAY, HIGH);
    execOTA(); // Update Firmware
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

  // Reconnect if no wifi
  if(WiFi.status() != WL_CONNECTED){
    if(!wifi_counter){
      wifiConnect();
      wifi_counter = WIFI_COUNTER;
    }else{
      wifi_counter--;
    }
  }
  
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
