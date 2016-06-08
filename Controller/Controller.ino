/*
 * nRF905 wiring
 *
 * 7 -> CE
 * 8 -> PWR
 * 9 -> TXE
 * 2 -> CD
 * 3 -> DR
 * 10 -> CSN
 * 12 -> SO
 * 11 -> SI
 * 13 -> SCK
 */

#include <nRF905.h>
#include <SPI.h>

#define ACK 1

int x;
byte swAddr = 0xaa;

#define MAX_PACKET_SIZE (NRF905_MAX_PAYLOAD - 2)
typedef struct {
	byte dstAddress[NRF905_ADDR_SIZE];
	byte len;
	byte data[MAX_PACKET_SIZE];
} packet_s;

void setup()
{
  Serial.begin(9600);
  
	// Start up
	nRF905_init();

  //search for available address
  byte revAddr = swAddr;

  int c = 0;

  packet_s testPacket;
  
  while(1){
    
    revAddr += c;
    
    byte addr[NRF905_ADDR_SIZE] = {0xcc,0xcc,0xcc,revAddr};
    
    nRF905_setRXAddress(addr);
    
    // Put into receive mode
    nRF905_receive();
    
    if(!getPacket(&testPacket))break;
    
    c++;
    if(c >= 10){
      revAddr = swAddr;
      c=0;
      } 
    }

  swAddr = revAddr;

  //send the available address continuously until the ack message received
  while(1){

    bool ackRev = false;
    
    packet_s addrPacket;
    packet_s ackPacket;

    addrPacket.dstAddress[0] = 0xcc;
    addrPacket.dstAddress[1] = 0xcc;
    addrPacket.dstAddress[2] = 0xcc;
    addrPacket.dstAddress[3] = swAddr;

    addrPacket.len = MAX_PACKET_SIZE;

    addrPacket.data[0] = swAddr;

    // Send packet
    sendPacket(&addrPacket);

    // Put into receive mode
    nRF905_receive();

    //wait for ack message packet
    byte startTime = millis();
    while(1){
      
      bool timeout = false;
      
      while(1){
        
        if(getPacket(&ackPacket)) break;

        else if((byte)(millis()-startTime) > 50){ //50ms timeout       
          timeout = true;
          break;
          }
        }

     if(timeout){ //timed out     
        Serial.println(F("time out, resend..."));
        break;  
        }

     else if(ackPacket.data[0] == ACK){ //recieved the ack message
      ackRev = true;
      break;
      } 
      
     }
    
    if(ackRev)break;
    }

  //now can start transmitting control signal
	Serial.println("Ready");
  Serial.println("The address is: ");
  Serial.println(swAddr, HEX);
}

void loop()
{
//  if(Serial.available()){
//    if(Serial.read() == '0'){
//        if(swAddr == 0xcc)swAddr = 0xaa;
//        else swAddr = 0xcc;
//      }
//    }
  
  x = analogRead(0);    
  int mapx = map(x, 0, 1023, 0, 180);

  Serial.print("X = ");
  Serial.println(mapx);
  
	packet_s packet;

	// Send control data
	byte dataSize;
	
		// Make sure we don't try to send more than max packet size
		if(dataSize > MAX_PACKET_SIZE)
			dataSize = MAX_PACKET_SIZE;

    packet.dstAddress[0] = 0xcc;
    packet.dstAddress[1] = 0xcc;
    packet.dstAddress[2] = 0xcc;
    packet.dstAddress[3] = swAddr;

		packet.len = dataSize;

		// Copy data from serial to packet buffer
//		for(byte i=0;i<dataSize;i++)
//			packet.data[i] = Serial.read();

    packet.data[0] = mapx;

		// Send packet
		sendPacket(&packet);
}

// Send a packet
static void sendPacket(void* _packet)
{
	// Void pointer to packet_s pointer hack
	// Arduino puts all the function defs at the top of the file before packet_s being declared :/
	packet_s* packet = (packet_s*)_packet;

	// Convert packet data to plain byte array
	byte totalLength = packet->len + 1;
	byte tmpBuff[totalLength];

	tmpBuff[0] = packet->len;
	memcpy(&tmpBuff[1], packet->data, packet->len);

	// Set address of device to send to
	nRF905_setTXAddress(packet->dstAddress);

	// Set payload data
	nRF905_setData(tmpBuff, totalLength);

	// Send payload (send fails if other transmissions are going on, keep trying until success)
	while(!nRF905_send());
}

// Get a packet
static bool getPacket(void* _packet)
{
  // Void pointer to packet_s pointer hack
  // Arduino puts all the function defs at the top of the file before packet_s being declared :/
  packet_s* packet = (packet_s*)_packet;

  byte buffer[NRF905_MAX_PAYLOAD];

  // See if any data available
  if(!nRF905_getData(buffer, sizeof(buffer)))
    return false;

  // Convert byte array to packet
  packet->len = buffer[0];

  // Sanity check
  if(packet->len > MAX_PACKET_SIZE)
    packet->len = MAX_PACKET_SIZE;

  memcpy(packet->data, &buffer[1], packet->len);
  
  return true;
}
