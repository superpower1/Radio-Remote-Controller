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
#include <Servo.h>

#define ACK 1

Servo myservo1;  // servo control object

int servoCtrl;
int previous;

byte swAddr = 0xaa;

#define MAX_PACKET_SIZE (NRF905_MAX_PAYLOAD - 2)
typedef struct {
	byte dstAddress[NRF905_ADDR_SIZE];
	byte len;
	byte data[MAX_PACKET_SIZE];
} packet_s;

void setup()
{
  myservo1.attach(4);

  Serial.begin(9600);
  
	// Start up
	nRF905_init();

  int c = 0;

  packet_s addrPacket;

  byte revAddr = swAddr;

  //Loop from address 0xaa to 0xb3 (10 addresses), exit if the matched address is found
  while(1){
    revAddr += c;
    
    byte addr[NRF905_ADDR_SIZE] = {0xcc,0xcc,0xcc,revAddr};
  
    nRF905_setRXAddress(addr);

    // Put into receive mode
    nRF905_receive();

    //a packet is received and the address is correct 
    if(!getPacket(&addrPacket)&&(addrPacket.data[0] == revAddr))break;

    c++;
    if(c >= 10){
      revAddr = swAddr;
      c=0;
      }
      
    Serial.println(F("Matching..."));
    }

  swAddr = revAddr;

  //send ack message to controller 
  packet_s ackPacket;
  
  addrPacket.dstAddress[0] = 0xcc;
  addrPacket.dstAddress[1] = 0xcc;
  addrPacket.dstAddress[2] = 0xcc;
  addrPacket.dstAddress[3] = swAddr;

  addrPacket.len = MAX_PACKET_SIZE;
  ackPacket.data[0] = ACK;  

  sendPacket(&ackPacket);
	
	Serial.println("Ready");
  Serial.println("The address is: ");
  Serial.println(swAddr, HEX);
}

void loop()
{
  packet_s packet;
	// Put into receive mode
	nRF905_receive();

	// Wait for data

		if(getPacket(&packet)) // Got a packet?
		{
			// Print data
      servoCtrl = (int) packet.data[0];
//      Serial.print("servoCtrl: ");
//      Serial.println(servoCtrl);  

		}
   if(previous != servoCtrl){
      myservo1.write(servoCtrl);
      previous = servoCtrl;
    }
	
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
