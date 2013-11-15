#include <ArduinoNodeNetwork.h>

//*****************************************************
// ArduinoNodeNetwork 
//
// Constructor, stores my ID for encdoing/decoding packets
// and initializing timer for update period
//*****************************************************
ArduinoNodeNetwork::ArduinoNodeNetwork(){
  //INIT Timer
  lastTimeUpdated = millis();

  //INIT Packet buffers
  for (byte i=0; i<PACKET_MAX; i++){ ClearPacket( i ); } 
}

//*****************************************************
// setId( id )
//*****************************************************
void ArduinoNodeNetwork::SetId(byte in_myId){ myId = in_myId; }


//*****************************************************
// Send New (to, payload pointer)
//
// Applys the application to sned a new packet without
// a request from the host (IE Trigger based event)
// and handels ensuring the packet is delivered (ack).
//*****************************************************
void ArduinoNodeNetwork::SendNew(byte in_to, const char * in_payload){ 
  byte pNum = GetFreePacketNum(); 
  if (pNum == 255){  return; }  //Invalid packet id, buffer must be full
  packet[pNum].isUsed = true;
  packet[pNum].to = in_to;
  packet[pNum].from = myId;
  packet[pNum].type = 'N';
  packet[pNum].seqNum = GetRandomSeqNum();

  strncpy( packet[pNum].payload, in_payload, PACKET_MAX_PAYLOAD );
  packet[pNum].payload[PACKET_MAX_PAYLOAD + 1] = 0;  //Make sure last char of sting is 0 terminated

  lastTimeUpdated = 0;    //Reset time to 0 so transmit que picks up right away on next update     
}


//*****************************************************
// Send Reply (packetId) 
//
// Sends a reply from an incoming request
//*****************************************************
void ArduinoNodeNetwork::SendReply(byte pNum){
//Swap around to and from addresses
byte newTo = packet[pNum].from;
packet[pNum].from =  packet[pNum].to;
packet[pNum].to = newTo;

//Switch packet from New to Ack
packet[pNum].type = 'A';
SendPacket( pNum );
}


//*****************************************************
// GetPayloadPointer (packetId)
//
// Returns the pointer to the payload of a packet so
// the application can read or modify the payload for reply
//*****************************************************
char * ArduinoNodeNetwork::GetPayloadPointer(byte pNum){
return packet[pNum].payload;
}


//*****************************************************
// GetNextRequestPacket ()
//
// Returns the packet number of the next request packet
// on the packet que so the system can reply to the request
//*****************************************************
byte ArduinoNodeNetwork::GetNextRequestPacket(){
byte pNum = 255;
for (byte i=0; i<PACKET_MAX; i++){
   if (packet[i].isUsed == true && packet[i].to == myId && packet[i].type=='N'){ pNum = i; break; }
}
return pNum;
}


//*****************************************************
// Update ()
//
// Handles checking and re-transmitting packets in the que
//*****************************************************
void ArduinoNodeNetwork::Update(){

//Only check for transmit according to PACKET_TRANS_DELAY
if (lastTimeUpdated + PACKET_TRANS_DELAY > millis()){ return; }
lastTimeUpdated = millis();

//Look for packets that need to be transmitted and send them
for (byte i=0; i<PACKET_MAX; i++){
  if (packet[i].isUsed == true && packet[i].to != myId){ SendPacket( i ); } 
} 
}


//*****************************************************
// SerialRecieve ()
// 
// Handles reading new serial data and invoking packet
// decoding
//*****************************************************
void ArduinoNodeNetwork::SerialRecieved(){
delay (20);  //Wait for full string to arrive hopefully
char serialByte[2];
serialByte[1] = '\0';

while (Serial.available()) {
  serialByte[0] = (char) Serial.read();
  if (serialByte[0] == '\n'){
    DecodeInputBuffer();
    inBuff[0] = '\0';
  }else{
    strncat( inBuff, serialByte, 11 + PACKET_MAX_PAYLOAD );
  }
}


}

//*****************************************************
// ClearPacket (pNum)
//
// Resets a packet to defaults and readies it for new data
//*****************************************************
void ArduinoNodeNetwork::ClearPacket( byte pNum ){
  packet[pNum].isUsed = false;    //Is the packet in use or free
  packet[pNum].retryCount = 0;    //How many times have we tried to send this packet
  packet[pNum].to = 0;            //Where is the packet going (destination)
  packet[pNum].from = 0;          //Where did the packet come from (source)
  packet[pNum].type = '\0';          //What kind of packet is it (N = New, A = Ack)
  packet[pNum].seqNum = 0;        //A unique number to identify the packet, used for ensuring packets are delivered
  packet[pNum].payload[0]='\0';      //The contents of the packet
}

//*****************************************************    
// GetFreePacketNum
//
// Retruns the packet number of the next free packet buffer
// 255 Means failure to find a free packet
//*****************************************************
byte ArduinoNodeNetwork::GetFreePacketNum(){
  byte freePacket = 255;
  for (byte i=0; i<PACKET_MAX; i++){
    if (packet[i].isUsed == false){ freePacket = i; break; }
  } 
  
  return freePacket;
}


