/*
 * ESP-RFID-Tool
 * by Corey Harding of www.Exploit.Agency / www.LegacySecurityGroup.com
 * ESP-RFID-Tool Software is distributed under the MIT License. The license and copyright notice can not be removed and must be distributed alongside all future copies of the software.
 * MIT License
    
    Copyright (c) [2018] [Corey Harding]
    
    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/
#include "HelpText.h"
#include "License.h"
#include "version.h"
#include "strrev.h"
#include "aba2str.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <ArduinoJson.h> // ArduinoJson library 5.11.0 by Benoit Blanchon https://github.com/bblanchon/ArduinoJson
//#include <ESP8266FtpServer.h> // https://github.com/exploitagency/esp8266FTPServer/tree/feature/bbx10_speedup
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <TimeLib.h>


#define DATA0 14
#define DATA1 12

#define LED_BUILTIN 2
#define RESTORE_DEFAULTS_PIN 4 //GPIO 4
int jumperState = 0; //For restoring default settings
#include "WiegandNG.h" //https://github.com/jpliew/Wiegand-NG-Multi-Bit-Wiegand-Library-for-Arduino

// Port for web server
ESP8266WebServer server(80);
ESP8266WebServer httpServer(1337);
ESP8266HTTPUpdateServer httpUpdater;
//FtpServer ftpSrv;
const byte DNS_PORT = 53;
DNSServer dnsServer;

HTTPClient http;

const char* update_path = "/update";
int accesspointmode;
char ssid[32];
char password[64];
int channel;
int hidden;
char local_IPstr[16];
char gatewaystr[16];
char subnetstr[16];
char update_username[32];
char update_password[64];
char ftp_username[32];
char ftp_password[64];
int ftpenabled;
int ledenabled;
char logname[31];
unsigned int bufferlength;
unsigned int rxpacketgap;
int txdelayus;
int txdelayms;
int safemode;

int dos=0;
int TXstatus=0;
String pinHTML;

#include "pinSEND.h"

String dataCONVERSION="";

WiegandNG wg;

void LogWiegand(WiegandNG &tempwg) {
  volatile unsigned char *buffer=tempwg.getRawData();
  unsigned int bufferSize = tempwg.getBufferSize();
  unsigned int countedBits = tempwg.getBitCounted();

  unsigned int countedBytes = (countedBits/8);
  if ((countedBits % 8)>0) countedBytes++;
  //unsigned int bitsUsed = countedBytes * 8;

  bool binChunk2exists=false;
  volatile unsigned long cardChunk1 = 0;
  volatile unsigned long cardChunk2 = 0;
  volatile unsigned long binChunk2 = 0;
  volatile unsigned long binChunk1 = 0;
  String binChunk3="";
  bool unknown=false;
  binChunk2exists=false;
  int binChunk2len=0;
  int j=0;
  
  for (unsigned int i=bufferSize-countedBytes; i< bufferSize;i++) {
    unsigned char bufByte=buffer[i];
    for(int x=0; x<8;x++) {
      if ( (((bufferSize-i) *8)-x) <= countedBits) {
        j++;
        if((bufByte & 0x80)) {  //write 1
          if(j<23) {
            binChunk1 = binChunk1 << 1;
            binChunk1 |= 1;
          }
          else if(j<=52) {
            binChunk2exists=true;
            binChunk2len++;
            binChunk2 = binChunk2 << 1;
            binChunk2 |= 1;
          }
          else if(j>52){
            binChunk3=binChunk3+"1";
          }
        }
        else {  //write 0
          if(j<23) {
            binChunk1 = binChunk1 << 1;
          }
          else if(j<=52){
            binChunk2exists=true;
            binChunk2len++;
            binChunk2 = binChunk2 << 1;
          }
          else if(j>52){
            binChunk3=binChunk3+"0";
          }
        }
      }
      bufByte<<=1;
    }
  }
  j=0;

  switch (countedBits) {  //Add the preamble to known cards
    case 26:
      for(int i = 19; i >= 0; i--) {
        if(i == 13 || i == 2){
          bitWrite(cardChunk1, i, 1); // Write preamble 1's to the 13th and 2nd bits
        }
        else if(i > 2) {
          bitWrite(cardChunk1, i, 0); // Write preamble 0's to all other bits above 1
        }
        else {
          bitWrite(cardChunk1, i, bitRead(binChunk1, i + 20)); // Write remaining bits to cardChunk1 from binChunk1
        }
        if(i < 20) {
          bitWrite(cardChunk2, i + 4, bitRead(binChunk1, i)); // Write the remaining bits of binChunk1 to cardChunk2
        }
        if(i < 4) {
          bitWrite(cardChunk2, i, bitRead(binChunk2, i)); // Write the remaining bit of cardChunk2 with binChunk2 bits
        }
      }
      break;
    case 27:
      for(int i = 19; i >= 0; i--) {
        if(i == 13 || i == 3){
          bitWrite(cardChunk1, i, 1);
        }
        else if(i > 3) {
          bitWrite(cardChunk1, i, 0);
        }
        else {
          bitWrite(cardChunk1, i, bitRead(binChunk1, i + 19));
        }
        if(i < 19) {
          bitWrite(cardChunk2, i + 5, bitRead(binChunk1, i));
        }
        if(i < 5) {
          bitWrite(cardChunk2, i, bitRead(binChunk2, i));
        }
      }
      break;
    case 28:
      for(int i = 19; i >= 0; i--) {
        if(i == 13 || i == 4){
          bitWrite(cardChunk1, i, 1);
        }
        else if(i > 4) {
          bitWrite(cardChunk1, i, 0);
        }
        else {
          bitWrite(cardChunk1, i, bitRead(binChunk1, i + 18));
        }
        if(i < 18) {
          bitWrite(cardChunk2, i + 6, bitRead(binChunk1, i));
        }
        if(i < 6) {
          bitWrite(cardChunk2, i, bitRead(binChunk2, i));
        }
      }
      break;
    case 29:
      for(int i = 19; i >= 0; i--) {
        if(i == 13 || i == 5){
          bitWrite(cardChunk1, i, 1);
        }
        else if(i > 5) {
          bitWrite(cardChunk1, i, 0);
        }
        else {
          bitWrite(cardChunk1, i, bitRead(binChunk1, i + 17));
        }
        if(i < 17) {
          bitWrite(cardChunk2, i + 7, bitRead(binChunk1, i));
        }
        if(i < 7) {
          bitWrite(cardChunk2, i, bitRead(binChunk2, i));
        }
      }
      break;
    case 30:
      for(int i = 19; i >= 0; i--) {
        if(i == 13 || i == 6){
          bitWrite(cardChunk1, i, 1);
        }
        else if(i > 6) {
          bitWrite(cardChunk1, i, 0);
        }
        else {
          bitWrite(cardChunk1, i, bitRead(binChunk1, i + 16));
        }
        if(i < 16) {
          bitWrite(cardChunk2, i + 8, bitRead(binChunk1, i));
        }
        if(i < 8) {
          bitWrite(cardChunk2, i, bitRead(binChunk2, i));
        }
      }
      break;
    case 31:
      for(int i = 19; i >= 0; i--) {
        if(i == 13 || i == 7){
          bitWrite(cardChunk1, i, 1);
        }
        else if(i > 7) {
          bitWrite(cardChunk1, i, 0);
        }
        else {
          bitWrite(cardChunk1, i, bitRead(binChunk1, i + 15));
        }
        if(i < 15) {
          bitWrite(cardChunk2, i + 9, bitRead(binChunk1, i));
        }
        if(i < 9) {
          bitWrite(cardChunk2, i, bitRead(binChunk2, i));
        }
      }
      break;
    case 32:
      for(int i = 19; i >= 0; i--) {
        if(i == 13 || i == 8){
          bitWrite(cardChunk1, i, 1);
        }
        else if(i > 8) {
          bitWrite(cardChunk1, i, 0);
        }
        else {
          bitWrite(cardChunk1, i, bitRead(binChunk1, i + 14));
        }
        if(i < 14) {
          bitWrite(cardChunk2, i + 10, bitRead(binChunk1, i));
        }
        if(i < 10) {
          bitWrite(cardChunk2, i, bitRead(binChunk2, i));
        }
      }
      break;
    case 33:
      for(int i = 19; i >= 0; i--) {
        if(i == 13 || i == 9){
          bitWrite(cardChunk1, i, 1);
        }
        else if(i > 9) {
          bitWrite(cardChunk1, i, 0);
        }
        else {
          bitWrite(cardChunk1, i, bitRead(binChunk1, i + 13));
        }
        if(i < 13) {
          bitWrite(cardChunk2, i + 11, bitRead(binChunk1, i));
        }
        if(i < 11) {
          bitWrite(cardChunk2, i, bitRead(binChunk2, i));
        }
      }
      break;
    case 34:
      for(int i = 19; i >= 0; i--) {
        if(i == 13 || i == 10){
          bitWrite(cardChunk1, i, 1);
        }
        else if(i > 10) {
          bitWrite(cardChunk1, i, 0);
        }
        else {
          bitWrite(cardChunk1, i, bitRead(binChunk1, i + 12));
        }
        if(i < 12) {
          bitWrite(cardChunk2, i + 12, bitRead(binChunk1, i));
        }
        if(i < 12) {
          bitWrite(cardChunk2, i, bitRead(binChunk2, i));
        }
      }
      break;
    case 35:
      for(int i = 19; i >= 0; i--) {
        if(i == 13 || i == 11){
          bitWrite(cardChunk1, i, 1);
        }
        else if(i > 11) {
          bitWrite(cardChunk1, i, 0);
        }
        else {
          bitWrite(cardChunk1, i, bitRead(binChunk1, i + 11));
        }
        if(i < 11) {
          bitWrite(cardChunk2, i + 13, bitRead(binChunk1, i));
        }
        if(i < 13) {
          bitWrite(cardChunk2, i, bitRead(binChunk2, i));
        }
      }
      break;
    case 36:
      for(int i = 19; i >= 0; i--) {
        if(i == 13 || i == 12){
          bitWrite(cardChunk1, i, 1);
        }
        else if(i > 12) {
          bitWrite(cardChunk1, i, 0);
        }
        else {
          bitWrite(cardChunk1, i, bitRead(binChunk1, i + 10));
        }
        if(i < 10) {
          bitWrite(cardChunk2, i + 14, bitRead(binChunk1, i));
        }
        if(i < 14) {
          bitWrite(cardChunk2, i, bitRead(binChunk2, i));
        }
      }
      break;
    case 37:
      for(int i = 19; i >= 0; i--) {
        if(i == 13){
          bitWrite(cardChunk1, i, 0);
        }
        else {
          bitWrite(cardChunk1, i, bitRead(binChunk1, i + 9));
        }
        if(i < 9) {
          bitWrite(cardChunk2, i + 15, bitRead(binChunk1, i));
        }
        if(i < 15) {
          bitWrite(cardChunk2, i, bitRead(binChunk2, i));
        }
      }
      break;
    default:  //unknown card
      unknown=true;
      //String binChunk3 is like cardChunk0
      cardChunk1=binChunk2;
      cardChunk2=binChunk1;
      break;
  }

  File f = SPIFFS.open("/"+String(logname), "a"); //Open the log in append mode to store capture
  // It adds a timestamp
  String date_time_formated = String(year()) + "-" + month() + "-" +day() + " " + hour() + ":" + minute() + ":" + second() + " ";
  f.print(date_time_formated);

  int preambleLen;
  if (unknown==true && countedBits!=4 && countedBits!=8 && countedBits!=248) {
    f.print(F("Unknown "));
    preambleLen=0;
  }
  else {
    preambleLen=(44-countedBits);
  }
  
  f.print(String()+countedBits+F(" bit card,"));

  if (countedBits==4||countedBits==8) {
    f.print(F("Possible keypad entry,"));
  }

  if (countedBits==248) {
    f.print(F("possible magstripe card,"));
  }
  String magstripe="";

  if (unknown!=true) {
    f.print(String()+preambleLen+F(" bit preamble,"));
  }
  
  f.print(F("Binary:"));
  
  //f.print(" ");  //debug line
  if (binChunk2exists==true && unknown!=true) {
    for(int i = (((countedBits+preambleLen)-countedBits)+(countedBits-24)); i--;) {
      if (i==((((countedBits+preambleLen)-countedBits)+(countedBits-24))-preambleLen-1) && unknown!=true) {
        f.print(" ");
      }
      f.print(bitRead(cardChunk1, i));
      if(i == 0){
        break;
      }
    }
  }
  
  if ((countedBits>=24) && unknown!=true) {
    for(int i = 24; i--;) {
      f.print(bitRead(cardChunk2, i));
      if(i == 0){
        break;
      }
    }
  }
  else if ((countedBits>=23) && unknown==true) {
    int i;
    if (countedBits>=52) {
      i=22;
    }
    else {
      i =(countedBits-binChunk2len);
    }
    for(i; i--;) {
      f.print(bitRead(binChunk1, i));
      if (countedBits==248) {
        magstripe+=bitRead(binChunk1, i);
      }
      if(i == 0){
        break;
      }
    }
  }
  else {
    for(int i = countedBits; i--;) {
      f.print(bitRead(binChunk1, i));
      if(i == 0){
        break;
      }
    }
  }

  if (binChunk2exists==true && unknown==true) {
    int i;
    if (countedBits>=52) {
      i=30;
    }
    else {
      i=(binChunk2len);
    }
    for(i; i--;) {
      f.print(bitRead(binChunk2, i));
      if (countedBits==248) {
        magstripe+=bitRead(binChunk2, i);
      }
      if(i == 0){
        break;
      }
    }
  }

  if (countedBits>52) {
    f.print(binChunk3);
    if (countedBits==248) {
        magstripe+=binChunk3;
    }
  }

  if (countedBits<=52 && unknown!=true) {
    f.print(",HEX:");
    if (binChunk2exists==true) {
      f.print(cardChunk1, HEX);
    }
    //f.print(" "); //debug line
//    f.print(cardChunk2, HEX);
//    f.print(",Facility:")
//    f.print((cardChunk2 & 0x1fe0000) >> 17);
//    f.print(",CardCode:")
//    f.println((cardChunk2 & 0x1fffe) >> 1);
  }
  else if (countedBits==4||countedBits==8) {
    f.print(",Keypad Code:");
    if (binChunk1 == 0B0000||binChunk1 == 0b11110000) {
      f.print("0");
    }
    else if (binChunk1 == 0B0001||binChunk1 == 0b11100001) {
      f.print("1");
    }
    else if (binChunk1 == 0B0010||binChunk1 == 0b11010010) {
      f.print("2");
    }
    else if (binChunk1 == 0B0011||binChunk1 == 0b11000011) {
      f.print("3");
    }
    else if (binChunk1 == 0B0100||binChunk1 == 0b10110100) {
      f.print("4");
    }
    else if (binChunk1 == 0B0101||binChunk1 == 0b10100101) {
      f.print("5");
    }
    else if (binChunk1 == 0B0110||binChunk1 == 0b10010110) {
      f.print("6");
    }
    else if (binChunk1 == 0B0111||binChunk1 == 0b10000111) {
      f.print("7");
    }
    else if (binChunk1 == 0B1000||binChunk1 == 0b01111000) {
      f.print("8");
    }
    else if (binChunk1 == 0B1001||binChunk1 == 0b01101001) {
      f.print("9");
    }
    else if (binChunk1 == 0B1010||binChunk1 == 0b01011010) {
      f.print("*");
    }
    else if (binChunk1 == 0B1011||binChunk1 == 0b01001011) {
      f.print("#");
    }
    else if (binChunk1 == 0b1100||binChunk1 == 0b00111100) {
      f.print("F1");
    }
    else if (binChunk1 == 0b1101||binChunk1 == 0b00101101) {
      f.print("F2");
    }
    else if (binChunk1 == 0b1110||binChunk1 == 0b00011110) {
      f.print("F3");
    }
    else if (binChunk1 == 0b1111||binChunk1 == 0b00001111) {
      f.print("F4");
    }
    else {
      f.print("?");
    }
    f.print(",HEX:");
    if (countedBits==8) {
      char hexCHAR[3];
      sprintf(hexCHAR, "%02X", binChunk1);
      f.println(hexCHAR);
    }
    else if (countedBits==4) {
      f.println(binChunk1, HEX);
    }
  }
  else if (countedBits==248) {
    f.println(",");
  }
  else {
    f.println("");
  }

  if (countedBits==248) {
    int startSentinel=magstripe.indexOf("11010");
    int endSentinel=(magstripe.lastIndexOf("11111")+4);
    int magStart=0;
    int magEnd=1;
    //f.print("<pre>");
  
    f.print(" * Trying \"Forward\" Swipe,");
    magStart=startSentinel;
    magEnd=endSentinel;
    f.println(aba2str(magstripe,magStart,magEnd,"\"Forward\" Swipe"));
    
    f.print(" * Trying \"Reverse\" Swipe,");
    char magchar[249];
    magstripe.toCharArray(magchar,249);
    magstripe=String(strrev(magchar));
    //f.println(String()+"Reverse: "+magstripe);
    magStart=magstripe.indexOf("11010");
    magEnd=(magstripe.lastIndexOf("11111")+4);
    f.println(aba2str(magstripe,magStart,magEnd,"\"Reverse\" Swipe"));
  
    //f.print("</pre>");
    //f.println(String()+F(" * You can verify the data at the following URL: <a target=\"_blank\" href=\"https://www.legacysecuritygroup.com/aba-decode.php?binary=")+magstripe+F("\">https://www.legacysecuritygroup.com/aba-decode.php?binary=")+magstripe+F("</a>"));
  }

//Debug
//  f.print(F("Free heap:"));
//  f.println(ESP.getFreeHeap(),DEC);
  
  unknown=false;
  binChunk3="";
  binChunk2exists=false;
  binChunk1 = 0; binChunk2 = 0;
  cardChunk1 = 0; cardChunk2 = 0;
  binChunk2len=0;

  f.close(); //done
}

#include "api.h"

void settingsPage()
{
  if(!server.authenticate(update_username, update_password))
    return server.requestAuthentication();
  String accesspointmodeyes;
  String accesspointmodeno;
  if (accesspointmode==1){
    accesspointmodeyes=" checked=\"checked\"";
    accesspointmodeno="";
  }
  else {
    accesspointmodeyes="";
    accesspointmodeno=" checked=\"checked\"";
  }
  String ftpenabledyes;
  String ftpenabledno;
  if (ftpenabled==1){
    ftpenabledyes=" checked=\"checked\"";
    ftpenabledno="";
  }
  else {
    ftpenabledyes="";
    ftpenabledno=" checked=\"checked\"";
  }
  String ledenabledyes;
  String ledenabledno;
  if (ledenabled==1){
    ledenabledyes=" checked=\"checked\"";
    ledenabledno="";
  }
  else {
    ledenabledyes="";
    ledenabledno=" checked=\"checked\"";
  }
  String hiddenyes;
  String hiddenno;
  if (hidden==1){
    hiddenyes=" checked=\"checked\"";
    hiddenno="";
  }
  else {
    hiddenyes="";
    hiddenno=" checked=\"checked\"";
  }
  String safemodeyes;
  String safemodeno;
  if (safemode==1){
    safemodeyes=" checked=\"checked\"";
    safemodeno="";
  }
  else {
    safemodeyes="";
    safemodeno=" checked=\"checked\"";
  }
  time_t t = now();
  String date_time_formated = String(year(t)) + "-" + month(t) + "-" +day(t) + " " + hour(t) + ":" + minute(t) + ":" +second(t);

  server.send(200, "text/html", 
  String()+
  F(
  "<!DOCTYPE HTML>"
  "<html>"
  "<head>"
  "<meta name = \"viewport\" content = \"width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0\">"
  "<title>ESP-RFID-Tool Settings</title>"
  "<style>"
  "\"body { background-color: #808080; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }\""
  "</style>"
  "</head>"
  "<body>"
  "<a href=\"/\"><- BACK TO INDEX</a><br><br>"
  "<h1>ESP-RFID-Tool Settings</h1>"
  "<a href=\"/restoredefaults\"><button>Restore Default Configuration</button></a>"
  "<hr>"
  "<FORM action=\"/settings\"  id=\"configuration\" method=\"post\">"
  "<P>"
  "<b>WiFi Configuration:</b><br><br>"
  "<b>Network Type</b><br>"
  "<hr><b>Current time: </b> "
  ) +
  date_time_formated +
  F(
  "<br>"
  "<a href='' onclick='syncTime(); return false;'>Sync time</a>"
  ""
  "<script>"
  "function syncTime(){"
  "   today = new Date();"
  "   var day = today.getDate();"
  "   var month = today.getMonth()+1;"
  "   var year = today.getFullYear();"
  "   var hour = today.getHours();"
  "   var minute = today.getMinutes();"
  "   var second = today.getSeconds();"
  "   document.location.href = `./settime?year=${year}&month=${month}&day=${day}&hour=${hour}&minute=${minute}&second=${second}`;"
  "};"
  ""
  "</script><hr>"
  )+
  F("Access Point Mode: <INPUT type=\"radio\" name=\"accesspointmode\" value=\"1\"")+accesspointmodeyes+F("><br>"
  "Join Existing Network: <INPUT type=\"radio\" name=\"accesspointmode\" value=\"0\"")+accesspointmodeno+F("><br><br>"
  "<b>Hidden<br></b>"
  "Yes <INPUT type=\"radio\" name=\"hidden\" value=\"1\"")+hiddenyes+F("><br>"
  "No <INPUT type=\"radio\" name=\"hidden\" value=\"0\"")+hiddenno+F("><br><br>"
  "SSID: <input type=\"text\" name=\"ssid\" value=\"")+ssid+F("\" maxlength=\"31\" size=\"31\"><br>"
  "Password: <input type=\"password\" name=\"password\" value=\"")+password+F("\" maxlength=\"64\" size=\"31\"><br>"
  "Channel: <select name=\"channel\" form=\"configuration\"><option value=\"")+channel+"\" selected>"+channel+F("</option><option value=\"1\">1</option><option value=\"2\">2</option><option value=\"3\">3</option><option value=\"4\">4</option><option value=\"5\">5</option><option value=\"6\">6</option><option value=\"7\">7</option><option value=\"8\">8</option><option value=\"9\">9</option><option value=\"10\">10</option><option value=\"11\">11</option><option value=\"12\">12</option><option value=\"13\">13</option><option value=\"14\">14</option></select><br><br>"
  "IP: <input type=\"text\" name=\"local_IPstr\" value=\"")+local_IPstr+F("\" maxlength=\"16\" size=\"31\"><br>"
  "Gateway: <input type=\"text\" name=\"gatewaystr\" value=\"")+gatewaystr+F("\" maxlength=\"16\" size=\"31\"><br>"
  "Subnet: <input type=\"text\" name=\"subnetstr\" value=\"")+subnetstr+F("\" maxlength=\"16\" size=\"31\"><br><br>"
  "<hr>"
  "<b>Web Interface Administration Settings:</b><br><br>"
  "Username: <input type=\"text\" name=\"update_username\" value=\"")+update_username+F("\" maxlength=\"31\" size=\"31\"><br>"
  "Password: <input type=\"password\" name=\"update_password\" value=\"")+update_password+F("\" maxlength=\"64\" size=\"31\"><br><br>"
  "<hr>"
  "<b>FTP Server Settings</b><br>"
  "<small>Changes require a reboot.</small><br>"
  "Enabled <INPUT type=\"radio\" name=\"ftpenabled\" value=\"1\"")+ftpenabledyes+F("><br>"
  "Disabled <INPUT type=\"radio\" name=\"ftpenabled\" value=\"0\"")+ftpenabledno+F("><br>"
  "FTP Username: <input type=\"text\" name=\"ftp_username\" value=\"")+ftp_username+F("\" maxlength=\"31\" size=\"31\"><br>"
  "FTP Password: <input type=\"password\" name=\"ftp_password\" value=\"")+ftp_password+F("\" maxlength=\"64\" size=\"31\"><br><br>"
  "<hr>"
  "<b>Power LED:</b><br>"
  "<small>Changes require a reboot.</small><br>"
  "Enabled <INPUT type=\"radio\" name=\"ledenabled\" value=\"1\"")+ledenabledyes+F("><br>"
  "Disabled <INPUT type=\"radio\" name=\"ledenabled\" value=\"0\"")+ledenabledno+F("><br><br>"
  "<hr>"
  "<b>RFID Capture Log:</b><br>"
  "<small>Useful to change this value to differentiate between facilities during various security assessments.</small><br>"
  "File Name: <input type=\"text\" name=\"logname\" value=\"")+logname+F("\" maxlength=\"30\" size=\"31\"><br>"
  "<hr>"
  "<b>Experimental Settings:</b><br>"
  "<small>Changes require a reboot.</small><br>"
  "<small>Default Buffer Length is 256 bits with an allowed range of 52-4096 bits."
  "<br>Default Experimental TX mode timing is 40us Wiegand Data Pulse Width and a 2ms Wiegand Data Interval with an allowed range of 0-1000."
  "<br>Changing these settings may result in unstable performance.</small><br>"
  "Wiegand RX Buffer Length: <input type=\"number\" name=\"bufferlength\" value=\"")+bufferlength+F("\" maxlength=\"30\" size=\"31\" min=\"52\" max=\"4096\"> bit(s)<br>"
  "Wiegand RX Packet Length: <input type=\"number\" name=\"rxpacketgap\" value=\"")+rxpacketgap+F("\" maxlength=\"30\" size=\"31\" min=\"1\" max=\"4096\"> millisecond(s)<br>"
  "Experimental TX Wiegand Data Pulse Width: <input type=\"number\" name=\"txdelayus\" value=\"")+txdelayus+F("\" maxlength=\"30\" size=\"31\" min=\"0\" max=\"1000\"> microsecond(s)<br>"
  "Experimental TX Wiegand Data Interval: <input type=\"number\" name=\"txdelayms\" value=\"")+txdelayms+F("\" maxlength=\"30\" size=\"31\" min=\"0\" max=\"1000\"> millisecond(s)<br>"
  "<hr>"
  "<b>Safe Mode:</b><br>"
  "<small>Enable to reboot the device after every capture.<br>Disable to avoid missing quick consecutive captures such as keypad entries.</small><br>"
  "Enabled <INPUT type=\"radio\" name=\"safemode\" value=\"1\"")+safemodeyes+F("><br>"
  "Disabled <INPUT type=\"radio\" name=\"safemode\" value=\"0\"")+safemodeno+F("><br><br>"
  "<hr>"
  "<INPUT type=\"radio\" name=\"SETTINGS\" value=\"1\" hidden=\"1\" checked=\"checked\">"
  "<INPUT type=\"submit\" value=\"Apply Settings\">"
  "</FORM>"
  "<br><a href=\"/reboot\"><button>Reboot Device</button></a>"
  "</P>"
  "</body>"
  "</html>"
  )
  );
}

void handleSettings()
{
  if (server.hasArg("SETTINGS")) {
    handleSubmitSettings();
  }
  else {
    settingsPage();
  }
}

void returnFail(String msg)
{
  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(500, "text/plain", msg + "\r\n");
}

void handleSubmitSettings()
{
  String SETTINGSvalue;

  if (!server.hasArg("SETTINGS")) return returnFail("BAD ARGS");
  
  SETTINGSvalue = server.arg("SETTINGS");
  accesspointmode = server.arg("accesspointmode").toInt();
  server.arg("ssid").toCharArray(ssid, 32);
  server.arg("password").toCharArray(password, 64);
  channel = server.arg("channel").toInt();
  hidden = server.arg("hidden").toInt();
  server.arg("local_IPstr").toCharArray(local_IPstr, 16);
  server.arg("gatewaystr").toCharArray(gatewaystr, 16);
  server.arg("subnetstr").toCharArray(subnetstr, 16);
  server.arg("update_username").toCharArray(update_username, 32);
  server.arg("update_password").toCharArray(update_password, 64);
  server.arg("ftp_username").toCharArray(ftp_username, 32);
  server.arg("ftp_password").toCharArray(ftp_password, 64);
  ftpenabled = server.arg("ftpenabled").toInt();
  ledenabled = server.arg("ledenabled").toInt();
  server.arg("logname").toCharArray(logname, 31);
  bufferlength = server.arg("bufferlength").toInt();
  rxpacketgap = server.arg("rxpacketgap").toInt();
  txdelayus = server.arg("txdelayus").toInt();
  txdelayms = server.arg("txdelayms").toInt();
  safemode = server.arg("safemode").toInt();
  
  if (SETTINGSvalue == "1") {
    saveConfig();
    server.send(200, "text/html", F("<a href=\"/\"><- BACK TO INDEX</a><br><br><a href=\"/reboot\"><button>Reboot Device</button></a><br><br>Settings have been saved.<br>Some setting may require manually rebooting before taking effect.<br>If network configuration has changed then be sure to connect to the new network first in order to access the web interface."));
    delay(50);
    loadConfig();
  }
  else if (SETTINGSvalue == "0") {
    settingsPage();
  }
  else {
    returnFail("Bad SETTINGS value");
  }
}

bool loadDefaults() {
  StaticJsonBuffer<500> jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["version"] = version;
  json["accesspointmode"] = "1";
  json["ssid"] = "ESP-RFID-Tool";
  json["password"] = "";
  json["channel"] = "6";
  json["hidden"] = "0";
  json["local_IP"] = "192.168.1.1";
  json["gateway"] = "192.168.1.1";
  json["subnet"] = "255.255.255.0";
  json["update_username"] = "admin";
  json["update_password"] = "rfidtool";
  json["ftp_username"] = "ftp-admin";
  json["ftp_password"] = "rfidtool";
  json["ftpenabled"] = "0";
  json["ledenabled"] = "1";
  json["logname"] = "log.txt";
  json["bufferlength"] = "256";
  json["rxpacketgap"] = "15";
  json["txdelayus"] = "40";
  json["txdelayms"] = "2";
  json["safemode"] = "0";
  File configFile = SPIFFS.open("/esprfidtool.json", "w");
  if (!configFile ) {
    Serial.println(F("File creation failed! Is flash memory enabled?"));
   }
  json.printTo(configFile);
  configFile.close();
  jsonBuffer.clear();
  loadConfig();
  return true;
}

bool loadConfig() {
  File configFile = SPIFFS.open("/esprfidtool.json", "r");
  if (!configFile) {
    delay(3500);
    loadDefaults();
  }

  size_t size = configFile.size();

  std::unique_ptr<char[]> buf(new char[size]);
  configFile.readBytes(buf.get(), size);
  StaticJsonBuffer<500> jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());
  
  if (!json["version"]) {
    delay(3500);
    loadDefaults();
    ESP.restart();
  }

  //Resets config to factory defaults on an update.
  if (json["version"]!=version) {
    delay(3500);
    loadDefaults();
    ESP.restart();
  }

  strcpy(ssid, (const char*)json["ssid"]);
  strcpy(password, (const char*)json["password"]);
  channel = json["channel"];
  hidden = json["hidden"];
  accesspointmode = json["accesspointmode"];
  strcpy(local_IPstr, (const char*)json["local_IP"]);
  strcpy(gatewaystr, (const char*)json["gateway"]);
  strcpy(subnetstr, (const char*)json["subnet"]);

  strcpy(update_username, (const char*)json["update_username"]);
  strcpy(update_password, (const char*)json["update_password"]);

  strcpy(ftp_username, (const char*)json["ftp_username"]);
  strcpy(ftp_password, (const char*)json["ftp_password"]);
  ftpenabled = json["ftpenabled"];
  ledenabled = json["ledenabled"];
  strcpy(logname, (const char*)json["logname"]);
  bufferlength = json["bufferlength"];
  rxpacketgap = json["rxpacketgap"];
  txdelayus = json["txdelayus"];
  txdelayms = json["txdelayms"];
  safemode = json["safemode"];
 
  IPAddress local_IP;
  local_IP.fromString(local_IPstr);
  IPAddress gateway;
  gateway.fromString(gatewaystr);
  IPAddress subnet;
  subnet.fromString(subnetstr);

/*
  Serial.println(accesspointmode);
  Serial.println(ssid);
  Serial.println(password);
  Serial.println(channel);
  Serial.println(hidden);
  Serial.println(local_IP);
  Serial.println(gateway);
  Serial.println(subnet);
*/
  WiFi.persistent(false);
  //ESP.eraseConfig();
