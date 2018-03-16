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
  "<b>Optional Arguments</b><br>"
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
