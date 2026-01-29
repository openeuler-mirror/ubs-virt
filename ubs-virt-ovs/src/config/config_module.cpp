//
// Created by l00668584 on 2026/1/9.
//

#include <regex>

#include "config_module.h"
#include "config_manager.h"
#include "logger.h"

namespace virt::ovs::config {

const std::string CONFIG_DEFAULT_DIR = "/etc/ubs-virt-ovs";

uint32_t ConfModule::Init()
{
    auto& confMgrRef = ConfigManager::GetInstance();
    uint32_t ret = confMgrRef.Init(CONFIG_DEFAULT_DIR);
    if (ret != 0) {
        return ret;
    }
    return 0;
}


template <typename T>
uint32_t GetNumConf(const std::string& section, const std::string& configKey, T& configVal)
{
    auto [trimSection, trimConfigKey, configValString] = TrimConf(section, configKey, "");
    uint32_t ret = ConfigManager::GetInstance().GetConf(trimSection, trimConfigKey, configValString);
    if (ret != 0) {
        LOG_WARN << "Unable to find section: " << trimSection << ", configKey: " << trimConfigKey << ",ret is : "<< ret;
        return ret;
    }

    if (!IsValidNumber(configValString, !std::is_unsigned_v<T>)) {
        LOG_WARN << "Config is invalid number: " << trimSection << ", configKey: " << trimConfigKey;
        return 1;
    }

    try {
        if constexpr (std::is_same<T, float>::value) {
            configVal = std::stof(configValString);
        } else if constexpr (std::is_same<T, uint32_t>::value) {
            configVal = static_cast<uint32_t>(std::stoul(configValString));
            if (std::to_string(configVal) != configValString) {
                throw std::invalid_argument("Invalid argument: Configuring non-integer types.");
            }
        } else if constexpr (std::is_same<T, uint64_t>::value) {
            configVal = static_cast<uint64_t>(std::stoull(configValString));
            if (std::to_string(configVal) != configValString) {
                throw std::invalid_argument("Invalid argument: Configuring non-integer types.");
            }
        }
    } catch (const std::invalid_argument&) {
        LOG_WARN << "Config is invalid argument: " << trimSection << ", configKey: " << trimConfigKey;
        return 2;
    } catch (const std::out_of_range&) {
        LOG_WARN << "Config is out of range: " << trimSection << ", configKey: " << trimConfigKey;
        return 3;
    }

    return 0;
}

uint32_t ConfModule::GetUIntConf(const std::string& section, const std::string& configKey, uint32_t& configVal)
{
    return GetNumConf(section, configKey, configVal);
}

uint32_t ConfModule::GetULongConf(const std::string& section, const std::string& configKey, uint64_t& configVal)
{
    return GetNumConf(section, configKey, configVal);
}

uint32_t ConfModule::GetFloatConf(const std::string& section, const std::string& configKey, float& configVal)
{
    return GetNumConf(section, configKey, configVal);
}

uint32_t ConfModule::GetStringConf(const std::string& section, const std::string& configKey,
                                         std::string& configVal)
{
    auto [trimSection, trimConfigKey, configValString] = TrimConf(section, configKey, "");
    uint32_t ret = ConfigManager::GetInstance().GetConf(trimSection, trimConfigKey, configVal);
    if (ret != 0) {
        LOG_WARN << "Unable to find section: " << trimSection << ", configKey: " << trimConfigKey << ",ret is : "<< ret;
        return ret;
    }

    return 0;
}

uint32_t ConfModule::GetBoolConf(const std::string& section, const std::string& configKey, bool& configVal)
{
    auto [trimSection, trimConfigKey, configValString] = TrimConf(section, configKey, "");
    uint32_t ret = ConfigManager::GetInstance().GetConf(trimSection, trimConfigKey, configValString);
    if (ret != 0) {
        LOG_WARN << "Unable to find section: " << trimSection << ", configKey: " << trimConfigKey << ",ret is : "<< ret;
        return ret;
    }

    if (configValString == "true") {
        configVal = true;
    } else if (configValString == "false") {
        configVal = false;
    } else {
        LOG_WARN << "Config is invalid " << trimSection << ", configKey: " << trimConfigKey << ",ret is : "<< ret;
        return 1;
    }

    return 0;
}

bool IsValidNumber(const std::string& str, bool allowFloating)
{
    try {
        if (allowFloating) {
            std::regex re(R"(^-?(0|[1-9]\d*)?(\.\d+)?$)");
            return std::regex_match(str, re);
        }

        std::regex re(R"(^[1-9]\d*$|^0$)");
        return std::regex_match(str, re);
    } catch (const std::regex_error& e) {
        return false;
    }
}
std::tuple<std::string, std::string, std::string> TrimConf(const std::string& section, const std::string& configKey,
                                                           const std::string& configVal)
{
    return {Trim(section), Trim(configKey), Trim(configVal)};
}
}