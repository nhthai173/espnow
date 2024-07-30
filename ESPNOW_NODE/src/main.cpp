//
// #define RECEIVER
//
//
//Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
//
//#ifdef RECEIVER
//
//uint32_t lastRecvMillis = 0;
//
//#else // sender
//enum send_state_t {
//  IDLE,
//  SENDING,
//  SENT,
//  FAILED,
//  SUCCESS
//};
//
////uint8_t target_mac[] = {0x2C, 0x3A, 0xE8, 0x3D, 0xF3, 0x8D};
// uint8_t target_mac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
//uint16_t id = 0;
//uint16_t lastSendId = 0;
//send_state_t lastSendStatus = IDLE;
//uint32_t lastSendMillis = 0;
//// sender mac: 40:91:51:5A:5E:FA
//
//
//void displaySending(uint32_t num)
//{
//  lastSendStatus = SENDING;
//  display.clearDisplay();
//  display.setCursor(0, 0);
//  display.printf("Sending\n\n     %d", num);
//  display.display();
//}
//
//void displaySendSuccess()
//{
//  lastSendStatus = IDLE;
//  display.fillRect(0, 0, 128, 16, BLACK); // clear first row
//  display.display();
//  display.setCursor(0, 0);
//  display.print("Success");
//  display.display();
//}
//
//void displaySendFailed()
//{
//  lastSendStatus = IDLE;
//  display.fillRect(0, 0, 128, 16, BLACK); // clear first row
//  display.display();
//  display.setCursor(0, 0);
//  display.print("Failed");
//  display.display();
//}
//
//#endif // sender
//
//
//void send_cb(uint8_t *mac_addr, uint8_t status) {
//  Serial.printf("[%d] Send to %02x:%02x:%02x:%02x:%02x:%02x\n", status, mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
//  if (status != 0) {
//#ifndef RECEIVER
//    lastSendStatus = FAILED;
//#endif
//  }
//}
//
//
//void recv_cb(uint8_t *mac_addr, uint8_t *data, uint8_t len) {
//  digitalWrite(LED_BUILTIN, LOW);
//  Serial.printf("Received %d bytes from %02x:%02x:%02x:%02x:%02x:%02x\n", len, mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
//  espnow_data_t espnow_data(data);
//  Serial.printf("ID: %d\n", espnow_data.id);
//  Serial.printf("Data: %s\n", espnow_data.data.c_str());
//#ifdef RECEIVER
//  lastRecvMillis = millis();
////  addPeer(mac_addr, 1);
//  espnow_data_t ack;
//  ack.id = espnow_data.id;
//  ack.data = "ACK";
//  if (esp_now_send(mac_addr, (uint8_t *)&ack, sizeof(ack)) != 0)
//  {
//    Serial.println("-> Error sending data");
//  }
//#else
//  if (lastSendStatus != SENDING) return;
//  if (espnow_data.id == lastSendId) {
//    if (espnow_data.data == "ACK") {
//      lastSendStatus = SUCCESS;
//    } else {
//      lastSendStatus = FAILED;
//    }
//  }
//#endif
//}
//
//
//void setup()
//{
//  // button
//  pinMode(12, INPUT_PULLUP);
//  pinMode(LED_BUILTIN, OUTPUT);
//  digitalWrite(LED_BUILTIN, HIGH);
//
//  // Start serial
//  Serial.begin(115200);
//  while (!Serial)
//    delay(1000);
//
//  delay(5000);
//
//#ifndef RECEIVER
//  // Start display
//  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
//  {
//    Serial.println(F("SSD1306 allocation failed"));
//    for (;;)
//      ;
//  }
//#endif
//
//  // init espnow
//  Serial.print("ESP Board MAC Address:  ");
//  Serial.println(WiFi.macAddress());
//
//  WiFi.mode(WIFI_STA);
//  int esp_init_code = esp_now_init();
//  Serial.printf("ESP-NOW init code: %d\n", esp_init_code);
//  if (esp_init_code != 0)
//  {
//    Serial.println("Error initializing ESP-NOW");
//    return;
//  }
//
//  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
//  esp_now_register_send_cb(send_cb);
//  esp_now_register_recv_cb(recv_cb);
//
//#ifndef RECEIVER
//  // Add peer
//  if (esp_now_add_peer(target_mac, ESP_NOW_ROLE_COMBO, 1, NULL, 0) != 0)
//  {
//    Serial.println("Error adding peer");
//    return;
//  }
//
//  // Initialize display
//  display.clearDisplay();
//  display.setTextSize(2);
//  display.setTextColor(WHITE);
//  // display.setRotation(0);
//
//  display.setCursor(0, 0);
//  display.println("Ready");
//  display.display();
//#endif
//
//} // setup
//
//void loop()
//{
//#ifdef RECEIVER
//
//if (lastRecvMillis > 0 && millis() - lastRecvMillis > 3000)
//{
//  lastRecvMillis = 0;
//  digitalWrite(LED_BUILTIN, HIGH);
//}
//
//#else // sender
//  if (digitalRead(12) == LOW)
//  {
//    delay(250);
//     uint32_t num = millis() / 2000;
//     espnow_data_t data;
//     data.id = id++;
//     lastSendId = data.id;
//     data.data = String(num);
//     displaySending(num);
//     lastSendMillis = millis();
//     if (esp_now_send(target_mac, (uint8_t *)&data, sizeof(data)) != 0)
//     {
//       Serial.println("Error sending data");
//       displaySendFailed();
//     }
//  }
//   if (lastSendStatus == SUCCESS)
//   {
//     displaySendSuccess();
//   }
//   else if (lastSendStatus == FAILED)
//   {
//     displaySendFailed();
//   }
//   else if (lastSendStatus == SENDING && lastSendMillis > 0 && millis() - lastSendMillis > 5000)
//   {
//     lastSendMillis = 0;
//     displaySendFailed();
//   }
//#endif
//}



