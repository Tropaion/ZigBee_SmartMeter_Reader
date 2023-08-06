# ZigBee_SmartMeter_Reader
This is a open-source ZigBee device based on the new ESP32-C6 to collect data from smartmeters using the DLMS/COSEM protocol.

Ever since I got the smartmeter "Sagemcom T-210D" with the P1 user interface, I wanted to integrate it into my ZigBee network.
Since I didn't find any existing solution, I decided to develop my own.

THE PROJECT IS STILL WIP.

# Hardware
View the latest [PCB Interactive BOM](https://htmlpreview.github.io/?https://github.com/Tropaion/ZigBee_SmartMeter_Reader/blob/main/hardware/bom/ibom.html).

<img src="https://github.com/Tropaion/ZigBee_SmartMeter_Reader/blob/main/images/pcb.png?raw=true" width="35%" />
<img src="https://github.com/Tropaion/ZigBee_SmartMeter_Reader/blob/main/images/pcb_real.jpg?raw=true" width="35%" />

For this project, I used the [ESP32-C6-MINI-1U](https://www.espressif.com/sites/default/files/documentation/esp32-mini-1_datasheet_en.pdf), thanks to espressif for providing me a few samples.
I choose the version with external antenna, since, in most cases, the devices will be mounted in an isolated meter box. And, since many of them are made of metal (faraday cage), like in my case, 
I opted for and external antenna to mount on the outside of the meter box.

To provide easy flashing and debugging the USB-C-Socket is connected to the ESP internal USB-JTAG-Converter.
For the case that the converter is not working, the UART0 programming interface is connected to J2 and J3, which will not be assembled.

For production use in the meter box, only the 5V supply terminal (J5) which in my case will be connected to a 5V DIN Power Supply and RJ12 (J1) cable going to the smartmeter is needed.

Since most smartmeters send encrypted data, for which you need to request an decryption key from your provider.
This key has to be configured in the software. The current plan for this is to provide a simple configuration terminal which is accessible via the USB-C-Socket.

# Software
The software will be separated into four components/libraries.
Each will be individually tested and then combined.

- [ ] configuration_console - usb console to configure device (decryption key)
- [x] human_interface - handle button input and led (working)
- [ ] Zigbee - handle everything zigbee related
- [ ] UART/DLMS - read data via uart and decrypt

#### Structure of SmartMeter data
How the data is handled is a bit complicated and consists of three layers:
- MBUS-Layer
- DLMS-Layer
- OBIS-Layer

For each layer I'm writing a parser.
To understand the structure and how the parser handles the data I created a diagramm:
<img src="https://github.com/Tropaion/ZigBee_SmartMeter_Reader/blob/main/images/smartmeter_data.svg?raw=true" />

# Enclosue
The first version of the enclosure is released and now in printing.

<img src="https://github.com/Tropaion/ZigBee_SmartMeter_Reader/blob/main/images/Enclosure_1.png?raw=true" width="35%" />
<img src="https://github.com/Tropaion/ZigBee_SmartMeter_Reader/blob/main/images/Enclosure_2.png?raw=true" width="35%" />
<img src="https://github.com/Tropaion/ZigBee_SmartMeter_Reader/blob/main/images/Enclosure_3.png?raw=true" width="35%" />