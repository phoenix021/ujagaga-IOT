#ifndef CONFIG_H
#define CONFIG_H

#define WIFI_PASS_EEPROM_ADDR   (0)
#define WIFI_PASS_SIZE          (32)
#define SSID_EEPROM_ADDR        (WIFI_PASS_EEPROM_ADDR + WIFI_PASS_SIZE)
#define SSID_SIZE               (32)
#define STATION_IP_ADDR         (SSID_EEPROM_ADDR + SSID_SIZE)
#define STATION_IP_SIZE         (4)
#define EEPROM_SIZE             (WIFI_PASS_SIZE + SSID_SIZE + STATION_IP_SIZE)   

#define LARGE_TIMEOUT           (120) /* Used to check if configured AP appeared and connect to it if possible */ 
#define UDP_PORT                (4210)  /* local port to listen on */
#define DEV_ID                  "01"
#define DNS_PORT                53

#define AP_NAME_PREFIX          "ujagaga_led_"

#endif