#include "Arduino.h"
#include <Ticker.h>

// 2e:3a:e8:3d:f3:8d
// uint8_t gw_mac[] = {0x2C, 0x3A, 0xE8, 0x3D, 0xF3, 0x8D};
// uint8_t node_mac[] = {0x40, 0x91, 0x51, 0x5A, 0x5E, 0xFA};

#include <espnow-node.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "OneButton.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Ticker ticker;
ENNodeInfo NodeInfo;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
OneButton button(12, true);

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

void setup() {
    // IO
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    // Start serial
    Serial.begin(115200);
    while (!Serial)
        delay(1000);

    delay(5000);

    // Start display
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;);
    }

    // init espnow
    Serial.print("ESP Board MAC Address:  ");
    Serial.println(WiFi.macAddress());

    WiFi.mode(WIFI_STA);
    // wifi_set_channel(11);
    NodeBegin();

    WiFi.printDiag(Serial);

    // Initialize display
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    // display.setRotation(0);

    display.setCursor(0, 0);
    display.println("Ready");
    display.display();

    button.attachLongPressStart([]() {
        Serial.println("Enter pairing mode");
        Node.startPairing();
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Pairing");
        display.display();
        ticker.detach();
        ticker.attach(1, []() {
            schedule_function([](){
                if (Node.isPaired())
                    return;
                display.clearDisplay();
                display.setCursor(0, 0);
                display.printf("Pairing: %ds", Node.pairingRemaining());
                display.display();
            });
        });
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
                else
                    return false;
                return true;
            });

    Node.nodeInfo(&NodeInfo);

//    Node.setGatewayMac(gw_mac);

    Node.onPairingTimeout([]() {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.printf("Pairing -> timeout");
        displayForSeconds(5);
    });

    Node.onPairingSuccess([]() {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.printf("Pairing -> success\nGW ID: %s\nChannel: %d", Node.gatewayId().c_str(), Node.gatewayChannel());
        displayForSeconds(10);
    });

    Node.onCommand([](const String& cmd, const String& value) {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.printf("%s: %s", cmd.c_str(), value.c_str());
        displayForSeconds(5);
    });

    // esp_now_register_send_cb([](uint8_t *mac, uint8_t status) {
    //     Serial.printf("[%d] Send to %02x:%02x:%02x:%02x:%02x:%02x\n", status, mac[0], mac[1], mac[2], mac[3], mac[4],
    //                   mac[5]);
    // });

    displayGatewayInfo();
    displayForSeconds(10);

} // setup


void loop() {
    Node.loop();
    button.tick();

//    if (millis() - lastMillis > 20000) {
//        lastMillis = millis();
//        display.clearDisplay();
//        display.setCursor(0, 0);
//        display.printf("Sending sync\n%lu", millis());
//        display.display();
//        Node.sendSyncProps();
//    }
}