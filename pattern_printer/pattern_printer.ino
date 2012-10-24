#include "SoftwareSerial.h"
#include "Adafruit_Thermal.h"
#include "Stream.h"

/**
 * Declare which pins to communicate to the printer over
 */
int printer_RX_Pin = 5; // green wire
int printer_TX_Pin = 6; // yellow wire

boolean useCheck = false;
boolean useRandomCheck = true;

//must be multiple of 2
//must be >= 4
int checkSize = 20;
int minCheckSize = 1;
int maxCheckSize = 100;

int weftOver = 4;
int weftUnder = 4;
int weftOffset = -1;

/**
 * The Adafruit thermal printer library stipulates 384 pixel max width.
 * Currently this is a multiple of 8, which simplifies some logic.
 * If this changes to not be a multiple of 8, some code will need to be
 * added. See notes in loop().
 */
const int maxWidth = 384;
const int pixelSize = 1;
const int maxRows = 25;//31 is the max, otherwise we start seeing odd errors
const int repetitions = 10;
const int byteSize = 8;
  int pChangeStart = 5;
  int pChangeStepWarp = 1;
  int pChangeStepWeft = 0;
const int arrayWidth = maxWidth/byteSize;
const int arraySize = arrayWidth*maxRows;//maxWidth / 8 * pixelSize;
  //maxWidth is a multiple of 8, so we don't need to worry about 
  //losing data with rounding errors
//  uint8_t bitmap[arraySize];
 uint8_t bitmap[arraySize];
 boolean warpBlack[maxWidth];
 boolean didLoop = false;
 
  
/**
 * Used to keep track of which weft line we are on
 */
int line = 0;

/**
 * Initialize the thermal printer
 */
Adafruit_Thermal printer(printer_RX_Pin, printer_TX_Pin);

void setup(){
  printer.begin();
  //Serial.begin(9600);
  //Serial.println("beginning");
  
  //use random noise from input pin to seed
  randomSeed(analogRead(0));
  
  if (useCheck){
    if (useRandomCheck){
      checkSize = random(minCheckSize, maxCheckSize + 1);
    }
  }
  
  //Serial.println("setting up warp colors");
  initWarpColors();
  //Serial.println("setup warp colors");
  createPattern();
  printer.feed(1);
 // printer.printBitmap(maxWidth,maxRows,bitmap,false);
}

/**
 * Initialize the WarpBlack array which will be used to know
 * whether the warp threads in the particular column are black or white.
 */
void initWarpColors(){
  int pChange = pChangeStart;
  boolean change;
  warpBlack[0] = (random(0,2) == 1);
  for(int i = 1, numSameColor = 0; i < maxWidth; i++, numSameColor++){
    if (useCheck){
      change = numSameColor == checkSize;
    } else {
      change = random(0,100) < pChange;
    }
    warpBlack[i] = (change ? ~warpBlack[i-1] : warpBlack[i-1]);
    if (change){
      pChange = pChangeStart;
      numSameColor = 0;
    } else {
      pChange += pChangeStepWarp;
    }
  }
}