//*****************************************************    
// GetRandomSeqNum 
//
// Gets a random seq number not used by other packets
// Sequance numbers are used to ensure transmission and ack
//*****************************************************    
byte ArduinoNodeNetwork::GetRandomSeqNum(){
  byte seqNum = 0;
  byte pNum = 0;
  
  while (seqNum == 0){
    seqNum = random(1,255);
    for (pNum = 0; pNum<PACKET_MAX; pNum++){
      if ( seqNum == packet[pNum].seqNum ){ seqNum = 0; break; }
    }
  }  
  
  return seqNum;
}


//*****************************************************    
// SendPacket( pNum )
//
// Encodes and sends a packet 
//*****************************************************    
void ArduinoNodeNetwork::SendPacket(byte pNum){
  char packetHeader[12];
  
  //to & Address
  packetHeader[0] = 't';
  EncodeHexToBuff( packet[pNum].to, packetHeader, 1 );
  
  //from & address
  packetHeader[3] = 'f';
  EncodeHexToBuff( packet[pNum].from, packetHeader, 4 );
  
  //type & seq #
  packetHeader[6] = 'p';
  packetHeader[7] = packet[pNum].type;
  EncodeHexToBuff( packet[pNum].seqNum, packetHeader, 8 );
  
  //Payload seperator and null charactor
  packetHeader[10] = ':';
  packetHeader[11] = '\0';
  
  //Wait for serial buss to be free and send
  WaitForSerialBusFree();
  Serial.print( packetHeader );
  Serial.println( packet[pNum].payload );
  
  //Handle post send packet actions
  if ( packet[pNum].type == 'A' ){ ClearPacket( pNum ); }
  if ( packet[pNum].type == 'N' ){
    packet[pNum].retryCount++;
    if ( packet[pNum].retryCount >= PAKCET_MAX_RETRY ){ ClearPacket( pNum ); }
  }      
}


//*****************************************************    
// DecodeInputBuffer
//
// Decodes an incoming packet from inBuff
//***************************************************** 
void ArduinoNodeNetwork::DecodeInputBuffer(){  
        
  //Check & Decode to
  if ( inBuff[0] != 't' ){ return; }
  byte to = DecodeHexFromBuff( inBuff, 1 );
  if (to != myId){ return; }    
  
  //Check & Decode from
  if ( inBuff[3] != 'f' ){ return; }
  byte from = DecodeHexFromBuff( inBuff, 4 );
  
  //Check & Decode packet type
  if ( inBuff[6] != 'p'){ return; }
  char type = inBuff[7];
  if ( type != 'A' && type !='N' ){ return; }
        
  //Deocde Seq Num
  byte seqNum = DecodeHexFromBuff( inBuff, 8 );
  
  //Check for packet payload seperator
  if ( inBuff[10] != ':' ){ return; }
  
  //****************************************
  // Handle Ack Packets by deleteing N packets from que
  if (type == 'A'){ HandleAckPacket(from, seqNum); return; }  
  
  
  //****************************************
  // Handle New (new) inboud packets
  byte pNum = GetFreePacketNum(); 
  if (pNum == 255){  return; }  //Invalid packet id, buffer must be full    

  //Stuff info into packet que
  packet[pNum].isUsed = true;
  packet[pNum].retryCount = 0;
  packet[pNum].to = to;
  packet[pNum].from = from;
  packet[pNum].type = type;
  packet[pNum].seqNum = seqNum;
  
  char buffByte;
  byte i;
  for (i=0; i<PACKET_MAX_PAYLOAD; i++){
    buffByte = inBuff[i+11];
    packet[pNum].payload[i] = buffByte;
    
    if (buffByte == '\0'){break; }
  }
  packet[pNum].payload[i + 1] = '\0';      
}


//*****************************************************
// HandleAckPacket (from, seq)    
//
// Removes outbound packets from the que when an ack is recieved
//*****************************************************
void ArduinoNodeNetwork::HandleAckPacket(byte from, byte seqNum){
  for (byte i=0; i<PACKET_MAX; i++){
    if ( packet[i].to == from && packet[i].seqNum == seqNum){ ClearPacket(i); }
  }
}


//*****************************************************
// DecodeHexFromBuff (buff, offset)
//
// Converts hex to a byte number
//*****************************************************
byte ArduinoNodeNetwork::DecodeHexFromBuff( char buff[], byte offset){
  char HexArray[17] = "0123456789ABCDEF";
  byte num_1 = 0;
  byte num_2 = 0;

  for (byte i=0; i<16; i++){
    if ( buff[offset] == HexArray[i] ){ num_1 = i; } 
    if ( buff[offset + 1] == HexArray[i] ){ num_2 = i; } 
  }
  
  return (num_1 * 16) + num_2;
}


//*****************************************************
// EncodeHexToBuff (num, buff, offset)
//
// Adds a 2-digit hex num (0 padded) to a buffer at offset
//*****************************************************
void ArduinoNodeNetwork::EncodeHexToBuff(byte num, char * buff, byte offset){
  char HexArray[17] = "0123456789ABCDEF";
  byte num_1 = num / 16;
  byte num_2 = num - (num_1 * 16);
  buff[ offset ] = HexArray[ num_1 ];
  buff[ offset+1 ] = HexArray[ num_2 ];
}


//*****************************************************
// WaitForSerialBusFree
//
// Waits for activity to stop on the serial bus
//*****************************************************
void ArduinoNodeNetwork::WaitForSerialBusFree(){
  //TODO!!
  delay(0);
} 


