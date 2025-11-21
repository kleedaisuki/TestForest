#ifndef _UTILS_HPP
#define _UTILS_HPP

/**
 * @file utils.hpp
 * @brief 通用工具：测时、日志与并发 IO 接口 / Utility helpers: timing, logging and concurrent I/O interfaces.
 */

#include <cstdint>
#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <functional>

namespace test_forest
{
    namespace utils
    {

        // ============================
        // 时间与测时工具 / Timing tools
        // ============================

        /**
         * @brief
         *  测量一次函数调用的耗时（秒）/ Measure the elapsed time (seconds) of a single function call.
         *
         * @tparam F
         *  可调用对象类型 / callable type.
         * @tparam Args
         *  参数类型 / argument types.
         *
         * @param f
         *  可调用对象 / callable object to invoke.
         * @param args
         *  传给可调用对象的参数 / arguments forwarded to the callable.
         *
         * @return
         *  函数执行耗时（单位：秒）/ elapsed time in seconds.
         *
         * @note
         *  使用 std::chrono::steady_clock 以避免时钟跳变影响。/ Uses std::chrono::steady_clock to avoid clock jumps.
         */
        template <typename F, typename... Args>
        double measure_seconds(F &&f, Args &&...args)
        {
            using clock = std::chrono::steady_clock;
            auto start = clock::now();
            std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
            auto end = clock::now();
            std::chrono::duration<double> diff = end - start;
            return diff.count();
        }

        /**
         * @brief
         *  连续调用无参函数 N 次并测量总耗时（秒）/ Invoke a nullary function N times and measure total elapsed time in seconds.
         *
         * @tparam F
         *  可调用对象类型，须可接受零参数 / callable type invocable with no arguments.
         *
         * @param f
         *  无参可调用对象 / nullary callable.
         * @param n
         *  调用次数 / number of iterations.
         *
         * @return
         *  总耗时（单位：秒）/ total elapsed time in seconds.
         */
        template <typename F>
        double measure_seconds_n(F &&f, std::size_t n)
        {
            using clock = std::chrono::steady_clock;
            auto start = clock::now();
            for (std::size_t i = 0; i < n; ++i)
            {
                std::invoke(f);
            }
            auto end = clock::now();
            std::chrono::duration<double> diff = end - start;
            return diff.count();
        }

        /**
         * @brief
         *  作用域计时器：在析构时调用回调函数汇报耗时（秒）。/ Scope-based timer that reports elapsed time (seconds) to a callback on destruction.
         *
         * @tparam Callback
         *  回调类型，需可接受一个 double（耗时秒）。/ callback type, must be invocable with a single double (elapsed seconds).
         * @tparam Clock
         *  使用的时钟类型（缺省为 std::chrono::steady_clock）。/ clock type to use (defaults to std::chrono::steady_clock).
         *
         * @example
         *  @code
         *  // 中文：在作用域结束时，把耗时写入日志
         *  // EN : write elapsed time to logger at scope end
         *  ScopeTimer timer([&](double sec) {
         *      logger.append("test_func", count, sec);
         *  });
         *  @endcode
         */
        template <typename Callback, typename Clock = std::chrono::steady_clock>
        class ScopeTimer
        {
            static_assert(std::is_invocable_v<Callback, double>,
                          "Callback must be invocable with a double argument.");

        public:
            using clock = Clock;
            using time_point = typename clock::time_point;

            /**
             * @brief 构造并记录起始时间 / Construct and record start time.
             *
             * @param cb
             *  回调函数，被保存起来以在析构时调用 / callback to be stored and invoked on destruction.
             */
            explicit ScopeTimer(Callback cb)
                : callback_(std::move(cb)),
                  start_(clock::now()),
                  active_(true)
            {
            }

            /**
             * @brief 移动构造 / Move constructor.
             */
            ScopeTimer(ScopeTimer &&other) noexcept
                : callback_(std::move(other.callback_)),
                  start_(other.start_),
                  active_(other.active_)
            {
                other.active_ = false;
            }

            ScopeTimer(const ScopeTimer &) = delete;
            ScopeTimer &operator=(const ScopeTimer &) = delete;

            /**
             * @brief 移动赋值 / Move assignment.
             */
            ScopeTimer &operator=(ScopeTimer &&other) noexcept
            {
                if (this != &other)
                {
                    // 先结束本对象的计时 / finish current timer first
                    finish_if_active();

                    callback_ = std::move(other.callback_);
                    start_ = other.start_;
                    active_ = other.active_;
                    other.active_ = false;
                }
                return *this;
            }

            /**
             * @brief 析构时，如仍激活则计算耗时并调用回调。/ On destruction, if still active, compute elapsed time and invoke callback.
             */
            ~ScopeTimer()
            {
                finish_if_active();
            }

            /**
             * @brief 手动停止计时，并立刻调用回调。/ Manually stop timing and immediately invoke callback.
             */
            void stop()
            {
                finish_if_active();
            }

        private:
            void finish_if_active()
            {
                if (!active_)
                    return;

                auto end = clock::now();
                std::chrono::duration<double> diff = end - start_;
                double seconds = diff.count();
                callback_(seconds);
                active_ = false;
            }

            Callback callback_;
            time_point start_;
            bool active_;
        };

        // =====================================
        // 路径与时间戳工具 / Path & timestamp
        // =====================================

