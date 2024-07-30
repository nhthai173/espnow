#ifndef ESPNOW_NODE_H
#define ESPNOW_NODE_H

#include "espnow-base.h"
#include "Schedule.h"

#ifdef ESP8266

#include "LittleFS.h"
#define ENFS LittleFS

#endif


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

    void addProp(const String &key, const String &value = "", const getter_fn_t &getter = nullptr,
                 const setter_fn_t &setter = nullptr) {
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
                    for (auto &prop: props_value) {
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
                prop.setter = setter;
                props_value.push_back(prop);
            }
        }
    }

    void getProps(ENData *data) {
        for (auto &prop: props_value) {
            if (prop.getter != nullptr) {
                prop.value = prop.getter();
            }
            data->addParam(prop.key, prop.value);
        }
    }

    String getProp(const String &key) {
        for (auto &prop: props_value) {
            if (prop.key == key) {
                if (prop.getter != nullptr) {
                    prop.value = prop.getter();
                }
                return prop.value;
            }
        }
        return "";
    }

    bool setProp(const String &key, const String &value) {
        for (auto &prop: props_value) {
            if (prop.key == key) {
                if (prop.setter != nullptr) {
                    DEBUG_ESP_NOW("Executing setter for %s\n", key.c_str());
                    bool result = prop.setter(value);
                    if (result) prop.value = value;
                    return result;
                }
                DEBUG_ESP_NOW("Setter for %s not found\n", key.c_str());
                return false;
            }
        }
        DEBUG_ESP_NOW("Property %s not found\n", key.c_str());
        return false;
    }

    bool hasAction(const String &action) const {
        String fsupport = support + ",";
        if (fsupport.indexOf(action + ",") > -1) {
            return true;
        }
        return false;
    }

};


class ENNode {

private:
    String _path = "/gateway";

    bool paired = false;
    uint8_t gwMac[6] = {0};
    String gwId;
    uint8_t gwChannel = 0;

    String _requestFor;
    uint16_t _requestId = 0;
    uint32_t _requestTime = 0;

    uint32_t _pairingTimeout = 30000;
    uint32_t _sendTimeout = 1000;

    uint32_t _lastSendTime = 0;
    uint8_t _sendMaxRetries = 3;
    uint8_t _sendRetries = 0;

    ENNodeInfo *_nodeInfo = nullptr;

    struct ENMessage {
        ENData data;
        uint32_t time;
        uint8_t retries = 0;
        ENMessage() = default;
        ENMessage(const ENData &data, uint32_t time) : data(data), time(time) {}
    };

    std::vector<ENMessage> _sendingMessages;
    std::vector<ENMessage *> _receivedMessages;

    // Callbacks
    std::function<void()> _onPairingSuccess;
    std::function<void(const String&)> _onPairingFail;
    std::function<void()> _onPairingTimeout;
    std::function<void(const String &, const String &)> _onCommand;
    std::function<void(uint16_t, bool, const String &)> _onResponse;

    /**
     * @brief Generate random id
     * 
     * @return uint16_t 
     */
    static uint16_t _generateId() {
        return random(1, 65535);
    }

    /**
     * @brief Send data to gateway
     * 
     * @param data ENData
     * @param fallback true: resend if timeout
     */
    void _sendToGateway(ENData *data, bool fallback = true) {
        data->send(const_cast<uint8_t *>(gwMac));
        if (fallback) {
            ENMessage message(*data, millis());
            _sendingMessages.push_back(message);
        }
    }

    /**
     * @brief broadcast pairing request
     * 
     */
    void _sendRequestRequest() {
        if (_sendRetries == 0) {
            ++gwChannel;
            _sendRetries = _sendMaxRetries;
            wifi_set_channel(gwChannel);
        }

        ENData data;
        data.id(_requestId);
        data.MAN("pairing-request");
        data.send(ESP_NOW_BROADCAST_ADDRESS);
        DEBUG_ESP_NOW("Pairing request sent: requestId = %d, channel = %d\n", _requestId, WiFi.channel());
        --_sendRetries;
    }

    /**
     * @brief Send device info to gateway
     * 
     * @param id 
     */
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

    void _requestWiFiCredentials() {
        ENData req;
        _requestId = _generateId();
        _requestTime = millis();
        _requestFor = "wifi";
        req.id(_requestId);
        req.MAN("request-wifi");
        _sendToGateway(&req, false);
    }

public:

