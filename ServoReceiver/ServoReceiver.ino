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

Servo myservo1;  // servo control object

int servoCtrl;
int previous;

#define MAX_PACKET_SIZE (NRF905_MAX_PAYLOAD - 2)
typedef struct {
	byte dstAddress[NRF905_ADDR_SIZE];
	byte len;
	byte data[MAX_PACKET_SIZE];
} packet_s;

void setup()
{
  myservo1.attach(4);
  
	// Start up
	nRF905_init();

  byte addr[NRF905_ADDR_SIZE] = {0xcc,0xcc,0xcc,0xaa};
  
  nRF905_setRXAddress(addr);

	// Put into receive mode
	nRF905_receive();

	Serial.begin(9600);
	
	Serial.println(F("Ready"));
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
      Serial.print("servoCtrl: ");
      Serial.println(servoCtrl);  

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
