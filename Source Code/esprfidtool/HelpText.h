const char HelpText[] PROGMEM = R"=====(
<!DOCTYPE HTML>
<html>
<head><title>ESP-RFID-Tool Help Page</title></head>
<body>
<a href="/"><- BACK TO INDEX</a><br><br>
-----<br>
HELP<br>
-----<br>
<br>
ESP-RFID-Tool<br>
<br>
Created by Corey Harding<br>
www.LegacySecurityGroup.com / www.Exploit.Agency<br>
<a href="https://github.com/rfidtool/ESP-RFID-Tool">https://github.com/rfidtool/ESP-RFID-Tool</a> - See Link for Updated Firmware or for more detailed Help<br>
<br>
The ESP-RFID-Tool is a tool created for logging Wiegand data and also for testing devices that contain a Wiegand Interface.  The primary target group is 26-37bit HID Cards but it will also work with most devices that output Wiegand data.  ESP-RFID-Tool can be combined with a RFID reader and a battery to create a portable standalone RFID badge logger, it can also be integrated into existing systems without the need for a battery and instead drawing its power directly from the wiring in the existing installation. The ESP-RFID-Tool can read the data from any device that contains a Wiegand Interface and outputs data from 1 bit long up to 4,096 bits long, although anything other than 26-37bit is experimental.  The ESP-RFID-Tool is not even limited to RFID technologies, many other devices also contain a Wiegand Interface as it is an access control system standard, this includes pin pads(keypad), magnetic stripe(magstripe), there are even non access control related devices that utilize a Wiegand Interface.<br>
<br>
The ESP-RFID-Tool software is distributed under the MIT License. The license and copyright notice can not be removed and must be distributed alongside all future copies of the software.<br>
<br>
-----<br>
Accessing ESP-RFID-Tool Web Interface<br>
-----<br>
<br>
SSID: "ESP-RFID-Tool"<br>
URL:  http://192.168.1.1<br>
<br>
-----<br>
Configure ESP-RFID-Tool<br>
-----<br>
<br>
Default credentials to access the configuration page:<br>
Username: "admin"<br>
Password: "rfidtool"<br>
<br>
Default credentials for ftp server:<br>
Username: "ftp-admin"<br>
Password: "rfidtool"<br>
<br>
WiFi Configuration:<br>
<br>
Network Type:<br>
Access Point Mode: Create a standalone access point(No Internet Connectivity-Requires Close Proximity)<br>
Join Existing Network: Join an existing network(Possible Internet Connectivity-Could use Device Remotely)<br>
<br>
Hidden: Choose whether or not to use a hidden SSID when creating an access point<br>
<br>
SSID: SSID of the access point to create or of the network you are choosing to join<br>
Password: Password of the access point which you wish to create or of the network you are choosing to join<br>
Channel: Channel of the access point you are creating<br>
<br>
IP: IP to set for device<br>
Gateway: Gateway to use, make it the same as ESP-RFID-Tool's IP if an access point or the same as the router if joining a network<br>
Subnet: Typically set to 255.255.255.0<br>
<br>
Web Interface Administration Settings:<br>
<br>
Username: Username to configure/upgrade ESP-RFID-Tool<br>
Password: Password to configure/upgrade ESP-RFID-Tool<br>
<br>
FTP Server Settings:<br>
<br>
Note: Supports Passive(PASV) Mode Only!<br>
Enabled: Turn FTP Server ON<br>
Disabled: Turn FTP Server OFF<br>
Username: Username to login to ftp server<br>
Password: Password to login to ftp server<br>
<br>
Power LED:<br>
<br>
Enabled: Turn ON Power LED<br>
Disabled: Turn OFF Power LED<br>
<br>
RFID Capture Log:<br>
<br>
Useful to change this value to differentiate between facilities during various security assessments.<br>
File Name: File name to save captured RFID tags to for the current security assessment.<br>
<br>
-----<br>
List Exfiltrated Data<br>
-----<br>
<br>
Displays all log files containing RFID tag captures.<br>
<br>
-----<br>
Format File System<br>
-----<br>
<br>
This will erase the contents of the SPIFFS file system including ALL RFID tag captures.<br>
Formatting may take up to 90 seconds.<br>
All current settings will be retained unless you reboot your device during this process.<br>
<br>
-----<br>
Upgrade ESP-RFID-Tool Firmware<br>
-----<br>
<br>
Authenticate using your username and password set in the configuration page.<br>
<br>
Default credentials to access the firmware upgrade page:<br>
Username: "admin"<br>
Password: "rfidtool"<br>
<br>
Select "Browse" choose the new firmware to be uploaded and then click "Upgrade".<br>
<br>
You will need to manually reset the device upon the browser alerting you that the upgrade was successful.<br>
<br>
-----<br>
Licensing Information<br>
-----<br>
<br>
Created by Corey Harding<br>
https://github.com/rfidtool/ESP-RFID-Tool<br>
ESP-RFID-Tool software is licensed under the MIT License<br>
/*<br>
 MIT License<br>
<br>
 Copyright (c) [2017] [Corey Harding]<br>
<br>
 Permission is hereby granted, free of charge, to any person obtaining a copy<br>
 of this software and associated documentation files (the "Software"), to deal<br>
 in the Software without restriction, including without limitation the rights<br>
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell<br>
 copies of the Software, and to permit persons to whom the Software is<br>
 furnished to do so, subject to the following conditions:<br>
<br>
 The above copyright notice and this permission notice shall be included in all<br>
 copies or substantial portions of the Software.<br>
<br>
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR<br>
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,<br>
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE<br>
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER<br>
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,<br>
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE<br>
 SOFTWARE.<br>
*/<br><br>
<a href="/license">Click here for additional licensing information</a>
</body>
</html>
)=====";
