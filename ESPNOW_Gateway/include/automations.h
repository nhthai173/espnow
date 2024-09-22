//
// Created by Thái Nguyễn on 2/8/24.
//

#ifndef ESPNOW_GATEWAY_AUTOMATIONS_H
#define ESPNOW_GATEWAY_AUTOMATIONS_H

#include "Arduino.h"
#include "espnow-gateway.h"
#include <vector>

#define AUTOMATIONS_DEBUG

#ifdef AUTOMATIONS_DEBUG
#define AT_Print(...) Serial.printf(__VA_ARGS__)
#else
#define AT_Print(...)
#endif


typedef enum {
    CONDITION_MATCH_ANY = 0,
    CONDITION_MATCH_ALL,
} condition_match_t;

typedef enum {
    CONDITION_TYPE_TIME = 0,
    CONDITION_TYPE_GPIO,
    CONDITION_TYPE_VALUE,
} condition_type_t;

const String CONDITION_TYPE_GPIO_DIGITAL = "d";
const String CONDITION_TYPE_GPIO_ANALOG = "a";

typedef enum {
    ACTION_TYPE_TIME = 0,
    ACTION_TYPE_GPIO,
    ACTION_TYPE_VALUE,
} action_type_t;

typedef struct {
    condition_type_t type;
    String first;
    String second;
    String third;
} condition_t;

typedef struct {
    action_type_t type;
    String first;
    String second;
    String third;
} action_t;

typedef struct {
    uint16_t id;
    bool enabled = true;
    String name;
    condition_match_t match = CONDITION_MATCH_ANY;
    std::vector<condition_t> conditions;
    std::vector<action_t> actions;
} automation_t;

class AutomationClass {

private:
    String dir = "/automations/";
    std::vector<automation_t> automations{};

    std::vector<automation_t> _atMatches{};

    bool _isExist(uint16_t id) {
        File f = ENFS.open(dir + String(id), "r");
        if (f && f.size()) {
            f.close();
            return true;
        }
        return false;
    }

    static automation_t _parseAutomation(File file) {
        automation_t automation{};
        automation.id = 0;
        if (!file || !file.size()) {
            return automation;
        }

        AT_Print("====== Parsing automation [%s] ======\n", file.name());
        AT_Print("%s", file.readString().c_str());
        AT_Print("\n=====================================\n");
        file.seek(0, SeekSet);

        automation.id = String(file.name()).toInt();
        automation.enabled = file.readStringUntil('\n').toInt() == 1;   // first line is enabled
        automation.name = file.readStringUntil('\n');                   // second line is name
        automation.match = (condition_match_t) file.readStringUntil('\n').toInt();  // third line is match type

        AT_Print("+ id: %d\n", automation.id);
        AT_Print("+ enabled: %d\n", automation.enabled);
        AT_Print("+ name: %s\n", automation.name.c_str());
        AT_Print("+ match: %d\n", automation.match);

        // conditions
        while (file.available()) {
            String line = file.readStringUntil('\n');
            if (line == "\r") {
                break;
            }
            condition_t condition{};
            ENSeparator items(line, "|");
            condition.type = (condition_type_t) items[0].toInt();
            condition.first = items[1];
            condition.second = items[2];
            condition.third = items[3];

            AT_Print("- condition [%d], %s-%s == %s\n", condition.type, condition.first.c_str(),
                     condition.second.c_str(), condition.third.c_str());

            automation.conditions.push_back(condition);
        }
        // actions
        while (file.available()) {
            String line = file.readStringUntil('\n');
            if (line == "\r") {
                break;
            }
            action_t action{};
            ENSeparator items(line, "|");
            action.type = (action_type_t) items[0].toInt();
            action.first = items[1];
            action.second = items[2];
            action.third = items[3];

            AT_Print("- action [%d], %s-%s -> %s\n", action.type, action.first.c_str(), action.second.c_str(),
                     action.third.c_str());

            automation.actions.push_back(action);
        }

        AT_Print("=====================================\n");

        file.close();
        return automation;
    }

    void removeFile(uint16_t id) {
        ENFS.remove(dir + String(id));
    }

