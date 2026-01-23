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
#include "logger.h"

namespace virt::ovs::config {

uint32_t TravelDepthLimitedFiles(std::vector<std::string> &filePaths, const std::string &path, int depth);

uint32_t UbseConfigManager::Init(const std::string &confDir, const std::string &filePrefix)
{
    std::vector<std::string> filePaths;
    uint32_t ret = TravelDepthLimitedFiles(filePaths, confDir, 0);
    if (ret != UBSE_OK) {
        return ret;
    }

    std::unique_lock<std::shared_mutex> guard(rwLock);
    for (const auto &filePath : filePaths) {
        // 文件已加载, 不可重复加载
        if (fileSet.count(filePath)) {
            LOG_ERROR << "Warning: File: " << filePath << "has already been loaded." << std::endl;
            continue;
        }
        // 文件前缀非空且与文件名不匹配
        if (!filePrefix.empty() && filePath.substr(filePath.rfind('/') + 1).find(filePrefix) != 0) {
            continue;
        }
        ret = ParseFile(filePath);
        // 解析文件失败
        if (ret != UBSE_OK) {
            LOG_ERROR << "Warning: Unable to parse file: " << filePath << "." << std::endl;
        } else {
            fileSet.emplace(filePath);
        }
    }
    if (!parseErrors.empty()) {
        LOG_ERROR << "Unable to parse the configuration." << std::endl;
        for (const auto &pair : parseErrors) {
            LOG_ERROR << "fileName: " << pair.first << ",warnings:\n" << CatString(pair.second, "\n") << std::endl;
        }
    }
    parseErrors.clear();
    return ret;
}

} // namespace virt::ovs::config