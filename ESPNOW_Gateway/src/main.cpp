#include "Arduino.h"

#include "espnow-gateway.h"
#include "OneButton.h"
#include <ESPAsyncWebServer.h>


//#define USE_AUTOMATIONS

#ifdef USE_AUTOMATIONS
#include "Automations.h"
#endif

//#include "Automations.h"

// #include "DevicePage.h"

OneButton button(0, true);

AsyncWebServer sv(80);
AsyncWebSocket ws("/ws");

Ticker ticker;

String reqBodyBuffer;


void setup() {
    // IO
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    // Start serial
    Serial.begin(115200);
    while (!Serial)
        delay(1000);

//    delay(5000);

    Serial.print("ESP Board MAC Address:  ");
    Serial.println(WiFi.macAddress());

#ifdef USE_AUTOMATIONS
    Serial.println("======= Load automations =======");
    Automations.load();
#endif

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

    Gateway.onPairingSuccess([](){
        Serial.println("Pairing success");
        ws.textAll("PAIRING_SUCCESS");
    });

    Gateway.onPairingTimeout([](){
        Serial.println("Pairing timeout");
        ws.textAll("PAIRING_TIMEOUT");
    });

    Gateway.onPairingFail([](){
        Serial.println("Pairing fail");
        ws.textAll("PAIRING_FAILURE");
    });

    Gateway.onPairingStart([](){
        Serial.println("Pairing start");
        ws.textAll("PAIRING_START");
        ticker.attach_ms(1000, [](){
            int8_t remaining = Gateway.getPairingRemaining();
            if (remaining <= 0) {
                ticker.detach();
                return;
            }
            Serial.printf("> Pairing remaining: %d\n", remaining);
            ws.textAll("PAIRING_REMAINING_" + String(remaining) + "_" + String(Gateway.getPairingTimeout()));
        });
    });

    Gateway.onPairingEnd([](){
        Serial.println("Pairing end");
        ws.textAll("PAIRING_END");
        ticker.detach();
    });

    Gateway.onDeviceStatusChange([](const String& id, bool online){
        String str = online ? "ONLINE" : "OFFLINE";
        ws.textAll(str + "_" + id);
    });

    Gateway.onDevicePropertyChange([](ENData *data) {
        String id = data->did();
        ENSeparator sep(data->getParam("props_name"), ",");
        sep.forEach([id, data](const String &prop){
            String value = data->getParam(prop);
            ws.textAll("PROPCHANGED_" + id + "_" + prop + "_" + value);

#ifdef USE_AUTOMATIONS
            // Get automations match with this device and property
            uint8_t matchesCnt = Automations.matches(id, prop, value);
            for (uint8_t i = 0; i < matchesCnt; i++) {
                automation_t at = Automations.getMatch(i);
                Serial.printf("==> Executing AT[%d]: %s\n", at.id, at.name.c_str());

                bool execute = false; // can execute this automation
                if (at.conditions.size() > 1) {
                    // Have multiple conditions
                    uint8_t matchCnt = 0;
                    for(auto &cond : at.conditions) {
                        // Currently only execute CONDITION_TYPE_VALUE
                        if (cond.type != CONDITION_TYPE_VALUE) continue;
                        String did = cond.first;
                        String prop = cond.second;
                        String value = cond.third;
                        if (did == id) {
                            // condition from this device
                            if (data->getParam(prop) == value) {
                                matchCnt++;
                                if (at.match == CONDITION_MATCH_ANY) {
                                    execute = true;
                                    break;
                                }
                            }
                        } else {
                            // condition from other device
                            ENNodeDevice device(did);
                            if (!device.isValid()) continue;
                            if (device.getProp(prop) == value) {
                                matchCnt++;
                                if (at.match == CONDITION_MATCH_ANY) {
                                    execute = true;
                                    break;
                                }
                            }
                        }
                    }
                    execute = execute || matchCnt == at.conditions.size();
                } else {
                    // Only 1 condition
                    execute = true;
                }
                if (execute) {
                    for(auto &act : at.actions) {
                        // Currently only execute ACTION_TYPE_VALUE
                        if (act.type == ACTION_TYPE_VALUE) {
                            Gateway.sendCommand(act.first, "set_prop", act.second + "," + act.third, nullptr);
                        }
                    }
                }
            } // End automation
#endif

        });
    });

//    addPeer(node_mac);

    sv.on("/", HTTP_GET, [](AsyncWebServerRequest *req) {
        req->send(200, "text/plain", "Hello World");
    });

    sv.on("/device", HTTP_GET, [](AsyncWebServerRequest *req) {
        String id = req->arg("id");
        if (id == "") {
            req->send(400, "text/plain", "Bad Request. Missing id");
            return;
        }
        ENNodeDevice device(id);
        if (!device.isValid()) {
            req->send(404, "text/plain", "Device not found");
            return;
        }
        req->send(200, "application/json", device.toJSON());
    });

    sv.on("/devices", HTTP_GET, [](AsyncWebServerRequest *req){
        AsyncWebServerResponse *res = req->beginResponse(200, "application/json", Gateway.getDevicesJSON());
        res->addHeader("Access-Control-Allow-Origin", "*");
        req->send(res);
    });

    // sv.on("/devices", HTTP_GET, [](AsyncWebServerRequest *req){
    //     req->send(LittleFS, "/devices.html", "text/html", false);
    // });

    sv.on("/pair", [](AsyncWebServerRequest *req) {
        AsyncWebServerResponse *res;
        if (req->hasArg("stop")) {
            Gateway.endPairing();
            res = req->beginResponse(200, "application/json", R"({"success": true})");
        } else if (req->hasArg("start")) {
            Gateway.startPairing();
            res = req->beginResponse(200, "application/json", R"({"success": true, "remaining": )" + String(Gateway.getPairingRemaining()) + R"(, "timeout": )" + String(Gateway.getPairingTimeout()) + "}");
        } else {
            res = req->beginResponse(200, "application/json", R"({"remaining": )" + String(Gateway.getPairingRemaining()) + R"(, "timeout": )" + String(Gateway.getPairingTimeout()) + "}");
        }
        res->addHeader("Access-Control-Allow-Origin", "*");
        req->send(res);
    });

    sv.on("/unpair", [](AsyncWebServerRequest *req) {
        AsyncWebServerResponse *res;
        if (req->hasArg("id")) {
            Gateway.unpair(req->arg("id"));
            res = req->beginResponse(200, "application/json", R"({"success": true})");
        } else {
            res = req->beginResponse(400, "application/json", R"({"success": false, "message": "missing id"})");
        }
        res->addHeader("Access-Control-Allow-Origin", "*");
        req->send(res);
    });

    sv.on("/command", [](AsyncWebServerRequest *req) {
        String id = req->arg("id");
        String action = req->arg("action");
        String params = req->arg("params");
        Gateway.sendCommand(id, action, params, [req](uint16_t id, bool success, ENData *response) {
            String msg = response->getParam("message");
            AsyncWebServerResponse *res = req->beginResponse(200, "application/json",
                    R"({"success": )" + String(success ? "true" : "false") + R"(, "message": ")" + msg + "\"}");
            res->addHeader("Access-Control-Allow-Origin", "*");
            req->send(res);
        });
    });

    sv.on("/restart", [](AsyncWebServerRequest *req) {
        if (req->hasArg("id")) {
            Gateway.restartDevice(req->arg("id"), [req](uint16_t id, bool success, ENData *response) {
                String msg = response->getParam("message");
                AsyncWebServerResponse *res = req->beginResponse(200, "application/json",
                        R"({"success": )" + String(success ? "true" : "false") + R"(, "message": ")" + msg + "\"}");
                res->addHeader("Access-Control-Allow-Origin", "*");
                req->send(res);
            });
            return;
        }
        AsyncWebServerResponse *res = req->beginResponse(200, "application/json", R"({"success": true, "message": "restarting gateway"})");
        res->addHeader("Access-Control-Allow-Origin", "*");
        req->send(res);
        delay(1000);
        ESP.restart();
    });

    sv.on("/sync", [](AsyncWebServerRequest *req) {
        if (req->hasArg("id")) {
            Gateway.syncDevice(req->arg("id"), [req](uint16_t id, bool success, ENData *response) {
                String msg = response->getParam("message");
                AsyncWebServerResponse *res = req->beginResponse(200, "application/json",
                        R"({"success": )" + String(success ? "true" : "false") + R"(, "message": ")" + msg + "\"}");
                res->addHeader("Access-Control-Allow-Origin", "*");
                req->send(res);
            });
            return;
        }
        Gateway.syncAll();
        AsyncWebServerResponse *res = req->beginResponse(200, "application/json", R"({"success": true})");
        res->addHeader("Access-Control-Allow-Origin", "*");
        req->send(res);
    });

