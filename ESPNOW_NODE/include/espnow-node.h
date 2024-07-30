#ifndef ESPNOW_NODE_H
#define ESPNOW_NODE_H

#include "espnow-base.h"


class ENNodeInfo {

    friend class ENNode;

private:
    typedef std::function<String()> getter_fn_t;
    typedef std::function<bool(const String &)> setter_fn_t;
    struct prop_t {
        String key;
        String value;
        getter_fn_t getter;
        setter_fn_t setter;
    };
    std::vector<prop_t> props_value;

public:
    String id = String(ESP.getChipId());
    String name;
    String type;
    String model;
    String fw_ver;
    String support;
    String props_name;

    void getMetaData(ENData *data) const {
        data->addParam("id", id);
        data->addParam("name", name);
        data->addParam("type", type);
        data->addParam("model", model);
        data->addParam("fw_ver", fw_ver);
        data->addParam("support", support);
        data->addParam("props_name", props_name);
    }

    void addProp(const String &key, const String &value = "", const getter_fn_t &getter = nullptr, const setter_fn_t &setter = nullptr) {
        if (key == "id") {
            id = value;
        } else if (key == "name") {
            name = value;
        } else if (key == "type") {
            type = value;
        } else if (key == "model") {
            model = value;
        } else if (key == "fw_ver") {
            fw_ver = value;
        } else if (key == "support") {
            support = value;
        } else if (key == "props_name") {
            props_name = value;
        } else {
            if (props_name.indexOf(key) != -1) {
                if (value.length() || getter != nullptr || setter != nullptr) {
                    for (auto &prop : props_value) {
                        if (prop.key == key) {
                            prop.value = value;
                            prop.getter = getter;
                            prop.setter = setter;
                            return;
                        }
                    }
                }
            } else {
                if (props_name.length()) {
                    props_name += "," + key;
                } else {
                    props_name = key;
                }
                prop_t prop;
                prop.key = key;
                prop.value = value;
                prop.getter = getter;
                props_value.push_back(prop);
            }
        }
    }

    void getProps(ENData *data) {
        for (auto &prop : props_value) {
            if (prop.getter != nullptr) {
                prop.value = prop.getter();
            }
            data->addParam(prop.key, prop.value);
        }
    }

    String getProp(const String& key) {
        for (auto &prop : props_value) {
            if (prop.key == key) {
                if (prop.getter != nullptr) {
                    prop.value = prop.getter();
                }
                return prop.value;
            }
        }
        return "";
    }

    bool setProp(const String& key, const String& value) {
        for (auto &prop : props_value) {
            if (prop.key == key) {
                if (prop.setter != nullptr) {
                    DEBUG_ESP_NOW("Executing setter for %s\n", key.c_str());
                    bool result = prop.setter(value);
                    if (result) prop.value = value;
                    return result;
                }
                DEBUG_ESP_NOW("Setter for %s not found\n", key.c_str());
            }
        }
        DEBUG_ESP_NOW("Property %s not found\n", key.c_str());
        return false;
    }

    bool hasAction(const String& action) const {
        int index = support.indexOf(action);
        if (index == -1) {
            return false;
        }
        if (support.charAt(index + action.length()) == ',') {
            return true;
        }
        if (index + action.length() == support.length() - 1) {
            return true;
        }
        return false;
    }

};


class ENNode {

private:
    bool paired = false;
    uint8_t gatewayMac[6];

    uint16_t _pairingRequestId = 0;
    uint32_t _pairingRequestTime = 0;

    uint32_t _pairingTimeout = 30000;
    uint32_t _sendTimeout = 1000;

    uint32_t _lastSendTime = 0;

    ENNodeInfo *_nodeInfo;

    struct ENMessage {
        ENData data;
        uint32_t time;
        uint8_t retries = 0;
    };

    std::vector<ENMessage> _sendingMessages;
    std::vector<ENMessage *> _receivedMessages;

    // Callbacks
    std::function<void()> _onPairingSuccess;
    std::function<void()> _onPairingTimeout;
    std::function<void(const String &, const String &)> _onCommand;
    std::function<void(uint16_t, bool, const String &)> _onResponse;

    static uint16_t _generateId() {
        return random(1, 65535);
    }

    void _sendToGateway(ENData *data, bool fallback = true) {
        data->send(const_cast<uint8_t *>(gatewayMac));
        if (fallback) {
            ENMessage message{*data, millis()};
            _sendingMessages.push_back(message);
        }
    }

    void _sendPairingRequest() const {
        ENData data;
        data.id(_pairingRequestId);
        data.MAN("pairing-request");
        data.send(ESP_NOW_BROADCAST_ADDRESS);
        DEBUG_ESP_NOW("Pairing request sent [%d]\n", _pairingRequestId);
    }

