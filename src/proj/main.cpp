/**
 * @file main.cpp
 * @brief
 *  启动并行性能测试，调用多种树结构容器并将结果写入 CSV。
 *  Start parallel benchmarks on multiple tree containers and log results to CSV.
 */

#include <algorithm>
#include <atomic>
#include <chrono>
#include <exception>
#include <functional>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "utils.hpp"
#include "Binary-Tree.hpp"
#include "AVL-Tree.hpp"
#include "Red-Black-Tree.hpp"
#include "B-Tree.hpp"

/// @brief 项目主命名空间 / Main project namespace.
namespace test_forest
{

    // 为了少打一点字，给几个类型起别名 / Short aliases for containers we benchmark.
    using BinaryTreeInt = BinaryTree<int>;
    using AvlTreeInt = avl_tree<int>;
    using RedBlackTreeInt = RedBlackTree<int>;
    using BTreeInt = BTreeSet<int, 32>;

    /**
     * @brief
     *  检测容器是否提供 contains(key) 成员函数的辅助模板。
     *  Helper trait to detect whether container type has member function contains(key).
     */
    template <class T, class = void>
    struct has_contains : std::false_type
    {
    };

    template <class T>
    struct has_contains<T,
                        std::void_t<decltype(std::declval<const T &>().contains(0))>>
        : std::true_type
    {
    };

    /**
     * @brief
     *  统一风格的“是否包含 key”接口：优先调用 contains(key)，否则调用 find(key)。
     *  Unified "contains key" helper: prefer contains(key), otherwise fall back to find(key).
     *
     * @tparam Set
     *  集合类型（树容器）/ set-like tree container type.
     *
     * @param set
     *  集合实例 / container instance.
     * @param key
     *  要查找的 key / key to look up.
     *
     * @return
     *  是否包含该 key / whether the key is present.
     */
    template <class Set>
    bool tree_contains(const Set &set, int key)
    {
        if constexpr (has_contains<Set>::value)
        {
            return set.contains(key);
        }
        else
        {
            return set.find(key) != set.end();
        }
    }

    /**
     * @brief
     *  生成 0..n-1 的整数并打乱顺序，用于插入顺序测试。
     *  Generate integers 0..n-1 and shuffle them for insertion order tests.
     *
     * @param n
     *  元素个数 / number of elements.
     * @param rng
     *  随机数引擎 / random engine.
     *
     * @return
     *  被打乱的整数序列 / shuffled sequence of integers.
     */
    std::vector<int> make_shuffled_sequence(std::size_t n, std::mt19937 &rng)
    {
        std::vector<int> data;
        data.reserve(n);
        for (std::size_t i = 0; i < n; ++i)
        {
            data.push_back(static_cast<int>(i));
        }
        std::shuffle(data.begin(), data.end(), rng);
        return data;
    }

    /**
     * @brief
     *  生成不存在于 [0, n-1] 中的“缺失 key”序列，例如 [n, 2n)。
     *  Generate "missing keys" not in [0, n-1], e.g. [n, 2n).
     *
     * @param n
     *  元素个数 / baseline count.
     *
     * @return
     *  [n, 2n) 的整数序列 / integers from [n, 2n).
     */
    std::vector<int> make_missing_keys(std::size_t n)
    {
        std::vector<int> data;
        data.reserve(n);
        for (std::size_t i = 0; i < n; ++i)
        {
            data.push_back(static_cast<int>(n + i));
        }
        return data;
    }

    /**
     * @brief
     *  对一个 set-like 容器在多个 N 上进行基准测试，并将结果写入 CsvLogger。
     *  Run benchmarks for a single set-like container on multiple N values and log to CsvLogger.
     *
     * @tparam Set
     *  容器类型 / container type.
     *
     * @param set_name
     *  用于 CSV test_func_name 的前缀（例如 "BinaryTree"）。
     *  Prefix used for CSV test_func_name (e.g. "BinaryTree").
     * @param logger
     *  CSV 日志对象（线程安全）/ CSV logger (thread-safe).
     * @param sizes
     *  要测试的 N 列表 / list of input sizes N.
     */
    template <class Set>
    void run_benchmark_for_set(const std::string &set_name,
                               utils::CsvLogger &logger,
                               const std::vector<std::size_t> &sizes)
    {
        using clock = std::chrono::steady_clock;

        // 固定种子，保证不同容器看到相同的数据顺序
        // Fixed seed so different containers see identical data order.
        std::mt19937 rng(42);

        for (std::size_t n : sizes)
        {
            // 1) 生成数据 / generate data
            auto insert_keys = make_shuffled_sequence(n, rng);
            auto miss_keys = make_missing_keys(n);

            Set set;

            // 2) 插入测试 / insertion benchmark
            {
                auto start = clock::now();
                for (int key : insert_keys)
                {
                    (void)set.insert(key);
                }
                auto end = clock::now();
                double seconds =
                    std::chrono::duration_cast<std::chrono::duration<double>>(end - start)
                        .count();

                std::string name =
                    set_name + ".insert.N=" + std::to_string(n);
                logger.append(name,
                              static_cast<std::uint64_t>(n),
                              seconds);
            }

            // 3) 命中查找 / successful lookups (search_hit)
            {
                auto start = clock::now();
                std::uint64_t count = 0;
                for (int key : insert_keys)
                {
                    (void)tree_contains(set, key);
                    ++count;
                }
                auto end = clock::now();
                double seconds =
                    std::chrono::duration_cast<std::chrono::duration<double>>(end - start)
                        .count();

                std::string name =
                    set_name + ".search_hit.N=" + std::to_string(n);
                logger.append(name, count, seconds);
            }

            // 4) 失败查找 / unsuccessful lookups (search_miss)
            {
                auto start = clock::now();
                std::uint64_t count = 0;
                for (int key : miss_keys)
                {
                    (void)tree_contains(set, key);
                    ++count;
                }
                auto end = clock::now();
                double seconds =
                    std::chrono::duration_cast<std::chrono::duration<double>>(end - start)
                        .count();

                std::string name =
                    set_name + ".search_miss.N=" + std::to_string(n);
                logger.append(name, count, seconds);
            }

            // 5) 删除测试 / erase benchmark
            {
                auto start = clock::now();
                std::uint64_t count = 0;
                for (int key : insert_keys)
                {
                    (void)set.erase(key);
                    ++count;
                }
                auto end = clock::now();
                double seconds =
                    std::chrono::duration_cast<std::chrono::duration<double>>(end - start)
                        .count();

                std::string name =
                    set_name + ".erase.N=" + std::to_string(n);
                logger.append(name, count, seconds);
            }
        }
    }