// Determine if set to Access point mode
  if (accesspointmode == 1) {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_AP);

//    Serial.print("Starting Access Point ... ");
//    Serial.println(WiFi.softAP(ssid, password, channel, hidden) ? "Success" : "Failed!");
    WiFi.softAP(ssid, password, channel, hidden);
    Serial.println("Wait 100 ms for AP_START...");
    delay(100);
//    Serial.print("Setting up Network Configuration ... ");
//    Serial.println(WiFi.softAPConfig(local_IP, gateway, subnet) ? "Success" : "Failed!");
    WiFi.softAPConfig(local_IP, gateway, subnet);

//    WiFi.reconnect();

//    Serial.print("IP address = ");
//    Serial.println(WiFi.softAPIP());
  }
// or Join existing network
  else if (accesspointmode != 1) {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);
//    Serial.print("Setting up Network Configuration ... ");
    WiFi.config(local_IP, gateway, subnet);
//    WiFi.config(local_IP, gateway, subnet);

//    Serial.print("Connecting to network ... ");
//    WiFi.begin(ssid, password);
    WiFi.begin(ssid, password);
    WiFi.reconnect();

//    Serial.print("IP address = ");
//    Serial.println(WiFi.localIP());
  }
  configFile.close();
  jsonBuffer.clear();
  return true;
}