    void load() {
        File file = ENFS.open(_path, "r");
        if (file && file.size()) {
            // read mac
            String buffer = file.readStringUntil('\n');
            gwMac[0] = strtoul(buffer.substring(0, 2).c_str(), nullptr, 16);
            gwMac[1] = strtoul(buffer.substring(3, 5).c_str(), nullptr, 16);
            gwMac[2] = strtoul(buffer.substring(6, 8).c_str(), nullptr, 16);
            gwMac[3] = strtoul(buffer.substring(9, 11).c_str(), nullptr, 16);
            gwMac[4] = strtoul(buffer.substring(12, 14).c_str(), nullptr, 16);
            gwMac[5] = strtoul(buffer.substring(15, 17).c_str(), nullptr, 16);            
            // read id
            gwId = file.readStringUntil('\n');
            // read channel
            gwChannel = file.readString().toInt();
            wifi_set_channel(gwChannel);
            DEBUG_ESP_NOW("Loaded gateway info:\nMAC: %02x:%02x:%02x:%02x:%02x:%02x\nid: %s\nchannel: %d\n", gwMac[0], gwMac[1],
                          gwMac[2], gwMac[3], gwMac[4], gwMac[5], gwId.c_str(), gwChannel);
            if (gwId.length()) {
                paired = true;
            }
        } else {
            DEBUG_ESP_NOW("Error loading gateway info\n");
        }
    }

