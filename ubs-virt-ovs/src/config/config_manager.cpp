/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * ubs-virt-ovs is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */
#include "config_manager.h"

#include <dirent.h>
#include <sys/stat.h>
#include <fstream>
#include <mutex>
#include <regex>
#include <sstream>

#include "config_common_def.h"
#include "logger.h"

namespace virt::ovs::config {


uint32_t ConfigManager::Init(const std::string &confDir, const std::string &filePrefix)
{
    std::vector<std::string> filePaths;
    uint32_t ret = TravelDepthLimitedFiles(filePaths, confDir, 0);
    if (ret != 0) {
        return ret;
    }

    std::unique_lock<std::shared_mutex> guard(rwLock);
    for (const auto &filePath : filePaths) {
        // 文件已加载, 不可重复加载
        if (fileSet.count(filePath)) {
            LOG_ERROR << "Warning: File: " << filePath << "has already been loaded.";
            continue;
        }
        // 文件前缀非空且与文件名不匹配
        if (!filePrefix.empty() && filePath.substr(filePath.rfind('/') + 1).find(filePrefix) != 0) {
            continue;
        }
        ret = ParseFile(filePath);
        // 解析文件失败
        if (ret != 0) {
            LOG_ERROR << "Warning: Unable to parse file: " << filePath << ".";
        } else {
            fileSet.emplace(filePath);
        }
    }
    if (!parseErrors.empty()) {
        LOG_ERROR << "Unable to parse the configuration.";
        for (const auto &pair : parseErrors) {
            LOG_ERROR << "fileName: " << pair.first << ",warnings:\n" << CatString(pair.second, "\n");
        }
    }
    parseErrors.clear();
    return ret;
}


uint32_t TravelDepthLimitedFiles(std::vector<std::string>& filePaths, const std::string& path, int depth)
{
    if (depth > CONFIG_DIR_MAX_DEPTH) {
        return 1;
    }

    DIR* pd = opendir(path.c_str());
    if (pd == nullptr) {
        LOG_WARN << "Unable to open dir " << path;
        return 2;
    }
    const dirent* dir;
    struct stat statBuf {};
    while ((dir = readdir(pd)) != nullptr) {
        std::string dName = dir->d_name;
        if (dName == "." || dName == "..") {
            continue;
        }
        std::string subFile = PathJoin(path, dName);
        if (lstat(subFile.c_str(), &statBuf)) {
            LOG_WARN << "Unable to get file status " << subFile << ".";
            continue;
        }

        if (S_ISDIR(statBuf.st_mode)) {
            uint32_t ret = TravelDepthLimitedFiles(filePaths, subFile, depth + 1);
            if (ret != 0) {
                closedir(pd);
                return ret;
            }
        } else if (S_ISREG(statBuf.st_mode) && IsConfFile(dName)) {
            filePaths.emplace_back(subFile);
        }
    }
    closedir(pd);
    return 0;
}


uint32_t ConfigManager::ParseFile(const std::string& filePath)
{
    char* canonicalPath = new (std::nothrow) char[PATH_MAX];
    if (canonicalPath == nullptr) {
        std::cerr << "Warning: Memory allocation failed for canonicalPath" << std::endl;
        return 1;
    }
    if (realpath(filePath.c_str(), canonicalPath) == nullptr) {
        std::cerr << "Warning: Could not canonicalize file path " << filePath << " ,err: " << std::strerror(errno)
                  << std::endl;
        delete[] canonicalPath;
        return 1;
    }
    delete[] canonicalPath;
    return ReadConfFile(filePath);
}


uint32_t ConfigManager::ReadConfFile(const std::string &filePath)
{
    std::ifstream fileStream(filePath);
    // 文件无法打开
    if (!fileStream.is_open()) {
        std::cerr << "Warning: Can not open file: " << filePath << " to read." << std::endl;
        return 1;
    }
    std::string defaultSection = filePath.substr(filePath.find_last_of("/\\") + 1,
        filePath.size() - filePath.find_last_of("/\\") - 1 - SUFFIX_SIZE);
    std::string tempSection = Trim(defaultSection);

    // 逐行读取
    std::string line; // 存储当前行内容
    size_t lineCount = 1;
    while (std::getline(fileStream, line) && (lineCount <= (CONFIG_MAX_LINES + 1))) {
        ParseLine(filePath, line, lineCount++, tempSection);
    }
    fileStream.close();
    return 0;
}


void ConfigManager::ParseLine(const std::string& filePath, std::string line, const size_t& lineCount,
                                  std::string& tempSection)
{
    if (lineCount > CONFIG_MAX_LINES) {
        parseErrors[filePath].emplace_back("Warning: Maximum line count exceeded.");
        return;
    }

    line = Trim(line);
    size_t equalPos = line.find('=');
    // 处理注释
    if (line.empty() || format.IsComment(std::string(1, line.front()))) {
        return;
    }
    if (format.IsSectionStart(std::string(1, line.front())) && format.IsSectionEnd(std::string(1, line.back()))) {
        ParseSection(filePath, line, lineCount, tempSection);
    } else if (equalPos != std::string::npos && equalPos > 0 && equalPos < line.size() - 1) {
        ParseConf(filePath, line, lineCount, tempSection);
    } else {
        parseErrors[filePath].emplace_back("Warning: Invalid line content. Line: " + std::to_string(lineCount) + ".");
    }
}


void ConfigManager::ParseSection(const std::string& filePath, const std::string& line, const size_t& lineCount,
                                     std::string& tempSection)
{
    std::string section = std::regex_replace(line, SECTION_CHARS, R"($1)");
    // 长度不合法
    if (section.size() < CONFIG_MIN_FIELD_LENGTH || section.size() > CONFIG_SECTION_MAX_FIELD_LENGTH) {
        std::string message = "Warning: The length of section is out of range( " +
                              std::to_string(CONFIG_MIN_FIELD_LENGTH) + " to " +
                              std::to_string(CONFIG_SECTION_MAX_FIELD_LENGTH) + ").";
        parseErrors[filePath].emplace_back(FormatErrorMessage(message, lineCount, section));
        return;
    }
    // 含有非法字符

    if (!CheckNoIllegalChars(section)) {
        std::string message = "Warning: Section has illegal character.";
        parseErrors[filePath].emplace_back(FormatErrorMessage(message, lineCount, section));
        return;
    }
    tempSection = section;
    // 首次遇到该section
    if (configMap.find(tempSection) == configMap.end()) {
        configMap[tempSection];
    }
}

void ConfigManager::ParseConf(const std::string& filePath, const std::string& line, const size_t& lineCount,
                                  std::string& tempSection)
{
    size_t pos = line.find('=');
    std::string key = line.substr(0, pos);
    std::string value = line.substr(pos + 1);
    key = Trim(key);
    value = Trim(value);
    // 长度不合法
    if (key.size() > CONFIG_KEY_MAX_FIELD_LENGTH || key.size() < CONFIG_MIN_FIELD_LENGTH ||
        value.size() > CONFIG_VALUE_MAX_FIELD_LENGTH) {
        std::string message =
            "Warning: The length of key is out of range(key: " + std::to_string(CONFIG_MIN_FIELD_LENGTH) + " to " +
            std::to_string(CONFIG_KEY_MAX_FIELD_LENGTH) + " ,value:" + std::to_string(CONFIG_MIN_FIELD_LENGTH) +
            " to " + std::to_string(CONFIG_VALUE_MAX_FIELD_LENGTH) + ").";
        parseErrors[filePath].emplace_back(FormatErrorMessage(message, lineCount, "", key, value));
        return;
        }
    // 含有非法字符
    if (!CheckNoIllegalChars(key)) {
        std::string message = "Warning: Section's key has illegal character.";
        parseErrors[filePath].emplace_back(FormatErrorMessage(message, lineCount, tempSection, key));
        return;
    }
    if (!CheckNoIllegalChars(value, true)) {
        std::string message = "Warning: The configuration value contains illegal chars.";
        parseErrors[filePath].emplace_back(FormatErrorMessage(message, lineCount, tempSection, key, value));
        return;
    }
    // 重复配置项
    if (configMap[tempSection].find(key) != configMap[tempSection].end()) {
        std::string message = "Warning: Duplicate key in section.";
        parseErrors[filePath].emplace_back(FormatErrorMessage(message, lineCount, tempSection, key));
        return;
    }
    configMap[tempSection][key] = value;
}

uint32_t ConfigManager::GetConf(const std::string& section, const std::string& configKey, std::string& configVal)
{
    uint32_t ret = CheckParamValidation(section, configKey, configVal, false);
    if (ret != 0) {
        return ret;
    }

    // 不存在该section
    if (configMap.find(section) == configMap.end()) {
        LOG_WARN << "Unable to find section: " << section;
        return 1;
    }

    // section中不存在该key
    if (configMap[section].find(configKey) == configMap[section].end()) {
        LOG_WARN << "Unable to find key: " << configKey << " in section: " << section;
        return 2;
    }
    configVal = configMap[section][configKey];
    return 0;

}


std::string FormatErrorMessage(const std::string& message, size_t lineCount, const std::string& section,
                               const std::string& configKey, const std::string& configVal)
{
    std::ostringstream oss;
    oss << message << " Line: " << std::to_string(lineCount) << ".";
    if (!section.empty()) {
        oss << " Section: " << section << ".";
    }
    if (!configKey.empty()) {
        oss << " Key: " << configKey << ".";
    }
    if (!configVal.empty()) {
        oss << " Value: " << configVal << ".";
    }
    return oss.str();
}




std::string CatString(const std::vector<std::string> &infoVec, const std::string &delimiter)
{
    if (infoVec.empty()) {
        return "";
    }
    std::string outputStr;
    auto it = infoVec.begin();
    outputStr += *it;
    it++;
    for (; it != infoVec.end(); it++) {
        outputStr += delimiter + (*it);
    }
    return std::move(outputStr);
}

bool CheckNoIllegalChars(const std::string &str, bool isConfigVal)
{
    const std::regex &legalChars = isConfigVal ? VAL_CHARS : NON_VAL_CHARS;

    if (str.empty()) {
        return true;
    }
    return std::regex_match(str, legalChars);
}

std::string Trim(const std::string &str, const std::locale &loc)
{
    std::string s = str;
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [&loc](char ch) { return !std::isspace(ch, loc); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [&loc](char ch) { return !std::isspace(ch, loc); }).base(), s.end());
    return s;
}
bool IsConfFile(const std::string& filename)
{
    return filename.size() > SUFFIX_SIZE && filename.compare(filename.size() - SUFFIX_SIZE, SUFFIX_SIZE, ".conf") == 0;
}
std::string PathJoin(const std::string& baseDir, const std::string& baseName)
{
    if (baseDir.empty()) {
        return baseName;
    }

    const char delimiter = '/';
    std::string result = baseDir;
    if (result.back() != delimiter) {
        result.append(1, delimiter);
    }
    result += baseName;
    return result;
}
uint32_t CheckParamValidation(const std::string& section, const std::string& configKey, const std::string& configVal,
                                bool checkValue)
{

    // 长度检查
    if (section.size() > CONFIG_SECTION_MAX_FIELD_LENGTH || section.size() < CONFIG_MIN_FIELD_LENGTH) {
        LOG_WARN << "Section length invalid";
        return 4;
    }
    if (configKey.size() > CONFIG_KEY_MAX_FIELD_LENGTH || configKey.size() < CONFIG_MIN_FIELD_LENGTH) {
        LOG_WARN << "Key length invalid";
        return 5;
    }
    if ((configVal.empty() || configVal.size() > CONFIG_VALUE_MAX_FIELD_LENGTH) && checkValue) {
        LOG_WARN << "Value length too long or too short";
        return 6;
    }
    return 0;
}

} // namespace virt::ovs::config