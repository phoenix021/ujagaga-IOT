#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ESP8266HTTPClient.h>
#include <pgmspace.h>
#include "logo.h"
#include "http.h"
#include "udp.h"
#include "wifi_connection.h"
#include "web_socket.h"
#include "config.h"
#include "thermometer.h"

static const char HTML_BEGIN[] PROGMEM = R"(
<!DOCTYPE HTML>
<html>
  <head>
    <meta name = "viewport" content = "width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0">
    <title>Ujagaga WiFi Thermometer</title>
    <style>
      body { background-color: white; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }
      .contain{width: 100%;}.center_div{margin:0 auto; max-width: 400px;position:relative;}
    </style>
  </head>
  <body>
)";

static const char HTML_END[] PROGMEM = "</body></html>";

static const char INDEX_HTML_0[] PROGMEM = R"(
<style>
.btn_cfg{border:0;border-radius:0.3rem;color:#fff;line-height:1.4rem;font-size:0.8rem;margin:1ch;height:2rem;width:15ch;background-color:#ff3300;}
h1{text-align: center;font-size:1.5rem;}
#temp{text-align: center;font-size:9em;padding: 0; margin:0;font-weight:bold;color:#1fa3ec;}
p{text-align:center;}
</style>
<div class="contain">
  <div class="center_div">
)";

const char INDEX_HTML_1[] PROGMEM = R"( 
<h1>Ujagaga WiFi Thermometer</h1>
<p><span id="temp"></span><span style=\"font-size:2em;\">&#8451;</span></p><br>
</div>
<hr>
<p id='status'></p> 
  <br><button class="btn_cfg" type="button" onclick="location.href='/selectap';">Configure wifi</button><br/>
</div>
<script>
  var cn=new WebSocket('ws://'+location.hostname+':81/');
  cn.onopen=function(){
    cn.send('{"CURRENT":,"STATUS":}');    
  };
  cn.onmessage=function(e){
    var data=JSON.parse(e.data);  
    if(data.hasOwnProperty('CURRENT')){ 
)"    
#ifdef DISPLAY_INTEGER_TEMPERATURE
      "document.getElementById('temp').innerHTML=parseInt(data.CURRENT).toString();"
#else
      "document.getElementById('temp').innerHTML=data.CURRENT;"
#endif    
R"(  
    }  
  };  
</script>
)";

static const char APLIST_HTML_0[] PROGMEM = R"(
<style>
  .c{text-align: center;}
  div,input{padding:5px;font-size:1em;}
  input{width:95%;}
  body{text-align: left;}
  button{width:100%;border:0;border-radius:0.3rem;color:#fff;line-height:2.4rem;font-size:1.2rem;height:40px;background-color:#1fa3ec;}
  .q{float: right;width: 64px;text-align: right;}
  .radio{width:2em;}
  #vm{width:100%;height:50vh;overflow-y:auto;margin-bottom:1em;}
</style>
</head><body>  
  <div class="contain">
    <div class="center_div">
)";    

/* Placeholder for the wifi list */
static const char APLIST_HTML_1[] PROGMEM = R"(   
      <h1 id='ttl'>Networks found:</h1>
      <div id='vm'>
)";  

static const char APLIST_HTML_2[] PROGMEM = R"( 
      </div>
      <form method='get' action='wifisave'>
        <button type='button' onclick='refresh();'>Rescan</button><br/><br/>
        <input id='s' name='s' length=32 placeholder='SSID (Leave blank for AP mode)'><br>      
        <input id='p' name='p' length=32 placeholder='password'><br>
        <input id='a' name='a' length=16 placeholder='static IP address (optional)'><br>       
        <br><button type='submit'>save</button></form>
     </div>
  </div>
<script>
  function c(l){
    document.getElementById('s').value=l.innerText||l.textContent;
    document.getElementById('p').focus();
  }
  
  var cn=new WebSocket('ws://'+location.hostname+':81/');
  cn.onopen=function(){
    cn.send('{"APLIST":}');
  }
  cn.onmessage=function(e){
    var data=JSON.parse(e.data);
    if(data.hasOwnProperty('APLIST')){
      rsp=data.APLIST.split('|');
      document.getElementById('vm').innerHTML='';
      for(var i=0;i<(rsp.length);i++){
        document.getElementById('vm').innerHTML+='<span>'+(i+1)+": </span><a href='#p' onclick='c(this)'>" + rsp[i] + '</a><br>';
      }
      if(!document.getElementById('vm').innerHTML.replace(/\\s/g,'').length){
        document.getElementById('ttl').innerHTML='No networks found.'
      } 
    }
  };
  function refresh(){
    document.getElementById('vm').innerHTML='Please wait...'
    cn.send('{"APLIST":}');
  }
</script>
)";

static const char REDIRECT_HTML[] PROGMEM = R"(
<p id="tmr"></p>
<script>
  var c=6;   
  function count(){
    var tmr=document.getElementById('tmr');   
    if(c>0){
      c--;
      tmr.innerHTML="You will be redirected to home page in "+c+" seconds.";
      setTimeout('count()',1000);
    }else{
      window.location.href="/";
    }
  }
  count();
