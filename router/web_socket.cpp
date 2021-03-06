#include <ArduinoJson.h>
#include <WebSocketsServer.h>
#include <WebSocketsClient.h>
#include "wifi_connection.h"
#include "udp.h"
#include "http.h"

WebSocketsServer wsServer = WebSocketsServer(81);
WebSocketsClient wsClient;
String ws_statusMessage = ""; 

void WS_process(){
  wsServer.loop();  
  wsClient.loop();
}

void wsClientEvent(WStype_t type, uint8_t * payload, size_t length) {
  if(type == WStype_TEXT){
    //Serial.printf("[WSc] get text: %s\n", payload);
    char textMsg[length + 1];
    for(int i = 0; i < length; i++){
      textMsg[i] = payload[i];          
    }
    textMsg[length] = 0; /* Finish the string */

    DynamicJsonBuffer jsonBuffer(128);
    JsonObject& root = jsonBuffer.parseObject(textMsg);

    if (root.success()) { 
      if(root.containsKey("ID")){    
       
      }else{
        // send message to the connected client
        wsServer.broadcastTXT(payload);
      }
    }     
  }
}

static void serverEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{  
  /* Should check here if talking to authenticated client */
  
  switch(type) {   
    case WStype_CONNECTED:
      {
//        IPAddress ip = wsServer.remoteIP(num);
//        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\r\n", num, ip[0], ip[1], ip[2], ip[3], payload);                    
      }
      break;          
          
    case WStype_DISCONNECTED:
      wsClient.disconnect();
      break;      
      
    case WStype_TEXT:
      {
        //Serial.printf("[%u] get Text: %s\r\n", num, payload);
        char textMsg[length + 1];
        for(int i = 0; i < length; i++){
          textMsg[i] = payload[i];          
        }
        textMsg[length] = 0; /* Finish the string */
      
        //Serial.print("WS_rcv:");
        //Serial.println(textMsg);        
        
        DynamicJsonBuffer jsonBuffer(128);
        JsonObject& root = jsonBuffer.parseObject(textMsg);

        // Test if parsing succeeds.
        if (root.success()) {         
          String msg;
           
          if(root.containsKey("DEVICES")){    
            String msg = UDP_devListToJson();   
            Serial.println("WS6: " + msg);
            wsServer.broadcastTXT(msg); 
          }
          
          if(root.containsKey("APLIST")){  
            String APList = "{\"APLIST\":\"" + WIFIC_getApList() + "\"}";
            //Serial.println("WS4: " + APList);  
            wsServer.sendTXT(num, APList);   
          }
          
          if(root.containsKey("STATUS")){  
            msg = "{\"STATUS\":\"" + HTTP_getStatus() + "\"}";
            //Serial.println("WS5: " + msg);              
            wsServer.broadcastTXT(msg);   
          }

          if(root.containsKey("CURRENT")){
            /* Forward to currently active device */
            wsClient.sendTXT(payload);
          }                      
        }      
      }
      break;        
  }
}

void WS_connectAsWsClient(uint8_t devId){
  /* Setup websocket client */
  wsClient.begin(UDP_getDeviceIP(devId), 81, "/");
  wsClient.onEvent(wsClientEvent);
  wsClient.setReconnectInterval(5000);
}

void WS_init(){
  wsServer.begin(); 
  wsServer.onEvent(serverEvent);  
}

void WS_ServerBroadcast(String msg){
  wsServer.broadcastTXT(msg);
}



