// Code by Jeffrey Carr and Yannis Lam

#include <Servo.h>  // Servo
#include <Adafruit_SSD1306.h> // display
#include <Adafruit_NeoPixel.h> //neopixels
#include <SPI.h>
#include <MFRC522.h>

#define PHOTO A1  // Photoresistor
#define UBUTTON 8  // Upper button
#define DBUTTON 9
#define LBUTTON 7
#define PIR A0  //pir sensor
#define NEOPIXEL 4 //neopixel strip
#define SS_PIN 10
#define RST_PIN 5 //change this!

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.
Servo myservo;  // Create servo
Adafruit_NeoPixel strip = Adafruit_NeoPixel(56, NEOPIXEL, NEO_RGB + NEO_KHZ800);

void setup() {
  // Serial
  Serial.begin(9600);
  
  // Servo
  //myservo.attach(A2);  // Having problems with servo, so only attach and detach when it is needed

  // Buttons
  pinMode(UBUTTON, INPUT_PULLUP);
  pinMode(DBUTTON, INPUT_PULLUP);
  pinMode(LBUTTON, INPUT_PULLUP);

  // Sensor
  pinMode(PIR, INPUT);  // Declare sensor as input

  // Neopixels
  strip.begin();
  strip.setBrightness(50);
  strip.clear();
  strip.show();  // show all pixels

  //RFID
  SPI.begin();      // Initiate  SPI bus
  mfrc522.PCD_Init();   // Initiate MFRC522
}

/**** GLOBAL DECLARATIONS *****/
byte prevUpState = HIGH, prevDownState = HIGH, prevLState = HIGH;  // Button states
int windowTime = 2350;  // Takes 5 seconds to go down window, start at top
int currentWindowTime = 0;  // Keeps track of the current distance away from top shade is
int lightThreshold = 900;  // Threshold for when it is considered "dark"
int pirState = LOW;  // we start, assuming no motion detected
int pirVal = 0;  // variable for reading the pin status
unsigned long uPrevt = 0, dPrevt = 0, lPrevt;  // Times for button debouncing
unsigned long prevScan = 0;  // Time since last card scan
unsigned long motionStart, motionEnd;  // Keeps track of time between PIR sensing
boolean alreadyDark = false, alreadyBright = true;  // For determining whether the button was used to raise/lower shade or not
boolean lightOn = false, childLock = false;  // Keeps track if light button is pressed & child lock is turned on
/***** END GLOBAL DECLARATIONS *****/

/***** EXTERNAL FUNCTIONS *****/
// Sets properties of it being dark out (or if curtain is lowered all the way)
void dark(){
  currentWindowTime = windowTime;
  alreadyBright = false;
  alreadyDark = true;
  lightOn = true;
}

// Sets properties of it being light out (or if curtains is raised all the way)
void bright(){
  currentWindowTime = 0;
  alreadyBright = true;
  alreadyDark = false;
  lightOn = false;
}

// Rotate downwards for the given amount of time
void rotateDown(unsigned long t){
  myservo.attach(A2);
  myservo.write(130);
  delay(t);  // Eventually try to not use delay
  myservo.write(90);
  myservo.detach();
}
// Rotate upwards for the given amount of time
void rotateUp(unsigned long t){
  myservo.attach(A2);
  myservo.write(50);
  delay(t);  // Eventually try to not use delay
  myservo.write(90);
  myservo.detach();
}

// Turn on LEDS
void whiteLEDs(){
  strip.clear();
  for(int i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, 255, 255, 255);
  }
  strip.show();
}
/***** END EXTERNAL FUNCTIONS *****/

