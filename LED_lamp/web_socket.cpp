#include <ArduinoJson.h>
#include <WebSocketsServer.h>
#include "wifi_connection.h"
#include "udp.h"
#include "http.h"
#include "LED_lamp.h"

WebSocketsServer wsServer = WebSocketsServer(81);

void WS_process(){
  wsServer.loop(); 
  
}

void WS_ServerBroadcast(String msg){
  wsServer.broadcastTXT(msg);
}

static void serverEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{ 
  if(type == WStype_TEXT){
    //Serial.printf("[%u] get Text: %s\r\n", num, payload);
    char textMsg[length];
    for(int i = 0; i < length; i++){
      textMsg[i] = payload[i];          
    }
    
    DynamicJsonBuffer jsonBuffer(128);
    JsonObject& root = jsonBuffer.parseObject(textMsg);

    // Test if parsing succeeds.
    if (root.success()) {
      if(root.containsKey("CURRENT")){
        String cmd = root["CURRENT"]; 
        if(cmd.length() > 0){
          //Serial.println("Setting new target");            
          MAIN_writeLED(root["CURRENT"]);        
        } 
        wsServer.broadcastTXT("{\"CURRENT\":" + String(MAIN_getCurrentLedVal()) + "}"); 
      }
      
      if(root.containsKey("ID")){
        String features = HTTP_getFeatures();
        wsServer.sendTXT(num, features);             
      }
      
      if(root.containsKey("APLIST")){  
        String APList = "{\"APLIST\":\"" + WIFIC_getApList() + "\"}";
        wsServer.sendTXT(num, APList);   
      }
      
      if(root.containsKey("STATUS")){  
        String statusMsg = "{\"STATUS\":\"" + MAIN_getStatusMsg() + "\"}";               
        wsServer.broadcastTXT(statusMsg);   
      }
    }      
  }   
}

void WS_init(){
  wsServer.begin(); 
  wsServer.onEvent(serverEvent);  
}




