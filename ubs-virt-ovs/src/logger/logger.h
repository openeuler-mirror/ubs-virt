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
#ifndef LOGGER_H
#define LOGGER_H

#include <sys/stat.h>
#include <unistd.h>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

namespace virt::logger {
enum class LoggerLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR
};

class LoggerEntry {
public:
    LoggerEntry(LogLevel LoggerLevel, const char *file, const char *func, int line);

    template <typename T>
    LoggerEntry &operator<<(const T &val)
    {
        ss_ << val;
        return *this;
    }

    void Submit();

private:
    LoggerLevel level_;
    std::string file_;
    std::string func_;
    int line_;

    pid_t pid_;
    uint64_t tid_;
    std::chrono::system_clock::time_point timestamp_;
    std::stringstream ss_;

    static std::string Basename(const char *path);
    static uint64_t ThreadIdToU64(std::thread::id tid);
    std::string FormatTime(const std::chrono::system_clock::time_point &tp);
};

class Logger {
public:
    bool operator==(LoggerEntry &entry)
    {
        entry.Submit();
        return true;
    }
};

inline bool LogInternal(LoggerLevel level, const char *file, const char *func, int line)
{
    return Logger() == LoggerEntry(level, file, func, line);
}

#define LOG_DEBUG LogInternal(LoggerLevel::DEBUG, __FILE__, __FUNCTION__, __LINE__)
#define LOG_INFO LogInternal(LoggerLevel::INFO, __FILE__, __FUNCTION__, __LINE__)
#define LOG_WARN LogInternal(LoggerLevel::WARN, __FILE__, __FUNCTION__, __LINE__)
#define LOG_ERROR LogInternal(LoggerLevel::ERROR, __FILE__, __FUNCTION__, __LINE__)
} // namespace virt::logger
#endif