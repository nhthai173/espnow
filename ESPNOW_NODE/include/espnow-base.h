//
// Created by Thái Nguyễn on 25/7/24.
//

#ifndef ESPNOW_BASE_H
#define ESPNOW_BASE_H

#define ESPNOW_DEBUG

#ifdef ESPNOW_DEBUG
#define DEBUG_ESP_NOW(...) Serial.printf(__VA_ARGS__)
#else
#define DEBUG_ESP_NOW(...)
#endif

#include <Arduino.h>
#include <vector>

#ifdef ESP8266

#include <ESP8266WiFi.h>
#include <espnow.h>

#else // ESP32

#include <WiFi.h>
#include <esp_now.h>

#endif



#ifndef ESP_OK
#define ESP_OK 0
#endif



uint8_t ESP_NOW_BROADCAST_ADDRESS[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};


void addPeer(uint8_t *addr, uint8_t channel = 0) {
    if (esp_now_add_peer(addr, ESP_NOW_ROLE_COMBO, channel, NULL, 0) != ESP_OK) {
        DEBUG_ESP_NOW("Error adding peer");
        return;
    }
    DEBUG_ESP_NOW("Added peer %02x:%02x:%02x:%02x:%02x:%02x\n", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
}


bool compareMacAddress(const uint8_t *mac1, const uint8_t *mac2) {
    if (mac1 == nullptr || mac2 == nullptr) {
        return false;
    }
    for (int i = 0; i < 6; i++) {
        if (mac1[i] != mac2[i]) {
            return false;
        }
    }
    return true;
}




class ENData {

private:
    uint16_t _id = 0;
    String _buffer;

public:

    ENData() = default;

    explicit ENData(uint8_t *buffer) {
        _buffer = (char *) buffer;
        int index = _buffer.indexOf("\r\n\r");
        if (index == -1) {
            DEBUG_ESP_NOW("Invalid data\n");
            _buffer = "";
            return;
        }
        _buffer = _buffer.substring(0, index + 2);
        _id = getParam("uid").toInt();
    }

    explicit ENData(const String &data) {
        _buffer = data;
    }

    void addParam(const String &key, const String &value) {
        _buffer += key + ":" + value + "\r\n";
    }

    String getParam(const String &key) const {
        int index = _buffer.indexOf(key);
        if (index == -1) {
            return "";
        }
        int start = index + key.length() + 1;
        int end = _buffer.indexOf("\r\n", start);
        return _buffer.substring(start, end);
    }

    uint16_t id() const {
        return _id;
    }

    void id (uint16_t id) {
        _id = id;
    }

    String MAN() const {
        return getParam("MAN");
    }

    void MAN(const String &value) {
        addParam("MAN", value);
    }

    String did() {
        return getParam("did");
    }

    void send(uint8_t *addr) {
        if (!_buffer.startsWith("uid:")) {
            _buffer = "uid:" + String(_id) + "\r\ndid:" + String(ESP.getChipId()) + "\r\n" + _buffer;
            _buffer += "\r";
        }
        esp_now_send(addr, (uint8_t *) _buffer.c_str(), _buffer.length());
        DEBUG_ESP_NOW("Sent %d bytes to %02x:%02x:%02x:%02x:%02x:%02x\n", _buffer.length(), addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
        DEBUG_ESP_NOW("------ Buffer ------\n");
        DEBUG_ESP_NOW("%s", _buffer.c_str());
        DEBUG_ESP_NOW("---- End Buffer ----\n");
    }

    const char* c_str() {
        return _buffer.c_str();
    }
};




class ENBase {

};



/* Helper */
class ENSeperator {
private:
    std::vector<String> _seperated;
public:

    ENSeperator(const String& str, const String& sep) {
        int lastIndex = 0;
        int index = -1;
        while(true) {
            index = str.indexOf(sep, lastIndex);
            if (index == -1) {
                _seperated.push_back(str.substring(lastIndex));
                break;
            }
            _seperated.push_back(str.substring(lastIndex, index));
            lastIndex = index + sep.length();
        }
    }

    unsigned int count() {
        return _seperated.size();
    }

    void forEach(const std::function<void(String&)> &callback) {
        for (auto &item : _seperated) {
            callback(item);
        }
    }

    String& operator[](int index) {
        return _seperated[index];
    }
};


#endif //ESPNOW_BASE_H