    void _sendDeviceInfo(uint16_t id = 0) {
        ENData data;
        if (!id) {
            data.id(_generateId());
            data.MAN("info");
        } else {
            data.id(id);
            data.MAN("response");
        }
        if (_nodeInfo) {
            _nodeInfo->getMetaData(&data);
            _nodeInfo->getProps(&data);
        }
        _sendToGateway(&data, false);
    }

public:

    void startPairing() {
        _pairingRequestId = _generateId();
        _pairingRequestTime = millis();
        // clear current gateway mac
        memset(gatewayMac, 0, 6);
        _sendPairingRequest();
    }

    void endPairing() {
        _pairingRequestId = 0;
        _pairingRequestTime = 0;
    }

    void sendResponse(uint16_t id, bool isSuccess, const String &data) {
        ENData response;
        response.id(id);
        response.MAN("response");
        if (isSuccess) {
            response.addParam("success", "true");
        } else {
            response.addParam("success", "false");
        }
        if (data) {
            response.addParam("data", data);
        }
        _sendToGateway(&response, false);
    }

    void sendSyncProps(uint16_t id = 0) {
        if (!_nodeInfo) {
            return;
        }
        ENData data;
        if (id) {
            data.id(id);
            data.MAN("response");
            data.addParam("success", "true");
        }
        else {
            data.id(_generateId());
            data.MAN("sync");
        }
        data.addParam("props_name", _nodeInfo->props_name);
        _nodeInfo->getProps(&data);
        _sendToGateway(&data);
        DEBUG_ESP_NOW("Sync props sent\n");
    }

    void sendSyncProp(const String &propName, const String &value) {
        ENData data;
        data.id(_generateId());
        data.MAN("sync");
        data.addParam("props_name", propName);
        data.addParam(propName, value);
        _sendToGateway(&data);
    }

    void receiveHandler(uint8_t *mac, uint8_t *data, uint8_t len) {
        DEBUG_ESP_NOW("Received %d bytes from %02x:%02x:%02x:%02x:%02x:%02x\n", len, mac[0], mac[1], mac[2], mac[3],
                      mac[4], mac[5]);

        ENData enData(data);
        DEBUG_ESP_NOW("Data\n%s\n", enData.c_str());

        // In pairing mode
        if (_pairingRequestId && _pairingRequestTime) {
            if (millis() - _pairingRequestTime > _pairingTimeout) {
                DEBUG_ESP_NOW("Pairing timeout\n");
                endPairing();
                if (_onPairingTimeout) {
                    _onPairingTimeout();
                }
            } else {
                DEBUG_ESP_NOW("Received pairing message\n");
                if (enData.id() == _pairingRequestId && enData.getParam("accept") == "true") {
                    DEBUG_ESP_NOW("Pairing request accepted\n");
                    paired = true;
                    addPeer(mac);
                    gatewayMac[0] = mac[0];
                    gatewayMac[1] = mac[1];
                    gatewayMac[2] = mac[2];
                    gatewayMac[3] = mac[3];
                    gatewayMac[4] = mac[4];
                    gatewayMac[5] = mac[5];
                }
            }
        } else {
            if (compareMacAddress(mac, gatewayMac)) {
                DEBUG_ESP_NOW("Received message from gateway\n");
            } else {
                DEBUG_ESP_NOW("Received message from unknown device\n");
                return;
            }

            // Info request from gateway
            if (enData.MAN() == "info") {
                _sendDeviceInfo(enData.id());
            }

            // Response from gateway or command
            else if (enData.MAN() == "response" || enData.MAN() == "command") {
                ENMessage message = {enData, millis()};
                _receivedMessages.push_back(&message);
            }
        }
    }

