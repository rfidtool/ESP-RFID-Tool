server.on("/api/help", [](){
  String apihelpHTML=F(
  "<a href=\"/\"><- BACK TO INDEX</a><br><br>"
  "<b>/api/info</b><br>"
  "<small>Usage: [server]/api/info</small><br>"
  "<b>/api/viewlog</b><br>"
  "<small>Usage: [server]/api/viewlog?logfile=[log.txt]</small><br>"
  "<b>/api/listlogs</b><br>"
  "<small>Usage: [server]/api/listlogs</small><br>"
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
