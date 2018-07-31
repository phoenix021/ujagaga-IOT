#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include "config.h"

static WiFiUDP Udp;
static char incomingPacket[255];        /* buffer for incoming packets */
char UdpRreply[21] = {0};               /* Setup prepares the reply here. The content is only the chips MAC. Different for each chip, so must be populated at startup */
const char ping_msg[] = "ujagaga ping"; /* String to respond to */

void UDP_init(){
  Udp.begin(UDP_PORT);
  Serial.printf("Listenning UDP on port %d\n\r", UDP_PORT);
  String UdpRreplyStr = WiFi.macAddress();
  UdpRreplyStr += ":";
  UdpRreplyStr += DEV_ID;
  UdpRreplyStr += "\n";
  UdpRreplyStr.toCharArray(UdpRreply, UdpRreplyStr.length() + 1);
}

void UDP_process(){
  int packetSize = Udp.parsePacket();
  
  if (packetSize == String(ping_msg).length())
  {
    // receive incoming UDP packets
    int len = Udp.read(incomingPacket, String(ping_msg).length());
    incomingPacket[len] = 0;
    if(String(incomingPacket).equals(ping_msg)){
      //Serial.println("Ping received");
      // send back a reply, to the IP address and port we got the packet from
      delay(random(13));
      Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
      Udp.write(UdpRreply);
      Udp.endPacket(); 
    }    
  }
}