void loop() {
  /***** LOCAL VARIABLE DECLARATION *****/
  int photoVal = analogRead(PHOTO);
  pirVal = digitalRead(PIR);
  int timeToAdd = 0;
  byte upState, downState, lState;
  boolean stopRot = false;
  /***** END LOCAL VARIABLE DECLARATION *****/

  /* AUTOMATIC WINDOW ROTATE CODE */
  // if it's dark and window isn't already down and it hasn't already been dark (aka nobody has used the button)
  if(photoVal > lightThreshold && currentWindowTime < windowTime && alreadyDark == false){
    rotateDown(windowTime - currentWindowTime);
    dark();
  }
  // if it's bright and window isn't already up and it already hasn't been bright (aka nobody has used the button)
  if(photoVal < lightThreshold && currentWindowTime > 0 && alreadyBright == false){
    rotateUp(currentWindowTime+250);
    bright();
  }
  /* END AUTOMATIC WINDOW ROTATE CODE */

  /* BUTTON CODE */
  if(!childLock){
    // user pressing button to raise shade
    upState = digitalRead(UBUTTON);
    if(upState != prevUpState){
      if(millis() > uPrevt + 250){  // button debounce 250ms
        uPrevt = millis();
        if(currentWindowTime > 0){\
          timeToAdd = millis();
          myservo.attach(A2);
          myservo.write(50);
          while(upState == LOW){
            // rotate
            upState = digitalRead(UBUTTON);
          }
        }
        myservo.write(90);
        myservo.detach();
        timeToAdd = millis() - timeToAdd;
        if(timeToAdd > currentWindowTime){
          currentWindowTime = 0;
          bright();
        }
        else
          currentWindowTime -= timeToAdd;
      }
    }
  //  user pressing button to lower shade
    downState = digitalRead(DBUTTON);
    if(downState != prevDownState){
      if(millis() > dPrevt + 250){  // button debounce 250ms
        if(currentWindowTime < windowTime){
          timeToAdd = millis();
          myservo.attach(A2);
          myservo.write(130);
          while(downState == LOW){
            // rotate
            downState = digitalRead(DBUTTON);
          }
          myservo.write(90);
          myservo.detach();
          timeToAdd = millis() - timeToAdd;
          if(timeToAdd > windowTime){
            currentWindowTime = windowTime;
            dark();
          }
          else
            currentWindowTime += timeToAdd;
        }
  
        dPrevt = millis();
      }
    }
    // User pressing button to toggle lights
    lState = digitalRead(LBUTTON);
    if(lState != prevLState){
      if(millis() > lPrevt + 250){  // Button debounce 250ms
        lightOn = !lightOn;
  
          lPrevt = millis();
      }  
    }
    // Update state variables
    prevUpState = upState;
    prevDownState = downState;
    prevLState = lState;
  }
  /* END BUTTON CODE */
  
  /* PIR CODE */
  if (pirVal == HIGH) { // check if the input is HIGH
    if (pirState == LOW) {
      // we have just turned on 
      Serial.println("Motion detected!");
      motionStart = millis();
      Serial.println(motionStart);
      // We only want to print on the output change, not state
      pirState = HIGH;
    }
  } 
  else {
    if (pirState == HIGH){
      // we have just turned off
      Serial.println("Motion ended!");
      motionEnd = millis();
      Serial.println(motionEnd);
      // We only want to print on the output change, not state
      pirState = LOW;
    }
  }
  /* END PIR CODE */

  /* LED CODE */
  //if photoresistor value is over threshold 
  // && last motion was 20 secs ago or motion started 1 second ago, 
  // it will turn on the LED
  if ((photoVal > lightThreshold && (((millis() - motionEnd) < 5000) || ((millis() - motionStart) < 1000)) && lightOn == true) || lightOn == true){ 
    whiteLEDs();
    Serial.println(lightOn);
  }
  else{
    strip.clear();
    strip.show();
  }
  /* END LED CODE */

  /* RFID CODE */
  if(millis() > prevScan + 2){  // Must wait 3 seconds before scanning a card again
    if (  mfrc522.PICC_IsNewCardPresent()) 
    {
      Serial.println("Card present");
      if (  mfrc522.PICC_ReadCardSerial()) 
    {
        prevScan = millis();  // monitor scan time
        //Show UID on serial monitor
        Serial.print("UID tag :");
        String content= "";
        for (byte i = 0; i < mfrc522.uid.size; i++) 
        {
           Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
           Serial.print(mfrc522.uid.uidByte[i], HEX);
           content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
           content.concat(String(mfrc522.uid.uidByte[i], HEX));
        }
        Serial.println();
        Serial.print("Message : ");
        content.toUpperCase();
        if ((content.substring(1) == "40 2E 7B A4") || content.substring(1) == "80 51 81 A4") //change here the UID of the card/cards that you want to give access
        {
          if(childLock){
            Serial.println("CHILD LOCK OFF");
            // animate LEDs to show that child lock is off
            strip.clear();
            for(int i=0; i<strip.numPixels(); i++){
              strip.setPixelColor(i, 209, 33, 48);
              strip.show();
              delay(25);  
            }
            int count = 0;
            while(count < 3){
              for(int i=0; i<strip.numPixels(); i++){
                strip.setPixelColor(i, 209, 33, 48);  
              }
              strip.show();
              delay(500);
              for(int i=0; i<strip.numPixels(); i++){
                strip.setPixelColor(i, 0, 0, 0);  
              }
              strip.show();
              delay(500);
              count++;
            }
            strip.clear();
            childLock = false;
          }
          else{
            Serial.println("CHILD LOCK ON");
            // Flash LEDs to show that child lock is on
            strip.clear();
            for(int i=0; i<strip.numPixels(); i++){
              strip.setPixelColor(i, 0, 255, 0);
              strip.show();
              delay(25);  
            }
            int count = 0;
            while(count < 3){
              for(int i=0; i<strip.numPixels(); i++){
                strip.setPixelColor(i, 0, 255, 0);  
              }
              strip.show();
              delay(500);
              for(int i=0; i<strip.numPixels(); i++){
                strip.setPixelColor(i, 0, 0, 0);  
              }
              strip.show();
              delay(500);
              count++;
            }
            strip.clear();
            childLock = true;
          }
        }
       
       else   {
          Serial.println("Unrecognized card");
          int count = 0;
            while(count < 3){
              for(int i=0; i<strip.numPixels(); i++){
                strip.setPixelColor(i, 0, 0, 255);  
              }
              strip.show();
              delay(500);
              for(int i=0; i<strip.numPixels(); i++){
                strip.setPixelColor(i, 0, 0, 0);  
              }
              strip.show();
              delay(500);
              count++;
            }
        }
      }
    }
  }
}