bool saveConfig() {
  StaticJsonBuffer<500> jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["version"] = version;
  json["accesspointmode"] = accesspointmode;
  json["ssid"] = ssid;
  json["password"] = password;
  json["channel"] = channel;
  json["hidden"] = hidden;
  json["local_IP"] = local_IPstr;
  json["gateway"] = gatewaystr;
  json["subnet"] = subnetstr;
  json["update_username"] = update_username;
  json["update_password"] = update_password;
  json["ftp_username"] = ftp_username;
  json["ftp_password"] = ftp_password;
  json["ftpenabled"] = ftpenabled;
  json["ledenabled"] = ledenabled;
  json["logname"] = logname;
  json["bufferlength"] = bufferlength;
  json["rxpacketgap"] = rxpacketgap;
  json["txdelayus"] = txdelayus;
  json["txdelayms"] = txdelayms;
  json["safemode"] = safemode;

  File configFile = SPIFFS.open("/esprfidtool.json", "w");
  json.printTo(configFile);
  configFile.close();
  jsonBuffer.clear();
  return true;
}

File fsUploadFile;
String webString;

void ListLogs(){
  String directory;
  directory="/";
  FSInfo fs_info;
  SPIFFS.info(fs_info);
  String total;
  total=fs_info.totalBytes;
  String used;
  used=fs_info.usedBytes;
  String freespace;
  freespace=fs_info.totalBytes-fs_info.usedBytes;
  Dir dir = SPIFFS.openDir(directory);
  String FileList = String()+F("<a href=\"/\"><- BACK TO INDEX</a><br><br>File System Info Calculated in Bytes<br><b>Total:</b> ")+total+" <b>Free:</b> "+freespace+" "+" <b>Used:</b> "+used+"<br><br><small>NOTE: Larger log files will need to be downloaded instead of viewed from the browser.</small><br><table border='1'><tr><td><b>Display File Contents</b></td><td><b>Size in Bytes</b></td><td><b>Download File</b></td><td><b>Delete File</b></td></tr>";
  while (dir.next()) {
    String FileName = dir.fileName();
    File f = dir.openFile("r");
    FileList += " ";
    if((!FileName.startsWith("/payloads/"))&&(!FileName.startsWith("/esploit.json"))&&(!FileName.startsWith("/esportal.json"))&&(!FileName.startsWith("/esprfidtool.json"))&&(!FileName.startsWith("/config.json"))) FileList += "<tr><td><a href=\"/viewlog?payload="+FileName+"\">"+FileName+"</a></td>"+"<td>"+f.size()+"</td><td><a href=\""+FileName+"\"><button>Download File</button></td><td><a href=\"/deletelog?payload="+FileName+"\"><button>Delete File</button></td></tr>";
    f.close();
  }
  FileList += "</table>";
  server.send(200, "text/html", FileList);
}

// It reads a line from a file, max size including terminating string
int readLine(File f, char *line, size_t maxsize) {
  int index = 0;
  while (f.available()) {
    line[index] = f.read();
    // We just ignored \r
    if (line[index] == '\r') {
      index++;
      continue;
    }
    if (line[index] == '\n') {
        line[index] = '\0';
        return index;
    }
    index++;
    if (index == maxsize - 1) {
      line[index] = '\0';
      return index;
    }
  }  
  return -1;
}


//This function returns an HTML select item with the unique card numbers to be shown in the replay (TX Mode) menu
String ListCards(){
  String html;
  html += F("<select form='transmitbinary' name='binHTML'>\n");
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
    String FileName = dir.fileName();
    Serial.println(FileName);
    if(!FileName.startsWith("/payloads/") && !FileName.startsWith("/esploit.json") && !FileName.startsWith("/esportal.json") && !FileName.startsWith("/esprfidtool.json") && !FileName.startsWith("/config.json")) {
      File f = dir.openFile("r");
      char line[120];
      int n;
      while ((n = readLine(f, line, sizeof(line))) != -1) {
        char *binary = strstr (line, "Binary:");
        if (binary != 0) {
          binary = strstr (binary, " ");
          binary += 1;
          char *binary_end = strstr (binary, ",");
          *binary_end = '\0';
          //If it's already on the list, ignore
          if (html.indexOf(binary) != -1)
            continue;
          char *hex = strstr(line, "HEX:");
          html = html + F("<option value='") + binary +"'>" + hex + "(" + binary + ")" + F("</option>\n");
        }
      }
      f.close();
    }
  }
  html += "</select>";
  return html;
}

bool RawFile(String rawfile) {
  if (SPIFFS.exists(rawfile)) {
    if(!server.authenticate(update_username, update_password)){
      server.requestAuthentication();}
    File file = SPIFFS.open(rawfile, "r");
    size_t sent = server.streamFile(file, "application/octet-stream");
    file.close();
    return true;
  }
  return false;
}

/* Set the time so we can print it in the log and track users office time schedules*/
void mysettime(){
  if (server.hasArg("hour") && server.hasArg("minute") && server.hasArg("second") && server.hasArg("year") && server.hasArg("month") && server.hasArg("day")) {
    int h = server.arg("hour").toInt();
    int m = server.arg("minute").toInt();
    int s = server.arg("second").toInt();
    int y = server.arg("year").toInt();
    int M = server.arg("month").toInt();
    int d = server.arg("day").toInt();
    
    setTime(h, m, s, d, M, y);
    server.send(200, "text/html", String()+F("<a href=\"/\"><- BACK TO INDEX</a><br><br>Time synchronized!"));
  }  
}

void ViewLog(){
  webString="";
  String payload;
  String ShowPL;
  payload += server.arg(0);
  File f = SPIFFS.open(payload, "r");
  String webString = f.readString();
  f.close();
  ShowPL = String()+F(
    "<html><head></head><body>"
    "<a href=\"/\"><- BACK TO INDEX</a><br><br>"
    "<a href=\"/logs\">List Exfiltrated Data</a> - <a href=\"/experimental\">Experimental TX Mode</a> - <a href=\"/data-convert\">Data Conversion Tools</a><br><br>"
    "<FORM action=\"/api/tx/bin\" id=\"api_tx\" method=\"get\"  target=\"_blank\">"
      "<small>Binary: </small><INPUT form=\"api_tx\" type=\"text\" name=\"binary\" value=\"\" pattern=\"[01,]{1,}\" required title=\"Allowed characters(0,1,\",\"), must not be empty\" minlength=\"1\" size=\"52\"> "
      "<INPUT form=\"api_tx\" type=\"submit\" value=\"Transmit\"><br>"
      "<small>Pulse Width: </small><INPUT form=\"api_tx\" type=\"number\" name=\"pulsewidth\" value=\"40\" minlength=\"1\" min=\"0\" size=\"8\"><small>us</small> "
      "<small>Data Interval: </small><INPUT form=\"api_tx\" type=\"number\" name=\"interval\" value=\"2000\" minlength=\"1\" min=\"0\" size=\"8\"><small>us</small> "
      "<small>Delay Between Packets: </small><INPUT form=\"api_tx\" type=\"number\" name=\"wait\" value=\"100000\" minlength=\"1\" min=\"0\" size=\"8\"><small>us</small><br>"
      "<INPUT form=\"api_tx\" type=\"hidden\" name=\"prettify\" id=\"prettify\" value=\"1\">"
    "</FORM>"
    "<small>Use commas to separate the binary for transmitting multiple packets(useful for sending multiple keypresses for imitating keypads)</small><br>"
    "<hr>"
    "<a href=\"")+payload+F("\"><button>Download File</button><a><small> - </small><a href=\"/deletelog?payload=")+payload+F("\"><button>Delete File</button></a>"
    "<pre>")
    +payload+
    F("\n"
    "Note: Preambles shown are only a guess based on card length and may not be accurate for every card format.\n"
    "-----\n")
    +webString+
    F("</pre></body></html>")
    ;
  webString="";
  server.send(200, "text/html", ShowPL);
}

