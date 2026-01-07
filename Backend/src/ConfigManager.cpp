#include "ConfigManager.h"
#include "LogManager.h"
#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>


ConfigManager::ConfigManager() {
    // get current directory
    auto current_path = boost::filesystem::current_path();
    auto config_path = current_path / "config.ini";
    LOG_INFO("Config path: {}", config_path.c_str());
    boost::property_tree::ptree pt;
    boost::property_tree::read_ini(config_path.string(), pt);

    for(const auto& section_pair : pt) {
        const std::string& section_name = section_pair.first;
        auto section_tree = section_pair.second;

        std::map<std::string, std::string> section_config;
        for(const auto& key_value_pair : section_tree) {
            auto key = key_value_pair.first;
            auto value = key_value_pair.second.get_value<std::string>();
            section_config[key] = value;
        }

        SectionInfo sectionInfo;
        sectionInfo._section_datas = section_config;
        _config_map[section_name] = sectionInfo;
    }


    for(const auto& section_entry : _config_map) {
        auto section_name = section_entry.first;
        auto section_config = section_entry.second;
        std::cout << "[" << section_name << "]" << std::endl;
        LOG_INFO("[{}]", section_name);
        for(const auto& key_value_pair : section_config._section_datas) {
            LOG_INFO("{} = {}", key_value_pair.first, key_value_pair.second);
        }
    }


}