    bool printToFile(const automation_t &automation) {
        File f = ENFS_W(dir + String(automation.id));
        if (!f) {
            AT_Print("[Error][Automation][add] failed to open file\n");
            return false;
        }
        f.printf("%d\n", automation.enabled);       // first line is enabled
        f.printf("%s\n", automation.name.c_str());  // second line is name
        f.printf("%d\n", automation.match);         // third line is match type
        for (const auto &condition: automation.conditions) {
            f.printf("%d|%s|%s|%s\n", condition.type, condition.first.c_str(), condition.second.c_str(),
                     condition.third.c_str());
        }
        f.printf("\r\n");
        for (const auto &action: automation.actions) {
            f.printf("%d|%s|%s|%s\n", action.type, action.first.c_str(), action.second.c_str(), action.third.c_str());
        }
        f.printf("\r\n");
        f.close();
        return true;
    }

public:

    AutomationClass() {
//        ENFS.begin();
        if (!ENFS.exists(dir)) {
            ENFS.mkdir(dir);
        }
//        load();
    }

    uint16_t generateUid() {
        uint16_t id;
        do {
            id = random(0, 65535);
        } while (_isExist(id));
        return id;
    }

    bool add(const automation_t &automation) {
        if (!automation.id) {
            AT_Print("[Error][Automation][add] missing id\n");
            return false;
        }
        if (_isExist(automation.id)) {
            AT_Print("[Error][Automation][add] automation id %d exists\n", automation.id);
            return false;
        }
        if (automation.conditions.empty()) {
            AT_Print("[Error][Automation][add] condition cannot empty\n");
            return false;
        }
        if (automation.actions.empty()) {
            AT_Print("[Error][Automation][add] action cannot empty\n");
            return false;
        }

        if (printToFile(automation)) {
            automations.push_back(automation);
            return true;
        }

        return false;
    }

    /**
     * @brief Add an automation from raw string. If automation exists, it will be updated.
     * 
     * @param raw String
     * @param id uint16_t optional
     * @return true 
     * @return false 
     */
    bool add(String &raw, uint16_t id = 0) {
        if (!raw.length()) {
            AT_Print("[Error][Automation][addRaw] raw cannot empty\n");
            return false;
        }
        int eof = raw.lastIndexOf("\n\r\n");
        if (eof == -1) {
            AT_Print("[Error][Automation][addRaw] invalid format\n");
            return false;
        }
        raw = raw.substring(0, eof + 3);

        // Write to file
        if (!id) {
            id = generateUid();
        }
        String fileName = dir + String(id);
        File f = ENFS_W(fileName);
        if (!f) {
            AT_Print("[Error][Automation][addRaw] failed to open file\n");
            return false;
        }
        f.print(raw);
        f.close();

        // Parse again to validate
        f = ENFS.open(fileName, "r");
        if (!f) {
            AT_Print("[Error][Automation][addRaw] failed to open file\n");
            return false;
        }
        automation_t a = _parseAutomation(f);
        f.close();
        if (!a.id || !a.name || a.conditions.empty() || a.actions.empty()) {
            AT_Print("[Error][Automation][addRaw] invalid automation\n");
            remove(a.id);
        } else {
            for (auto &at: automations) {
                if (at.id == a.id) {
                    at = a;
                    return true;
                }
            }
            automations.push_back(a);
            return true;
        }
        return false;
    }

    bool remove(uint16_t id) {
        if (!id) {
            AT_Print("[Error][Automation][remove] missing id\n");
            return false;
        }
        removeFile(id);
        for (auto it = automations.begin(); it != automations.end(); ++it) {
            if (it->id == id) {
                automations.erase(it);
                return true;
            }
        }
        return false;
    }

    void removeAll() {
        for (auto &automation: automations) {
            removeFile(automation.id);
        }
        automations.clear();
    }

    bool update(const automation_t &automation) {
        for (auto &at: automations) {
            if (at.id == automation.id) {
                at = automation;
                removeFile(automation.id);
                return printToFile(automation);
            }
        }
        return false;
    }

    bool enable(uint16_t id, bool enable) {
        for (auto &automation: automations) {
            if (automation.id == id) {
                automation.enabled = enable;
                removeFile(automation.id);
                return printToFile(automation);
            }
        }
        return false;
    }