// Start Networking
void setup() {
  Serial.begin(9600);
  delay(1000);
  Serial.println(F("....."));
  Serial.println(String()+F("ESP-RFID-Tool v")+version);
  //SPIFFS.format();
  
  SPIFFS.begin();

  //loadDefaults(); //uncomment to restore default settings if double reset fails for some reason

//Jump RESTORE_DEFAULTS_PIN to GND while powering on device to reset the device to factory defaults
  pinMode(RESTORE_DEFAULTS_PIN, INPUT_PULLUP);
  jumperState = digitalRead(RESTORE_DEFAULTS_PIN);
  if (jumperState == LOW) {
    Serial.println(String()+F("Pin ")+RESTORE_DEFAULTS_PIN+F("Grounded"));
    Serial.println(F("Loading default config..."));
    loadDefaults();
  }

  loadConfig();

 if(!wg.begin(DATA0,DATA1,bufferlength,rxpacketgap)) {       
   Serial.println(F("Could not begin Wiegand logging,"));            
   Serial.println(F("Out of memory!"));
 }

//Set up Web Pages
  server.on("/",[]() {
    FSInfo fs_info;
    SPIFFS.info(fs_info);
    String total;
    total=fs_info.totalBytes;
    String used;
    used=fs_info.usedBytes;
    String freespace;
    freespace=fs_info.totalBytes-fs_info.usedBytes;
    server.send(200, "text/html", String()+F("<html><body><b>ESP-RFID-Tool v")+version+F("</b><br>"
    "<img width='86' height='86' src='data:image/svg+xml;base64,PD94bWwgdmVyc2lvbj0iMS4wIiBlbmNvZGluZz0iVVRGLTgiIHN0YW5kYWxvbmU9Im5vIj8+CjwhLS0gQ3JlYXRlZCB3aXRoIElua3NjYXBlIChodHRwOi8vd3d3Lmlua3NjYXBlLm9yZy8pIC0tPgoKPHN2ZwogICB4bWxuczpkYz0iaHR0cDovL3B1cmwub3JnL2RjL2VsZW1lbnRzLzEuMS8iCiAgIHhtbG5zOmNjPSJodHRwOi8vY3JlYXRpdmVjb21tb25zLm9yZy9ucyMiCiAgIHhtbG5zOnJkZj0iaHR0cDovL3d3dy53My5vcmcvMTk5OS8wMi8yMi1yZGYtc3ludGF4LW5zIyIKICAgeG1sbnM6c3ZnPSJodHRwOi8vd3d3LnczLm9yZy8yMDAwL3N2ZyIKICAgeG1sbnM9Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvc3ZnIgogICB4bWxuczpzb2RpcG9kaT0iaHR0cDovL3NvZGlwb2RpLnNvdXJjZWZvcmdlLm5ldC9EVEQvc29kaXBvZGktMC5kdGQiCiAgIHhtbG5zOmlua3NjYXBlPSJodHRwOi8vd3d3Lmlua3NjYXBlLm9yZy9uYW1lc3BhY2VzL2lua3NjYXBlIgogICB3aWR0aD0iNTYuNTkwNzI5bW0iCiAgIGhlaWdodD0iNDMuODMwODMzbW0iCiAgIHZpZXdCb3g9IjAgMCA1Ni41OTA3MjggNDMuODMwODM0IgogICB2ZXJzaW9uPSIxLjEiCiAgIGlkPSJzdmczMDAwIgogICBpbmtzY2FwZTp2ZXJzaW9uPSIwLjkyLjIgKDVjM2U4MGQsIDIwMTctMDgtMDYpIgogICBzb2RpcG9kaTpkb2NuYW1lPSJyZmlkdG9vbDQuc3ZnIj4KICA8ZGVmcwogICAgIGlkPSJkZWZzMjk5NCI+CiAgICA8ZmlsdGVyCiAgICAgICBzdHlsZT0iY29sb3ItaW50ZXJwb2xhdGlvbi1maWx0ZXJzOnNSR0I7IgogICAgICAgaW5rc2NhcGU6bGFiZWw9IkNyb3NzIEJsdXIiCiAgICAgICBpZD0iZmlsdGVyMzEyMCI+CiAgICAgIDxmZUNvbG9yTWF0cml4CiAgICAgICAgIGluPSJTb3VyY2VHcmFwaGljIgogICAgICAgICB2YWx1ZXM9IjEgMCAwIDAgMCAwIDEgMCAwIDAgMCAwIDEgMCAwIC0wLjIxMjUgLTAuNzE1NCAtMC4wNzIxIDEgMCAiCiAgICAgICAgIHJlc3VsdD0iY29sb3JtYXRyaXgiCiAgICAgICAgIGlkPSJmZUNvbG9yTWF0cml4MzExMCIgLz4KICAgICAgPGZlQ29tcG9zaXRlCiAgICAgICAgIGluPSJTb3VyY2VHcmFwaGljIgogICAgICAgICBpbjI9ImNvbG9ybWF0cml4IgogICAgICAgICBvcGVyYXRvcj0iYXJpdGhtZXRpYyIKICAgICAgICAgazI9IjEiCiAgICAgICAgIGszPSIwIgogICAgICAgICBrND0iMCIKICAgICAgICAgcmVzdWx0PSJjb21wb3NpdGUiCiAgICAgICAgIGlkPSJmZUNvbXBvc2l0ZTMxMTIiIC8+CiAgICAgIDxmZUdhdXNzaWFuQmx1cgogICAgICAgICBzdGREZXZpYXRpb249IjAuNzUgMC4wMSIKICAgICAgICAgcmVzdWx0PSJibHVyMSIKICAgICAgICAgaWQ9ImZlR2F1c3NpYW5CbHVyMzExNCIgLz4KICAgICAgPGZlR2F1c3NpYW5CbHVyCiAgICAgICAgIGluPSJjb21wb3NpdGUiCiAgICAgICAgIHN0ZERldmlhdGlvbj0iMC4wMSAwLjc1IgogICAgICAgICByZXN1bHQ9ImJsdXIyIgogICAgICAgICBpZD0iZmVHYXVzc2lhbkJsdXIzMTE2IiAvPgogICAgICA8ZmVCbGVuZAogICAgICAgICBpbj0iYmx1cjIiCiAgICAgICAgIGluMj0iYmx1cjEiCiAgICAgICAgIG1vZGU9ImRhcmtlbiIKICAgICAgICAgcmVzdWx0PSJibGVuZCIKICAgICAgICAgaWQ9ImZlQmxlbmQzMTE4IiAvPgogICAgPC9maWx0ZXI+CiAgPC9kZWZzPgogIDxzb2RpcG9kaTpuYW1lZHZpZXcKICAgICBpZD0iYmFzZSIKICAgICBwYWdlY29sb3I9IiNmZmZmZmYiCiAgICAgYm9yZGVyY29sb3I9IiM2NjY2NjYiCiAgICAgYm9yZGVyb3BhY2l0eT0iMS4wIgogICAgIGlua3NjYXBlOnBhZ2VvcGFjaXR5PSIwLjAiCiAgICAgaW5rc2NhcGU6cGFnZXNoYWRvdz0iMiIKICAgICBpbmtzY2FwZTp6b29tPSIyLjA2MTY4NDgiCiAgICAgaW5rc2NhcGU6Y3g9IjE5NC44Njc5IgogICAgIGlua3NjYXBlOmN5PSI4Ny4zNDU1MTUiCiAgICAgaW5rc2NhcGU6ZG9jdW1lbnQtdW5pdHM9Im1tIgogICAgIGlua3NjYXBlOmN1cnJlbnQtbGF5ZXI9ImxheWVyMSIKICAgICBzaG93Z3JpZD0iZmFsc2UiCiAgICAgZml0LW1hcmdpbi10b3A9IjAiCiAgICAgZml0LW1hcmdpbi1sZWZ0PSIwIgogICAgIGZpdC1tYXJnaW4tcmlnaHQ9IjAiCiAgICAgZml0LW1hcmdpbi1ib3R0b209IjAiCiAgICAgaW5rc2NhcGU6d2luZG93LXdpZHRoPSIxMzY2IgogICAgIGlua3NjYXBlOndpbmRvdy1oZWlnaHQ9IjY1OSIKICAgICBpbmtzY2FwZTp3aW5kb3cteD0iMCIKICAgICBpbmtzY2FwZTp3aW5kb3cteT0iMzEiCiAgICAgaW5rc2NhcGU6d2luZG93LW1heGltaXplZD0iMSIgLz4KICA8bWV0YWRhdGEKICAgICBpZD0ibWV0YWRhdGEyOTk3Ij4KICAgIDxyZGY6UkRGPgogICAgICA8Y2M6V29yawogICAgICAgICByZGY6YWJvdXQ9IiI+CiAgICAgICAgPGRjOmZvcm1hdD5pbWFnZS9zdmcreG1sPC9kYzpmb3JtYXQ+CiAgICAgICAgPGRjOnR5cGUKICAgICAgICAgICByZGY6cmVzb3VyY2U9Imh0dHA6Ly9wdXJsLm9yZy9kYy9kY21pdHlwZS9TdGlsbEltYWdlIiAvPgogICAgICAgIDxkYzp0aXRsZSAvPgogICAgICA8L2NjOldvcms+CiAgICA8L3JkZjpSREY+CiAgPC9tZXRhZGF0YT4KICA8ZwogICAgIGlua3NjYXBlOmxhYmVsPSJMYXllciAxIgogICAgIGlua3NjYXBlOmdyb3VwbW9kZT0ibGF5ZXIiCiAgICAgaWQ9ImxheWVyMSIKICAgICB0cmFuc2Zvcm09InRyYW5zbGF0ZSgtMTgxLjkzNTEsLTIyNi4xMTMpIj4KICAgIDxwYXRoCiAgICAgICBpZD0icGF0aDM1NTYiCiAgICAgICB0cmFuc2Zvcm09Im1hdHJpeCgwLjI2NDU4MzMzLDAsMCwwLjI2NDU4MzMzLDE2Mi44OTQyMywyMTguNDkzODEpIgogICAgICAgc3R5bGU9ImZpbGw6IzAwMDAwMDtzdHJva2Utd2lkdGg6MS4wMDAwMDAxMjtmaWx0ZXI6dXJsKCNmaWx0ZXIzMTIwKSIKICAgICAgIGQ9Im0gMjI3LjkwODIsMTY2LjE0NDYyIC01LjEwMTU2LDQuNDYyODkgYyAtMjQuNDMxNTUsMjEuMzcyMTMgLTY1LjU3MjQsMjEuNzYwMjYgLTg5LjQ3MjY2LDAuODQzNzUgbCAtNS4zMjgxMiwtNC42NjIxMSAtMi4wNTY2NCwyLjA1ODYgLTIuMDU4NiwyLjA1NjY0IDMuNzU1ODYsMy4zMzc4OSBjIDEwLjgwMDY4LDkuNTk5ODUgMjQuMDYxLDE1Ljc4Mjk4IDM5LjAxMzY4LDE4LjE5MTQgNi40NjIwNCwxLjA0MDg1IDI0LjA5MTgsLTAuMjg4MjYgMzEuNDcyNjUsLTIuMzczMDQgOS45MTM0LC0yLjgwMDE0IDIxLjQ1OTMxLC04LjkxNTM3IDI4LjExOTE0LC0xNC44OTI1OCA1LjM1Nzc1LC00LjgwODU4IDUuMzkxNjQsLTQuODczNzkgMy41MzMyMSwtNi45Mzk0NSB6IG0gLTExLjEzMDg2LC0xMC4zMTA1NSBjIC0wLjg2NDc4LDAuMDgzOCAtMi4wOTk2NCwxLjA1MTExIC00Ljg1MTU2LDMuMjQyMTkgLTEwLjEwMjYsOC4wNDM4NiAtMjEuMzM5MTMsMTEuOTczMjkgLTM0LjMwNDY5LDExLjk5MjE5IC0xMy4wNDM0MSwwLjAxOTMgLTIyLjU2ODY2LC0zLjE4Njk0IC0zMy4zMTgzNiwtMTEuMjE0ODQgbCAtNS4zNTkzNywtNC4wMDE5NiAtMi4wMDE5NSwyLjAwMzkxIC0yLjAwMzkxLDIuMDAxOTUgNS4zMDI3Myw0LjE5MTQxIGMgNy42MzQ2OSw2LjAzNDg1IDE3LjU2Mzg2LDEwLjYwODU1IDI2LjkxOTkzLDEyLjQwMDM5IDEuMzc0OTksMC4yNjM0MyA2Ljk5OTk5LDAuMzU1ODggMTIuNSwwLjIwNTA4IDEyLjI3OTIzLC0wLjMzNjc2IDIxLjgwMTUyLC0zLjM3NzM4IDMxLjY0ODQzLC0xMC4xMDU0NyA4LjkwMzAyLC02LjA4MzE1IDkuNjAyNDQsLTYuOTgxOTYgNy4zNTU0NywtOS40NjQ4NSAtMC43NjQzNSwtMC44NDQ1OSAtMS4yMTQxMSwtMS4zMTUxOSAtMS44ODY3MiwtMS4yNSB6IG0gLTY2LjI2OTUzLC0xMS43OTQ5MiAtMS44MjgxMiwyLjAxOTUzIGMgLTEuNzcyODMsMS45NTg5NyAtMS43NzA2NiwyLjA4NTc1IDAuMDc2Miw0LjE1MDM5IDIuOTAwNDgsMy4yNDI2OSA4LjYwNDAzLDYuNTcyNTUgMTUuMDcwMzEsOC43OTY4OCAxMy44MjM3LDQuNzU1MjkgMzMuNjY4MzYsMC42NDA3MyA0Mi43ODEyNSwtOC44NzEwOSAyLjU0Nzg5LC0yLjY1OTQzIDIuNTU5OTUsLTIuNzM4NzYgMC42Njk5MiwtNC40NDkyMiAtMS44NDQ0NCwtMS42NjkxOSAtMi4xMTIwOSwtMS41OTY3NiAtNi40ODYzMiwxLjc0MjE4IC03LjY1NTcsNS44NDM2OCAtMTQuNjIzNTksNy45MDgwOCAtMjUuMTMwODYsNy40NDUzMiAtNy4zNDEwNiwtMC4zMjMzIC0xMC4wMTM4MiwtMC45MDY0NyAtMTQuNSwtMy4xNjYwMiAtMy4wMjQ5OSwtMS41MjM2IC02LjY1OTIzLC0zLjg3MTggLTguMDc2MTgsLTUuMjE4NzUgeiBtIDQzLjc1MzkxLC0xMS4zMDI3MyAtMy44MDA3OCwyLjYyMTA5IGMgLTcuNjkyMzIsNS4zMDI1NyAtMTcuNTkyMzYsNS4zOTA4NiAtMjUuNDQ1MzIsMC4yMjg1MiAtMy41ODg2MiwtMi4zNTkwNyAtMy42NzI2NSwtMi4zNjQ5NCAtNS4zODg2NywtMC40Njg3NSAtMi4yNDk4NywyLjQ4NjEgLTEuMDc2NTIsMy45ODExMiA1LjgxMjUsNy40MTQwNiA4Ljg1NTk3LDQuNDEzMTMgMjEuMzc3ODcsMy4wOTkzMiAyOS41LC0zLjA5NTcgbCAzLjQxNjAyLC0yLjYwNTQ3IC0yLjA0Njg4LC0yLjA0Njg4IHogbSAtMTEuNDA4MiwtMTEuMDEzNjcgYyAtMC41NzAxOSwtMC4wNjA0IC0xLjIwNzcsMC4zMDQ0NSAtMi4zNDM3NSwxLjA0ODgyIC0xLjk4MTM4LDEuMjk4MjcgLTIuOTA5MjQsMS4zNzU1OCAtNS4wMTk1NCwwLjQxNDA3IC0yLjE2NCwtMC45ODYwMSAtMi45MDkzMywtMC44OTA5IC00LjUzOTA2LDAuNTgzOTggLTEuOTI4NzMsMS43NDUzOSAtMS45MjYzLDEuNzgyNzcgMC4yNDAyNCwzLjUzNzExIDIuOTgyNjUsMi40MTUzMSAxMC4wNDIxOSwyLjM0NDg4IDEzLjA5OTYxLC0wLjEzMDg2IDIuMjk5MiwtMS44NjE3OSAyLjMxMDA1LC0xLjk1MTA4IDAuNSwtMy45NTExNyAtMC44NjU1LC0wLjk1NjM1IC0xLjM2NzMyLC0xLjQ0MTU2IC0xLjkzNzUsLTEuNTAxOTUgeiBtIC0zLjU2MjUsLTEzLjM2NzIzIGMgLTAuMzk5NjQsLTAuMDIxMyAtMC44NzkyMSwwLjA3NDggLTEuNDM3NSwwLjI4OTA2IC0xLjg1Mjk2LDAuNzExMDQgLTIuOTAyNTksNS42ODgzMiAtMS4zOTY0OSw2LjYxOTE0IDEuNzg0OTYsMS4xMDMxNyAzLjk4MzU1LC0wLjcxNTggNC4yODUxNiwtMy41NDI5NyAwLjIzMjM4LC0yLjE3OTczIC0wLjI1MjI2LC0zLjMwMTQ2IC0xLjQ1MTE3LC0zLjM2NTIzIHogbSAtMjcuODk4NDQsLTMuNjg5NDYgYyAyLjYyODAzLC0wLjEyMDQ0IDcuMzMzNTgsMS40Njg5NyA4LjQ5MDIzLDMuMDUwNzkgMi4wMzI3NSwyLjc3OTk1IDEuNjA5MTMsMTAuMDMxNTQgLTAuNzIyNjUsMTIuMzYzMjggLTEuMzMzMzEsMS4zMzMzNCAtMy4zMzMzMiwyIC02LDIgaCAtNCB2IC04LjQxNzk3IGMgMCwtNS44NDMwNyAwLjM4MjIyLC04LjU0NDkyIDEuMjUsLTguODM1OTQgMC4yNzQyMiwtMC4wOTIgMC42MDY5OCwtMC4xNDI5NSAwLjk4MjQyLC0wLjE2MDE2IHogbSA3NC40NDcyNiwtMC40NjY3OSBjIDAuODg4MjYsLTAuMDU0NSAxLjg3NTIsMC4wNTQyIDIuOTU4OTksMC4zMjYxNyA1LjM4NDczLDEuMzUxNDggNi43ODA5OSwxMi4yMjI2OCAyLjEyNjk1LDE2LjU1ODU5IC0yLjc4Njg3LDIuNTk2MzUgLTUuMDQyNjEsMi41MjI0IC04LjI5Mjk3LC0wLjI3MzQzIC0yLjE0Njc3LC0xLjg0NjU3IC0yLjYzODY3LC0zLjEzNTc3IC0yLjYzODY3LC02LjkxMDE2IDAsLTYuMTc5MTMgMS45OTY1OCwtOS40NjQ4NyA1Ljg0NTcsLTkuNzAxMTcgeiBtIC0xNDUuMzIyMjYyLC0wLjAxMTcgYyAwLjgwODE0NywwLjAyODUgMS43ODA5NCwwLjIxNjAxIDIuOTQxNDA2LDAuNTQ4ODMgMy40Mzk3NDgsMC45ODY0OSA0LjYwMTA0NSwzLjU1NDM1IDIuNjYyMTEsNS44OTA2MiAtMC42NzQyNjgsMC44MTI0OSAtMi45MTQyMzMsMS40NTMxMyAtNS4wODM5ODUsMS40NTMxMyAtMy43NzEzNjMsMCAtMy44NzY5NTMsLTAuMDk2NiAtMy44NzY5NTMsLTMuNTMxMjUgMCwtMy4xMDUxIDAuOTMyOTgyLC00LjQ0Njg1IDMuMzU3NDIyLC00LjM2MTMzIHogbSAxNzAuNDI5NjkyLC0wLjAzNTIgYyAxLjc0OTk3LC0wLjAzNDYgMy40NjAzMSwwLjU0MzUxIDQuNTQ2ODcsMS43NDQxNCAzLjYzNzA4LDQuMDE4OTYgMi44MDg5OSwxMy45NjY4NSAtMS4zNDc2NiwxNi4xOTE0MSAtMi45MjIyNSwxLjU2MzkzIC00Ljg4NjQxLDEuMTgxOTUgLTcuNTMxMjUsLTEuNDYyODkgLTMuMjUzODcsLTMuMjUzODggLTMuNTExNzMsLTExLjQ4ODI3IC0wLjQ1NTA3LC0xNC41NDQ5MyAxLjI0NjcxLC0xLjI0NjczIDMuMDM3MTMsLTEuODkzMTUgNC43ODcxMSwtMS45Mjc3MyB6IG0gMTkuNzEyODksLTIuMDcyMjcgYyAtNC44ODYyNSwwIC02LjU1NTUyLDAuMzMzODQgLTYuMjUzOTEsMS4yNSAwLjIyNjQzLDAuNjg3NSAxLjM4NzYyLDEuMzk0NTcgMi41ODIwMywxLjU3MDMxIDIuMDMxODQsMC4yOTkgMi4xNzE4OCwwLjg2MjIxIDIuMTcxODgsOC43NTAwMSAwLDguMzI1NzcgLTAuMDMxLDguNDI5NjggLTIuNSw4LjQyOTY4IC0xLjU4MDg3LDAgLTIuNSwwLjU2NDUgLTIuNSwxLjUzNTE1IDAsMS4yOTg3NiAxLjU3Nzk4LDEuNDkxMTQgMTAuMjUsMS4yNSBsIDEwLjI1LC0wLjI4NTE1IDAuMzA0NjgsLTQuNzUgYyAwLjI0MTMzLC0zLjc2MDY3IC0wLjAxOTEsLTQuNzUgLTEuMjUsLTQuNzUgLTEuMTEwMjcsMCAtMS41NTQ2OCwxLjAwMDc1IC0xLjU1NDY4LDMuNSB2IDMuNSBoIC01IC01IHYgLTguNDI5NjggYyAwLC02LjkwMTgyIDAuMTA4MTIsLTguMTk1NDEgMS40OTYwOSwtOC42MTEzMyAwLjE5ODI4LC0wLjA1OTQgMC40MjE3OSwtMC4xMDEzMSAwLjY3NTc4LC0wLjEzODY4IDEuMTk0NDEsLTAuMTc1NzQgMi4zNTc1OSwtMC44ODI4MSAyLjU4Mzk5LC0xLjU3MDMxIDAuMzAxNzIsLTAuOTE2MjcgLTEuMzY5NjUsLTEuMjUgLTYuMjU1ODYsLTEuMjUgeiBtIC02OC4wNjgzNiwwIGMgLTkuMDc5MDMsMCAtOS40MTQzMSwwLjA4MDMgLTkuOTk2MSwyLjM5ODQ0IC0wLjgyNzM3LDMuMjk2NDMgLTAuMTkwMDYsNi44NTk5NyAxLjMxNDQ2LDcuMzU1NDYgMC44NDQwNCwwLjI3Nzk2IDEuMjUsLTAuNzUyMjkgMS4yNSwtMy4xNzE4NyAwLC0zLjMxNDYgMC4yMjQ4LC0zLjU4MjAzIDMsLTMuNTgyMDMgaCAzIHYgOC41IGMgMCw4LjM5OTg1IC0wLjAyOTQsOC41IC0yLjUsOC41IC0xLjU1NTU4LDAgLTIuNSwwLjU2NjY1IC0yLjUsMS41IGggMC4wMDIgYyAwLDEuMTc5NDggMS4zODg4NywxLjUgNi41LDEuNSAxLjI3Nzc3LDAgMi4zMjI5MiwtMC4wMTk5IDMuMTcxODcsLTAuMDY4NCAyLjU0Njg4LC0wLjE0NTQzIDMuMzI4MTMsLTAuNTQ3MDMgMy4zMjgxMywtMS40MzE2NCAwLC0wLjkzMzM1IC0wLjk0NDQyLC0xLjUgLTIuNSwtMS41IC0yLjQ2OTc3LDAgLTIuNSwtMC4xIC0yLjUsLTguNSB2IC04LjUgaCAzIGMgMi43NjE4OSwwIDMsMC4yNzc3NyAzLDMuNSAwLDMuMzkzNjEgMS4yMTM0Niw0LjU3OTMyIDIuNTIxNDgsMi40NjI4OSAwLjM1MTA1LC0wLjU3MDE3IDAuMzM4ODMsLTIuODIwMiAtMC4wMjkzLC01IGwgLTAuNjY5OTIsLTMuOTYyODkgeiBtIC01MS4zNTM1MiwwIGMgLTYuNjQxMTIsMCAtOS4yNDQ5OCwxLjIzMTU5IC01LjU3ODEyLDIuNjM4NjcgMi4yMzEwNSwwLjg1NjE4IDIuMTc0NzksMTYuODg3NTUgLTAuMDYyNSwxNy43NDYwOSAtMC44NTk3MywwLjMyOTk2IC0xLjI4Mzg0LDEuMDUzNTQgLTAuOTQxNDEsMS42MDc0MyAwLjkzMzc3LDEuNTEwODYgMTEuMjA3NzgsMS4yMTE4IDE0LjM1OTM3LC0wLjQxNzk3IDMuNDgyMjcsLTEuODAwNzYgNS4xNDQ1NCwtNS40MjAyOSA1LjE0NDU0LC0xMS4xOTkyMiAwLC0zLjY3MzM2IC0wLjU1MTQ3LC01LjA4Mjc2IC0yLjkyMTg4LC03LjQ1MzEyIC0yLjY4MDAyLC0yLjY4MDA3IC0zLjUxMTQ2LC0yLjkyMTg4IC0xMCwtMi45MjE4OCB6IG0gLTIxLjA3ODEyLDAgYyAtNi4wODU1MywwIC04LjA2MTYzLDAuMzE4NTQgLTcuNzU1ODYsMS4yNSAwLjIyNTcxLDAuNjg3NSAxLjgzODg2LDEuMzkwMTYgMy41ODM5OCwxLjU2MjUgbCAzLjE3MTg4LDAuMzE0NDUgdiA4LjQzNTU1IDguNDM3NSBoIC0zLjUgYyAtMi4xMzg5LDAgLTMuMjEzNzYsMC4zNDYzNCAtMy40NDkyMiwxLjEzNDc2IC0wLjAzMzYsMC4xMTI2NCAtMC4wNTA4LDAuMjM0MjggLTAuMDUwOCwwLjM2NTI0IDAsMC45MDYxNSAwLjg3NDk5LDEuMjk2ODYgNC4wMzEyNSwxLjQzNTU0IDEuMDUyMDgsMC4wNDYyIDIuMzU3NjMsMC4wNjQ1IDMuOTY4NzUsMC4wNjQ1IDEuNjExMSwwIDIuOTE2NjcsLTAuMDE4MiAzLjk2ODc1LC0wLjA2NDUgMy4xNTYyNCwtMC4xMzg2NyA0LjAzMTI1LC0wLjUyOTMgNC4wMzEyNSwtMS40MzU1NCAwLC0wLjEyNTAxIC0wLjAxNTYsLTAuMjQyMTggLTAuMDQ2OSwtMC4zNTE1NiAtMC4yMTg3NSwtMC43NjU2NSAtMS4yMDMxMSwtMS4xNDg0NCAtMi45NTMxMiwtMS4xNDg0NCBoIC0zIHYgLTguNDMzNiBjIDAsLTguMzk1ODcgMC4wMTMsLTguNDM0NzggMi42NzM4MiwtOC43NDk5OSAxLjQ3MDA5LC0wLjE3NDAxIDIuODU2NCwtMC44Nzg5MSAzLjA4MjA0LC0xLjU2NjQxIDAuMzA1ODcsLTAuOTMxMzkgLTEuNjcwMywtMS4yNSAtNy43NTU4NiwtMS4yNSB6IG0gLTIzLjkzOTQ2LDAgYyAtNi4yNDQzNDMsMCAtMTAuMDYwNTQ0LDAuMzkxMDggLTEwLjA2MDU0NCwxLjAzMTI1IDAsMC41NjcyNyAwLjY3NTAwNSwxLjI5MDcgMS41LDEuNjA3NDIgMS4xNjg1MTcsMC40NDg0MSAxLjUsMi40MzA3NyAxLjUsOC45Njg3NiAwLDYuNzkzNzcgLTAuMjg1ODI3LDguMzkyNTcgLTEuNSw4LjM5MjU3IC0wLjcyMTg3MSwwIC0xLjMyODI2NywwLjUxNzMxIC0xLjQ2ODc1LDEuMTk5MjIgLTAuMDIwMDcsMC4wOTc0IC0wLjAzMTI1LDAuMTk3NjUgLTAuMDMxMjUsMC4zMDA3OCAwLDEuMTY2NjIgMS4zMzMzNDIsMS41IDYuMDAwMDA0LDEuNSAxLjE2NjY3LDAgMi4xMjQ5OSwtMC4wMjA4IDIuOTA2MjUsLTAuMDcwMyAyLjM0Mzc1LC0wLjE0ODQ0IDMuMDkzNzUsLTAuNTU0NjkgMy4wOTM3NSwtMS40Mjk2OSAwLC0wLjEyNTc4IC0wLjAxNTMsLTAuMjQzNyAtMC4wNDY5LC0wLjM1MzUyIC0wLjIyMDY3LC0wLjc2ODY0IC0xLjIxNTA0LC0xLjE0NjQ4IC0zLjAwOTc2LC0xLjE0NjQ4IC0wLjM3MjM0LDAgLTAuNjk5OTEsLTAuMDAzIC0wLjk4NjMzLC0wLjAxMzcgLTAuMjg2NDMsLTAuMDExMSAtMC41MzE3NiwtMC4wMzA5IC0wLjc0MjE5LC0wLjA2ODQgLTAuMjEwNDMsLTAuMDM3NSAtMC4zODQ5NiwtMC4wOTI3IC0wLjUyOTMsLTAuMTcxODkgLTAuNzIxNjgsLTAuMzk2MDEgLTAuNjc5NTEsLTEuNDA4MzcgLTAuNDkyMTgsLTMuOTk2MDggMC4yNTU3OSwtMy41MzU3NiAwLjY4NDk1LC00LjMwMzg2IDIuNTYwNTQsLTQuNTcwMzIgMS4zNTIyOCwtMC4xOTIwNCAyLjQ5NjEsMC4zMDgxOCAyLjg1NzQyLDEuMjUgMC4zMzEzOSwwLjg2MzU4IDEuMDA0OTUsMS41NzAzMiAxLjQ5NjEsMS41NzAzMiAxLjIyODQ2LDAgMS4xMzE5MiwtNi43MzQwNCAtMC4xMDc0MiwtNy41IC0wLjU1LC0wLjMzOTkzIC0xLDAuMDgzOSAtMSwwLjk0MTQgMCwwLjY2NiAtMC4zNzE2MSwxLjEwNzUgLTEuMTUwMzksMS4zNDU3MSAtMC4xNTU3NiwwLjA0NzYgLTAuMzI2OTUsMC4wODc3IC0wLjUxNTYzLDAuMTE5MTQgLTAuMzc3MzcsMC4wNjI5IC0wLjgyMTM3LDAuMDkzNyAtMS4zMzM5OCwwLjA5MzcgLTIuNjY2NjUsMCAtMywtMC4zMzMzMiAtMywtMyAwLC0yLjk2NTk5IDAuMDY0NSwtMy4wMDAwMSA1LjQzMTY0LC0zLjAwMDAxIDQuNjk0NDcsMCA1LjQ3MTc2LDAuMjc1ODggNS43NDgwNCwyLjAzOTA3IDAuNTQ4NzUsMy41MDI0OSAyLjMwNTQ1LDIuNjQyMDYgMi42MzA4NiwtMS4yODkwNyBsIDAuMzEwNTUsLTMuNzUgeiBNIDc1Ljc5ODgyOCwxMDIuMDQzIGMgLTIuNjM2NjcxLDAuMDcxNCAtNC42Mzg2NzIsMC40MDE2NCAtNC42Mzg2NzIsMS4wMzcxMSAwLDAuNTUgMC42NzQ5NjcsMSAxLjUsMSAxLjIyMDY3NCwwIDEuNSwxLjY1NDM3IDEuNSw4Ljg5MjU5IDAsNi45ODI0MSAtMC4zMjIyOTksOS4wMTY4MyAtMS41LDkuNDY4NzUgLTAuODI1MDMzLDAuMzE2NTcgLTEuNSwxLjA0MDE5IC0xLjUsMS42MDc0MiB2IDAuMDAyIGMgMCwxLjM2NTI0IDguNjk0MDk5LDEuMzMxMjMgOS41NDEwMTYsLTAuMDM5MSAwLjM2MzIxMiwtMC41ODc3MyAtMC4yODI3NjcsLTEuMzEyODggLTEuNDM5NDUzLC0xLjYxNTI0IC0xLjYxMjAwNywtMC40MjE1NCAtMi4xMDE1NjMsLTEuMzM3MjEgLTIuMTAxNTYzLC0zLjkzMzU5IDAsLTIuOTQyODUgMC4zMjYwMDQsLTMuMzgyODIgMi41MDE5NTMsLTMuMzgyODIgMS44NTYzNTMsMCAzLjQyODE2MywxLjI4OTcxIDYuMDkzNzUsNSAzLjUwMzQ3MSw0Ljg3NjYxIDcuNDA0Mjk3LDYuNzIwNTMgNy40MDQyOTcsMy41IDAsLTAuODI1MDIgLTAuNTYyNDY2LC0xLjUwNjExIC0xLjI1LC0xLjUxMzY3IC0wLjY4NzQ5NiwtMC4wMDcgLTIuMjk2Njg4LC0xLjYyNzA3IC0zLjU3NjE3MiwtMy42MDE1NyBsIC0yLjMyNjE3MiwtMy41ODk4MyAyLjA3NjE3MiwtMS40NTUwOCBjIDIuOTE5NDU5LC0yLjA0NDk1IDIuODM0ODA3LC03LjY5NTAyIC0wLjE0NjQ4NCwtOS43ODMyMSAtMS41ODczMDcsLTEuMTExNzcgLTcuNzQ0MjIsLTEuNzEyNzEgLTEyLjEzODY3MiwtMS41OTM3NSB6IG0gMTc1LjEzMDg2MiwtMC41OTE3OSBjIC00LjE3MTE1LC0wLjAzMzcgLTguMjU1NjEsMi41NDE3OSAtOS43NjU2Myw3LjExNzE5IC0xLjU1MDM2LDQuNjk3NzMgLTAuNTgxNzQsOS42NzE0MSAyLjYyNSwxMy40ODI0MiAyLjA1MDk2LDIuNDM3NDIgMy4zMjU3MiwzLjAyOTMgNi41MjUzOSwzLjAyOTMgMi4xODcyNSwwIDQuODQyMjQsLTAuNDYyOTcgNS45MDAzOSwtMS4wMjkzIDUuNDc5MywtMi45MzI0MiA2LjU1ODI5LC0xNC44MTI4NSAxLjc3OTMsLTE5LjU5MTc5IC0yLjAyODkxLC0yLjAyODkyIC00LjU2MTc3LC0yLjk4NzYyIC03LjA2NDQ1LC0zLjAwNzgyIHogbSAtMjQuNjA3NDIsLTAuMjY1NjIgYyAtMS40NjczMSwwLjA2NjIgLTIuOTMxOTYsMC42NzU1OSAtNC44NTc0MywxLjg0OTYxIC02LjAyMDg2LDMuNjcxMTYgLTYuOTA5NDUsMTQuMTY2NzIgLTEuNjQ0NTMsMTkuNDMxNjQgMi44MTc2NCwyLjgxNzYzIDguNjg2NDQsMy41Njg1IDEyLjM5NDUzLDEuNTgzOTggNi45NTg0OSwtMy43MjQwNCA2LjI5NDg5LC0xNy42NzE2MiAtMS4wMjE0OCwtMjEuNDU1MDggLTEuOTMzNTcsLTAuOTk5ODcgLTMuNDAzNzksLTEuNDc2MzYgLTQuODcxMDksLTEuNDEwMTUgeiBtIC00Ny40OTgwNSwtNi4wNjY0NDkgYyAtMi42Nzk4MiwwLjA3MDk0IC01LjM2MDM3LDAuNzAzNTk2IC02Ljc5NDkyLDEuODY1MjM0IC0wLjI4NzQsMC4yMzI3MiAtMC41MzkwNSwwLjQzODUzNCAtMC43NTU4NiwwLjYyMzA0NyAtMC4yMTY4MSwwLjE4NDUxMyAtMC4zOTg4MywwLjM0OTMwMyAtMC41NDY4OCwwLjUgLTAuODg4MjgsMC45MDQxODMgLTAuNTU0OCwxLjMyODA4NyAwLjgwMjc0LDIuODI4MTI4IDEuNzMwOTgsMS45MTI3NCAyLjAxMTA2LDEuOTQxODggNC4yODMyLDAuNDUzMTIgMS45OTQ3NiwtMS4zMDcwMyAyLjkwMTEsLTEuMzc2NjggNS4wNTY2NCwtMC4zOTQ1MyAyLjI2NTE4LDEuMDMyMDggMi45MDcwOSwwLjkxNTE4IDQuNjY3OTcsLTAuODQ1NyAyLjAyMzc1LC0yLjAyMzU5OSAyLjAyMzczLC0yLjA1NTI0NSAwLjA4MiwtMy41MjM0NCAtMS40MzU4LC0xLjA4NTU5NCAtNC4xMTUxLC0xLjU3NjgwMSAtNi43OTQ5MiwtMS41MDU4NTkgeiBtIDAuMzI2MTcsLTE2LjA5MTc5NyBjIC00LjAzMTMzLDAuMDAzMiAtOC4wNjAxMiwwLjgxMDc1NyAtMTEuNDkwMjMsMi40MjE4NzUgLTYuNTEyMDMsMy4wNTg3MDUgLTkuMjUyMTUsNi4wMTgzMjcgLTcuNTE1NjMsOC4xMTMyODEgMS40NDc1OSwxLjc0MzIzOSAzLjYxMDQxLDEuOTgyNDU2IDQuNTI1MzksMC41MDE5NTMgMC4xNzIzOSwtMC4yNzg5MjkgMC43ODc5NSwtMC43NjYyNTEgMS42NDI1OCwtMS4zMjgxMjUgMC40MjczMSwtMC4yODA5MzcgMC45MTUwMywtMC41ODAwNDMgMS40MzU1NSwtMC44ODA4NTkgMC41MjA1MSwtMC4zMDA4MTYgMS4wNzQ1NywtMC42MDM4MzUgMS42MzY3MiwtMC44OTA2MjUgNy4yMTU4MywtMy42ODEyMjIgMTcuOTIwMTYsLTIuNDUxNjAyIDIzLjg2MzI4LDIuNzQyMTg3IDEuMzcwMywxLjE5NzU0NCAxLjgzNDgzLDEuMTE1NDAyIDMuMzg4NjcsLTAuNjAxNTYyIDIuMzUzOTYsLTIuNjAxMTA5IDAuODk2OTIsLTQuNDcxMjcxIC01Ljk3NjU2LC03LjY3NTc4MSAtMy40NDQ1NywtMS42MDUxNjYgLTcuNDc4NDQsLTIuNDA1NTggLTExLjUwOTc3LC0yLjQwMjM0NCB6IG0gLTAuMjI0MzgsLTE1LjkyNTc4MiBjIC0xMC4yMDc4NiwwLjA2NDM5IC0yMC40Mzk1MywzLjYyNTYwNCAtMjguNzIwNywxMC41OTM3NSAtMi45NzQzNCwyLjUwMjcyOCAtMy4wNDQ1MiwyLjcyNzYwNSAtMS40MzE2NCw0LjUwOTc2NiAwLjkzMzUsMS4wMzE1NDcgMS45MDA1LDEuODc1IDIuMTQ4NDQsMS44NzUgMC4yNDgxMiwwIDIuMjM1LC0xLjQxMzIzIDQuNDE2MDEsLTMuMTQwNjI1IDEyLjk4NjM4LC0xMC4yODU0MTcgMzMuNzQwODksLTEwLjY4NzM4NyA0NS45MzE2NCwtMC44OTA2MjUgNC40Mjc0OSwzLjU1ODA0NyA1LjAyMDcyLDMuNjUwMTI5IDcuMjk4ODMsMS4xMzI4MTMgMS42NTIwMywtMS44MjU0NzQgMS40OTY2OSwtMi4wNTkwOSAtNC4wMzEyNSwtNi4wMTc1NzkgLTcuNjE0ODcsLTUuNDUyODgzIC0xNi42MDQzOSwtOC4xMTkzMTQgLTI1LjYxMTMzLC04LjA2MjUgeiBtIDEuNjM4NjgsLTE1Ljk1MzEyNCBjIC0xNC42MjEyOSwtMC4wMDMgLTMwLjQ5MTc5LDUuMzkwNTIzIC00MC44MTQ0NSwxNC40Nzg1MTUgLTMuODE4LDMuMzYxMzIzIC0zLjg2ODA0LDMuNDg4ODY2IC0yLjA5NTcsNS40NDcyNjYgMC45OTgyOCwxLjEwMzA5MyAyLjAxNDc5LDIuMDA1ODU5IDIuMjU3ODEsMi4wMDU4NTkgMC4yNDI4NywwIDIuMzc4MTgsLTEuNzA0NzI5IDQuNzQ2MDksLTMuNzg5MDYyIDguNzgyMzQsLTcuNzMwNTMzIDIxLjY0NTI3LC0xMi4yMTgxMTcgMzQuOTYyODksLTEyLjE5OTIxOSAxMi43MTgwOCwwLjAxODc4IDIzLjg5Mjg5LDMuOTM5MTA3IDMzLjM3NSwxMS43MDg5ODQgNC4zMzAxMywzLjU0ODIyMSA0LjMzODI4LDMuNTUwMzkzIDYuMzI4MTMsMS41NjA1NDcgMS45ODk4OCwtMS45ODk4ODMgMS45ODYzMywtMS45OTU5NDIgLTIuMzM1OTQsLTUuNzI2NTYyIC01LjgwODQxLC01LjAxMzM5MyAtMTYuMzQ3NTIsLTEwLjIzMzE4IC0yNC41ODM5OCwtMTIuMTc1NzgyIC0zLjc0ODg2LC0wLjg4NDE3NCAtNy43NDU4OCwtMS4zMDk3MDYgLTExLjgzOTg1LC0xLjMxMDU0NiB6IG0gLTEuNTUyNzMsLTE1LjkwMjM0NCBjIC0xMC43MTM4MywwLjA0MDk4IC0yMS41MjcwNSwyLjMwNjg4NyAtMzEuNjYyMTEsNi45MDIzNDQgLTYuNjU3MjYsMy4wMTg1MTkgLTE3LjMxNjQ1LDEwLjE2OTY2NSAtMjAuNTkzNzUsMTMuODE0NDUzIC0xLjgxNDEsMi4wMTc1NDkgLTEuODIxNDcsMi4yMDk0MDYgLTAuMTYyMTEsNC4wNDI5NjggMS43MDM1OCwxLjg4MjQzMiAxLjg3OTQzLDEuODE0MTg4IDcuOTA0MywtMy4wNTY2NCAyNS44MTI4MSwtMjAuODY4Mzk3IDYzLjc4MjM0LC0yMS4wNTQwNTQgODguNjQ0NTMsLTAuNDMzNTk0IGwgNS40ODA0Nyw0LjU0Njg3NSAyLjAxOTUzLC0yLjA0Njg3NSAyLjAxNzU4LC0yLjA0ODgyOCAtNC41LC0zLjg3MzA0NyBDIDIxNC40NDk0NSwzNy4yODk5MDcgMTk2Ljg2ODExLDMxLjE3Nzc5NiAxNzkuMDExNzIsMzEuMjQ2MDk0IFoiIC8+CiAgPC9nPgo8L3N2Zz4K'><br><i>"
    "by Corey Harding<br>"
    "www.RFID-Tool.com<br>"
    "www.LegacySecurityGroup.com / www.Exploit.Agency</i><br>"
    "-----<br>"
    "File System Info Calculated in Bytes<br>"
    "<b>Total:</b> ")+total+" <b>Free:</b> "+freespace+" "+" <b>Used:</b> "+used+F("<br>-----<br>"
    "<a href=\"/logs\">List Exfiltrated Data</a><br>-<br>"
    "<a href=\"/experimental\">Experimental TX Mode</a><br>-<br>"
    "<a href=\"/data-convert\">Data Conversion Tools</a><br>-<br>"
    "<a href=\"/settings\">Configure Settings</a><br>-<br>"
    "<a href=\"/format\">Format File System</a><br>-<br>"
    "<a href=\"/firmware\">Upgrade Firmware</a><br>-<br>"
    "<a href=\"/api/help\">API Info</a><br>-<br>"
    "<a href=\"/help\">Help</a>"
    "</body></html>"));
  });

  server.onNotFound([]() {
    if (!RawFile(server.uri()))
      server.send(404, "text/plain", F("Error 404 File Not Found"));
  });
  server.on("/settings", handleSettings);

  server.on("/firmware", [](){
    server.send(200, "text/html", String()+F("<html><body style=\"height: 100%;\"><a href=\"/\"><- BACK TO INDEX</a><br><br>Open Arduino IDE.<br>Pull down \"Sketch\" Menu then select \"Export Compiled Binary\".<br>On this page click \"Browse\", select the binary you exported earlier, then click \"Update\".<br>You may need to manually reboot the device to reconnect.<br><iframe style =\"border: 0; height: 100%;\" src=\"http://")+local_IPstr+F(":1337/update\"><a href=\"http://")+local_IPstr+F(":1337/update\">Click here to Upload Firmware</a></iframe></body></html>"));
  });

  server.on("/restoredefaults", [](){
    server.send(200, "text/html", F("<html><body>This will restore the device to the default configuration.<br><br>Are you sure?<br><br><a href=\"/restoredefaults/yes\">YES</a> - <a href=\"/\">NO</a></body></html>"));
  });

  server.on("/restoredefaults/yes", [](){
    if(!server.authenticate(update_username, update_password))
      return server.requestAuthentication();
    server.send(200, "text/html", F("<a href=\"/\"><- BACK TO INDEX</a><br><br>Network<br>---<br>SSID: <b>ESP-RFID-Tool</b><br><br>Administration<br>---<br>USER: <b>admin</b> PASS: <b>rfidtool</b>"));
    delay(50);
    loadDefaults();
    ESP.restart();
  });

  server.on("/deletelog", [](){
    String deletelog;
    deletelog += server.arg(0);
    server.send(200, "text/html", String()+F("<html><body>This will delete the file: ")+deletelog+F(".<br><br>Are you sure?<br><br><a href=\"/deletelog/yes?payload=")+deletelog+F("\">YES</a> - <a href=\"/\">NO</a></body></html>"));
  });

  server.on("/settime", mysettime);

  server.on("/viewlog", ViewLog);

  server.on("/deletelog/yes", [](){
    if(!server.authenticate(update_username, update_password))
      return server.requestAuthentication();
    String deletelog;
    deletelog += server.arg(0);
    if (!deletelog.startsWith("/payloads/")) server.send(200, "text/html", String()+F("<a href=\"/\"><- BACK TO INDEX</a><br><br><a href=\"/logs\">List Exfiltrated Data</a><br><br>Deleting file: ")+deletelog);
    delay(50);
    SPIFFS.remove(deletelog);
  });

  server.on("/format", [](){
    server.send(200, "text/html", F("<html><body><a href=\"/\"><- BACK TO INDEX</a><br><br>This will reformat the SPIFFS File System.<br><br>Are you sure?<br><br><a href=\"/format/yes\">YES</a> - <a href=\"/\">NO</a></body></html>"));
  });

  server.on("/logs", ListLogs);

  server.on("/reboot", [](){
    if(!server.authenticate(update_username, update_password))
    return server.requestAuthentication();
    server.send(200, "text/html", F("<a href=\"/\"><- BACK TO INDEX</a><br><br>Rebooting Device..."));
    delay(50);
    ESP.restart();
  });
  
  server.on("/format/yes", [](){
    if(!server.authenticate(update_username, update_password))
      return server.requestAuthentication();
    server.send(200, "text/html", F("<a href=\"/\"><- BACK TO INDEX</a><br><br>Formatting file system: This may take up to 90 seconds"));
    delay(50);
//    Serial.print("Formatting file system...");
    SPIFFS.format();
//    Serial.println(" Success");
    saveConfig();
  });
  
  server.on("/help", []() {
    server.send_P(200, "text/html", HelpText);
  });
  
  server.on("/license", []() {
    server.send_P(200, "text/html", License);
  });

  server.on("/data-convert", [](){

    if (server.hasArg("bin2hexHTML")) {

      int bin2hexBUFFlen=(((server.arg("bin2hexHTML")).length())+1);
      char bin2hexCHAR[bin2hexBUFFlen];
      (server.arg("bin2hexHTML")).toCharArray(bin2hexCHAR,bin2hexBUFFlen);

      dataCONVERSION+=String()+F("Binary: ")+bin2hexCHAR+F("<br><br>");

      String hexTEMP="";

      int binCOUNT=(bin2hexBUFFlen-1);
      for (int currentBINpos=0; currentBINpos<binCOUNT; currentBINpos=currentBINpos+4) {
        char hexCHAR[2];
        char tempNIBBLE[5];
        strncpy(tempNIBBLE, &bin2hexCHAR[currentBINpos], 4);
        tempNIBBLE[4]='\0';
        sprintf(hexCHAR, "%X", (strtol(tempNIBBLE, NULL, 2)));
        hexTEMP+=hexCHAR;
      }

      dataCONVERSION+=String()+F("Hexadecimal: ")+hexTEMP+F("<br><small>You may want to drop the leading zero(if there is one) and if your cloning software does not handle it for you.</small><br><br>");
      hexTEMP="";
      
      dataCONVERSION+=F("<br><br>");
      
      bin2hexBUFFlen=0;
    }

    if (server.hasArg("hex2binHTML")) {

      int hex2binBUFFlen=(((server.arg("hex2binHTML")).length())+1);
      char hex2binCHAR[hex2binBUFFlen];
      (server.arg("hex2binHTML")).toCharArray(hex2binCHAR,hex2binBUFFlen);

      dataCONVERSION+=String()+F("Hexadecimal: ")+hex2binCHAR+F("<br><br>");

      String binTEMP="";

      int charCOUNT=(hex2binBUFFlen-1);
      for (int currentHEXpos=0; currentHEXpos<charCOUNT; currentHEXpos++) {
        char binCHAR[5];
        char tempHEX[2];
        strncpy(tempHEX, &hex2binCHAR[currentHEXpos], 1);
        tempHEX[1]='\0';
        int decimal=(unsigned char)strtoul(tempHEX, NULL, 16);
        itoa(decimal,binCHAR,2);
        while (strlen(binCHAR) < 4) {
          char *dup;
          sprintf(binCHAR,"%s%s","0",(dup=strdup(binCHAR)));
          free(dup);
        }
        binTEMP+=binCHAR;
      }

      dataCONVERSION+=String()+F("Binary: ")+binTEMP+F("<br><br>");
      binTEMP="";
      
      dataCONVERSION+=F("<br><br>");
      
      hex2binBUFFlen=0;
    }
    
    if (server.hasArg("abaHTML")) {
      String abaHTML=(server.arg("abaHTML"));

      dataCONVERSION="Trying \"Forward\" Swipe<br>";
      dataCONVERSION+=("Forward Binary:"+abaHTML+"<br>");
      int abaStart=abaHTML.indexOf("11010");
      int abaEnd=(abaHTML.lastIndexOf("11111")+4);
      dataCONVERSION+=aba2str(abaHTML,abaStart,abaEnd,"\"Forward\" Swipe");
      
      dataCONVERSION+=" * Trying \"Reverse\" Swipe<br>";
      int abaBUFFlen=((abaHTML.length())+1);
      char abachar[abaBUFFlen];
      abaHTML.toCharArray(abachar,abaBUFFlen);
      abaHTML=String(strrev(abachar));
      dataCONVERSION+=("Reversed Binary:"+abaHTML+"<br>");
      abaStart=abaHTML.indexOf("11010");
      abaEnd=(abaHTML.lastIndexOf("11111")+4);
      dataCONVERSION+=aba2str(abaHTML,abaStart,abaEnd,"\"Reverse\" Swipe");
    
      //dataCONVERSION+=(String()+F(" * You can verify the data at the following URL:<br><a target=\"_blank\" href=\"https://www.legacysecuritygroup.com/aba-decode.php?binary=")+abaHTML+F("\">https://www.legacysecuritygroup.com/aba-decode.php?binary=")+abaHTML+F("</a>"));
      dataCONVERSION.replace("*", "<br><br>");
      dataCONVERSION.replace(":", ": ");

      abaHTML="";
      abaStart=0;
      abaEnd=0;
    }
    
    server.send(200, "text/html", String()+F(
      "<a href=\"/\"><- BACK TO INDEX</a><br><br>")
      +dataCONVERSION+
      F(
      "<hr>"
      "<FORM action=\"/data-convert\" id=\"aba2ascii\" method=\"post\">"
      "<b>Convert ABA Binary Data to ASCII:</b><br>"
      "<INPUT form=\"aba2ascii\" type=\"text\" name=\"abaHTML\" value=\"\" pattern=\"[0-1]{1,}\" required title=\"Only 0's & 1's allowed, must not be empty\" minlength=\"1\" size=\"52\"><br>"
      "<INPUT form=\"aba2ascii\" type=\"submit\" value=\"Convert\"><br>"
      "</FORM>"
      "<br>"
      "<FORM action=\"/data-convert\" id=\"bin2hex\" method=\"post\">"
      "<b>Convert Binary Data to Hexadecimal:</b><br>"
      "<small>For use with card cloning, typically includes both the preamble and card data(binary before and after the space in log).</small><br>"
      "<INPUT form=\"bin2hex\" type=\"text\" name=\"bin2hexHTML\" value=\"\" pattern=\"[0-1]{1,}\" required title=\"Only 0's & 1's allowed, no spaces allowed, must not be empty\" minlength=\"1\" size=\"52\"><br>"
      "<INPUT form=\"bin2hex\" type=\"submit\" value=\"Convert\"><br>"
      "</FORM>"
      "<br>"
      "<FORM action=\"/data-convert\" id=\"hex2bin\" method=\"post\">"
      "<b>Convert Hexadecimal Data to Binary:</b><br>"
      "<small>In some situations you may want to add a leading zero to pad the output to come up with the correct number of bits.</small><br>"
      "<INPUT form=\"hex2bin\" type=\"text\" name=\"hex2binHTML\" value=\"\" pattern=\"[0-9a-fA-F]{1,}\" required title=\"Only characters 0-9 A-F a-f allowed, no spaces allowed, must not be empty\" minlength=\"1\" size=\"52\"><br>"
      "<INPUT form=\"hex2bin\" type=\"submit\" value=\"Convert\"><br>"
      "</FORM>"
      )
    );
      
    dataCONVERSION="";
  });

  #include "api_server.h"

  server.on("/stoptx", [](){
    server.send(200, "text/html", F("<html><body>This will kill any ongoing transmissions.<br><br>Are you sure?<br><br><a href=\"/stoptx/yes\">YES</a> - <a href=\"/\">NO</a></body></html>"));
  });

  server.on("/stoptx/yes", [](){
    TXstatus=0;
    server.send(200, "text/html", F("<a href=\"/\"><- BACK TO INDEX</a><br><br><a href=\"/experimental\"><- BACK TO EXPERIMENTAL TX MODE</a><br><br>All transmissions have been stopped."));
  });

  server.on("/experimental", [](){
    String experimentalStatus="Awaiting Instructions";

    if (server.hasArg("pinHTML")||server.hasArg("bruteEND")) {
      pinHTML=server.arg("pinHTML");
      int pinBITS=server.arg("pinBITS").toInt();
      int pinHTMLDELAY=server.arg("pinHTMLDELAY").toInt();
      int bruteforcing;
      int brutePAD=(server.arg("bruteSTART").length());
      if (server.hasArg("bruteSTART")) {
        bruteforcing=1;
      }
      else {
        bruteforcing=0;
      }

      TXstatus=1;
      
      wg.pause();
      digitalWrite(DATA0, HIGH);
      pinMode(DATA0,OUTPUT);
      digitalWrite(DATA1, HIGH);
      pinMode(DATA1,OUTPUT);

      pinHTML.replace("F1","C");
      pinHTML.replace("F2","D");
      pinHTML.replace("F3","E");
      pinHTML.replace("F4","F");

      experimentalStatus=String()+"Transmitting "+pinBITS+"bit Wiegand Format PIN: "+pinHTML+" with a "+pinHTMLDELAY+"ms delay between \"keypresses\"";
      delay(50);
      
      int bruteSTART;
      int bruteEND;
      if (server.hasArg("bruteSTART")) {
        bruteSTART=server.arg("bruteSTART").toInt();
      }
      else {
        bruteSTART=0;
      }
      
      if (server.hasArg("bruteEND")) {
        bruteEND=server.arg("bruteEND").toInt();
      }
      else {
        bruteEND=0;
      }

      if (server.hasArg("bruteSTART")) {
        server.send(200, "text/html", String()+"<a href=\"/\"><- BACK TO INDEX</a><br><br><a href=\"/experimental\"><- BACK TO EXPERIMENTAL TX MODE</a><br><br>Brute forcing "+pinBITS+"bit Wiegand Format PIN from "+(server.arg("bruteSTART"))+" to "+(server.arg("bruteEND"))+" with a "+pinHTMLDELAY+"ms delay between \"keypresses\"<br>This may take a while, your device will be busy until the sequence has been completely transmitted!<br>Please \"STOP CURRENT TRANSMISSION\" before attempting to use your device or simply wait for the transmission to finish.<br>You can view if the brute force attempt has completed by returning to the Experimental TX page and checking the status located under \"Transmit Status\"<br><br><a href=\"/stoptx\"><button>STOP CURRENT TRANSMISSION</button></a>");
        delay(50);
      }

      String bruteSTARTchar="";
      String bruteENDchar="";
      if (server.hasArg("bruteSTARTchar")&&(server.arg("bruteSTARTchar")!="")) {
        bruteSTARTchar=(server.arg("bruteSTARTchar"));
        bruteSTARTchar.replace("F1","C");
        bruteSTARTchar.replace("F2","D");
        bruteSTARTchar.replace("F3","E");
        bruteSTARTchar.replace("F4","F");
      }
      if (server.hasArg("bruteENDchar")&&(server.arg("bruteENDchar")!="")) {
        bruteENDchar=(server.arg("bruteENDchar"));
        bruteENDchar=(server.arg("bruteENDchar"));
        bruteENDchar.replace("F1","C");
        bruteENDchar.replace("F2","D");
        bruteENDchar.replace("F3","E");
        bruteENDchar.replace("F4","F");
      }

      unsigned long bruteFAILdelay=0;
      unsigned long bruteFAILS=0;
      int bruteFAILmultiplier=0;
      int bruteFAILmultiplierCURRENT=0;
      int bruteFAILmultiplierAFTER=0;
      int delayAFTERpin=0;
      int bruteFAILSmax=0;
      bruteFAILSmax=(server.arg("bruteFAILSmax")).toInt();
      delayAFTERpin=(server.arg("delayAFTERpin")).toInt();
      bruteFAILdelay=(server.arg("bruteFAILdelay")).toInt();
      bruteFAILmultiplier=(server.arg("bruteFAILmultiplier")).toInt();
      bruteFAILmultiplierAFTER=(server.arg("bruteFAILmultiplierAFTER")).toInt();

      for (int brute=bruteSTART; brute<=bruteEND; brute++) {

        if (bruteforcing==1) {
          pinHTML=String(brute);
          while (pinHTML.length()<brutePAD) {
            pinHTML="0"+pinHTML;
          }
        }

        if (bruteSTARTchar!="") {
          pinHTML=bruteSTARTchar+pinHTML;
        }

        if (bruteENDchar!="") {
          pinHTML=pinHTML+bruteENDchar;
        }
          
        for (int i=0; i<=pinHTML.length(); i++) {
          if (pinHTML.charAt(i) == '0') {
            if (pinBITS==4) {
              pinSEND(pinHTMLDELAY,"0000");
            }
            else if (pinBITS==8) {
              pinSEND(pinHTMLDELAY,"11110000");
            }
          }
          else if (pinHTML.charAt(i) == '1') {
            if (pinBITS==4) {
              pinSEND(pinHTMLDELAY,"0001");
            }
            else if (pinBITS==8) {
              pinSEND(pinHTMLDELAY,"11100001");
            }
          }
          else if (pinHTML.charAt(i) == '2') {
            if (pinBITS==4) {
              pinSEND(pinHTMLDELAY,"0010");
            }
            else if (pinBITS==8) {
              pinSEND(pinHTMLDELAY,"11010010");
            }
          }
          else if (pinHTML.charAt(i) == '3') {
            if (pinBITS==4) {
              pinSEND(pinHTMLDELAY,"0011");
            }
            else if (pinBITS==8) {
              pinSEND(pinHTMLDELAY,"11000011");
            }
          }
          else if (pinHTML.charAt(i) == '4') {
            if (pinBITS==4) {
              pinSEND(pinHTMLDELAY,"0100");
            }
            else if (pinBITS==8) {
              pinSEND(pinHTMLDELAY,"10110100");
            }
          }
          else if (pinHTML.charAt(i) == '5') {
            if (pinBITS==4) {
              pinSEND(pinHTMLDELAY,"0101");
            }
            else if (pinBITS==8) {
              pinSEND(pinHTMLDELAY,"10100101");
            }
          }
          else if (pinHTML.charAt(i) == '6') {
            if (pinBITS==4) {
              pinSEND(pinHTMLDELAY,"0110");
            }
            else if (pinBITS==8) {
              pinSEND(pinHTMLDELAY,"10010110");
            }
          }
          else if (pinHTML.charAt(i) == '7') {
            if (pinBITS==4) {
              pinSEND(pinHTMLDELAY,"0111");
            }
            else if (pinBITS==8) {
              pinSEND(pinHTMLDELAY,"10000111");
            }
          }
          else if (pinHTML.charAt(i) == '8') {
            if (pinBITS==4) {
              pinSEND(pinHTMLDELAY,"1000");
            }
            else if (pinBITS==8) {
              pinSEND(pinHTMLDELAY,"01111000");
            }
          }
          else if (pinHTML.charAt(i) == '9') {
            if (pinBITS==4) {
              pinSEND(pinHTMLDELAY,"1001");
            }
            else if (pinBITS==8) {
              pinSEND(pinHTMLDELAY,"01101001");
            }
          }
          else if ((pinHTML.charAt(i) == '*')||(pinHTML.charAt(i) == 'A')) {
            if (pinBITS==4) {
              pinSEND(pinHTMLDELAY,"1010");
            }
            else if (pinBITS==8) {
              pinSEND(pinHTMLDELAY,"01011010");
            }
          }
          else if ((pinHTML.charAt(i) == '#')||(pinHTML.charAt(i) == 'B')) {
            if (pinBITS==4) {
              pinSEND(pinHTMLDELAY,"1011");
            }
            else if (pinBITS==8) {
              pinSEND(pinHTMLDELAY,"01001011");
            }
          }
          else if (pinHTML.charAt(i) == 'C') { //F1
            if (pinBITS==4) {
              pinSEND(pinHTMLDELAY,"1100");
            }
            else if (pinBITS==8) {
              pinSEND(pinHTMLDELAY,"00111100");
            }
          }
          else if (pinHTML.charAt(i) == 'D') { //F2
            if (pinBITS==4) {
              pinSEND(pinHTMLDELAY,"1101");
            }
            else if (pinBITS==8) {
              pinSEND(pinHTMLDELAY,"00101101");
            }
          }
          else if (pinHTML.charAt(i) == 'E') { //F3
            if (pinBITS==4) {
              pinSEND(pinHTMLDELAY,"1110");
            }
            else if (pinBITS==8) {
              pinSEND(pinHTMLDELAY,"00011110");
            }
          }
          else if (pinHTML.charAt(i) == 'F') { //F4
            if (pinBITS==4) {
              pinSEND(pinHTMLDELAY,"1111");
            }
            else if (pinBITS==8) {
              pinSEND(pinHTMLDELAY,"00001111");
            }
          }
        }

        server.handleClient();
        if (TXstatus!=1) {
          break;
        }

        bruteFAILS++;

        if (bruteFAILS>=4294967000) {
          bruteFAILS=(4294966000);
        }
        if (bruteFAILdelay>=4294967000) {
          bruteFAILdelay=(4294966000);
        }
        
        if (bruteFAILmultiplier!=0) {
          bruteFAILmultiplierCURRENT++;
          if (bruteFAILmultiplierCURRENT>=bruteFAILmultiplierAFTER) {
            bruteFAILmultiplierCURRENT=0;
            bruteFAILdelay=(bruteFAILdelay*bruteFAILmultiplier);
          }
        }
        
        if ((bruteFAILS>=bruteFAILSmax)&&(bruteFAILSmax!=0)) {
          delay(bruteFAILdelay*1000);
        }
        else {
          delay(delayAFTERpin);
        }
        
      }
      pinMode(DATA0, INPUT);
      pinMode(DATA1, INPUT);
      wg.clear();
      pinHTML="";
      pinHTMLDELAY=100;
      TXstatus=0;
      bruteforcing=0;
      brutePAD=0;
      bruteSTARTchar="";
      bruteENDchar="";
      bruteFAILdelay=0;
      bruteFAILS=0;
      bruteFAILmultiplier=0;
      bruteFAILmultiplierCURRENT=0;
      bruteFAILmultiplierAFTER=0;
      delayAFTERpin=0;
      bruteFAILSmax=0;
    }


    if (server.hasArg("binHTML")) {
      String binHTML=server.arg("binHTML");
      wg.pause();
      digitalWrite(DATA0, HIGH);
      pinMode(DATA0,OUTPUT);
      digitalWrite(DATA1, HIGH);
      pinMode(DATA1,OUTPUT);

      for (int i=0; i<=binHTML.length(); i++) {
        if (binHTML.charAt(i) == '0') {
          digitalWrite(DATA0, LOW);
          delayMicroseconds(txdelayus);
          digitalWrite(DATA0, HIGH);
        }
        else if (binHTML.charAt(i) == '1') {
          digitalWrite(DATA1, LOW);
          delayMicroseconds(txdelayus);
          digitalWrite(DATA1, HIGH);
        }
        delay(txdelayms);
      }

      pinMode(DATA0, INPUT);
      pinMode(DATA1, INPUT);
      wg.clear();

      experimentalStatus=String()+"Transmitting Binary: "+binHTML;
      binHTML="";
    }

    if (server.arg("fuzzType")=="simultaneous") {

      int fuzzTimes=0;
      dos=0;
      if ((server.arg("fuzzTimes"))=="dos") {
        dos=1;
        server.send(200, "text/html", String()+
        "<a href=\"/\"><- BACK TO INDEX</a><br><br>"
        "<a href=\"/experimental\"><- BACK TO EXPERIMENTAL TX MODE</a><br><br>"
        "Denial of Service mode active.<br>Transmitting D0 and D1 bits simultaneously until stopped."
        "<br>This may take a while, your device will be busy until the sequence has been completely transmitted!"
        "<br>Please \"STOP CURRENT TRANSMISSION\" before attempting to use your device or simply wait for the transmission to finish.<br>"
        "You can view if the fuzzing attempt has completed by returning to the Experimental TX page and checking the status located under \"Transmit Status\"<br><br>"
        "<a href=\"/stoptx\"><button>STOP CURRENT TRANSMISSION</button></a>");
        delay(50);
      }
      else {
        fuzzTimes=server.arg("fuzzTimes").toInt();
        server.send(200, "text/html", String()+
        "<a href=\"/\"><- BACK TO INDEX</a><br><br>"
        "<a href=\"/experimental\"><- BACK TO EXPERIMENTAL TX MODE</a><br><br>"
        "Transmitting D0 and D1 bits simultaneously "+fuzzTimes+" times."
        "<br>This may take a while, your device will be busy until the sequence has been completely transmitted!"
        "<br>Please \"STOP CURRENT TRANSMISSION\" before attempting to use your device or simply wait for the transmission to finish.<br>"
        "You can view if the fuzzing attempt has completed by returning to the Experimental TX page and checking the status located under \"Transmit Status\"<br><br>"
        "<a href=\"/stoptx\"><button>STOP CURRENT TRANSMISSION</button></a>");
        delay(50);
      }
      
      wg.pause();
      digitalWrite(DATA0, HIGH);
      pinMode(DATA0,OUTPUT);
      digitalWrite(DATA1, HIGH);
      pinMode(DATA1,OUTPUT);

      TXstatus=1;

      for (int i=0; i<=fuzzTimes || dos==1; i++) {
        digitalWrite(DATA0, LOW);
        digitalWrite(DATA1, LOW);
        delayMicroseconds(txdelayus);
        digitalWrite(DATA0, HIGH);
        digitalWrite(DATA1, HIGH);
        delay(txdelayms);
        server.handleClient();
        if (TXstatus!=1) {
          break;
        }
      }

      pinMode(DATA0, INPUT);
      pinMode(DATA1, INPUT);
      wg.clear();
      TXstatus=0;
      dos=0;

      //experimentalStatus=String()+"Transmitting D0 and D1 bits simultaneously "+fuzzTimes+" times.";
    }

    if (server.arg("fuzzType")=="alternating") {

      int fuzzTimes=0;
      dos=0;
      if ((server.arg("fuzzTimes"))=="dos") {
        dos=1;
        server.send(200, "text/html", String()+
        "<a href=\"/\"><- BACK TO INDEX</a><br><br>"
        "<a href=\"/experimental\"><- BACK TO EXPERIMENTAL TX MODE</a><br><br>"
        "Denial of Service mode active.<br>Transmitting bits alternating between D0 and D1 until stopped."
        "<br>This may take a while, your device will be busy until the sequence has been completely transmitted!"
        "<br>Please \"STOP CURRENT TRANSMISSION\" before attempting to use your device or simply wait for the transmission to finish.<br>"
        "You can view if the fuzzing attempt has completed by returning to the Experimental TX page and checking the status located under \"Transmit Status\"<br><br>"
        "<a href=\"/stoptx\"><button>STOP CURRENT TRANSMISSION</button></a>");
        delay(50);
      }
      else {
        fuzzTimes=server.arg("fuzzTimes").toInt();
        server.send(200, "text/html", String()+
        "<a href=\"/\"><- BACK TO INDEX</a><br><br>"
        "<a href=\"/experimental\"><- BACK TO EXPERIMENTAL TX MODE</a><br><br>"
        "Transmitting "+fuzzTimes+" bits alternating between D0 and D1."
        "<br>This may take a while, your device will be busy until the sequence has been completely transmitted!"
        "<br>Please \"STOP CURRENT TRANSMISSION\" before attempting to use your device or simply wait for the transmission to finish.<br>"
        "You can view if the fuzzing attempt has completed by returning to the Experimental TX page and checking the status located under \"Transmit Status\"<br><br>"
        "<a href=\"/stoptx\"><button>STOP CURRENT TRANSMISSION</button></a>");
        delay(50);
      }
      
      wg.pause();
      digitalWrite(DATA0, HIGH);
      pinMode(DATA0,OUTPUT);
      digitalWrite(DATA1, HIGH);
      pinMode(DATA1,OUTPUT);

      String binALT="";
      TXstatus=1;

      for (int i=0; i<fuzzTimes || dos==1; i++) {
        if (i%2==0) {
          digitalWrite(DATA0, LOW);
          delayMicroseconds(txdelayus);
          digitalWrite(DATA0, HIGH);
          binALT=binALT+"0";
        }
        else {
           digitalWrite(DATA1, LOW);
           delayMicroseconds(txdelayus);
           digitalWrite(DATA1, HIGH);
           binALT=binALT+"1";
        }
        delay(txdelayms);
        server.handleClient();
        if (TXstatus!=1) {
          break;
        }
      }

      pinMode(DATA0, INPUT);
      pinMode(DATA1, INPUT);
      wg.clear();
      TXstatus=0;
      dos=0;

      //experimentalStatus=String()+"Transmitting alternating bits: "+binALT;
      binALT="";
    }

    if (server.arg("pushType")=="Ground") {
      Serial.end();
      digitalWrite(3,LOW);
      pinMode(3,OUTPUT);
      delay(server.arg("pushTime").toInt());
      pinMode(3,INPUT);
      Serial.begin(9600);

      experimentalStatus=String()+"Grounding \"Push to Open\" wire for "+(server.arg("pushTime").toInt())+"ms.";
    }

    if (server.arg("pushType")=="High") {
      Serial.end();
      digitalWrite(3,HIGH);
      pinMode(3,OUTPUT);
      delay(server.arg("pushTime").toInt());
      pinMode(3,INPUT);
      Serial.begin(9600);

      experimentalStatus=String()+"Outputting 3.3V on \"Push to Open\" wire for "+(server.arg("pushTime").toInt())+"ms.";
    }

    String activeTX="";
    if (TXstatus==1) {
      
      if (pinHTML!="") {
        String currentPIN=pinHTML;
        activeTX="Brute forcing PIN: "+currentPIN+"<br><a href=\"/stoptx\"><button>STOP CURRENT TRANSMISSION</button></a>";
        currentPIN="";
      }
      else if (dos==1) {
        activeTX="Denial of Service mode active...<br><a href=\"/stoptx\"><button>STOP CURRENT TRANSMISSION</button></a>";
      }
      else {
        activeTX="Transmitting...<br><a href=\"/stoptx\"><button>STOP CURRENT TRANSMISSION</button></a>";
      }
      
    }
    else {
      activeTX="INACTIVE<br><button>NOTHING TO STOP</button>";
    }

    server.send(200, "text/html", 
      String()+
      F(
      "<!DOCTYPE HTML>"
      "<html>"
      "<head>"
      "<title>Experimental TX Mode</title>"
      "</head>"
      "<body>"
      )+experimentalStatus+"<br><br>"
      +F(
      "<b>Transmit Status:</b> ")+activeTX+F("<br><br>"
      "<a href=\"/\"><- BACK TO INDEX</a><br>"
      "<P>"
      "<h1>Experimental TX Mode</h1>"
      "<hr>"
      "<small>"
      "<b>Warning:</b> This mode is highly experimental, use at your own risk!<br>"
      "Note: Timings for the Wiegand Data Pulse Width and Wiegand Data Interval may be changed on the settings page."
      "</small>"
      "<br>"
      "<hr>"
      "<br>"
      "<FORM action=\"/experimental\" id=\"transmitbinary\" method=\"post\">"
      "<b>Binary Data:</b><br>"
      "<small>Typically no need to include preamble</small><br>")+
      ListCards() +
      //"<INPUT form=\"transmitbinary\" type=\"text\" name=\"binHTML\" value=\"\" pattern=\"[0-1]{1,}\" required title=\"Only 0's & 1's allowed, must not be empty\" minlength=\"1\" size=\"52\"><br>"
      F("<INPUT form=\"transmitbinary\" type=\"submit\" value=\"Transmit\"><br>"
      "</FORM>"
      "<br>"
      "<hr>"
      "<br>"
      "<FORM action=\"/experimental\" id=\"transmitpin\" method=\"post\">"
      "<b>Transmit PIN:</b><br>"
      "<small>Available keys 0-9, * or A, # or B, F1 or C, F2 or D, F3 or E, F4 or F</small><br>"
      "<small>PIN: </small><INPUT form=\"transmitpin\" type=\"text\" name=\"pinHTML\" value=\"\" pattern=\"[0-9*#A-F]{1,}\" required title=\"Available keys 0-9, * or A, # or B, F1 or C, F2 or D, F3 or E, F4 or F, must not be empty\" minlength=\"1\" size=\"52\"><br>"
      "<small>Delay between \"keypresses\": </small><INPUT form=\"transmitpin\" type=\"number\" name=\"pinHTMLDELAY\" value=\"100\" minlength=\"1\" min=\"0\" size=\"8\"><small>ms</small><br>"
      "<INPUT form=\"transmitpin\" type=\"radio\" name=\"pinBITS\" id=\"pinBITS\" value=\"4\" checked required> <small>4bit Wiegand PIN Format</small>   "
      "<INPUT form=\"transmitpin\" type=\"radio\" name=\"pinBITS\" id=\"pinBITS\" value=\"8\" required> <small>8bit Wiegand PIN Format</small><br>"
      "<INPUT form=\"transmitpin\" type=\"submit\" value=\"Transmit\"><br>"
      "</FORM>"
      "<br>"
      "<hr>"
      "<br>"
      "<FORM action=\"/experimental\" id=\"brutepin\" method=\"post\">"
      "<b>Bruteforce PIN:</b><br>"
      "<small>Delay between \"keypresses\": </small><INPUT form=\"brutepin\" type=\"number\" name=\"pinHTMLDELAY\" value=\"3\" minlength=\"1\" min=\"0\" size=\"8\"><small>ms</small><br>"
      "<small>Delay between entering complete PINs: </small><INPUT form=\"brutepin\" type=\"number\" name=\"delayAFTERpin\" value=\"0\" minlength=\"1\" min=\"0\" size=\"8\"><small>ms</small><br>"
      "<small>PIN begins with character(s): </small><INPUT form=\"brutepin\" type=\"text\" name=\"bruteSTARTchar\" value=\"\" pattern=\"[0-9*#A-F]{0,}\" title=\"Available keys 0-9, * or A, # or B, F1 or C, F2 or D, F3 or E, F4 or F, must not be empty\" size=\"8\"><br>"
      "<small>PIN start position: </small><INPUT form=\"brutepin\" type=\"number\" name=\"bruteSTART\" value=\"0000\" minlength=\"1\" min=\"0\" size=\"8\"><br>"
      "<small>PIN end position: </small><INPUT form=\"brutepin\" type=\"number\" name=\"bruteEND\" value=\"9999\" minlength=\"1\" min=\"0\" size=\"8\"><br>"
      "<small>PIN ends with character(s): </small><INPUT form=\"brutepin\" type=\"text\" name=\"bruteENDchar\" value=\"#\" pattern=\"[0-9*#A-F]{0,}\" title=\"Available keys 0-9, * or A, # or B, F1 or C, F2 or D, F3 or E, F4 or F, must not be empty\" size=\"8\"><br>"
      "<small>NOTE: The advanced timing settings listed below override the \"Delay between entering complete PINs\" setting(listed above) when the conditions listed below are met.</small><br>"
      "<small>Number of failed PIN attempts(X) before a delay: </small><INPUT form=\"brutepin\" type=\"number\" name=\"bruteFAILSmax\" value=\"0\" minlength=\"1\" min=\"0\" size=\"8\"><br>"
      "<small>Delay in seconds(Y) after [X] failed PINs: </small><INPUT form=\"brutepin\" type=\"number\" name=\"bruteFAILdelay\" value=\"0\" minlength=\"1\" min=\"0\" size=\"8\"><small>s</small><br>"
      "<small>Multiply delay [Y] by <INPUT form=\"brutepin\" type=\"number\" name=\"bruteFAILmultiplier\" value=\"0\" minlength=\"1\" min=\"0\" size=\"4\"> after every <INPUT form=\"brutepin\" type=\"number\" name=\"bruteFAILmultiplierAFTER\" value=\"0\" minlength=\"1\" min=\"0\" size=\"4\"> failed pin attempts</small><br>"
      "<INPUT form=\"brutepin\" type=\"radio\" name=\"pinBITS\" id=\"pinBITS\" value=\"4\" checked required> <small>4bit Wiegand PIN Format</small>   "
      "<INPUT form=\"brutepin\" type=\"radio\" name=\"pinBITS\" id=\"pinBITS\" value=\"8\" required> <small>8bit Wiegand PIN Format</small><br>"
      "<INPUT form=\"brutepin\" type=\"submit\" value=\"Transmit\"></FORM><br>"
      "<br>"
      "<hr>"
      "<br>"
      "<b>Fuzzing:</b><br><br>"
      "<FORM action=\"/experimental\" id=\"fuzz\" method=\"post\">"
      "<b>Number of bits:</b>"
      "<INPUT form=\"fuzz\" type=\"number\" name=\"fuzzTimes\" value=\"100\" minlength=\"1\" min=\"1\" max=\"2147483647\" size=\"32\"><br>"
      //"<INPUT form=\"fuzz\" type=\"text\" name=\"fuzzTimes\" value=\"\" pattern=\"^[1-9]+[0-9]*$\" required title=\"Must be a number > 0, must not be empty \" minlength=\"1\" size=\"32\"><br>"
      "<INPUT form=\"fuzz\" type=\"radio\" name=\"fuzzType\" id=\"simultaneous\" value=\"simultaneous\" required> <small>Transmit a bit simultaneously on D0 and D1 (X bits per each line)</small><br>"
      "<INPUT form=\"fuzz\" type=\"radio\" name=\"fuzzType\" id=\"alternating\" value=\"alternating\"> <small>Transmit X bits alternating between D0 and D1 each bit (01010101,etc)</small><br>"
      "<INPUT form=\"fuzz\" type=\"submit\" value=\"Fuzz\"><br>"
      "</FORM>"
      "<br>"
      "<hr>"
      "<br>"
      "<b>Denial Of Service Mode:</b><br><br>"
      "<FORM action=\"/experimental\" id=\"dos\" method=\"post\">"
      "<b>Type of Attack:</b>"
      "<INPUT hidden=\"1\" form=\"dos\" type=\"text\" name=\"fuzzTimes\" value=\"dos\"><br>"
      "<INPUT form=\"dos\" type=\"radio\" name=\"fuzzType\" id=\"simultaneous\" value=\"simultaneous\" required> <small>Transmit a bit simultaneously on D0 and D1 until stopped</small><br>"
      "<INPUT form=\"dos\" type=\"radio\" name=\"fuzzType\" id=\"alternating\" value=\"alternating\"> <small>Transmit bits alternating between D0 and D1 each bit (01010101,etc) until stopped</small><br>"
      "<INPUT form=\"dos\" type=\"submit\" value=\"Start DoS\"><br>"
      "</FORM>"
      "<br>"
      "<hr>"
      "<br>"
      "<b>Push Button for Door Open:</b><br>"
      "<small>Connect \"Push to Open\" wire from the reader to the RX pin(GPIO3) on the programming header on ESP-RFID-Tool.</small><br>"
      "<small>Warning! Selecting the wrong trigger signal type may cause damage to the connected hardware.</small><br><br>"
      "<FORM action=\"/experimental\" id=\"push\" method=\"post\">"
      "<b>Time in ms to push the door open button:</b>"
      "<INPUT form=\"push\" type=\"text\" name=\"pushTime\" value=\"50\" pattern=\"^[1-9]+[0-9]*$\" required title=\"Must be a number > 0, must not be empty\" minlength=\"1\" size=\"32\"><br>"
      "<b>Does the wire expect a High or Low signal to open the door:</b>"
      "<INPUT form=\"push\" type=\"radio\" name=\"pushType\" id=\"Ground\" value=\"Ground\" checked required> <small>Low Signal[Ground]</small>   "
      "<INPUT form=\"push\" type=\"radio\" name=\"pushType\" id=\"Ground\" value=\"High\" required> <small>High Signal[3.3V]</small><br>"
      "<INPUT form=\"push\" type=\"submit\" value=\"Push\"><br>"
      "</FORM>"
      "<br>"
      "<hr>"
      "<br>"
      "</P>"
      "</body>"
      "</html>"
      )
    );

    if (server.args()>=1) {
      if (safemode==1) {
        delay(50);
        ESP.restart();
      }
    }

  });

  server.begin();
  WiFiClient client;
  client.setNoDelay(1);

//  Serial.println("Web Server Started");

  MDNS.begin("ESP");

  httpUpdater.setup(&httpServer, update_path, update_username, update_password);
  httpServer.begin();

  MDNS.addService("http", "tcp", 1337);
  
//  if (ftpenabled==1){
//    ftpSrv.begin(String(ftp_username),String(ftp_password));
//  }

//Start RFID Reader
  pinMode(LED_BUILTIN, OUTPUT);  // LED
  if (ledenabled==1){
    digitalWrite(LED_BUILTIN, LOW);
  }
  else{
    digitalWrite(LED_BUILTIN, HIGH);
  }

}
//

//Do It!

///////////////////////////////////////////////////////
// LOOP function
void loop()
{
//  if (ftpenabled==1){
//    ftpSrv.handleFTP();
//  }
  server.handleClient();
  httpServer.handleClient();
  while (Serial.available()) {
    String cmd = Serial.readStringUntil(':');
    if(cmd == "ResetDefaultConfig"){
      loadDefaults();
      ESP.restart();
    }
  }

//Serial.print("Free heap-");
//Serial.println(ESP.getFreeHeap(),DEC);

  if(wg.available()) {
    wg.pause();             // pause Wiegand pin interrupts
    LogWiegand(wg);
    wg.clear();             // compulsory to call clear() to enable interrupts for subsequent data
    if (safemode==1) {
      ESP.restart();
    }
  }

}
