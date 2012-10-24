#include "Adafruit_Thermal.h"
#include "SoftwareSerial.h"
#include "Stream.h"

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
int newlineTime;
boolean printUpsidedown = true;

/**
 * Initialize the thermal printer
 */
Adafruit_Thermal printer(printer_RX_Pin, printer_TX_Pin);

void setup(){
  printer.begin();
  Serial.begin(9600);
  //make the line array all white
  for(int i = 0; i < arrayWidth; i++){
    line[i] = 0x00;
  }
  if (printUpsidedown){
    printer.upsideDownOn();
  }
  printer.println("Hello");
  printer.feed(2);
}

void loop(){
  if (Serial.available() > 0){
    char type = Serial.read();
    Serial.write(type);
    char val;
    if (type == 's'){//string
      int v;
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
      }
      printer.feed(1);
      waitForNewline = true;
      newlineTime = millis() + newlineTimeout;
    } else if (type == 'p'){//picture
      printer.printBitmap(&Serial);
      printer.feed(1);
      waitForNewline = true;
      newlineTime = millis() + newlineTimeout;
    } else if (type == 'r'){//range
      int v = Serial.read();
      byte lineWidth = 3;
      char offset[lineWidth];
      for(int i = -lineWidth/2, indx = 0; indx < lineWidth; i++, indx++){
        uint8_t dot = (1 << (7 - (v+i)%8));
        offset[indx] = (v+i)/8;
        line[offset[indx]] |= dot;
      }
      printer.printBitmap(maxWidth, 1, line, false);
      for(int indx = 0; indx < lineWidth; indx++){
        line[offset[indx]] = 0x00;
      }
    } else if (type == 'b'){//boolean
      printer.println(Serial.read()?"True":"False");
      printer.feed(1);
      waitForNewline = true;
      newlineTime = millis() + newlineTimeout;
    }
    Serial.write(1);
  } else {
    if (waitForNewline){
      if (millis() > newlineTime){
        printer.feed(1);
        waitForNewline = false;
      }
    }
  }
}
