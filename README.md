# ujagaga-IOT

This is my personal IOT opensource project.

I am building my own WiFi controlled devices and controlling them through their own web interface.
Most devices can be an access point or a client device on a WiFi network. After it connects it gets 
difficult to find its IP address, so I also provided an Android app to list all devices on the same network. 
Every device responds to an UDP broadcast string "ujagaga ping".

So far I have an LED lamp (in LED_lamp,
a PC remote controller (in lazy_control folder), 
an Android locator app (in UjagagaUDPfinder folder),
a WiFi thermometer (in thermometer folder),
a WiFi thermostat (in thermostat folder)
and a router (for outside LAN access to all devices)

I had some help from my colegues at work to get this started.
