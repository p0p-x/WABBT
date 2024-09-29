// WebServerModule.h

#ifndef WEBSERVERMODULE_H
#define WEBSERVERMODULE_H

#include <WiFi.h>
#include <ESPAsyncWebServer.h>

class WebServerModule {
private:
  const char* ssid;
  const char* password;
  AsyncWebServer server;

  static void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
  void handleRoot(AsyncWebServerRequest *request);
  void handlePostData(AsyncWebServerRequest *request);

public:
  static int client_count;
  static AsyncWebSocket ws;
  String getContentType(String filename);
  static void sta_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
  static void notifyClients(String rsp);
  static void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
  static void startAP();
  static void stopAP();
  WebServerModule(const char* ssid, const char* password);
  void setup();
  void loop();
};

#endif // WEBSERVERMODULE_H
