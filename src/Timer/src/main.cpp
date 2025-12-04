#include <fmt/format.h>
#include <fmt/chrono.h> // 添加chrono支持

#include <chrono>
#include <string>
#include <functional>
#include <source_location>
#include <iostream>
#include <iomanip>
#include <thread>
#include <limits>
#include <ctime>

/// @brief 时间单位枚举
enum class TimeUnit
{
    Nanoseconds,  ///< 纳秒
    Microseconds, ///< 微秒
    Milliseconds, ///< 毫秒
    Seconds,      ///< 秒
    Minutes,      ///< 分钟
    Hours         ///< 小时
};

/// @brief 高性能计时器类，支持多种时间单位和格式化输出
class Timer
{
public:
    /// @brief 构造函数，自动开始计时
    /// @param name 计时器名称（可选）
    /// @param auto_print 析构时是否自动打印耗时
    /// @param loc 源码位置信息
    explicit Timer(std::string_view name = "", bool auto_print = false,
                   const std::source_location &loc = std::source_location::current()) :
        name_(name.empty() ? loc.function_name() : name), location_(loc), auto_print_(auto_print),
        begin_(std::chrono::high_resolution_clock::now())
    {
        if (auto_print_)
        {
            auto now        = std::chrono::system_clock::now();
            auto now_time_t = std::chrono::system_clock::to_time_t(now);

/// 转换为本地时间
#ifdef _WIN32
            std::tm local_tm;
            localtime_s(&local_tm, &now_time_t);
#else
            std::tm local_tm;
            localtime_r(&now_time_t, &local_tm);
#endif

            std::cout << fmt::format("[TIMER] '{}' started at {:%H:%M:%S}\n", name_, local_tm);
        }
    }

    /// @brief 析构函数，如果启用auto_print则自动打印耗时
    virtual ~Timer()
    {
        if (auto_print_)
        {
            print();
        }
    }

    /// @brief 获取经过的时间（根据模板参数指定的单位）
    /// @tparam Duration 时间单位类型（如std::chrono::milliseconds）
    /// @return 经过的时间数量
    template <typename Duration = std::chrono::milliseconds>
    [[nodiscard]] auto elapse() const -> typename Duration::rep
    {
        const auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<Duration>(end - begin_).count();
    }

    /// @brief 获取经过的时间（指定时间单位）
    /// @param unit 时间单位枚举
    /// @return 经过的时间（浮点数，精度更高）
    [[nodiscard]] auto elapse(TimeUnit unit = TimeUnit::Milliseconds) const -> double
    {
        const auto nanoseconds = elapse<std::chrono::nanoseconds>();
        return convertFromNanoseconds(nanoseconds, unit);
    }

    /// @brief 获取经过的时间并重新开始计时
    /// @tparam Duration 时间单位类型
    /// @return 经过的时间数量
    template <typename Duration = std::chrono::milliseconds>
    auto elapseAndReset() -> typename Duration::rep
    {
        const auto elapsed = elapse<Duration>();
        reset();
        return elapsed;
    }

    /// @brief 重新开始计时
    void reset()
    {
        begin_ = std::chrono::high_resolution_clock::now();
    }

    /// @brief 打印经过的时间
    /// @param unit 时间单位
    /// @param message 附加消息
    void print(TimeUnit unit = TimeUnit::Milliseconds, std::string_view message = "") const
    {
        const double      elapsed  = elapse(unit);
        const std::string unit_str = timeUnitToString(unit);

        if (message.empty())
        {
            std::cout << fmt::format("[TIMER] '{}' elapsed: {:.3f} {} (at {}:{})\n", name_, elapsed, unit_str,
                                     location_.file_name(), location_.line());
        }
        else
        {
            std::cout << fmt::format("[TIMER] '{}' {}: {:.3f} {} (at {}:{})\n", name_, message, elapsed, unit_str,
                                     location_.file_name(), location_.line());
        }
    }

    /// @brief 获取当前时间戳字符串
    /// @param with_milliseconds 是否包含毫秒
    /// @return 格式化的时间戳字符串
    [[nodiscard]] static auto timestamp(bool with_milliseconds = true) -> std::string
    {
        const auto now = std::chrono::system_clock::now();

        if (with_milliseconds)
        {
            /// 获取毫秒部分
            auto since_epoch = now.time_since_epoch();
            auto ms          = std::chrono::duration_cast<std::chrono::milliseconds>(since_epoch) % 1000;

            /// 转换为time_t获取日期部分
            auto    now_time_t = std::chrono::system_clock::to_time_t(now);
            std::tm local_time;

#ifdef _WIN32
            localtime_s(&local_time, &now_time_t);
#else
            localtime_r(&now_time_t, &local_time);
#endif

            // 使用fmt格式化日期，手动添加毫秒
            return fmt::format("{:%Y-%m-%d %H:%M:%S}.{:03d}", fmt::gmtime(now_time_t), ms.count());
        }
        else
        {
            auto now_time_t = std::chrono::system_clock::to_time_t(now);
            return fmt::format("{:%Y-%m-%d %H:%M:%S}", fmt::gmtime(now_time_t));
        }
    }


