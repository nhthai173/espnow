#include "Arduino.h"

// 2e:3a:e8:3d:f3:8d
uint8_t gw_mac[] = {0x2C, 0x3A, 0xE8, 0x3D, 0xF3, 0x8D};
uint8_t node_mac[] = {0x40, 0x91, 0x51, 0x5A, 0x5E, 0xFA};


#include <espnow-gateway.h>
#include "OneButton.h"
#include "ESP8266WebServer.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

OneButton button(0, true);

ESP8266WebServer sv(80);

void setup() {
    // IO
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    // Start serial
    Serial.begin(115200);
    while (!Serial)
        delay(1000);

    delay(5000);

    Serial.print("ESP Board MAC Address:  ");
    Serial.println(WiFi.macAddress());

    wifi_get_macaddr(STATION_IF, gw_mac);
    Serial.printf("Gateway MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", gw_mac[0], gw_mac[1], gw_mac[2], gw_mac[3], gw_mac[4],
                  gw_mac[5]);

    WiFi.mode(WIFI_AP_STA);
    Serial.print("Connecting to WiFi..");
    WiFi.begin("C21.20", "diamondc2120");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("Connected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("RSSI: ");
    Serial.println(WiFi.RSSI());
    Serial.print("Channel: ");
    Serial.println(WiFi.channel());

    // init espnow
    wifi_set_macaddr(STATION_IF, gw_mac);
    GatewayBegin();

    button.attachLongPressStart([]() {
        Serial.println("Enter pairing mode");
        Gateway.startPairing();
    });

//    Node.onPairingTimeout([](){
//        display.clearDisplay();
//        display.setCursor(0, 0);
//        display.printf("Pairing\ntimeout");
//        display.display();
//    });

    Gateway.onPairingRequest([](uint8_t *mac) {
        digitalWrite(LED_BUILTIN, LOW);
    });

//    addPeer(node_mac);

    sv.on("/", []() {
        sv.send(200, "text/plain", "Hello World");
    });

    sv.on("/command", []() {
        String action = sv.arg("action");
        String params = sv.arg("params");
        Gateway.sendCommand(node_mac, action, params);
        sv.send(200, "text/plain", "OK");
    });

    sv.begin();

} // setup


void loop() {
    Gateway.loop();
    button.tick();
    sv.handleClient();
}