#ifdef USE_AUTOMATIONS
    sv.on("/auto/enable", [](AsyncWebServerRequest *req){
        AsyncWebServerResponse *res;
        uint16_t id = req->arg("id").toInt();
        bool disable = false;
        if (req->hasArg("disable")) {
            disable = true;
        }
        if (id == 0) {
            res = req->beginResponse(400, "application/json", R"({"success":false,"message":"missing id"})");
            return;
        }
        if (Automations.enable(id, !disable)) {
            res = req->beginResponse(200, "application/json", R"({"success":true})");
        } else {
            res = req->beginResponse(400, "application/json", R"({"success":false,"message":"automation not found"})");
        }
        res->addHeader("Access-Control-Allow-Origin", "*");
        req->send(res);
    });

    sv.on("/auto/remove", [](AsyncWebServerRequest *req){
        AsyncWebServerResponse *res;
        uint16_t id = req->arg("id").toInt();
        if (id == 0) {
            res = req->beginResponse(400, "application/json", R"({"success":false,"message":"missing id"})");
            return;
        }
        if (Automations.remove(id)) {
            res = req->beginResponse(200, "application/json", R"({"success":true})");
        } else {
            res = req->beginResponse(400, "application/json", R"({"success":false,"message":"automation not found"})");
        }
        res->addHeader("Access-Control-Allow-Origin", "*");
        req->send(res);
    });

    sv.on("/auto/get", [](AsyncWebServerRequest *req){
        AsyncWebServerResponse *res = req->beginResponse(200, "application/json", Automations.toJSON());
        res->addHeader("Access-Control-Allow-Origin", "*");
        req->send(res);
    });

    sv.on("/auto/remove-all", [](AsyncWebServerRequest *req){
        Automations.removeAll();
        req->send(200, "text/plain", "success");
    });