    /**
     * @brief
     *  并行执行多个 benchmark 任务的小型线程池实现。
     *  A tiny thread-pool style runner to execute multiple benchmark tasks in parallel.
     *
     * @param tasks
     *  任务列表，每个任务是 `void()` 可调用对象。
     *  List of tasks, each is a `void()` callable.
     */
    void run_tasks_parallel(const std::vector<std::function<void()>> &tasks)
    {
        if (tasks.empty())
        {
            return;
        }

        std::atomic<std::size_t> next_index{0};

        const std::size_t task_count = tasks.size();
        std::size_t thread_count = std::thread::hardware_concurrency();
        if (thread_count == 0)
        {
            thread_count = 2;
        }
        if (thread_count > task_count)
        {
            thread_count = task_count;
        }

        auto worker = [&]()
        {
            while (true)
            {
                std::size_t i = next_index.fetch_add(1, std::memory_order_relaxed);
                if (i >= task_count)
                {
                    break;
                }

                try
                {
                    tasks[i]();
                }
                catch (const std::exception &ex)
                {
                    utils::log_error(std::string("Benchmark task threw exception: ") +
                                     ex.what());
                }
                catch (...)
                {
                    utils::log_error("Benchmark task threw unknown exception.");
                }
            }
        };

        std::vector<std::thread> threads;
        threads.reserve(thread_count);
        for (std::size_t i = 0; i < thread_count; ++i)
        {
            threads.emplace_back(worker);
        }
        for (auto &t : threads)
        {
            t.join();
        }
    }

    /**
     * @brief
     *  组合四种树容器的所有基准任务并并行执行。
     *  Construct and execute all benchmark tasks for four tree containers in parallel.
     *
     * @param logger
     *  CSV 日志对象 / CSV logger.
     */
    void run_all_benchmarks(utils::CsvLogger &logger)
    {
        // 这里可以根据需要调整 N 的规模 / Adjust N values as needed.
        const std::vector<std::size_t> sizes{1'000, 5'000, 10'000, 50'000};

        std::vector<std::function<void()>> tasks;
        tasks.reserve(4);

        // 为每一种容器添加一个任务 / One task per container.
        tasks.emplace_back([&logger, &sizes]()
                           {
            utils::log_info("Running BinaryTree benchmarks...");
            run_benchmark_for_set<BinaryTreeInt>("BinaryTree", logger, sizes);
            utils::log_info("BinaryTree benchmarks finished."); });

        tasks.emplace_back([&logger, &sizes]()
                           {
            utils::log_info("Running AVL tree benchmarks...");
            run_benchmark_for_set<AvlTreeInt>("AVLTree", logger, sizes);
            utils::log_info("AVL tree benchmarks finished."); });

        tasks.emplace_back([&logger, &sizes]()
                           {
            utils::log_info("Running RedBlackTree benchmarks...");
            run_benchmark_for_set<RedBlackTreeInt>("RedBlackTree", logger, sizes);
            utils::log_info("RedBlackTree benchmarks finished."); });

        tasks.emplace_back([&logger, &sizes]()
                           {
            utils::log_info("Running BTreeSet benchmarks...");
            run_benchmark_for_set<BTreeInt>("BTreeSet", logger, sizes);
            utils::log_info("BTreeSet benchmarks finished."); });

        run_tasks_parallel(tasks);
    }

} // namespace test_forest

/**
 * @brief
 *  程序入口：打开 CSV 日志文件，运行全部基准测试。
 *  Program entry point: open CSV log file and run all benchmarks.
 */
int main()
{
    using namespace test_forest;

    try
    {
        // 打开默认 CSV 日志文件：test-works/logs/{timestamp}.csv
        // Open default CSV log file: test-works/logs/{timestamp}.csv
        auto logger = utils::CsvLogger::open_default(true);

        utils::log_info(std::string("CSV logger opened at: ") +
                        logger.filepath().string());

        run_all_benchmarks(logger);

        logger.flush();
        utils::log_info("All benchmarks finished.");
        return EXIT_SUCCESS;
    }
    catch (const std::exception &ex)
    {
        utils::log_error(std::string("Fatal error in main: ") + ex.what());
        return EXIT_FAILURE;
    }
    catch (...)
    {
        utils::log_error("Fatal unknown error in main.");
        return EXIT_FAILURE;
    }
}
