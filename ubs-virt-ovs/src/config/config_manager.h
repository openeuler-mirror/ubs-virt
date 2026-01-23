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
#ifndef UBSVIRTOVS_CONFIG_MANAGER_H
#define UBSVIRTOVS_CONFIG_MANAGER_H
#include <cstdint>
#include <map>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace virt::ovs::config {

class Format {
public:
    const std::string charSectionStart;  // 配置节首字符 [
    const std::string charSectionEnd;  // 配置节尾字符 [
    const std::string charAssign;  // 等号
    const std::string charCommentSemicolon;  // 参数注释（分号）
    const std::string charCommentHash;  // 参数注释（警号）

    explicit Format(std::string section_start = "[", std::string section_end = "]", std::string assign = "=",
                    std::string commentSemicolon = ";", std::string commentHash = "#")
        : charSectionStart(std::move(section_start)),
          charSectionEnd(std::move(section_end)),
          charAssign(std::move(assign)),
          charCommentSemicolon(std::move(commentSemicolon)),
          charCommentHash(std::move(commentHash))
    {
    }

    [[nodiscard]] inline bool IsSectionStart(const std::string& ch) const
    {
        return ch == charSectionStart;
    }

    [[nodiscard]] inline bool IsSectionEnd(const std::string& ch) const
    {
        return ch == charSectionEnd;
    }

    [[nodiscard]] inline bool IsAssign(const std::string& ch) const
    {
        return ch == charAssign;
    }

    [[nodiscard]] inline bool IsComment(const std::string& ch) const
    {
        return ch == charCommentSemicolon || ch == charCommentHash;
    }
};

using SECTION = std::map<std::string, std::string>;
using ConfigMap = std::map<std::string, SECTION>;


class UbseConfigManager {
public:
    static UbseConfigManager& GetInstance()
    {
        static UbseConfigManager instance;
        return instance;
    }

    UbseConfigManager operator=(const UbseConfigManager&) = delete;

    UbseConfigManager(const UbseConfigManager&) = delete;
    
    
    uint32_t Init(const std::string& confDir, const std::string& filePrefix = "");


    uint32_t Start();

    void Stop();

    /* *
    * @brief 读取单条配置
    * @param[in] section: 配置节
    * @param[in] configKey: 配置参数key
    * @param[out] configVal: 配置参数值
    * @return uint32_t, 成功返回0, 失败返回非0
    */
    uint32_t GetConf(const std::string& section, const std::string& configKey, std::string& configVal);

private:
    UbseConfigManager();

    uint32_t ParseFile(const std::string& filePath);  // 解析文件, 校验文件并读入内存

    void ParseLine(const std::string& filePath, std::string line, const size_t& lineCount, std::string& tempSection);
    void ParseSection(const std::string& filePath, const std::string& line, const size_t& lineCount,
                      std::string& tempSection);
    void ParseConf(const std::string& filePath, const std::string& line, const size_t& lineCount,
                   std::string& tempSection);

    uint32_t ReadConfFile(const std::string& filePath);  // 读取文件到内存

    Format format;
    std::unordered_map<std::string, std::vector<std::string>> parseErrors;  // 解析错误信息
    std::shared_mutex rwLock;
    ConfigMap configMap;  // 全部配置
    std::unordered_set<std::string> fileSet;
};
}  // namespace ubse::config




#endif //UBSVIRTOVS_CONFIG_MANAGER_H
