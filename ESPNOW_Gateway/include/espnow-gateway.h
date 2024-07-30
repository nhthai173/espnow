//
// Created by Thái Nguyễn on 25/7/24.
//

#ifndef ESPNOW_NODE_ESPNOW_GATEWAY_H
#define ESPNOW_NODE_ESPNOW_GATEWAY_H

#include "espnow-base.h"

class ENGateway {

private:
    bool _isPairing = false;
    uint32_t _pairingTime = 0;
    uint32_t _pairingTimeout = 30000;

    std::function<void(uint8_t* mac)> _onPairingRequest;

public:

    void startPairing() {
        _isPairing = true;
        _pairingTime = millis();
    }

    void endPairing() {
        _isPairing = false;
        _pairingTime = 0;
    }

    void sendCommand(uint8_t* nodeMac, const String& action, const String& params) {
        ENData data;
        data.id(random(1, 255));
        data.MAN("command");
        data.addParam("action", action);
        data.addParam("params", params);
        data.send(nodeMac);
    }

    void receiveHandler(uint8_t* mac, uint8_t* data, uint8_t len) {
        DEBUG_ESP_NOW("Received %d bytes from %02x:%02x:%02x:%02x:%02x:%02x\n", len, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

        ENData enData(data);
        DEBUG_ESP_NOW("Data:\n%s", enData.c_str());

        if (_isPairing && enData.MAN() == "pairing-request" && millis() - _pairingTime < _pairingTimeout) {
            endPairing();
            DEBUG_ESP_NOW("Received a pairing request\n");
            addPeer(mac);
            if (_onPairingRequest) {
                _onPairingRequest(mac);
            }
            ENData response;
            response.id(enData.id());
            response.MAN("response");
            response.addParam("accept", "true");
            response.send(mac);
            DEBUG_ESP_NOW("Sent pair success response\n");
        }
    }

    void loop() {
        if (_isPairing && millis() - _pairingTime > _pairingTimeout) {
            endPairing();
            DEBUG_ESP_NOW("Pairing timeout\n");
        }
    }

    void onPairingRequest(std::function<void(uint8_t* mac)> callback) {
        _onPairingRequest = std::move(callback);
    }

};



ENGateway Gateway;


void GatewayBegin() {
//    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        DEBUG_ESP_NOW("Error initializing ESP-NOW");
        return;
    }
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
    esp_now_register_recv_cb([](uint8_t *mac, uint8_t *data, uint8_t len) {
        Gateway.receiveHandler(mac, data, len);
    });
}





#endif //ESPNOW_NODE_ESPNOW_GATEWAY_H