    /* ====== Send functions ====== */

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
        } else {
            data.id(_generateId());
            data.MAN("sync");
        }
        data.addParam("props_name", _nodeInfo->props_name);
        _nodeInfo->getProps(&data);
        _sendToGateway(&data, false);
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


    /* ====== Pairing functions ====== */

    void startPairing() {
        _requestId = _generateId();
        _requestTime = millis();
        _requestFor = "pairing";
        
        // clear current gateway info
        paired = false;
        memset(gwMac, 0, 6);
        gwId = "";
        ENFS.open(_path, "w").close();

        // Send request to each channel 3 times. channel 0 - 13
        gwChannel = 0;
        _sendRetries = _sendMaxRetries;
        wifi_set_channel(gwChannel);
        _sendRequestRequest();
    }

    void endRequest() {
        _requestId = 0;
        _requestTime = 0;
        _requestFor = "";
    }

    uint8_t pairingRemaining() {
        uint8_t remaining = 0;
        if (_requestFor == "pairing" && _requestId && _requestTime) {
            remaining = (_pairingTimeout - (millis() - _requestTime)) / 1000;
        }
        return remaining;
    }

    [[nodiscard]] bool isPaired() const {
        return paired;
    }


    /* ====== setter & getter ====== */

    void nodeInfo(ENNodeInfo *nodeInfo) {
        _nodeInfo = nodeInfo;
    }

    ENNodeInfo* nodeInfo() {
        return _nodeInfo;
    }

    void gatewayId(const String &id) {
        gwId = id;
    }

    [[nodiscard]] String gatewayId() const {
        return gwId;
    }

    void gatewayChannel(uint8_t channel) {
        gwChannel = channel;
        wifi_set_channel(gwChannel);
    }

    [[nodiscard]] uint8_t gatewayChannel() const {
        return gwChannel;
    }

    void gatewayMac(uint8_t *mac, int8_t channel = -1) {
        memcpy(gwMac, mac, 6);
        if (channel > -1) {
            gatewayChannel(channel);
        }
//        addPeer(gwMac, channel);
    }

    uint8_t *gatewayMac() {
        return gwMac;
    }

    
    /* ====== Handler ====== */

    void receiveHandler(uint8_t *mac, uint8_t *data, uint8_t len) {
        DEBUG_ESP_NOW("Received %d bytes from %02x:%02x:%02x:%02x:%02x:%02x\n", len, mac[0], mac[1], mac[2], mac[3],
                      mac[4], mac[5]);

        ENData enData(data);
        DEBUG_ESP_NOW("Data\n%s\n", enData.c_str());

        // In request mode
        if (_requestId && _requestTime) {
            if (_requestFor == "pairing") {
                if (millis() - _requestTime > _pairingTimeout) {
                    DEBUG_ESP_NOW("Pairing timeout\n");
                    endRequest();
                    if (_onPairingTimeout) {
                        DEBUG_ESP_NOW("Executing onPairingTimeout callback\n");
                        schedule_function(_onPairingTimeout);
                    }
                } else {
                    DEBUG_ESP_NOW("Received pairing message\n");
                    if (enData.id() == _requestId) {
                        if (enData.getParam("accept") == "true") {
                            DEBUG_ESP_NOW("-> Pairing request accepted\n");
                            paired = true;
                            endRequest();
                            addPeer(mac);
                            gwMac[0] = mac[0];
                            gwMac[1] = mac[1];
                            gwMac[2] = mac[2];
                            gwMac[3] = mac[3];
                            gwMac[4] = mac[4];
                            gwMac[5] = mac[5];
                            gwId = enData.did();
                            gwChannel = enData.getParam("channel").toInt();
                            wifi_set_channel(gwChannel);
                            File file = ENFS.open(_path, "w");
                            if (file) {
                                file.printf("%02x:%02x:%02x:%02x:%02x:%02x\n", gwMac[0], gwMac[1], gwMac[2],
                                            gwMac[3], gwMac[4], gwMac[5]);
                                file.printf("%s\n", gwId.c_str());
                                file.printf("%d\n", gwChannel);
                                file.close();
                                DEBUG_ESP_NOW("=== Gateway info saved ===\n");
    #ifdef ESPNOW_DEBUG
                                file = ENFS.open(_path, "r");
                                if (file) {
                                    DEBUG_ESP_NOW("====== File content ======\n");
                                    DEBUG_ESP_NOW("%s\n", file.readString().c_str());
                                    DEBUG_ESP_NOW("===========================\n");
                                    file.close();
                                }
    #endif
                            } else {
                                DEBUG_ESP_NOW("-> Error saving gateway info\n");
                            }

                            schedule_function([this](){
                                _sendDeviceInfo();
                                if (_onPairingSuccess) {
                                    DEBUG_ESP_NOW("Executing onPairingSuccess callback\n");
                                    _onPairingSuccess();
                                }
                            });
                        } else {
                            endRequest();
                            String reason = enData.getParam("data");
                            DEBUG_ESP_NOW("-> Pairing request rejected\n");
                            DEBUG_ESP_NOW("-> Reason: %s\n", reason.c_str());
                            if (_onPairingFail) {
                                schedule_function([this, reason](){
                                    DEBUG_ESP_NOW("Executing onPairingFail callback\n");
                                    _onPairingFail(reason);
                                });
                            }
                        }
                    }
                }
            }
            
            else if (_requestFor == "wifi") {
                if (enData.id() == _requestId) {
                    DEBUG_ESP_NOW("Received WiFi credential from gateway");
                    String ssid = enData.getParam("ssid");
                    String password = enData.getParam("password");
                    DEBUG_ESP_NOW("SSID: %s\n", ssid.c_str());
                    DEBUG_ESP_NOW("Password: %s\n", password.c_str());
                    WiFi.disconnect();
                    WiFi.mode(WIFI_AP_STA);
                    WiFi.persistent(true);
                    WiFi.setAutoConnect(true);
                    WiFi.setAutoReconnect(true);
                    WiFi.begin(ssid, password);
                    DEBUG_ESP_NOW("Connecting to WiFi\n");
                    endRequest();
                }
            }

        } else {
            // compareMacAddress(mac, gwMac)
            if (enData.did() == gwId) {
                DEBUG_ESP_NOW("=> Received message from gateway\n");
            } else {
                DEBUG_ESP_NOW("=> Received message from unknown device\n");
                return;
            }

            // Info request from gateway
            if (enData.MAN() == "info") {
                _sendDeviceInfo(enData.id());
            }


            // response from gateway
            else if (enData.MAN() == "=== response ===") {
                schedule_function([this, enData]() {
                    String success = enData.getParam("success");
                    String data = enData.getParam("data");
                    DEBUG_ESP_NOW("Received response for [%d] -> [%s] %s\n", enData.id(), success.c_str(), data.c_str());
                    DEBUG_ESP_NOW("-> Clearing message\n");
                    for (auto jt = _sendingMessages.begin(); jt != _sendingMessages.end(); jt++) {
                        if (jt->data.id() == enData.id()) {
                            _sendingMessages.erase(jt);
                            break;
                        }
                    }
                    if (_onResponse) {
                        DEBUG_ESP_NOW("-> Executing onResponse callback\n");
                        _onResponse(enData.id(), success == "true", data);
                    }
                });
            }

            // command from gateway
            else if (enData.MAN() == "command") {
                schedule_function([this, enData]() {
                   DEBUG_ESP_NOW("=== command ===\n");
                    String action = enData.getParam("action");
                    String params = enData.getParam("params");
                    DEBUG_ESP_NOW("Action: %s\n", action.c_str());
                    DEBUG_ESP_NOW("Params: %s\n", params.c_str());

                    if (_nodeInfo && _nodeInfo->hasAction(action)) {
                        if (action == "get_props") {
                            sendSyncProps(enData.id());
                        } else if (action == "set_prop") {
                            ENSeperator p(params, ",");
                            DEBUG_ESP_NOW("-> Setting value [%s] for [%s]\n", p[1].c_str(), p[0].c_str());
                            yield();
                            if (_nodeInfo->setProp(p[0], p[1])) {
                                sendResponse(enData.id(), true, "");
                            } else {
                                sendResponse(enData.id(), false, "");
                            }
                        } else if (action == "get_prop") {
                            if (params.length() == 0) {
                                sendResponse(enData.id(), false, "missing params");
                            } else {
                                ENSeperator p(params, ",");
                                ENData response;
                                response.id(enData.id());
                                response.MAN("response");
                                response.addParam("success", "true");
                                p.forEach([&response, this](const String &prop) {
                                    response.addParam(prop, _nodeInfo->getProp(prop));
                                });
                                _sendToGateway(&response, false);
                            }
                        }
                    } else {
                        DEBUG_ESP_NOW("-> Action not supported\n");
                    }
                    if (_onCommand) {
                        DEBUG_ESP_NOW("-> Executing onCommand callback\n");
                        _onCommand(action, params);
                    }
                });
            }
        }
    }

    void loop() {
        // In pairing mode - send pairing request every 0.5 second
        if (_requestFor == "pairing" && _requestId && !paired) {
            if (millis() - _requestTime > _pairingTimeout) {
                DEBUG_ESP_NOW("Pairing timeout\n");
                endRequest();
                if (_onPairingTimeout) {
                    DEBUG_ESP_NOW("Executing onPairingTimeout callback\n");
                    _onPairingTimeout();
                }
            } else if (millis() - _lastSendTime > 700) {
                _lastSendTime = millis();
                _sendRequestRequest();
            }
        }

        // Process sending messages
        if (!_sendingMessages.empty()) {
            DEBUG_ESP_NOW("=== Processing [%d] sending messages ===\n", _sendingMessages.size());

            for (auto it = _sendingMessages.begin(); it != _sendingMessages.end();) {

                DEBUG_ESP_NOW("[Processing][Sending] id: %d - MAN: %s\n", it->data.id(), it->data.MAN().c_str());

                if (millis() - it->time > _sendTimeout) {
                    DEBUG_ESP_NOW("Resending message [%d]\n", it->data.id());
                    _sendToGateway(&it->data, false);
                    it->retries++;
                    it->time = millis();
                    if (it->retries > _sendMaxRetries) {
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

    
    /* ====== Callback functions ====== */

    void onPairing(std::function<void(uint32_t timeRemaining)> callback) {

    }

    void onPairingSuccess(std::function<void()> callback) {
        _onPairingSuccess = std::move(callback);
    }

    void onPairingFail(std::function<void(const String&)> callback) {
        _onPairingFail = std::move(callback);
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
    ENFS.begin();

    Node.load();

//    if (WiFi.SSID().length()) {
//        WiFi.mode(WIFI_AP_STA);
//        WiFi.begin();
//    } else {
//        WiFi.mode(WIFI_STA);
//    }
   
    if (esp_now_init() != 0) {
        DEBUG_ESP_NOW("Error initializing ESP-NOW\n");
        return;
    }
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
    esp_now_register_recv_cb([](uint8_t *mac, uint8_t *data, uint8_t len) {
        Serial.printf("[Received from] %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4],
                      mac[5]);
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