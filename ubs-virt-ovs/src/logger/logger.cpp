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
#include "logger.h"

namespace virt::logger {
constexpr size_t MAX_LOG_SIZE = 50 * 1024 * 1024;
constexpr size_t MAX_ROTATE_FILES = 100;
constexpr int BUFER_SIZE = 32;
constexpr int MILLI_WIDTH = 3;
constexpr int CHANGE_TO_MS = 1000;
constexpr mode_t CUR_LOG_MODE = 0640;
constexpr mode_t ROT_LOG_MODE = 0440;
constexpr mode_t LOG_DIR_MODE = 0750;

const char *LOG_DIR = "/var/log/ubs-virt-ovs";
const char *LOG_FILE = "/var/log/ubs-virt-ovs/ubs-virt-ovs.log";

std::ofstream &LogFile()
{
    static std::ofstream ofs;
    return ofs;
}

std::mutex &LogMutex()
{
    static std::mutex m;
    return m;
}

void EnsureLogDir()
{
    struct stat st{};
    if (stat(LOG_DIR, &st) != 0) {
        mkdir(LOG_DIR, LOG_DIR_MODE);
    }
}

size_t GetFileSize(const char *path)
{
    struct stat st{};
    if (stat(path, &st) != 0) {
        return 0;
    }
    return static_cast<size_t>(st.st_size);
}

std::string NowFilename()
{
    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);

    std::tm tm{};
    localtime_r(&tt, &tm);

    char buf[BUFER_SIZE];
    size_t len = strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", &tm);
    if (len == 0) {
        return "00000000_000000";
    }
    return buf;
}

void SetFileMode(const char *path, mode_t mode)
{
    chmod(path, mode);
}

void InitLogFile()
{
    auto &ofs = LogFile();
    if (ofs.is_open()) {
        return;
    }

    EnsureLogDir();
    ofs.open(LOG_FILE, std::ios::out | std::ios::app);
    if (!ofs.is_open()) {
        return;
    }

    SetFileMode(LOG_FILE, CUR_LOG_MODE);
}

void CleanupOldRotateLogFile()
{
    DIR *dir = opendir(LOG_DIR);
    if (!dir) {
        return;
    }

    std::vector<std::string> files;
    struct dirent *ent;

    while ((ent = readdir(dir)) != nullptr) {
        if (ent->d_type != DT_REG) {
            continue;
        }
        std::string name(ent->d_name);
        if (name.find("virt-ovs_")) == 0 &&
            name.size() > 5 &&
            name.rfind(".tar.gz") == name.size() - 7)
            {
                files.emplace_back(name);
            }
    }

    closedir(dir);
    if (files.size() <= MAX_ROTATE_FILES) {
        return;
    }

    std::sort(files.begin(), files.end());
    size_t removeCount = files.size() - MAX_ROTATE_FILES;
    for (size_t i = 0; i < removeCount; ++i) {
        std::string fullPath = std::string(LOG_DIR) + "/" + files[i];
        unlink(fullPath.c_str());
    }
}

void RotateLogFile()
{
    if (GetFileSize(LOG_FILE) < MAX_LOG_SIZE) {
        return;
    }

    auto &ofs = LogFile();
    ofs.close();

    std::string ts = NowFilename();
    std::string raw = std::string(LOG_DIR) + "/virt-ovs_" + ts + ".log";
    std::string tar = std::string(LOG_DIR) + "/virt-ovs_" + ts + ".tar.gz";

    if (rename(LOG_FILE, raw.c_str()) != 0) {
        return;
    }
    std::string cmd = "tar -czf " + tar + " -C " + LOG_DIR + " " + raw.substr(raw.find_last_of('/') + 1);
    system(cmd.c_str());

    unlink(raw.c_str());

    SetFileMode(tar.c_str(), ROT_LOG_MODE);

    ofs.open(LOG_FILE, std::ios::out | std::ios::app);
    if (ofs.is_open()) {
        SetFileMode(LOG_FILE, CUR_LOG_MODE);
    }

    CleanupOldRotateLogFile();
}

LoggerEntry::LoggerEntry(LoggerLevel level, const char *file, const char *func, int line)
    : level_(level),
      file_(Basename(file)),
      func_(func),
      line_(line),
      timestamp_(std::chrono::system_clock::now()),
      pid_(getpid()),
      tid_(ThreadIdToU64(std::this_thread::get_id())),
{
}

std::string LoggerEntry::Basename(const char *path)
{
    const char *lastSlash = strrchr(path, '/');
    return lastSlash ? lastSlash + 1 : path;
}

uint64_t LoggerEntry::ThreadIdToU64(std::thread::id tid)
{
    std::ostringstream oss;
    oss << tid;
    return std::stoull(oss.str());
}

std::string LoggerEntry::FormatTime(const std::chrono::system_clock::time_point &tp)
{
    auto tt = std::chrono::system_clock::to_time_t(tp);

    std::tm tm{};
    localtime_r(&tt, &tm);

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()) % CHANGE_TO_MS;
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "." << std::setw(MILLI_WIDTH) << std::setfill('0') << ms.count()
        << " +08:00";
    return oss.str();
}

void LoggerEntry::Submit()
{
    static const char *levelStr[] = {"DEBUG", "INFO", "WARN", "ERROR"};

    std::ostringstream oss;
    oss << FormatTime(timestamp_) << " ";
    oss << "[" << levelStr[static_cast<int>(level_)] << "]";
    oss << "[" << pid_ << "]"
        << "[" << tid_ << "]";
    oss << "[" << file_ << ":" << func_ << ":" << line_ << "] ";
    oss << ss_.str() << "\n";

    std::lock_guard<std::mutex> lock(LogMutex());

    InitLogFile();
    auto &ofs = LogFile();

    if (!ofs.is_open()) {
        std::cout << oss.str();
        return;
    }

    ofs << oss.str();
    ofs.flush();

    RotateLogFile();
}
} // namespace virt::logger