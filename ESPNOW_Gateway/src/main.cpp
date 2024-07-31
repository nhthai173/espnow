#include "Arduino.h"

#include "espnow-gateway.h"
#include "OneButton.h"
#include "ESP8266WebServer.h"

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
    WiFi.printDiag(Serial);

    // init espnow
    GatewayBegin();

    button.attachLongPressStart([]() {
        Serial.println("Enter pairing mode");
        Gateway.startPairing();
    });

    Gateway.onPairingRequest([](uint8_t *mac, ENData *request, ENData *response) {
        Serial.printf("Pairing request from: %s\n", request->did().c_str());
        response->addParam("accept", "true");
    });

//    addPeer(node_mac);

    sv.on("/", []() {
        sv.send(200, "text/plain", "Hello World");
    });

    sv.on("/device", []() {
        String id = sv.arg("id");
        if (id == "") {
            sv.send(400, "text/plain", "Bad Request. Missing id");
            return;
        }
        ENNodeDevice device(id);
        if (!device.isValid()) {
            sv.send(404, "text/plain", "Device not found");
            return;
        }
        sv.send(200, "application/json", device.toJSON());
//        sv.send(200, "text/plain", device.toString());
    });

    sv.on("/command", []() {
        String id = sv.arg("id");
        String action = sv.arg("action");
        String params = sv.arg("params");
        Gateway.sendCommand(id, action, params, [](uint16_t id, bool success, ENData *response) {
            String msg = response->getParam("message");
            sv.send(200, "application/json",
                    R"({"success": )" + String(success ? "true" : "false") + R"(, "message": ")" + msg + "\"}");
        });
    });

    sv.begin();

    Gateway.syncAll();

} // setup


void loop() {
    button.tick();
    sv.handleClient();
}