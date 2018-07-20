#include <ESP8266WiFi.h>
#include "config.h"
#include "udp.h"

static WiFiUDP UdpPing;
static WiFiUDP udpNTP;
static char incomingPacket[255];        /* buffer for incoming packets */
char UdpRreply[21] = {0};               /* Setup prepares the reply here. The content is only the chips MAC. Different for each chip, so must be populated at startup */
const char ping_msg[] = "ujagaga ping"; /* String to respond to */

void UDP_init(){
  /* Setup receive of ping messages */
  UdpPing.begin(UDP_PING_PORT);
  Serial.printf("Listenning ping messages on UDP port %d\n\r", UDP_PING_PORT);
  String UdpRreplyStr = WiFi.macAddress();
  UdpRreplyStr += ":";
  UdpRreplyStr += DEV_ID;
  UdpRreplyStr += "\n";
  UdpRreplyStr.toCharArray(UdpRreply, UdpRreplyStr.length() + 1);
  /* Setup receive of NTP messages for time sync */
  udpNTP.begin(UDP_NTP_PORT);
  
}

void UDP_process(){
  int packetSize = UdpPing.parsePacket();
  
  if (packetSize == String(ping_msg).length())
  {
    // receive incoming UDP packets
    int len = UdpPing.read(incomingPacket, String(ping_msg).length());
    incomingPacket[len] = 0;
    if(String(incomingPacket).equals(ping_msg)){
      //Serial.println("Ping received");
      // send back a reply, to the IP address and port we got the packet from
      delay(random(13));
      UdpPing.beginPacket(UdpPing.remoteIP(), UdpPing.remotePort());
      UdpPing.write(UdpRreply);
      UdpPing.endPacket(); 
    }    
  }
}

bool UDP_getNtpMessage(uint8_t* ntp_buffer){
  int msgLen = udpNTP.parsePacket();
  
  if(msgLen > 0){
    udpNTP.read(ntp_buffer, NTP_PACKET_SIZE); 

    return true;
  }
  
  return false;  
}

/* send an NTP request to the time server at the given address */
void UDP_sendNtpPackage(IPAddress ipAddr, uint8_t* data)
{
  udpNTP.beginPacket(ipAddr, 123); /* NTP requests are to port 123 */
  udpNTP.write(data, NTP_PACKET_SIZE);
  udpNTP.endPacket();
}