    /// @brief 静态方法：测量函数执行时间
    /// @tparam Func 函数类型
    /// @tparam Duration 时间单位类型
    /// @param func 要测量的函数
    /// @param func_name 函数名称
    /// @return 执行时间
    template <typename Duration = std::chrono::milliseconds, typename Func>
    [[nodiscard]] static auto measure(Func &&func, std::string_view func_name = "") -> typename Duration::rep
    {
        const auto start = std::chrono::high_resolution_clock::now();
        std::invoke(std::forward<Func>(func));
        const auto end = std::chrono::high_resolution_clock::now();

        const auto elapsed = std::chrono::duration_cast<Duration>(end - start).count();

        if (!func_name.empty())
        {
            std::cout << fmt::format("[MEASURE] '{}' took {} {}\n", func_name, elapsed, durationSuffix<Duration>());
        }

        return elapsed;
    }

    /// @brief 获取计时器名称
    [[nodiscard]] auto name() const -> const std::string &
    {
        return name_;
    }

    /// @brief 检查是否启用了自动打印
    [[nodiscard]] auto isAutoPrintEnabled() const -> bool
    {
        return auto_print_;
    }

private:
    /// @brief 将纳秒转换为指定单位
    /// @param nanoseconds 纳秒数
    /// @param unit 目标时间单位
    /// @return 转换后的时间值
    [[nodiscard]] static auto convertFromNanoseconds(int64_t nanoseconds, TimeUnit unit) -> double
    {
        switch (unit)
        {
            case TimeUnit::Nanoseconds:
                return static_cast<double>(nanoseconds);
            case TimeUnit::Microseconds:
                return nanoseconds / 1'000.0;
            case TimeUnit::Milliseconds:
                return nanoseconds / 1'000'000.0;
            case TimeUnit::Seconds:
                return nanoseconds / 1'000'000'000.0;
            case TimeUnit::Minutes:
                return nanoseconds / 60'000'000'000.0;
            case TimeUnit::Hours:
                return nanoseconds / 3'600'000'000'000.0;
            default:
                return 0.0;
        }
    }

    /// @brief 将时间单位转换为字符串
    /// @param unit 时间单位
    /// @return 单位字符串
    [[nodiscard]] static auto timeUnitToString(TimeUnit unit) -> std::string
    {
        switch (unit)
        {
            case TimeUnit::Nanoseconds:
                return "ns";
            case TimeUnit::Microseconds:
                return "μs";
            case TimeUnit::Milliseconds:
                return "ms";
            case TimeUnit::Seconds:
                return "s";
            case TimeUnit::Minutes:
                return "min";
            case TimeUnit::Hours:
                return "h";
            default:
                return "unknown";
        }
    }

    /// @brief 获取持续时间类型的后缀字符串
    /// @tparam Duration 持续时间类型
    /// @return 时间单位后缀
    template <typename Duration>
    [[nodiscard]] static auto durationSuffix() -> std::string_view
    {
        if constexpr (std::is_same_v<Duration, std::chrono::nanoseconds>)
        {
            return "ns";
        }
        else if constexpr (std::is_same_v<Duration, std::chrono::microseconds>)
        {
            return "μs";
        }
        else if constexpr (std::is_same_v<Duration, std::chrono::milliseconds>)
        {
            return "ms";
        }
        else if constexpr (std::is_same_v<Duration, std::chrono::seconds>)
        {
            return "s";
        }
        else if constexpr (std::is_same_v<Duration, std::chrono::minutes>)
        {
            return "min";
        }
        else if constexpr (std::is_same_v<Duration, std::chrono::hours>)
        {
            return "h";
        }
        else
        {
            return "units";
        }
    }

private:
    std::string                                                 name_;               ///< 计时器名称
    std::source_location                                        location_;           ///< 源码位置信息
    bool                                                        auto_print_ = false; ///< 是否自动打印
    std::chrono::time_point<std::chrono::high_resolution_clock> begin_;              ///< 开始时间点
};

/// @brief 带作用域的计时器，RAII风格
class ScopedTimer : public Timer
{
public:
    /// @brief 构造函数
    /// @param name 计时器名称
    /// @param unit 时间单位
    /// @param loc 源码位置
    explicit ScopedTimer(std::string_view name = "", TimeUnit unit = TimeUnit::Milliseconds,
                         std::source_location loc = std::source_location::current()) :
        Timer(name, false, loc), unit_(unit)
    {
    }

    /// @brief 析构函数，自动打印耗时
    ~ScopedTimer() override
    {
        print(unit_);
    }

private:
    TimeUnit unit_; ///< 时间单位
};

/// @brief 用于性能统计的计时器
class StatsTimer : public Timer
{
public:
    /// @brief 构造函数
    explicit StatsTimer(std::string_view name = "", std::source_location loc = std::source_location::current()) :
        Timer(name, false, loc), call_count_(0), total_time_(0), min_time_(std::numeric_limits<double>::max()),
        max_time_(0)
    {
    }

