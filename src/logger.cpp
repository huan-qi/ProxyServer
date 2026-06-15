#include "logger.h"
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/async.h>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace proxy
{

std::atomic<logger*> logger::instance_{nullptr};
std::mutex logger::mtx_;

logger* logger::instance()
{
    logger* tmp = instance_.load(std::memory_order_acquire);
    if (!tmp) {
        std::lock_guard<std::mutex> lock(mtx_);
        tmp = instance_.load(std::memory_order_relaxed);
        if (!tmp) {
            tmp = new logger();
            instance_.store(tmp, std::memory_order_release);
        }
    }
    return tmp;
}

logger::logger(size_t max_file_size = 1024 * 1024 * 20, size_t max_files = 5)
{

}

logger::~logger()
{
    spdlog::shutdown();
}

void logger::log_info(std::string strMsg)
{
    log(spdlog::level::info, "[Time: {}] INFO: {}", now_local_string(), strMsg);
}

void logger::log_err(std::string strMsg)
{
    log(spdlog::level::err, "[Time: {}] ERR: {}", now_local_string(), strMsg);
}

template <typename... Args>
void logger::log(spdlog::level::level_enum lvl, spdlog::format_string_t<Args...> fmt, Args&&... args) 
{
    m_pLogger->log(lvl, fmt, std::forward<Args>(args)...);
}

std::string logger::now_local_string()
{
    using namespace std::chrono;
    _V2::system_clock::time_point now = system_clock::now();
    std::time_t t = system_clock::to_time_t(now);
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

}