server.on("/api/tx/bin", [](){
  String api_binary="";
  int api_txdelayus=txdelayus;
  int api_txdelayms=txdelayms;
  int prettify=0;
  if (server.hasArg("binary")) {
    api_binary=(server.arg("binary"));
  }
  if (server.hasArg("pulsewidth")) {
    api_txdelayus=(server.arg("pulsewidth").toInt());
  }
  if (server.hasArg("interval")) {
    api_txdelayms=(server.arg("interval").toInt());
  }
  if (server.hasArg("prettify")) {
    prettify=1;
  }

  const size_t bufferSize = JSON_ARRAY_SIZE(4) + JSON_OBJECT_SIZE(4);
  DynamicJsonBuffer jsonAPIbuffer(bufferSize);
  JsonObject& apitxbin = jsonAPIbuffer.createObject();

  apitxbin["Device"] = "ESP-RFID-Tool";
  apitxbin["Firmware"] = version;
  apitxbin["API"] = APIversion;

  JsonObject& apitxbinary = apitxbin.createNestedObject("Transmission");
  apitxbinary["Bit Count"]=api_binary.length();
  apitxbinary["Binary"]=api_binary;
  apitxbinary["Wiegand Data Pulse Width"]=String()+api_txdelayus+"us";
  apitxbinary["Wiegand Data Interval"]=String()+api_txdelayms+"ms";
  
  if (api_binary=="") {
    server.send(200, "application/json", F("Binary to tx not specified."));
  }
  else {
    String API_Response="";
    if (prettify==1) {
      apitxbin.prettyPrintTo(API_Response);
    }
    else {
      apitxbin.printTo(API_Response);
    }
    server.send(200, "application/json", API_Response);
    delay(50);
    apiTX(api_binary,api_txdelayus,api_txdelayms);
  }
  jsonAPIbuffer.clear();
});

server.on("/api/help", [](){
  String apihelpHTML=String()+F(
  "<a href=\"/\"><- BACK TO INDEX</a><br><br>"
  "<b>API Version: "
  )+APIversion+F(
  "</b><br><br>"
  "<b><a href=\"/api/info?prettify=1\">/api/info</a></b><br>"
  "<small>Usage: [server]/api/info</small><br>"
  "<br>"
  "<b><a href=\"/api/viewlog?logfile="
  )+logname+F(
  "&prettify=1\">/api/viewlog</a></b><br>"
  "<small>Usage: [server]/api/viewlog?logfile=[log.txt]</small><br>"
  "<br>"
  "<b><a href=\"/api/listlogs?prettify=1\">/api/listlogs</a></b><br>"
  "<small>Usage: [server]/api/listlogs</small><br>"
  "<br>"
  "<b><a href=\"/api/tx/bin?binary=0001&pulsewidth=40&interval=2&prettify=1\">/api/tx/bin</a></b><br>"
  "<small>Usage: [server]/api/tx/bin?binary=[binary]&pulsewidth=[delay_us]&interval=[delay_ms]</small><br>"
  "<br>"
  "<b>Universal Arguments</b><br>"
  "<small>Prettify: [api-url]?[args]<u>&prettify=1</u></small><br>"
  );
  server.send(200, "text/html", apihelpHTML);
});

server.on("/api/info", [](){
  int prettify=0;
  if (server.hasArg("prettify")) {
    prettify=1;
  }
  apiinfo(prettify);
});

server.on("/api/listlogs", [](){
  int prettify=0;
  if (server.hasArg("prettify")) {
    prettify=1;
  }
  apilistlogs(prettify);
});

server.on("/api/viewlog", [](){
  int prettify=0;
  if (server.hasArg("prettify")) {
    prettify=1;
  }
  if (server.hasArg("logfile")) {
    apilog(server.arg("logfile"),prettify);
  }
  else {
    server.send(200, "application/json", F("Usage: [server]/api/viewlog?logfile=[logfile.txt]"));
  }
});
