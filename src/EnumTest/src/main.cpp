#include <iostream>

/// 测试枚举类型
/// 传统枚举类型
enum 枚举类型名
{
    枚举值1, //默认0
    枚举值2, //1
    枚举值3, //2
    枚举值4 = 1000,
    枚举值5,
};
enum Status
{
    PLAY,
    PAUSE,
    STOP,
};

/// c++11之后支持的枚举
enum class LogLevel
{
    DEBUG,
    INFO,
    ERROR,
    FATAL
};
int main()
{
    setlocale(LC_ALL, "zh_CN.UTF-8");
    LogLevel level{ LogLevel::DEBUG };
    std::cout << "level = " << (int)level << std::endl;
    std::cout << (int)LogLevel::FATAL << std::endl;
    level = LogLevel::FATAL;
    if (LogLevel::FATAL == level)
    {
        std::cout << "FATAL" << std::endl;
    }
    LogLevel confLevel{ LogLevel::INFO };
    if (level >= confLevel)
        std::cout << "log view" << std::endl;


    Status status{ STOP };
    if (status == 2)
    {
        status = PLAY;
    }
    std::cout << "status = " << status << std::endl;
    if (status == PLAY)
    {
        std::cout << "Playing" << std::endl;
    }


    枚举类型名 ev1{ 枚举值1 };
    std::cout << "ev1 = " << ev1 << std::endl;
    std::cout << "枚举值2 = " << 枚举值2 << std::endl;
    std::cout << "枚举值3 = " << 枚举值3 << std::endl;
    std::cout << "枚举值4 = " << 枚举值4 << std::endl;
    std::cout << "枚举值5 = " << 枚举值5 << std::endl;
}