        /**
         * @brief
         *  获取默认日志目录路径：当前工作目录下的 "test-works/logs"。/ Get default logs directory: "test-works/logs" under current working directory.
         *
         * @return
         *  默认日志目录路径（不保证已存在）。/ Default logs directory path (not guaranteed to exist yet).
         */
        std::filesystem::path default_logs_directory();

        /**
         * @brief
         *  生成当前时间戳字符串，格式 "YYYYMMDD_HHMMSS"。/ Generate current timestamp string in format "YYYYMMDD_HHMMSS".
         *
         * @return
         *  无空格时间戳字符串，精确到秒。/ Whitespace-free timestamp string with second-level precision.
         */
        std::string make_timestamp_string();

        // ============================
        // CSV 日志器 / CSV logger
        // ============================

        /**
         * @brief
         *  并发安全的 CSV 日志器。将测试结果以表头
         *  "test_func_name,count,time_usage" 写入文件。/ Thread-safe CSV logger writing results with header "test_func_name,count,time_usage".
         *
         * @note
         *  复制 CsvLogger 只是共享同一个实现（内部 shared_ptr），适合在线程之间传递。/
         *  Copying CsvLogger shares the same underlying implementation (via shared_ptr), suitable for passing between threads.
         */
        class CsvLogger
        {
        public:
            /**
             * @brief 默认构造为无效日志器。/ Default-construct as an invalid logger.
             *
             * @note
             *  无效对象上调用 append 会抛异常。/ Calling append on an invalid logger will throw.
             */
            CsvLogger() = default;

            /// @brief 复制/移动默认，内部共享实现 / Default copy/move, sharing underlying implementation.
            CsvLogger(const CsvLogger &) = default;
            CsvLogger &operator=(const CsvLogger &) = default;
            CsvLogger(CsvLogger &&) noexcept = default;
            CsvLogger &operator=(CsvLogger &&) noexcept = default;

            /**
             * @brief
             *  使用默认日志目录和时间戳文件名创建日志器。/ Create a logger in default logs directory with timestamp-based filename.
             *
             * @param write_header
             *  是否写入 CSV 表头。/ Whether to write CSV header to the file.
             *
             * @return
             *  创建好的日志器对象。/ Constructed logger instance.
             */
            static CsvLogger open_default(bool write_header = true);

            /**
             * @brief
             *  在指定目录创建时间戳命名的 CSV 日志文件。/ Create a timestamp-named CSV file under specified directory.
             *
             * @param directory
             *  目标目录路径，不存在则尝试创建。/ Target directory path; will be created if not existing.
             * @param write_header
             *  是否写入 CSV 表头。/ Whether to write CSV header.
             *
             * @return
             *  创建好的日志器对象。/ Constructed logger instance.
             */
            static CsvLogger open_at(const std::filesystem::path &directory,
                                     bool write_header = true);

            /**
             * @brief
             *  打开指定路径的 CSV 日志文件。/ Open a CSV log file at the specified path.
             *
             * @param filepath
             *  完整的文件路径。/ Full file path.
             * @param write_header
             *  是否写入 CSV 表头。/ Whether to write CSV header.
             *
             * @return
             *  创建好的日志器对象。/ Constructed logger instance.
             */
            static CsvLogger open_file(const std::filesystem::path &filepath,
                                       bool write_header = true);

            /**
             * @brief
             *  追加一行测试结果到 CSV：`test_func_name,count,time_usage`。/ Append one result row to CSV: `test_func_name,count,time_usage`.
             *
             * @param test_func_name
             *  测试函数或场景名称。/ Name of the test function or scenario.
             * @param count
             *  操作次数。/ Number of operations or iterations.
             * @param time_usage_seconds
             *  总耗时（秒）。/ Total time usage in seconds.
             */
            void append(const std::string &test_func_name,
                        std::uint64_t count,
                        double time_usage_seconds);

            /**
             * @brief
             *  刷新底层输出缓冲区。/ Flush underlying output buffer.
             */
            void flush();

            /**
             * @brief
             *  获取当前日志文件路径。/ Get the path of the current log file.
             *
             * @return
             *  文件路径引用，如未初始化则为空路径。/ Reference to file path; empty if logger is invalid.
             */
            const std::filesystem::path &filepath() const noexcept;

            /**
             * @brief
             *  是否是一个有效的日志器（已关联到文件）。/ Whether this logger is valid (associated with a file).
             *
             * @return
             *  有效返回 true，无效返回 false。/ True if valid; false otherwise.
             */
            bool valid() const noexcept;

        private:
            struct Impl;                   ///< 前向声明实现体 / forward declaration of implementation.
            std::shared_ptr<Impl> impl_{}; ///< 共享实现指针 / shared implementation pointer.

            explicit CsvLogger(std::shared_ptr<Impl> impl) : impl_(std::move(impl)) {}
        };

        // ====================
        // 文本日志 / Text log
        // ====================

        /**
         * @brief
         *  打印一条 INFO 日志到标准错误，线程安全。/ Print an INFO log line to stderr, thread-safe.
         *
         * @param message
         *  日志内容。/ Log message content.
         */
        void log_info(const std::string &message);

        /**
         * @brief
         *  打印一条 ERROR 日志到标准错误，线程安全。/ Print an ERROR log line to stderr, thread-safe.
         *
         * @param message
         *  日志内容。/ Log message content.
         */
        void log_error(const std::string &message);

    } // namespace utils
} // namespace test_forest

#endif // _UTILS_HPP
