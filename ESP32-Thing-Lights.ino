/*
  Lights whith PIR sensor. Lights can be turned on and off normally
  from the power switch. The touch sensor will set the automatic
  proximity system on or off.
  When the system is on, the lights will turn on for about 1 min until
  it detects no precense.

  TODO:
    - WiFi/Bluetooth conectivity
    - Send signal to server when lights are turned on.
  
  2017/06/20
  Alfredo Rius
  alfredo.rius@gmail.com
*/

#define TOUCH_PIN 4
#define PIR 13
#define RELAY 12

#define TOUCH_SENSITIVITY 60

#define LIGHT_PERIOD 1000

uint8_t touch_filter;
long d;
uint8_t relay_filter = 0;


void setup() {
  pinMode(LED_BUILTIN,OUTPUT);
  pinMode(RELAY,OUTPUT);
  pinMode(0,INPUT);
  pinMode(PIR,INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIR),pir,FALLING);
  Serial.begin(115200);
}

void loop() {
  if(touchRead(TOUCH_PIN)<TOUCH_SENSITIVITY){
    touch_filter = 0;
    while(touchRead(TOUCH_PIN)<TOUCH_SENSITIVITY){touch_filter++; delay(10);}
    if(touch_filter>10){
      digitalWrite(LED_BUILTIN,!digitalRead(LED_BUILTIN));
      if(!digitalRead(LED_BUILTIN)){
        digitalWrite(RELAY,HIGH);
        delay(800);
        digitalWrite(RELAY,LOW);
        d = LIGHT_PERIOD;
      }else{
        digitalWrite(RELAY,HIGH);
        delay(300);
        digitalWrite(RELAY,LOW);
        delay(300);
        digitalWrite(RELAY,HIGH);
        delay(300);
        digitalWrite(RELAY,LOW);
      }
      while(touchRead(TOUCH_PIN)<TOUCH_SENSITIVITY) delay(10);
    }
  }
  if(digitalRead(LED_BUILTIN)){
    if(d>0){
      digitalWrite(RELAY,LOW);
      d--;
    }else if(d == 0){
      relay_filter = 1;
      digitalWrite(RELAY,HIGH);
      delay(2000);
      relay_filter = 0;
      d--;
    }
  }
  delay(50);
}

void pir(){
  if(!relay_filter){
    digitalWrite(RELAY,LOW);
    d = LIGHT_PERIOD;
    Serial.println("PIR!");
  }
}
