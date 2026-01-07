#ifndef CONFIGMANAGER_H_
#define CONFIGMANAGER_H_

#include "const.h"
#include "singleton.h"
struct SectionInfo {
    SectionInfo()  = default;
    ~SectionInfo() = default;

    SectionInfo(const SectionInfo& other) {
        this->_section_datas = other._section_datas;
    }

    SectionInfo operator=(const SectionInfo& other) {
        if (&other == this) {
            return *this;
        } else {
            this->_section_datas = other._section_datas;
            return *this;
        }
    }


    std::string operator[](const std::string& key) {
        auto it = _section_datas.find(key);
        if (it == _section_datas.end()) {
            return "";
        }
        return _section_datas[key];
    }

    std::map<std::string, std::string> _section_datas;
};


class ConfigManager : public SingleTon<ConfigManager> {
    friend class SingleTon<ConfigManager>;

public:
    ~ConfigManager() { _config_map.clear(); }

    ConfigManager& operator=(const ConfigManager& other) {
        if (&other == this) {
            return *this;
        }
        this->_config_map = other._config_map;
        return *this;
    }

    ConfigManager(const ConfigManager& other) {
        this->_config_map = other._config_map;
    }

    SectionInfo operator[](const std::string& key) {
        auto it = _config_map.find(key);
        if (it == _config_map.end()) {
            return SectionInfo();
        }
        return _config_map[key];
    }


private:
    ConfigManager();
    std::map<std::string, SectionInfo> _config_map;
};


#endif   // CONFIGMANAGER_H_