    /// @brief 记录一次执行时间
    /// @param elapsed 经过的时间（毫秒）
    void record(double elapsed)
    {
        ++call_count_;
        total_time_ += elapsed;
        min_time_ = std::min(min_time_, elapsed);
        max_time_ = std::max(max_time_, elapsed);
    }

    /// @brief 获取统计信息
    /// @return 包含统计信息的字符串
    [[nodiscard]] auto getStats() const -> std::string
    {
        if (call_count_ == 0)
        {
            return "No data recorded";
        }

        const double avg_time = total_time_ / call_count_;

        return fmt::format("{} - Calls: {}, Avg: {:.3f}ms, Min: {:.3f}ms, Max: {:.3f}ms, Total: {:.3f}ms", name(),
                           call_count_, avg_time, min_time_, max_time_, total_time_);
    }

    /// @brief 打印统计信息
    void printStats() const
    {
        std::cout << "[STATS] " << getStats() << std::endl;
    }

    /// @brief 重置统计信息
    void resetStats()
    {
        call_count_ = 0;
        total_time_ = 0;
        min_time_   = std::numeric_limits<double>::max();
        max_time_   = 0;
    }

private:
    uint64_t call_count_; ///< 调用次数
    double   total_time_; ///< 总时间（毫秒）
    double   min_time_;   ///< 最短时间（毫秒）
    double   max_time_;   ///< 最长时间（毫秒）
};

/// @brief 方便使用的宏

/// @brief 创建具名计时器
#define TIMER(name) Timer timer_##name(#name)

/// @brief 创建作用域计时器
#define SCOPED_TIMER(name, unit) ScopedTimer scoped_timer_##name(#name, unit)

/// @brief 测量代码块执行时间
#define MEASURE_BLOCK(name, unit, code)                \
    do                                                 \
    {                                                  \
        Timer block_timer(#name);                      \
        code;                                          \
        block_timer.print(unit, "block completed in"); \
    }                                                  \
    while (0)

/// @brief 测量函数执行时间
#define MEASURE_FUNCTION(unit) ScopedTimer function_timer(__FUNCTION__, unit)

/// 使用示例
void exampleUsage()
{
    /// 示例0：基本使用
    {
        Timer timer;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << fmt::format("Elapsed time: {} ms\n", timer.elapse<std::chrono::milliseconds>());
    }

    /// 示例1：基本使用
    {
        Timer timer("Simple timer", true);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        /// 析构时会自动打印
    }

    /// 示例2：获取不同单位的时间
    {
        Timer timer("Multi-unit timer");
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        std::cout << fmt::format("Elapsed in nanoseconds: {}\n", timer.elapse<std::chrono::nanoseconds>());
        std::cout << fmt::format("Elapsed in microseconds: {}\n", timer.elapse<std::chrono::microseconds>());
        std::cout << fmt::format("Elapsed in milliseconds: {}\n", timer.elapse<std::chrono::milliseconds>());
        std::cout << fmt::format("Elapsed in seconds: {}\n", timer.elapse<std::chrono::seconds>());
    }

    /// 示例3：使用枚举单位
    {
        Timer timer("Enum unit timer");
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        timer.print(TimeUnit::Microseconds);
        timer.print(TimeUnit::Milliseconds);
        timer.print(TimeUnit::Seconds);
    }

    /// 示例4：作用域计时器
    {
        SCOPED_TIMER(ScopedExample, TimeUnit::Milliseconds);
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        /// 离开作用域时自动打印
    }

    /// 示例5：测量函数执行时间
    {
        auto result = Timer::measure<std::chrono::microseconds>(
                []()
                {
                    volatile int sum = 0;
                    for (int i = 0; i < 1000000; ++i)
                    {
                        sum += i;
                    }
                },
                "Sum calculation");

        std::cout << fmt::format("Function took {} μs\n", result);
    }

    /// 示例6：使用宏
    {
        TIMER(MacroTimer);
        std::this_thread::sleep_for(std::chrono::milliseconds(75));
        timer_MacroTimer.print();
    }

    /// 示例7：性能统计
    {
        StatsTimer stats("Performance test");

        for (int i = 0; i < 10; ++i)
        {
            Timer iteration_timer;
            std::this_thread::sleep_for(std::chrono::milliseconds(10 + i * 5));
            stats.record(iteration_timer.elapse(TimeUnit::Milliseconds));
        }

        stats.printStats();
    }

    /// 示例8：获取时间戳
    std::cout << fmt::format("Current timestamp: {}\n", Timer::timestamp());
    std::cout << fmt::format("Current timestamp (no ms): {}\n", Timer::timestamp(false));
}

int main()
{
    /// 设置本地化（支持中文输出）
    setlocale(LC_ALL, "zh_CN.UTF-8");


    std::cout << "Timer class examples:\n";
    std::cout << "=====================\n";

    exampleUsage();

    return 0;
}
