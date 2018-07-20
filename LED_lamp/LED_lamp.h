#ifndef LED_LAMP_H
#define LED_LAMP_H

extern void MAIN_writeLED(int val);
extern int MAIN_getCurrentLedVal( void );
extern void MAIN_setStatusMsg(String msg);
extern String MAIN_getStatusMsg(void);

#endif
