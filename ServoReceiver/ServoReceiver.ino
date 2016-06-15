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

#define TYPE_DATA 0
#define TYPE_ACK 1

Servo myservo1;  // servo control object

int servoCtrl;
int previous;

byte swAddr = 0x11;

#define MAX_PACKET_SIZE (NRF905_MAX_PAYLOAD - 2)
typedef struct {
	byte dstAddress[NRF905_ADDR_SIZE];
  byte type;
	byte len;
	byte data[MAX_PACKET_SIZE];
} packet_s;

void setup()
{
  myservo1.attach(4);

  Serial.begin(9600);

  Serial.println("Begin-------------------------");
  
	// Start up
	nRF905_init();

  packet_s addrPacket;

  byte revAddr = swAddr;

  //Loop from address 0x11 to 0x99 (10 addresses), exit if the matched address is found
  while(1){
    
    byte addr[NRF905_ADDR_SIZE] = {0xcc,0xcc,0xcc,revAddr};
  
    nRF905_setRXAddress(addr);
    
    // Put into receive mode
    nRF905_receive();

    Serial.print("Searching addr: ");
    Serial.println(revAddr,HEX);

    //a packet is received and the address is correct 
    if(getPacket(&addrPacket)){
      Serial.println("Packet receievd, Matching address...");
      
      Serial.print("Reciecved addr: ");
      Serial.println(addrPacket.data[0],HEX);
      if(revAddr == addrPacket.data[0])break;
      else Serial.println("Wrong address");
      
      
      }
    else Serial.println("No recieved packet, Searching...");

    revAddr = revAddr + 0x11;

    if(revAddr > 0x99){
      revAddr = swAddr;
      }

    }

  swAddr = revAddr;

  for(int i=0; i<100; i++){
    sendACK();
    }
	
	Serial.println("Ready");
  Serial.println("Using address: ");
  Serial.println(swAddr, HEX);
}

void loop()
{
  
  packet_s packet;
	// Put into receive mode
	nRF905_receive();

	// Wait for data

		if(getPacket(&packet) && packet.type == TYPE_DATA) // Got a data packet?
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

void sendACK(){
  //send ack message to controller 
  packet_s ackPacket;
  
  ackPacket.dstAddress[0] = 0xcc;
  ackPacket.dstAddress[1] = 0xcc;
  ackPacket.dstAddress[2] = 0xcc;
  ackPacket.dstAddress[3] = swAddr;

  ackPacket.type = TYPE_ACK;
  ackPacket.len = 0; 

  sendPacket(&ackPacket);
    
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
	packet->type = buffer[0];
  packet->len = buffer[1];

	// Sanity check
	if(packet->len > MAX_PACKET_SIZE)
		packet->len = MAX_PACKET_SIZE;

	memcpy(packet->data, &buffer[2], packet->len);
	
	return true;
}

// Send a packet
static void sendPacket(void* _packet)
{
  // Void pointer to packet_s pointer hack
  // Arduino puts all the function defs at the top of the file before packet_s being declared :/
  packet_s* packet = (packet_s*)_packet;

  // Convert packet data to plain byte array
  byte totalLength = packet->len + 2;
  byte tmpBuff[totalLength];
  tmpBuff[0] = packet->type;
  tmpBuff[1] = packet->len;
  memcpy(&tmpBuff[1], packet->data, packet->len);

  // Set address of device to send to
  nRF905_setTXAddress(packet->dstAddress);

  // Set payload data
  nRF905_setData(tmpBuff, totalLength);

  // Send payload (send fails if other transmissions are going on, keep trying until success)
  while(!nRF905_send());
}
