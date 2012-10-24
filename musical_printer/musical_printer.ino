#include "Adafruit_Thermal.h"
#include "SoftwareSerial.h"
#include "Stream.h"

/**
 * Declare which pins to communicate to the printer over
 */
int printer_RX_Pin = 5; // green wire
int printer_TX_Pin = 6; // yellow wire
const int minHeight = 30;
const int maxWidth = 384;
const int maxSize = maxWidth/8*minHeight;
  uint8_t a[maxSize];
  byte mode = 0;
  byte maxMode = 4;


/**
 * Initialize the thermal printer
 */
Adafruit_Thermal printer(printer_RX_Pin, printer_TX_Pin);

void setup(){
  printer.setTimes(15000, 2100);
  printer.begin(128);
  Serial.begin(9600);
  for(int i = 0; i < maxSize; i++){
    a[i] = 0xFF;//i&1?0xFF:0x00;
  }
/*  printer.timeoutWait();
  int times = 2;
  for(int i = 0; i < 1000; i++){
    if (times <= 0){
    printer._printer->write(27);
    printer._printer->write(74);
    printer._printer->write(1);
    } else if (times > 0){
      printer._printer->write(18);
      printer._printer->write(42);
      printer._printer->write(1);
      
      printer._printer->write(times);
      for(int i = 0;i < times; i++){
        printer._printer->write(0xff);
      }
    }
  }*/
  //printer.feedRows(100);
  //printer.feedRows(100);
  //printer.printBitmap(96,120,a,false);
}

void varyHeat(byte i){
  i = max(i, 1);
  printer.setTimes(15000/i, 2100);
  updateHeatTime(255/i);
  printer.printBitmap(maxWidth/2, minHeight*2, a, false);
}

void varyWidth(byte i){
  i = max(i, 1);
  printer.setTimes(15000, 2100);
  printer.printBitmap(maxWidth/i, minHeight*2, a, false);
}

void varyWidthAndHeight(byte i){
  i = max(i, 1);
  printer.setTimes(15000, 2100);
  int height = min(minHeight*i, 255);
  printer.printBitmap(maxWidth/1, height, a, false);
}

void varyDensity(byte i){
  i = max(i, 1) - 1;
  printer.setTimes(15000, 2100);
  byte curr = 0;
  for(int j = 0, mod = 0; 
      j < maxSize; 
      j++, mod++){
    for (int k = 0, curr=0; 
         k < 8; 
         k++, mod++, curr <<= 1){ 
      if (mod >= i){
        mod = 0;
        curr |= 1;
      }
    }
    a[i] = curr;
  }
  printer.printBitmap(maxWidth/2, minHeight*2, a, false);
}

void whateversClever(byte i){
  i = max(i, 1);
  int printtime = 8000;
    int height = i*minHeight;
    byte heatTime = 128;
    while(height > 255){
      heatTime /= 2;
      height /= 2;
      i /= 2;
      printtime = printtime*2/3;
    }
  printer.setTimes(printtime, 2100);
    
    updateHeatTime(heatTime);
    printer.printBitmap(maxWidth/i, i*minHeight, a, false);
}

void loop(){
  if (Serial.available() > 0){
    byte i = Serial.read();
    Serial.write(i);
    if (i == 62){//hit the tilde 126 - 64, switch modes
      mode++;
      if (mode > maxMode){
        mode = 0;
      }
      Serial.write(mode);
    }
    if (mode == 0){
      whateversClever(i);
    } else if (mode == 1){
      varyWidth(i);
    } else if (mode == 2){
      varyWidthAndHeight(i);
    } else if (mode == 3){
      varyDensity(i);
    } else if (mode == 4){
      varyHeat(i);
    }
    /*
    int height = i*minHeight;
    byte heatTime = 64;
    while(height > 255){
      heatTime /= 2;
      height /= 2;
      i /= 2;
    }
    
    for(int j = 0; j < maxSize; j++){
      
    
    updateHeatTime(heatTime);
    printer.printBitmap(maxWidth/i, i*minHeight, a, false);
    //printer.printBitmap(75,75,&Serial);*/
    Serial.write(1);
  }
}

void updateHeatTime(byte t){
  updateHeatTime(t,250);
}

void updateHeatTime(byte t, byte i){
  //reset();

  // Description of print settings from page 23 of the manual:
  // ESC 7 n1 n2 n3 Setting Control Parameter Command
  // Decimal: 27 55 n1 n2 n3
  // Set "max heating dots", "heating time", "heating interval"
  // n1 = 0-255 Max printing dots, Unit (8dots), Default: 7 (64 dots)
  // n2 = 3-255 Heating time, Unit (10us), Default: 80 (800us)
  // n3 = 0-255 Heating interval, Unit (10us), Default: 2 (20us)
  // The more max heating dots, the more peak current will cost
  // when printing, the faster printing speed. The max heating
  // dots is 8*(n1+1).  The more heating time, the more density,
  // but the slower printing speed.  If heating time is too short,
  // blank page may occur.  The more heating interval, the more
  // clear, but the slower printing speed.

  printer.writeBytes(27, 55);   // Esc 7 (print settings)
  printer.writeBytes(20);       // Heating dots (20=balance of darkness vs no jams)
  printer.writeBytes(t); // Library default = 255 (max)
  printer.writeBytes(i);      // Heat interval (500 uS = slower, but darker)
}