void createPattern(){
  int pChange = pChangeStart;
  uint8_t curr = 0;
  int warpOffset = 0;
  int weftOverCount;
  int bitmapOffset = 0;
  boolean change;
  boolean weftBlack = (random(0,2) == 1);
  //for each repetition
  for(int r = 0, numSameColor = 1; 
      r < repetitions; 
      r++){
    //for each row
    for(int i = 0, bitmapOffset = 0; 
        i < maxRows; 
        i++, bitmapOffset += arrayWidth, numSameColor++){
      weftOverCount = (i * weftOffset) % (weftOver + weftUnder);
      if (weftOverCount >= weftOver){
        weftOverCount -= (weftOver + weftUnder);
      } else if (weftOverCount < -weftUnder){
        weftOverCount += (weftOver + weftUnder);
      }
      //for each byte
      for(int j = 0, warpOffset = 0; 
          j < arrayWidth; 
          j++, warpOffset += byteSize){
        curr = 0;
        //for each bit
        for(int k = 0; 
            k < byteSize; 
            k++, weftOverCount++){
          if (weftOverCount >= weftOver){
            weftOverCount = -weftUnder;
          }
          curr <<= 1;
          //randomly decide whether to use weft or warp color
          //curr |= ((random(0,2) == 1 ? weftBlack : warpBlack[warpOffset+k]) & 1);
          //decide weft vs. warp based on weftOverCount
          curr |= (weftOverCount < 0 ? warpBlack[warpOffset+k] : weftBlack) & 1;
        }
        bitmap[bitmapOffset+j] = curr;
      }
      //update the weft color for the next row
      if (useCheck){
        change = numSameColor == checkSize;
      } else {
        change = random(0,100) < pChange;
      }
      weftBlack = (change ? ~weftBlack : weftBlack);
      if (change){
        pChange = pChangeStart;
        numSameColor = 1;
      } else {
        pChange += pChangeStepWeft;
      }
    }
    printer.printBitmap(maxWidth,maxRows,bitmap,false);
  }
}

/**
 * Houndstooth uses alternating 4 dark/4 light threads
 * in both the warp and weft. over 2, under 2.
 */
void loop(){
}
/*
  //Stream s = new Stream();
  //s.print(test);
  //printer.printBitmap(75,75,test, false);
  //printer.printBitmap(maxWidth, 8*pixelSize, bitmap, false);
  //delay(500);
  //return;
  boolean warpBlack = true;//re-evaluated each step
  boolean weftBlack = line < checkSize;//stays constant for this loop
  
  //weft pattern repeats every 4 lines
  //this is also re-evaluated each step since it controls weftOver param
  int weftCounter = line % checkSize;
  
  //under, over, over, under, so we xor the first and second bits
  //re-evaluated each step
  boolean weftOver =  (weftCounter ^ (weftCounter >> 1)) & 1;
  //Serial.print(line);
  //Serial.print(", ");
  //Serial.println(weftBlack);
  
  int bitmapIndex = 0;
  uint8_t curr = 0;
  uint8_t currCounter = 0;
  for(int column = 0; column < maxWidth; column++){
    warpBlack = ((column/pixelSize) % (checkSize*2)) < checkSize;
    weftOver = (weftCounter == 0 ? false : (weftCounter <= checkSize/2));//(weftCounter ^ (weftCounter >> 1)) & 1;
    
    boolean isBlack = (weftOver ? weftBlack : warpBlack);
    curr |= (isBlack ? 1 : 0);
    currCounter++;
    if (currCounter == 8){
      for(int i = 0; i < pixelSize; i++){
        bitmap[bitmapIndex + i * (arraySize/pixelSize)] = curr;
        //bitmap[bitmapIndex + arraySize/2] = curr;
      }
      bitmapIndex++;
      //Serial.print(bitmap[bitmapIndex - 1]);
      //Serial.print(' ');
      curr = 0;
      currCounter = 0;
    } else {
      curr = curr << 1;
    }
    
    //repeats every 4 columns
    if ((column % pixelSize) == pixelSize - 1){
      weftCounter--;// = (weftCounter - 1); % checkSize;
      if (weftCounter < 0){
        weftCounter = checkSize - 1;
      }
    }
  }
  //maxWidth is a multiple of 8, 
  //so we don't need to worry about having leftover bits 
  //to put in the array
    
  for(int i = 0; i < arraySize; i++){
    Serial.print("0x");
    Serial.print(bitmap[i], HEX);
    Serial.print(',');
    if ((i % (arraySize/pixelSize)) == arraySize/pixelSize - 1){
      Serial.println();
    }
  }
  
  printer.printBitmap(maxWidth,pixelSize,bitmap, false);
  Serial.println();
  //delay(100);
  //increment line
  //we only need to count up to 8 since it repeats every 8 rows
  line = (line + 1) % (checkSize*2);
}*/
