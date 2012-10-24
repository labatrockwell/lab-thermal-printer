import spacebrew.*;
import processing.serial.*;

String server="ec2-184-72-140-184.compute-1.amazonaws.com";
String name="thermalPrinter";
String description ="A thermal printer that will print out text, pictures, booleans, or a graph of whatever range you send to it. It will send a bang when it is ready for more input.";

Spacebrew spacebrewConnection;
Serial myPort;
boolean canSend = true;

void setup() {
  size(600, 400);
  println(Serial.list());
  myPort = new Serial(this, Serial.list()[0], 9600);
  
  spacebrewConnection = new Spacebrew( this );
  
  // add each thing you publish to
  spacebrewConnection.addPublish( "ready", true ); 

  spacebrewConnection.addSubscribe( "text", "textCallback", "string" );
  spacebrewConnection.addSubscribe( "bool", "boolCallback", "boolean" );
  spacebrewConnection.addSubscribe( "range", "rangeCallback", "range" );
  
  // connect!
  spacebrewConnection.connect("ws://"+server+":9000", name, description );
  
}

void draw() {
  if (myPort.available() > 0 && myPort.read() == 1){
    canSend = true;
    spacebrewConnection.send("ready", true);
  }
}

void textCallback( String value ){
  if (!canSend) return;
  canSend = false;
  myPort.write("s" + value);
  myPort.write((byte)255);
}

void boolCallback( boolean value){
  if (!canSend) return;
  canSend = false;
  myPort.write('b');
  myPort.write((byte)(value?1:0));
}

void rangeCallback( int value){
  if (!canSend) return;
  canSend = false;
  myPort.write('r');
  int output = floor(map(constrain(value, 0, 1023), 0, 1023, 0, 255));
  println(output);
  myPort.write((byte)output);
}

void onStringMessage( String name, String value){}
void onBooleanMessage( String name, boolean value){}
void onRangeMessage( String name, int value){}