    uint16_t addUpdate(String &raw, uint16_t id = 0) {
        if (id) {
            if (add(raw, id))
                return id;
            return 0;
        }
        return add(raw) ? automations.back().id : 0;
    }

    uint16_t addUpdate(automation_t &automation) {
        if (automation.id) {
            if (update(automation))
                return automation.id;
            return 0;
        }
        automation.id = generateUid();
        return add(automation) ? automations.back().id : 0;
    }

    void load() {
#ifdef ESP8266
        Dir d = ENFS.openDir(dir);
        while (d.next()) {
            if (d.isDirectory() || (d.isFile() && !d.fileSize())) {
                continue;
            }
            AT_Print("Loading automation [%s]\n", d.fileName().c_str());
            File f = d.openFile("r");
#else // ESP32
        File d = ENFS.open(dir);
        if (!d || !d.isDirectory()) {
            AT_Print("[Error][Automation][load] failed to open directory\n");
            return;
        }
        File f = d.openNextFile("r");
        while (f) {
            if (f.isDirectory() || !f.size())
                continue;
            AT_Print("Loading automation [%s]\n", f.name());
#endif
            automation_t a = _parseAutomation(f);
            f.close();
            if (a.id && a.name && !a.conditions.empty() && !a.actions.empty()) {
                automations.push_back(a);
            } else {
                removeFile(a.id);
            }
#ifdef ESP32
            f = d.openNextFile("r");
#endif
        }
    }

    String toJSON() {
        String json = "[";
        for (auto &automation: automations) {
            json += "{";
            json += "\"id\":" + String(automation.id) + ",";
            json += "\"enabled\":" + String(automation.enabled ? "true" : "false") + ",";
            json += "\"name\":\"" + automation.name + "\",";
            json += "\"match\":" + String(automation.match) + ",";
            json += "\"triggers\":[";
            for (auto &condition: automation.conditions) {
                json += "{";
                json += "\"type\":" + String(condition.type) + ",";
                json += "\"first\":\"" + condition.first + "\",";
                json += "\"second\":\"" + condition.second + "\",";
                json += "\"third\":\"" + condition.third + "\"";
                json += "},";
            }
            if (!automation.conditions.empty()) {
                json.remove(json.length() - 1); // remove last comma
            }
            json += "],";
            json += "\"actions\":[";
            for (auto &action: automation.actions) {
                json += "{";
                json += "\"type\":" + String(action.type) + ",";
                json += "\"first\":\"" + action.first + "\",";
                json += "\"second\":\"" + action.second + "\",";
                json += "\"third\":\"" + action.third + "\"";
                json += "},";
            }
            if (!automation.actions.empty()) {
                json.remove(json.length() - 1); // remove last comma
            }
            json += "]";
            json += "},";
        }
        if (!automations.empty()) {
            json.remove(json.length() - 1); // remove last comma
        }
        json += "]";
        return json;
    }

    void forEach(const std::function<void(automation_t &)> &callback) {
        for (auto &automation: automations) {
            callback(automation);
        }
    }

    /**
     * @brief Get all automations that has condition matches the given value (any of conditions matches and automation is enabled).
     * 
     * To get the automation, use getMatch(index). index start from 0.
     * 
     * @param did device ID
     * @param prop property name
     * @param value property value
     * @return uint8_t number of automations that matches
     */
    uint8_t matches(const String &did, const String &prop, const String &value) {
        _atMatches.clear();
        for (auto &at: automations) {
            if (!at.enabled) continue;
            for (auto &con : at.conditions) {
                if (con.type == CONDITION_TYPE_VALUE && con.first == did && con.second == prop && con.third == value) {
                    _atMatches.push_back(at);
                }
            }
        }
        return _atMatches.size();
    }

    automation_t &getMatch(uint8_t index) {
        return _atMatches[index];
    }

};

AutomationClass Automations;



/* Automation format */
// <enabled>\n
// <name>\n
// <match_type>\n
// <type>|<first>|<second>|<third>\n
// ...\n
// \r\n
// <type>|<first>|<second>|<third>\n
// ...\n
// \r\n



#endif //ESPNOW_GATEWAY_AUTOMATIONS_H