    void loop() {
        // In pairing mode - send pairing request every second
        if (_pairingRequestId) {
            if (paired) {
                endPairing();
                DEBUG_ESP_NOW("Gateway mac: %02x:%02x:%02x:%02x:%02x:%02x\n", gatewayMac[0], gatewayMac[1], gatewayMac[2],
                              gatewayMac[3], gatewayMac[4], gatewayMac[5]);
                _sendDeviceInfo();
                // @TODO save the mac of sender (gateway)
                if (_onPairingSuccess) {
                    _onPairingSuccess();
                }
            } else if (millis() - _pairingRequestTime > _pairingTimeout) {
                DEBUG_ESP_NOW("Pairing timeout\n");
                endPairing();
                if (_onPairingTimeout) {
                    _onPairingTimeout();
                }
            } else if (millis() - _lastSendTime > 1000) {
                _lastSendTime = millis();
                _sendPairingRequest();
            }
        }

        if (!_receivedMessages.empty()) {
            for (auto it = _receivedMessages.begin(); it != _receivedMessages.end(); it++) {
                ENMessage *message = *it;

                // Check if sent message is success or not, then remove it from sending list or resend it
                if (message->data.MAN() == "response") {
                    String success = message->data.getParam("success");
                    DEBUG_ESP_NOW("Received response for [%d] -> %s\n", message->data.id(), success.c_str());
                    DEBUG_ESP_NOW("Clearing message\n");
                    if (_onResponse) {
                        _onResponse(message->data.id(), success == "true", message->data.getParam("data"));
                    }
                    for (auto jt = _sendingMessages.begin(); jt != _sendingMessages.end(); jt++) {
                        if (jt->data.id() == message->data.id()) {
                            _sendingMessages.erase(jt);
                            break;
                        }
                    }
                    _receivedMessages.erase(it);
                }

                // Received command
                else if (message->data.MAN() == "command") {
                    DEBUG_ESP_NOW("Received command");
                    String action = message->data.getParam("action");
                    String params = message->data.getParam("params");
                    DEBUG_ESP_NOW("Action: %s\n", action.c_str());
                    DEBUG_ESP_NOW("Params: %s\n", params.c_str());

                    if (_nodeInfo && _nodeInfo->hasAction(action)) {
                        if (action == "get_props") {
                            sendSyncProps(message->data.id());
                        } else if (action == "set_prop") {
                            ENSeperator p(params, ",");
                            DEBUG_ESP_NOW("-> Setting value [%s] for [%s]\n", p[1].c_str(), p[0].c_str());
                            if (_nodeInfo->setProp(p[0], p[1])) {
                                sendResponse(message->data.id(), true, "");
                            } else {
                                sendResponse(message->data.id(), false, "");
                            }
                        } else if (action == "get_prop") {
                            ENSeperator p(params, ",");
                            ENData response;
                            response.id(message->data.id());
                            response.addParam("success", "true");
                            p.forEach([&response, this](const String &prop) {
                                response.addParam(prop, _nodeInfo->getProp(prop));
                            });
                            response.send(gatewayMac);
                        }
                    }

                    if (_onCommand) {
                        _onCommand(action, params);
                    }

                    _receivedMessages.erase(it);
                }
            }
        }

        if (!_sendingMessages.empty()) {
            for (auto it = _sendingMessages.begin(); it != _sendingMessages.end();) {
                if (millis() - it->time > _sendTimeout) {
                    DEBUG_ESP_NOW("Resending message [%d]\n", it->data.id());
                    _sendToGateway(&it->data, false);
                    it->retries++;
                    it->time = millis();
                    if (it->retries > 3) {
                        DEBUG_ESP_NOW("Message [%d] retries exceeded\n", it->data.id());
                        it = _sendingMessages.erase(it);
                    } else {
                        ++it;
                    }
                } else {
                    ++it;
                }
            }
        }

    }

    void setNodeInfo(ENNodeInfo *nodeInfo) {
        _nodeInfo = nodeInfo;
    }

    void setGatewayMac(uint8_t *mac, uint8_t channel = 0) {
        memcpy(gatewayMac, mac, 6);
//        addPeer(gatewayMac, channel);
    }

    uint8_t  *gatewayMacAddress() {
        return gatewayMac;
    }

    void onPairing(std::function<void(uint32_t timeRemaining)> callback) {

    }

    void onPairingSuccess(std::function<void()> callback) {
        _onPairingSuccess = std::move(callback);
    }

    void onPairingFail(std::function<void()> callback) {

    }

    void onPairingTimeout(std::function<void()> callback) {
        _onPairingTimeout = std::move(callback);
    }

    void onCommand(std::function<void(const String &, const String &)> callback) {
        _onCommand = std::move(callback);
    }

    void onResponse(std::function<void(uint16_t, bool, const String &)> callback) {
        _onResponse = std::move(callback);
    }

};

ENNode Node;


void NodeBegin() {
//    WiFi.mode(WIFI_STA);
    if (esp_now_init() != 0) {
        DEBUG_ESP_NOW("Error initializing ESP-NOW\n");
        return;
    }
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
    esp_now_register_recv_cb([](uint8_t *mac, uint8_t *data, uint8_t len) {
        Serial.printf("[Received from] %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        Node.receiveHandler(mac, data, len);
    });
}





/**
 * @frame
 * id: uint16_t
 * data: string
 */

/**
 * @data pairing request
 * MAN: pairing-request
 */

/**
 * @data pairing response
 * MAN: response
 * accept: true|false
 */

/**
 * @data get device info
 * MAN: info
 */

/**
 * @data device info response
 * MAN: response
 * id
 * name
 * type
 * model
 * fw_ver
 * support: actions list
 */

/**
 * @data command
 * MAN: command
 * action
 * params
 */

/**
 * @data response
 * MAN: response
 * success: true|false
 * data
 */

/**
 * @data sync props
 * MAN: sync
 * props_name: name1,name2,name3
 * ...<property_name>:<value>
 */

/**
 * @action common actions
 * - get_props: get properties list of the device
 *
 */

#endif // ESPNOW_NODE_H