#endif

    sv.onNotFound([](AsyncWebServerRequest *req) {
        req->send(404, "text/plain", "Not Found");
    });

    sv.onRequestBody([](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t index, size_t total) {
#ifdef USE_AUTOMATIONS
        if (req->url() == "/auto/add-update") {
            
            if (index == 0) {
                Serial.printf("====== POST Body: %d bytes ======\n", total);
                reqBodyBuffer = "";
            }
            reqBodyBuffer += (char *)data;
            if (index + len == total) {
                Serial.printf("%s\n", reqBodyBuffer.c_str());
                Serial.printf("==================================\n");
                
                AsyncWebServerResponse *res;
                uint16_t id = 0;
                if (req->hasArg("id")) {
                    id = req->arg("id").toInt();
                }
                id = Automations.addUpdate(reqBodyBuffer, id);
                if (id > 0) {
                    res = req->beginResponse(200, "application/json", R"({"success":true,"id":)" + String(id) + "}");
                } else {
                    res = req->beginResponse(400, "application/json", R"({"success":false,"message":"invalid automation"})");
                }
                res->addHeader("Access-Control-Allow-Origin", "*");
                req->send(res);
            }
        }
#endif
    });

    sv.begin();

    Gateway.syncAll();

    ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
        if (type == WS_EVT_CONNECT) {
            Serial.printf("WS Client connected: %s\n", client->remoteIP().toString().c_str());
        } else if (type == WS_EVT_DISCONNECT) {
            Serial.printf("WS Client disconnected: %s\n", client->remoteIP().toString().c_str());
        } else if (type == WS_EVT_ERROR) {
            Serial.printf("WS Error: %s\n", (char *)arg);
        }
    });

    sv.addHandler(&ws);

} // setup


void loop() {
    button.tick();
    ws.cleanupClients();
    Gateway.loop();
}