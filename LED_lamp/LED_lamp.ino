#include "http.h"
#include "udp.h"
#include "web_socket.h"
#include "wifi_connection.h"
#include "config.h"

/* wemos d1 mini has inverted onboeard LED, so if you are using it, uncomment this. */
//#define USE_D1_MINI   
/* If you need linear PWM scaling, uncoment this. */
//#define USE_LINEAR_SCALING

#define LONG_TOUCH_TIMEOUT        (500u)
#define DEBOUNCE_TIMEOUT          (100u)
#define LONG_TOUCH_LED_INCREMENT  (10u)
#define LED_LEVEL_MAX             (100u)

#ifdef USE_D1_MINI
static const int LEDPIN = D4;
static const int SENSPIN = D1;            
#else
static const int LEDPIN = 2;             /* 1=>UART TX, 2=>gpio2 */
static const int SENSPIN = 3;            /* Touch sensor pin: 3=>Rx */
#endif
static int currentLedVal = 0;            /* Current LED level. used in forming HTML and id */
static int lastLedVal = 0;               /* History to be used with touch sensor function (Value to restore to) */
static unsigned long touchTime = 0;      /* Duration of a touch */
static bool touchFlag = false;           
static unsigned long touchStartTime = 0; /* First touch detection timestamp */
static unsigned long executeTime = 0;    /* On touch LED level change timestamp */
static String statusMessage = ""; 
static unsigned long LedWriteTimestamp = 0;

void MAIN_setStatusMsg(String msg){
  statusMessage = msg;
}

String MAIN_getStatusMsg(void){
  return statusMessage;
}

int MAIN_getCurrentLedVal( void ){
  return currentLedVal;
}

/* Set LED level in percentage */
void MAIN_writeLED(int val)
{
  if((millis() - LedWriteTimestamp) < DEBOUNCE_TIMEOUT){
    return;
  }
  Serial.print("LED:");
  Serial.println(val);
  double level = val;
  
  if(level < 0){
      level = 0;
  }
  if(level > LED_LEVEL_MAX){
    level = LED_LEVEL_MAX;
  }
  currentLedVal = level;

#ifdef USE_LINEAR_SCALING 
  level = level * 10 + level / 4;
#else
  if(level < 30){
    level = level / 4;
  }else{
    level = pow(2, level / 10);
  }
#endif

  if(level > 1023){
    level = 1023;
  }    

#ifdef USE_D1_MINI
  /* Wemos D1 MNI onboard LED is reversed polarized. */
  analogWrite(LEDPIN, 1023 - level);
#else
  analogWrite(LEDPIN, level);
#endif 

  LedWriteTimestamp = millis();
}

/* Processes the touch detection */
void updateTouchSensor(){ 
  pinMode(SENSPIN, OUTPUT);
  digitalWrite(SENSPIN, LOW);  
//  delayMicroseconds(1);
  delay(1); 
    
  volatile uint8_t chargeTime = 0;
  pinMode(SENSPIN, INPUT_PULLUP);
  while(digitalRead(SENSPIN) == LOW){
    chargeTime++;
  }
  
  if(chargeTime > 1){ 
    touchTime = millis();
    
    if(!touchFlag){
      /* Just pressed */
      touchStartTime = touchTime;
      executeTime = touchTime;      
    }

    if((touchTime - executeTime) > LONG_TOUCH_TIMEOUT){
      /* Long press, so increment LED */  
      MAIN_writeLED(currentLedVal + LONG_TOUCH_LED_INCREMENT);
      executeTime = touchTime;
    } 

    touchFlag = true;   
  }else{
    /* Debounce */  
    if((millis() - touchTime) > DEBOUNCE_TIMEOUT){ 
      if(touchFlag){
        /* just released */        
        if((touchTime - touchStartTime) < LONG_TOUCH_TIMEOUT){  
          /* Short touch */
          if(currentLedVal > 0){
            /* LED is on. Turn it off. */
            lastLedVal = currentLedVal;
            MAIN_writeLED(0);
          }else{
            /* LED is off. Turn it on. */
            if(lastLedVal > 0){
              MAIN_writeLED(lastLedVal);
            }else{
              MAIN_writeLED(LED_LEVEL_MAX);
            }
          }
        }else{
          /* Do nothing becaouse we already changed LED level by long touch */ 
        }
      }
      touchFlag = false; 
    }
  }  
}


void setup(void) {
  Serial.begin(115200,SERIAL_8N1,SERIAL_TX_ONLY); /* Use only tx, so rx can be used as GPIO */   
  //ESP.eraseConfig();
  pinMode(LEDPIN, OUTPUT);
  MAIN_writeLED(0);
  
  WIFIC_init(); 
  ESP.wdtFeed();
  HTTP_init();
  ESP.wdtFeed();
  WS_init();  
  ESP.wdtFeed();
  UDP_init(); 
}


void loop(void) {
  ESP.wdtFeed();
  HTTP_process();
  ESP.wdtFeed();
  WS_process();
  ESP.wdtFeed();
  UDP_process();
  ESP.wdtFeed();
  WIFIC_process(); 
  ESP.wdtFeed();
  updateTouchSensor(); 
}
