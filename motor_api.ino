#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <StreamUtils.h>

#define WIFI_SSID ""
#define WIFI_PASSWD ""

ESP8266WebServer server(80);

#define OUTPUT_IN1_PIN 1
#define OUTPUT_IN2_PIN 3

#define DIRECTION_NORMAL 0
#define DIRECTION_REVERSE !DIRECTION_NORMAL

#define MAX_SPEED 1023
#define MAX_SPEED_PERCENTAGE 100.
#define OUT_IN1(dir,speed) ((dir == DIRECTION_NORMAL) ? speed : 0)
#define OUT_IN2(dir,speed) ((dir == DIRECTION_NORMAL) ? 0 : speed)

double current_speed_percentage = 100.;
int current_direction = DIRECTION_NORMAL;

void setup() {
  pinMode(OUTPUT_IN1_PIN, FUNCTION_3);
  pinMode(OUTPUT_IN2_PIN, FUNCTION_3); 
  pinMode(OUTPUT_IN1_PIN, OUTPUT);
  pinMode(OUTPUT_IN2_PIN, OUTPUT);
  // ---
  WiFi.begin(WIFI_SSID, WIFI_PASSWD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }
  // ---
  server.on("/healthz", HTTP_GET, []() {
    server.send(200, "text/plain",  "ok");
  });
  server.on("/api/v1/motors/1", HTTP_GET, []() {
    DynamicJsonDocument doc(512);
    StringPrint s;

    doc["status"] = "ok";
    doc["motor"]["speed"] = current_speed_percentage;
    doc["motor"]["direction"] = current_direction;
    serializeJson(doc, s);
    server.send(200, "application/json", s.str());
  });
  server.on("/api/v1/motors/1", HTTP_POST, []() {
    int status_code;
    DynamicJsonDocument doc(512);
    StringPrint s;

    if (server.hasArg("plain")) {
      DynamicJsonDocument request_body(512);
      deserializeJson(request_body, server.arg("plain"));
      if (request_body.containsKey("speed")) {
        current_speed_percentage = request_body["speed"];
      }
      if (request_body.containsKey("direction")) {
        current_direction = request_body["direction"];
      }
      analogWrite(OUTPUT_IN1_PIN, OUT_IN1(current_direction, MAX_SPEED * current_speed_percentage / 100));
      analogWrite(OUTPUT_IN2_PIN, OUT_IN2(current_direction, MAX_SPEED * current_speed_percentage / 100));
    } else {
      doc["errors"][0] = "body is empty.";
    }

    if (doc.containsKey("errors")) {
      status_code = 503;
      doc["status"] = "ng";
    } else {
      status_code = 200;
      doc["status"] = "ok";
    }
    serializeJson(doc, s);
    server.send(status_code, "application/json", s.str());
  });
  server.on("/api/v1/motors/1/stop", HTTP_POST, []() {
    DynamicJsonDocument doc(512);
    StringPrint s;

    analogWrite(OUTPUT_IN1_PIN, 0);
    analogWrite(OUTPUT_IN2_PIN, 0);

    doc["status"] = "ok";
    serializeJson(doc, s);
    server.send(200, "application/json", s.str());
  });
  server.onNotFound([] () {
    DynamicJsonDocument doc(512);
    StringPrint s;

    doc["status"] = "ng";
    doc["errors"][0] = "not found.";
    serializeJson(doc, s);
    server.send(404, "application/json", s.str());
  });
  server.begin();
}

void loop() {
  server.handleClient();
}
