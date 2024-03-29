# ZigBee_SmartMeter_Reader
This is an open-source ZigBee device based on the new ESP32-C6 to collect data from smartmeters using the DLMS/COSEM protocol.

Ever since I got the smartmeter "Sagemcom T-210D" with the P1 user interface, I wanted to integrate it into my ZigBee network.
Since I didn't find any existing solution, I decided to develop my own.

If someone is interested in buying an assembled PCB, pm me.
Currently I have one board with external antenna available.

THE PROJECT IS STILL WIP.

# Hardware
View the latest [PCB Interactive BOM](https://htmlpreview.github.io/?https://github.com/Tropaion/ZigBee_SmartMeter_Reader/blob/main/hardware/onboard_antenna/bom/ibom.html).

You can also view the latest [Schematic](https://kicanvas.org/?github=https%3A%2F%2Fgithub.com%2FTropaion%2FZigBee_SmartMeter_Reader%2Fblob%2Fmain%2Fhardware%2Fexternal_antenna%2FZBSmartMeter.kicad_sch) or [Layout](https://kicanvas.org/?github=https%3A%2F%2Fgithub.com%2FTropaion%2FZigBee_SmartMeter_Reader%2Fblob%2Fmain%2Fhardware%2Fexternal_antenna%2FZBSmartMeter.kicad_pcb).

The same hardware could also be used with WLAN and ESPHome.

<p float="left">
  <img src="https://github.com/Tropaion/ZigBee_SmartMeter_Reader/blob/main/images/pcb.png?raw=true" width="40%" />
  <img src="https://github.com/Tropaion/ZigBee_SmartMeter_Reader/blob/main/images/pcb_real.jpg?raw=true" width="40%" />
</p>


For this project, I used the [ESP32-C6-MINI-1U](https://www.espressif.com/sites/default/files/documentation/esp32-c6-mini-1_mini-1u_datasheet_en.pdf).
I chose the version with an external antenna, since, in most cases, the devices will be mounted in an isolated meter box. And, since many of them are made of metal (Faraday cage), like in my case, 
I opted for an external antenna to mount on the outside of the meter box.

To provide easy flashing and debugging the USB-C-Socket is connected to the ESP internal USB-JTAG-Converter.
In the case that the converter is not working, the UART0 programming interface is connected to J2 and J3, which will not be assembled.

For production use in the meter box, only the 5V supply terminal (J5) which in my case will be connected to a 5V DIN Power Supply and RJ12 (J1) cable going to the smartmeter is needed.

Since most smartmeters send encrypted data, you probably need to request an decryption key from your provider.
This key has to be configured in the software. The current plan for this is to provide a simple configuration terminal which is accessible via the USB-C-Socket.

# Software
The software will be separated into three core components/libraries.
Each will be individually tested and then combined.
- [x] human_interface - handles button input and led (working)
- [x] zigbee - handles zigbee basic functions
- [x] zigbee_electricity_meter - handles everything zigbee and electricity meter related (cluster)
- [ ] smartmeter - read data via uart, parse layers and decrypt data

Possible additional functionalities:
- [ ] zigbee_ota - updating firmware via zigbee
- [ ] configuration_console - usb console to configure device (decryption key)

OR
- [ ] zigbee custom configuration cluster - cluster configure device (decryption key)

## Structure of SmartMeter data
How the data is handled is a bit complicated and consists of three layers:
- MBUS-Layer
- DLMS-Layer
- OBIS-Layer

Each layer will be handled by a parser.

See the diagram to understand the structure and how the parser handles the data:
<img src="https://github.com/Tropaion/ZigBee_SmartMeter_Reader/blob/main/images/smartmeter_data.jpg?raw=true" />

## Sources
 * [esphome-dlms-meter](https://github.com/DomiStyle/esphome-dlms-meter)
 * [SmartMeter P1 Interface](https://www.netz-noe.at/Download-(1)/Smart-Meter/218_9_SmartMeter_Kundenschnittstelle_lektoriert_14.aspx)
 * [M-Bus Specification, Page 22+](https://m-bus.com/assets/downloads/MBDOC48.PDF)
 * [ZigBee Home Automation Profile](https://community.nxp.com/pwmxy87654/attachments/pwmxy87654/wireless-connectivity/698/1/075367r03ZB_AFG-Home_Automation_Profile_for_Public_Download.pdf)
 * [ZigBee Cluster Library Specification](https://zigbeealliance.org/wp-content/uploads/2019/12/07-5123-06-zigbee-cluster-library-specification.pdf)

# Enclosue
The first version of the enclosure is released and now in printing.

<img src="https://github.com/Tropaion/ZigBee_SmartMeter_Reader/blob/main/images/Enclosure.png?raw=true" width="35%" />

# Attribution
Thanks to EspressIf for sponsoring two samples of [ESP32-C6-MINI-1U](https://www.espressif.com/sites/default/files/documentation/esp32-mini-1_datasheet_en.pdf).

Thanks to [PCBWay](https://www.pcbway.com/) for sponsoring the production of the PCB and Stencil for v0.2. It was my first time using their service and they have nice customer service and great build quality. The image in the hardware section shows the result of v0.2.
