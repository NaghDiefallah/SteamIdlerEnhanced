#ifndef LOGGER_H
#define LOGGER_H

#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <memory>
#include <vector>

class Logger {
public:
    static void init() {
        try {
            // 1. Setup thread pool for async logging (8192 messages capacity, 1 worker thread)
            spdlog::init_thread_pool(8192, 1);

            // 2. Create Sinks (Where logs go)
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            // Rotate at 5MB, keep 7 files
            auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/session.log", 5 * 1024 * 1024, 7);

            std::vector<spdlog::sink_ptr> sinks { console_sink, file_sink };

            // 3. Create Async Logger
            m_logger = std::make_shared<spdlog::async_logger>(
                "SIEL", 
                sinks.begin(), 
                sinks.end(), 
                spdlog::thread_pool(), 
                spdlog::async_overflow_policy::block
            );

            // 4. Set Format: [Time] [Level] [Thread ID] Message
            m_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [tid %t] %v");
            
            spdlog::register_logger(m_logger);
            m_logger->set_level(spdlog::level::info);
            
            // Flush logs every 3 seconds
            spdlog::flush_every(std::chrono::seconds(3));

            m_logger->info("--- LOGGING ---");
        } catch (const spdlog::spdlog_ex& ex) {
            // Fallback if logging initialization fails
            printf("Log initialization failed: %s\n", ex.what());
        }
    }

    static std::shared_ptr<spdlog::async_logger>& log() {
        return m_logger;
    }

    static void shutdown() {
        spdlog::shutdown();
    }

private:
    static inline std::shared_ptr<spdlog::async_logger> m_logger;
};

#endif