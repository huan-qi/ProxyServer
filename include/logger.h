#pragma once

#include <spdlog/spdlog.h>

namespace proxy
{

class logger
{
public:
    static logger* instance();

    logger(const logger&) = delete;
    logger& operator=(const logger&) = delete;
    logger(logger&&) = delete;
    logger& operator=(logger&&) = delete;

public:
    void log_info(std::string strMsg);

    void log_err(std::string strMsg);

    template <typename... Args>
    void log(spdlog::level::level_enum lvl, spdlog::format_string_t<Args...> fmt, Args&&... args);
    
private:
    logger(size_t max_file_size = 1024 * 1024 * 10, size_t max_files = 5);
    ~logger();

    std::string now_local_string();

private:
    static std::atomic<logger*> instance_;
    static std::mutex mtx_;

    std::shared_ptr<spdlog::logger> m_pLogger;

};

}