#ifdef ENABLE_WEB_SERVER
#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <M5CardputerOS_core.h>

void startWebServer();
void stopWebServer();
void handleWebServerClient();

#endif
#endif

