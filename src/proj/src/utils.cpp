/**
 * @file utils.cpp
 * @brief 通用工具实现：测时、日志与并发 IO / Implementation of timing, logging and concurrent I/O utilities.
 */

#include "utils.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>

namespace test_forest
{
    namespace utils
    {

        // ================================
        // 路径与时间戳工具实现
        // ================================

        std::filesystem::path default_logs_directory()
        {
            // 当前工作目录 / current working directory
            std::filesystem::path cwd = std::filesystem::current_path();
            return cwd / "test-works" / "logs";
        }

        std::string make_timestamp_string()
        {
            namespace ch = std::chrono;

            auto now = ch::system_clock::now();
            std::time_t t = ch::system_clock::to_time_t(now);

            std::tm tm_snapshot{};
#if defined(_WIN32) || defined(_WIN64)
            localtime_s(&tm_snapshot, &t);
#else
            localtime_r(&t, &tm_snapshot);
#endif

            std::ostringstream oss;
            // 格式：YYYYMMDD_HHMMSS / format: YYYYMMDD_HHMMSS
            oss << std::put_time(&tm_snapshot, "%Y%m%d_%H%M%S");
            return oss.str();
        }

        // =============================
        // CsvLogger 实现 / CsvLogger
        // =============================

        struct CsvLogger::Impl
        {
            std::filesystem::path filepath;
            std::ofstream out;
            std::mutex mutex;
            bool header_written{false};

            Impl(const std::filesystem::path &path, bool write_header)
                : filepath(path)
            {
                // 确保目录存在 / ensure directory exists
                auto parent = filepath.parent_path();
                if (!parent.empty())
                {
                    std::filesystem::create_directories(parent);
                }

                out.open(filepath, std::ios::out | std::ios::trunc);
                if (!out.is_open())
                {
                    throw std::runtime_error("CsvLogger: failed to open file: " + filepath.string());
                }

                if (write_header)
                {
                    out << "test_func_name,count,time_usage\n";
                    header_written = true;
                }
            }

            void append(const std::string &name, std::uint64_t count, double time_usage_seconds)
            {
                std::lock_guard<std::mutex> lock(mutex);
                if (!out.is_open())
                {
                    throw std::runtime_error("CsvLogger: output stream is not open.");
                }

                // 简单 CSV：若 name 里有逗号则用双引号包裹 / Simple CSV: quote name if it contains comma
                if (name.find(',') != std::string::npos)
                {
                    out << '"';
                    for (char c : name)
                    {
                        if (c == '"')
                            out << "\"\""; // 转义双引号 / escape quote
                        else
                            out << c;
                    }
                    out << '"';
                }
                else
                {
                    out << name;
                }

                out << ',' << count << ',';

                // 固定精度输出秒数 / fixed precision seconds
                out << std::fixed << std::setprecision(9) << time_usage_seconds << '\n';
            }

            void flush()
            {
                std::lock_guard<std::mutex> lock(mutex);
                out.flush();
            }
        };

        CsvLogger CsvLogger::open_default(bool write_header)
        {
            auto dir = default_logs_directory();
            auto ts = make_timestamp_string();
            auto file = dir / (ts + ".csv");
            return open_file(file, write_header);
        }

        CsvLogger CsvLogger::open_at(const std::filesystem::path &directory, bool write_header)
        {
            auto ts = make_timestamp_string();
            auto file = directory / (ts + ".csv");
            return open_file(file, write_header);
        }

        CsvLogger CsvLogger::open_file(const std::filesystem::path &filepath, bool write_header)
        {
            auto impl = std::make_shared<Impl>(filepath, write_header);
            return CsvLogger{std::move(impl)};
        }

        void CsvLogger::append(const std::string &test_func_name,
                               std::uint64_t count,
                               double time_usage_seconds)
        {
            if (!impl_)
            {
                throw std::runtime_error("CsvLogger: append() on invalid logger.");
            }
            impl_->append(test_func_name, count, time_usage_seconds);
        }

        void CsvLogger::flush()
        {
            if (impl_)
            {
                impl_->flush();
            }
        }

        const std::filesystem::path &CsvLogger::filepath() const noexcept
        {
            static const std::filesystem::path empty_path{};
            return impl_ ? impl_->filepath : empty_path;
        }

        bool CsvLogger::valid() const noexcept
        {
            return static_cast<bool>(impl_);
        }

        // ====================
        // 文本日志实现 / Text log
        // ====================

        namespace
        {
            /// @brief 全局日志互斥量，保护 stderr 输出 / Global mutex to guard stderr logging.
            std::mutex &log_mutex()
            {
                static std::mutex m;
                return m;
            }

            /// @brief 构造带时间戳的前缀 / Build timestamped log prefix.
            std::string make_log_prefix(const char *level)
            {
                return "[" + std::string(level) + " " + make_timestamp_string() + "] ";
            }
        } // namespace

        void log_info(const std::string &message)
        {
            std::lock_guard<std::mutex> lock(log_mutex());
            std::cerr << make_log_prefix("INFO") << message << '\n';
        }

        void log_error(const std::string &message)
        {
            std::lock_guard<std::mutex> lock(log_mutex());
            std::cerr << make_log_prefix("ERROR") << message << '\n';
        }

    } // namespace utils
} // namespace test_forest
