#include <ArduinoNodeNetwork.h>
ArduinoNodeNetwork myNetwork(2);

void setup(){
  Serial.begin(57600);
  Serial.println( F("Running!") );
  myNetwork.SendNew(25, "Ready!");
  
}

void loop(){
  myNetwork.Update();  
  delay(50);
}

void serialEvent(){ 
  myNetwork.SerialRecieved(); 
  checkRequests();
  myNetwork.Update();
}

void checkRequests(){
  byte pNum;
  
  while (true){
    pNum = myNetwork.GetNextRequestPacket();
    if (pNum == 255){ break; }
    processesRequest(pNum);
  }  
}

void processesRequest(byte pNum){
  char * buffer = myNetwork.GetPayloadPointer( pNum );
  //Serial.print( F("DEBUG - Handeling command: ") );
  //Serial.println(buffer);
  
  boolean cmdFound = false;
  
  if ( strcmp(buffer, "ping") == 0 ){ cmdFound = true; strcpy(buffer, "pong"); }
  
  if ( !cmdFound ){ strcpy(buffer, "Unkown Command"); }
  myNetwork.SendReply( pNum );
}
   
