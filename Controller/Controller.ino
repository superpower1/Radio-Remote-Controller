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

int x;
byte SwAddr = 0xaa;

#define MAX_PACKET_SIZE (NRF905_MAX_PAYLOAD - 2)
typedef struct {
	byte dstAddress[NRF905_ADDR_SIZE];
	byte len;
	byte data[MAX_PACKET_SIZE];
} packet_s;

void setup()
{
  
	// Start up
	nRF905_init();

  byte addr[NRF905_ADDR_SIZE] = {0xcc,0xcc,0xcc,0xcc};
  
  nRF905_setRXAddress(addr);

	// Put into receive mode
	nRF905_receive();

	Serial.begin(9600);
	
	Serial.println(F("Ready"));
}

void loop()
{
  if(Serial.available()){
    if(Serial.read() == '0'){
        if(SwAddr == 0xcc)SwAddr = 0xaa;
        else SwAddr = 0xcc;
      }
    }
  
  x = analogRead(0);    
  int mapx = map(x, 0, 1023, 0, 180);

  Serial.print(F("X = "));
  Serial.println(x);
  
	packet_s packet;

	// Send serial data
	byte dataSize;
	
		// Make sure we don't try to send more than max packet size
		if(dataSize > MAX_PACKET_SIZE)
			dataSize = MAX_PACKET_SIZE;

    packet.dstAddress[0] = 0xcc;
    packet.dstAddress[1] = 0xcc;
    packet.dstAddress[2] = 0xcc;
    packet.dstAddress[3] = SwAddr;

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

