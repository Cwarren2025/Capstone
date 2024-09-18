#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <vector>

extern "C" {
  #include "user_interface.h"
}

const char* ap_ssid = "ConfigAP";
const char* ap_password = "password123";

std::vector<String> beacon_ssids = {"BeaconA", "Beacon2", "Beacon3", "Beacon4", "Beacon5","Beacon1", "Beacon2", "Beacon3", "Beacon4", "Beacon5"};

ESP8266WebServer server(80);
DNSServer dnsServer;

uint8_t packet[128] = { 0x80, 0x00, 0x00, 0x00, 
                        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
                        0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
                        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 
                        0xc0, 0x6c, 
                        0x83, 0x51, 0xf7, 0x8f, 0x0f, 0x00, 0x00, 0x00, 
                        0x64, 0x00, 
                        0x01, 0x04, 
                        0x00, 0x06, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72,
                        0x01, 0x08, 0x82, 0x84,
                        0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c, 0x03, 0x01, 
                        0x04};

unsigned long previousMillis = 0;
const long interval = 10; // Interval at which to send beacons (milliseconds)

void setup() {
  Serial.begin(115200);
  
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ap_ssid, ap_password);
  
  dnsServer.start(53, "*", WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/update", handleUpdate);
  server.begin();

  wifi_promiscuous_enable(1);

  Serial.println("Access Point and Web Server started");
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
  
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    sendBeacons();
  }
}

void sendBeacons() {
  for (const auto& ssid : beacon_ssids) {
    uint8_t channel = random(1, 12);
    wifi_set_channel(channel);

    // Randomize MAC address
    for (int i = 10; i < 16; i++) {
      packet[i] = packet[i + 6] = random(256);
    }

    // Set SSID
    int ssid_length = ssid.length();
    packet[37] = ssid_length;
    for (int i = 0; i < ssid_length; i++) {
      packet[38 + i] = ssid[i];
    }

    // Set channel
    packet[56] = channel;

    // Send packet
    wifi_send_pkt_freedom(packet, 57, 0);
    wifi_send_pkt_freedom(packet, 57, 0);
    wifi_send_pkt_freedom(packet, 57, 0);
  }
}

void handleRoot() {
  String html = "<html><body>";
  html += "<h1>Configure Beacon SSIDs</h1>";
  html += "<form action='/update' method='POST'>";
  for (int i = 0; i < 5; i++) {
    html += "Beacon " + String(i + 1) + ": ";
    html += "<input type='text' name='ssid" + String(i) + "' value='" + beacon_ssids[i] + "'><br>";
  }
  html += "<input type='submit' value='Update'>";
  html += "</form></body></html>";
  server.send(200, "text/html", html);
}

void handleUpdate() {
  for (int i = 0; i < 5; i++) {
    String ssid = server.arg("ssid" + String(i));
    if (ssid.length() > 0) {
      beacon_ssids[i] = ssid;
    }
  }
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "Updated");
}
