#include "Adafruit_Thermal.h"
#include "SoftwareSerial.h"
#include "Stream.h"
#include <SPI.h>
#include <Spacebrew.h>
#include <Ethernet.h>
#include <WebSocketClient.h>

/**
 * Declare which pins to communicate to the printer over
 */
int printer_RX_Pin = 5; // green wire
int printer_TX_Pin = 6; // yellow wire
const int maxWidth = 384;
const int arrayWidth = 384/8;
uint8_t line[arrayWidth];
const int charsPerLine = 32;
const int maxLines = 3;
const int maxChars = charsPerLine*maxLines;
char str[maxChars];
int newlineTimeout = 5000;
boolean waitForNewline = false;
long newlineTime;
boolean printUpsidedown = false;

//for spacebrew
Spacebrew sb;
uint8_t mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };


/**
 * Initialize the thermal printer
 */
Adafruit_Thermal printer(printer_RX_Pin, printer_TX_Pin);

void setup(){
  printer.begin();
  //make the line array all white
  for(int i = 0; i < arrayWidth; i++){
    line[i] = 0x00;
  }
  if (printUpsidedown){
    printer.upsideDownOn();
  }
  setupSpacebrew();
  printer.println("Hello");
  printer.feed(2);
}

void setupSpacebrew(){
  //connect to spacebrew library info
  sb.onOpen(onOpen);
  sb.onClose(onClose);
  sb.onError(onError);
  
  //connect to message callbacks
  sb.onBooleanMessage(onBooleanMessage);
  sb.onStringMessage(onStringMessage);
  sb.onRangeMessage(onRangeMessage);
  
  //register publishers and subscribers
  sb.addPublish("Ready", SB_BOOLEAN);
  sb.addSubscribe("Boolean", SB_BOOLEAN);
  sb.addSubscribe("Graph", SB_RANGE);
  sb.addSubscribe("Text", SB_STRING);
  
  //connect to the spacebrew server
  Ethernet.begin(mac);
  sb.connect("192.168.101.183", "Thermal Printer", "A thermal printer that can print various data from spacebrew");
}

void loop(){
  sb.monitor();
  if (waitForNewline){
    if (millis() > newlineTime){
      printer.feed(1);
      waitForNewline = false;
    }
  }
}



void onBooleanMessage(char *name, bool value){
  printer.println(value?"True":"False");
  printer.feed(1);
  waitForNewline = true;
  newlineTime = millis() + newlineTimeout;
}

void onStringMessage(char *name, char* message){
  printer.print(message);
  /*
      int chars = 0;
      while(true){
        v = Serial.read();
        while(v == -1){
          v = Serial.read();
        }
        if (v == 255){
          break;
        }
        if (printUpsidedown){
          if (chars < maxChars){
            str[chars++] = (char)v;
          } else if (chars == maxChars) {
            //add ellipsis at end
            for(int i = maxChars-3; i < maxChars; i++){
              str[i] = '.';
            }
            chars++;
          }
        } else {
          printer.print((char)v);
        }
      }
      if (printUpsidedown){
        chars = min(chars, maxChars);
        //for each line, (from back to front) print the characters in order
        int charsThisLine = chars%charsPerLine;
        for(int i = chars/charsPerLine, o = i*charsPerLine; i >= 0; i--, o -= charsPerLine){
          for(int j = 0; j < charsThisLine; j++){
            printer.print(str[o+j]);
          }
          if (charsThisLine > 0){
            printer.println("");
          }
          charsThisLine = charsPerLine;
        }
      } else {
        printer.println("");
      }*/
      printer.feed(1);
      waitForNewline = true;
      newlineTime = millis() + newlineTimeout;
}

void onRangeMessage(char *name, int value){
  //remap to fit on receipt paper
  value = map(value, 0, 1024, 0, maxWidth);
  //how thick we want the line
  byte lineWidth = 3;
  char offset[lineWidth];
  for(int i = -lineWidth/2, indx = 0; indx < lineWidth; i++, indx++){
    //create the one-pixel dot for this iteration
    uint8_t dot = (1 << (7 - ((value+i)%8)));
    //figure out which byte to place it in
    offset[indx] = (value+i)/8;
    //add it to the current bitmap
    line[offset[indx]] |= dot;
  }
  
  printer.printBitmap(maxWidth, 1, line, false);
  
  //clear the pixels we just set
  for(int indx = 0; indx < lineWidth; indx++){
    line[offset[indx]] = 0x00;
  }
  waitForNewline = false;
  
  sb.send("Ready", (bool)true);
}

void onOpen(){
  //we are connected!
}

void onClose(int code, char* message){
  //disconected :(
}

void onError(char* message){}
