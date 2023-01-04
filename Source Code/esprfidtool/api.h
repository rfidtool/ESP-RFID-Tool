void apiTX(String apiBIN, int apipulsewidth, int apidatainterval, int wait) {
  wg.pause();
  digitalWrite(DATA0, HIGH);
  pinMode(DATA0,OUTPUT);
  digitalWrite(DATA1, HIGH);
  pinMode(DATA1,OUTPUT);
  for (int i=0; i<=apiBIN.length(); i++) {
    if (apiBIN.charAt(i) == '0') {
      digitalWrite(DATA0, LOW);
      delayMicroseconds(apipulsewidth);
      digitalWrite(DATA0, HIGH);
    }
    else if (apiBIN.charAt(i) == '1') {
      digitalWrite(DATA1, LOW);
      delayMicroseconds(apipulsewidth);
      digitalWrite(DATA1, HIGH);
    }
    if (apiBIN.charAt(i) == ',') {
      delayMicroseconds(wait);
    }
    else {
      delayMicroseconds(apidatainterval);
    }
  }
  apiBIN="";
  pinMode(DATA0, INPUT);
  pinMode(DATA1, INPUT);
  wg.clear();
}

void apiinfo(int prettify) {

  FSInfo fs_info;
  SPIFFS.info(fs_info);
  String total;
  total=fs_info.totalBytes;
  String used;
  used=fs_info.usedBytes;
  String freespace;
  freespace=fs_info.totalBytes-fs_info.usedBytes;
  
  const size_t bufferSize = JSON_ARRAY_SIZE(5) + JSON_OBJECT_SIZE(3);
  DynamicJsonDocument apilog(bufferSize);

  apilog["Device"] = "ESP-RFID-Tool";
  apilog["Firmware"] = version;
  apilog["API"] = APIversion;
  JsonObject apifs = apilog.createNestedObject("File System");
  apifs["Total Space"]=total;
  apifs["Used Space"]=used;
  apifs["Free Space"]=freespace;
  apilog["Free Memory"] = String(ESP.getFreeHeap(),DEC);
  
  String API_Response="";
  if (prettify==1) {
    serializeJsonPretty(apilog, API_Response);
  }
  else {
    serializeJson(apilog, API_Response);
  }
  server.send(200, "application/json", API_Response);
  delay(50);
  apifs.clear();
  apilog.clear();
}

void apilistlogs(int prettify) {
  Dir dir = SPIFFS.openDir("/");
  String FileList = "";
  int logcount=0;
  
  while (dir.next()) {
    File f = dir.openFile("r");
    String FileName = dir.fileName();
    if((!FileName.startsWith("/esprfidtool.json"))&&(!FileName.startsWith("/config.json"))) {
      logcount++;
    }
    f.close();
  }
  
  const size_t bufferSize = JSON_ARRAY_SIZE(5) + JSON_OBJECT_SIZE(1);
  DynamicJsonDocument apilog(bufferSize);

  apilog["Device"] = "ESP-RFID-Tool";
  apilog["Firmware"] = version;
  apilog["API"] = APIversion;
  apilog["Log Count"] = logcount;

  int currentlog=0;
  Dir dir2ndrun = SPIFFS.openDir("/");
  while (dir2ndrun.next()) {
    File f = dir2ndrun.openFile("r");
    String FileName = dir2ndrun.fileName();
    if ((!FileName.startsWith("/esprfidtool.json"))&&(!FileName.startsWith("/config.json"))) {
      currentlog++;
      FileName.remove(0,1);
      JsonObject apilistlogs = apilog.createNestedObject(String(currentlog));
      apilistlogs["File Name"]=FileName;
    }
    f.close();
  }

  String API_Response="";
  if (prettify==1) {
    serializeJsonPretty(apilog, API_Response);
  }
  else {
    serializeJson(apilog, API_Response);;
  }
  server.send(200, "application/json", API_Response);
  delay(50);
  apilog.clear();
}

void apilog(String logfile,int prettify) {
  File f = SPIFFS.open(String()+"/"+logfile, "r");
  if (!f) {
    server.send(200, "application/json", "Log file not found");
    delay(50);
  }
  else {
    int apiCAPTUREcount=0;
    while(f.available()) {
      String line = f.readStringUntil('\n');
      if(line.indexOf(",Binary:") > 0) {
        apiCAPTUREcount++;
        int firstIndex = line.indexOf(",Binary:");
        int secondIndex = line.indexOf(",", firstIndex + 1);
        String binaryCaptureLINE=line.substring(firstIndex+8, secondIndex);
      }
    }
    f.close();
    DynamicJsonDocument apilog(2048);

    apilog["Device"] = "ESP-RFID-Tool";
    apilog["Firmware"] = version;
    apilog["API"] = APIversion;
    apilog["Log File"] = logfile;
    apilog["CaptureCount"] = apiCAPTUREcount;

    JsonArray captures = apilog.createNestedArray("Captures");

    int apiCURRENTcapture=0;
    File f = SPIFFS.open(String()+"/"+logfile, "r");
    DynamicJsonDocument apiCURRENTcaptureOBJECT(1024);

    while(f.available()) {
      String line = f.readStringUntil('\n');
      
      int firstIndex = line.indexOf(",Binary:");

      if(firstIndex > -1) {
        apiCURRENTcapture++;
        int secondIndex = line.indexOf(",", firstIndex + 1);
        String binaryCaptureLINE=line.substring(firstIndex+8, secondIndex);
        if ( binaryCaptureLINE.indexOf(" ") > 0 ) {
          binaryCaptureLINE=binaryCaptureLINE.substring(binaryCaptureLINE.indexOf(" ")+1);
        }
        binaryCaptureLINE.replace("\r","");
        apiCURRENTcaptureOBJECT["Bit Count"] = binaryCaptureLINE.length();
        apiCURRENTcaptureOBJECT["Binary"] = binaryCaptureLINE;
        if(line.indexOf(",HEX:") > 0) {
          int hfirstIndex = line.indexOf(",HEX:");
          int hsecondIndex = line.indexOf(",", hfirstIndex + 1);
          String hexCURRENT=line.substring(hfirstIndex+5, hsecondIndex);
          hexCURRENT.replace("\r","");
          apiCURRENTcaptureOBJECT["Hexadecimal"]=hexCURRENT;
        }
        if(line.indexOf(",Keypad Code:") > 0) {
          int kfirstIndex = line.indexOf(",Keypad Code:");
          int ksecondIndex = line.indexOf(",", kfirstIndex + 1);
          String pinCURRENT = line.substring(kfirstIndex + 13, ksecondIndex);
          pinCURRENT.replace("\r","");
          apiCURRENTcaptureOBJECT["Keypad Press"] = pinCURRENT;
        }

        captures.add(apiCURRENTcaptureOBJECT);
      }
    }
    f.close();
    String API_Response="";
    if (prettify==1) {
      serializeJsonPretty(apilog, API_Response);
    }
    else {
      serializeJson(apilog, API_Response);;
    }
    server.send(200, "application/json", API_Response);
    delay(50);
    apilog.clear();
  }
}