</script> 
)";


static ESP8266WebServer webServer(80);

static void showNotFound(void){
  webServer.send(404, "text/html; charset=iso-8859-1","<html><head> <title>404 Not Found</title></head><body><h1>Not Found</h1></body></html>");
}

String HTTP_getFeatures(){
  Serial.println("returnFeatures");
  String response = "{\"MAC\":\"";
  response += WiFi.macAddress();
  response += ":";
  response += DEV_ID;
  response += "\"MODEL\":\"ujagaga WiFi Thermometer\"";
  response += ",\"CURRENT\":";
  response += MAIN_getCurrentTempFloatString();  
  response += "}";  
  return response;     
}

void showID( void ) {    
  Serial.println("show ID");
  webServer.send(200, "text/plain", HTTP_getFeatures());  
}

void showStartPage() {    
  Serial.println("showStartPage");
  String response = FPSTR(HTML_BEGIN);
  response += FPSTR(INDEX_HTML_0);
  response += FPSTR(LOGO);
  response += FPSTR(INDEX_HTML_1); 
  response += FPSTR(HTML_END);
  webServer.send(200, "text/html", response);  
}

static void showStatusPage(bool goToHome = false) {    
  //Serial.println("showStatusPage");
  String response = FPSTR(HTML_BEGIN);
  response += "<h1>Connection Status</h1><p>";
  response += MAIN_getStatusMsg() + "</p>";
  if(goToHome){
    /* Add redirect timer. */
    response += FPSTR(REDIRECT_HTML);
  }
  response += FPSTR(HTML_END);
  webServer.send(200, "text/html", response);   
}

/* Sends response to http request for configuring AP to connect to */
static void selectAP(void) {   
  String response = FPSTR(HTML_BEGIN);
  response += FPSTR(APLIST_HTML_0);  
  response += FPSTR(LOGO);
  response += FPSTR(APLIST_HTML_1);
  response += "Please wait...";  
  response += FPSTR(APLIST_HTML_2);   
  response += FPSTR(HTML_END);
  webServer.send(200, "text/html", response);  
}

/* Saves wifi settings to EEPROM */
static void saveWiFi(void){
  String ssid = webServer.arg("s");
  String pass = webServer.arg("p");
  String ipaddr = webServer.arg("a");
  
  if((ssid.length() > 63) || (pass.length() > 63)){
      MAIN_setStatusMsg("Sorry, this module can only remember SSID and a PASSWORD up to 63 bytes long.");
      showStatusPage(); 
      return;
  } 

  IPAddress newStationIP;
  newStationIP.fromString(ipaddr);

  String st_ssid = "";
  String st_pass = "";
  IPAddress stationIP;

  if(ssid.length() > 0){
    bool cmpFlag = true;

    st_ssid = WIFIC_getStSSID();
    st_pass = WIFIC_getStPass();
    stationIP = WIFIC_getStIP();

    if(!st_ssid.equals(ssid) || !st_pass.equals(pass)){
      cmpFlag = false;
    }
      
    if(cmpFlag){
       if((newStationIP[0] != stationIP[0]) || (newStationIP[1] != stationIP[1]) || (newStationIP[2] != stationIP[2]) || (newStationIP[3] != stationIP[3])){
          cmpFlag = false;
       }
    }
  
    if(cmpFlag){
      MAIN_setStatusMsg("All parameters are already set as requested.");
      showStatusPage(true);      
      return;
    }   
  }
  
  WIFIC_setStSSID(ssid);
  WIFIC_setStPass(pass);
  WIFIC_setStIP(newStationIP);

  String http_statusMessage;
  
  if(ssid.length() > 3){    
    http_statusMessage = "Saving settings and connecting to SSID: ";
    http_statusMessage += ssid; 
    http_statusMessage += " ,IP: ";
    
    if(WIFIC_checkValidIp(newStationIP)){      
      http_statusMessage += ipaddr;
    }else{
      http_statusMessage += "dynamically assigned by DHCP";
    }
    
  }else{       
    http_statusMessage = "Saving settings and switching to AP mode only.";    
  }
  http_statusMessage += "<br>If you can not connect to this device 20 seconds from now, please, reset it.";

  MAIN_setStatusMsg(http_statusMessage);
  showStatusPage();

  volatile int i;

  /* Keep serving http to display the status page*/
  for(i = 0; i < 100000; i++){
    webServer.handleClient(); 
    ESP.wdtFeed();
  } 

  /* WiFI config changed. Restart to apply. 
   Note: ESP.restart is buggy after programming the chip. 
   Just reset once after programming to get stable results. */
  ESP.restart();
}

void HTTP_process(void){
  webServer.handleClient(); 
}

void HTTP_init(void){
 
  webServer.on("/", showStartPage);
  webServer.on("/id", showID);
  webServer.on("/favicon.ico", showNotFound);
  webServer.on("/selectap", selectAP);
  webServer.on("/wifisave", saveWiFi);
  webServer.onNotFound(showStartPage);
  
  webServer.begin();
}



