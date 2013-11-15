#ifndef ArduinoNodeNetwork_h
#define ArduinoNodeNetwork_h

#include <arduino.h>

#define PACKET_MAX 4
#define PACKET_MAX_PAYLOAD 30
#define PAKCET_MAX_RETRY 2
#define PACKET_TRANS_DELAY 1000

class ArduinoNodeNetwork{
	//***********************************************************************************************************
public:
	ArduinoNodeNetwork();
	void SetId( byte in_myId );
	void SendNew(byte in_to, const char * in_payload);
	void SendReply(byte pNum);
	char * GetPayloadPointer(byte pNum);
	byte GetNextRequestPacket();
	void Update();
	void SerialRecieved();

private:
	//***********************************************************************************************************
	//Internal Structures
	//***********************************************************************************************************
	struct PacketStruct{
		boolean isUsed;
		byte retryCount;
		byte to;
		byte from;
		char type;
		byte seqNum;
		char payload[PACKET_MAX_PAYLOAD + 1];
	};


	//***********************************************************************************************************
	//class global variables
	//***********************************************************************************************************
	PacketStruct packet[PACKET_MAX + 1];
	byte myId;
	unsigned long lastTimeUpdated;
	char inBuff[11 + PACKET_MAX_PAYLOAD + 1];


	//***********************************************************************************************************
	//Private functions
	//***********************************************************************************************************
	void ClearPacket( byte pNum );
	byte GetFreePacketNum();
	byte GetRandomSeqNum();
	void SendPacket(byte pNum);
	void DecodeInputBuffer();
	void HandleAckPacket(byte from, byte seqNum);
	byte DecodeHexFromBuff( char buff[], byte offset);
	void EncodeHexToBuff(byte num, char * buff, byte offset);
	void WaitForSerialBusFree();


};


#endif