# LED lamp

Here is an Arduino sketch using ESP8266 WiFi module to controll an LED lamp. It can also be used for other purposes like driving a DC motor,
or setting an analog level on the LED pin for a voltage source in which case you would also need a low frequency RC filter.
It provides a web interface, so you can connect to it using any web enabled device.
If it does not find the configured AP, it will provide its own open AP with SSID: "ujagaga_led_" followed by MAC address of the WiFi module.
For ESP8266 general module, as a touch sensor, UART RX pin is used connected to any metal surface of around 2 square cm. 
After programming, be sure to disconnect UART RX from the programmer. 
On D1 mini, for touch sensor pin GPIO5 is used. It is labeled D1. Also, just connect to a small metal surface and controll by touch.
For best results isolate it using nail polish or plastic foil.
A short touch will toggle the LED and a long one will increase LED light level.
This sketch works with ESP8266 general module and with Wemos D1 mini, except D1 Mini has the onboard LED polarity reversed.

logo.h contains a 64-base encoded logo used in the web interface.
