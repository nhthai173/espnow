#include "Arduino.h"
#include <Ticker.h>

// 2e:3a:e8:3d:f3:8d
// uint8_t gw_mac[] = {0x2C, 0x3A, 0xE8, 0x3D, 0xF3, 0x8D};
// uint8_t node_mac[] = {0x40, 0x91, 0x51, 0x5A, 0x5E, 0xFA};

//#define USE_OLED

#include <espnow-node.h>
#include "OneButton.h"

Ticker ticker;
ENNodeInfo NodeInfo;

OneButton button(0, true);
String buttonValue = "idle";

WiFiEventHandler stationConnectedHandler;

#ifdef USE_OLED

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void displayForSeconds(uint16_t seconds) {
    display.display();
    ticker.detach();
    ticker.once(seconds, []() {
        schedule_function([]() {
            display.clearDisplay();
            display.setCursor(0, 0);
            display.println("Idle");
            display.display();
        });
    });
}

void displayGatewayInfo() {
    if (!Node.isPaired())
        return;
    display.clearDisplay();
    display.setCursor(0, 0);
    display.printf("GW ID: %s\nChannel: %d", Node.gatewayId().c_str(), Node.gatewayChannel());
    display.display();
}

#endif // USE_OLED

void setup() {
    // IO
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    // Start serial
    Serial.begin(115200);
    while (!Serial)
        delay(1000);

    delay(5000);

#ifdef USE_OLED

    // Start display
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;);
    }

    // Initialize display
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    // display.setRotation(0);

    display.setCursor(0, 0);
    display.println("Ready");
    display.display();

#endif // USE_OLED

    // init espnow
    Serial.print("ESP Board MAC Address:  ");
    Serial.println(WiFi.macAddress());

//    WiFi.mode(WIFI_STA);
//    WiFi.persistent(false);
//    WiFi.begin("C21.20", "diamondc2120");

//    stationConnectedHandler = WiFi.onStationModeConnected([](const WiFiEventStationModeConnected& event) {
//        Serial.printf("Connected to %s\n", event.ssid.c_str());
//    });

    NodeBegin();

    WiFi.printDiag(Serial);

    button.attachLongPressStart([]() {
        Serial.println("Enter pairing mode");
        Node.startPairing();
#ifdef USE_OLED
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Pairing");
        display.display();
#endif // USE_OLED
        ticker.detach();
        ticker.attach(1, []() {
            schedule_function([](){
                if (Node.isPaired())
                    return;
#ifdef USE_OLED
                display.clearDisplay();
                display.setCursor(0, 0);
                display.printf("Pairing: %ds", Node.pairingRemaining());
                display.display();
#endif // USE_OLED
            });
        });
    });

    button.attachClick([]() {
       buttonValue = "click";
       Node.sendSyncProp("button", buttonValue);
    });
    button.attachDoubleClick([]() {
        buttonValue = "dbclick";
        Node.sendSyncProp("button", buttonValue);
    });
    button.attachIdle([]() {
        buttonValue = "idle";
        Node.sendSyncProp("button", buttonValue);
    });

    NodeInfo.name = "Node1";
    NodeInfo.type = "ESP8266";
    NodeInfo.model = "security-board";
    NodeInfo.fw_ver = "1";
    NodeInfo.support = "get_props,get_prop,set_prop";
    NodeInfo.addProp(
            "led",
            "false",
            []() {
                return String(digitalRead(LED_BUILTIN) == LOW ? "true" : "false");
            },
            [](const String& value) {
                if (value == "true")
                    digitalWrite(LED_BUILTIN, LOW);
                else if (value == "false")
                    digitalWrite(LED_BUILTIN, HIGH);
                else if (value == "toggle") {
                    bool newState = !digitalRead(LED_BUILTIN);
                    digitalWrite(LED_BUILTIN, newState);
                    schedule_function([newState](){
                        Node.sendSyncProp("led", newState == LOW ? "true" : "false");
                    });
                } else
                    return false;
                return true;
            });
    NodeInfo.addProp(
            "button",
            buttonValue,
            []() {
                return buttonValue;
            });

    Node.nodeInfo(&NodeInfo);

    Node.onPairingTimeout([]() {
        Serial.println("Pairing timeout");
#ifdef USE_OLED
        display.clearDisplay();
        display.setCursor(0, 0);
        display.printf("Pairing -> timeout");
        displayForSeconds(5);
#endif // USE_OLED
    });

    Node.onPairingSuccess([]() {
        Serial.println("Pairing success");
#ifdef USE_OLED
        display.clearDisplay();
        display.setCursor(0, 0);
        display.printf("Pairing -> success\nGW ID: %s\nChannel: %d", Node.gatewayId().c_str(), Node.gatewayChannel());
        displayForSeconds(10);
#endif // USE_OLED
    });

    Node.onCommand([](const String& cmd, const String& value) {
        Serial.printf("Command: %s, Value: %s\n", cmd.c_str(), value.c_str());
#ifdef USE_OLED
        display.clearDisplay();
        display.setCursor(0, 0);
        display.printf("%s: %s", cmd.c_str(), value.c_str());
        displayForSeconds(5);
#endif // USE_OLED
    });

    // esp_now_register_send_cb([](uint8_t *mac, uint8_t status) {
    //     Serial.printf("[%d] Send to %02x:%02x:%02x:%02x:%02x:%02x\n", status, mac[0], mac[1], mac[2], mac[3], mac[4],
    //                   mac[5]);
    // });

#ifdef USE_OLED
    displayGatewayInfo();
    displayForSeconds(10);
#endif // USE_OLED
    Serial.printf("GW ID: %s - Channel: %d\n", Node.gatewayId().c_str(), Node.gatewayChannel());

    if (Node.isPaired()) {
        Node.sendSyncProps();
    }

} // setup


void loop() {
    button.tick();

    // enter paring mode from serial
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        if (cmd == "pair") {
            Serial.println("Enter pairing mode");
            Node.startPairing();
        } else if (cmd == "unpair") {
            Serial.println("Unpairing");
//            Node.unpair();
        }
    }
}