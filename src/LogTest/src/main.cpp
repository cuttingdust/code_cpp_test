#include <iostream>
#include <string>

/// 组合 Logger 组合LogFac
/// 接口类
class LogOutput
{
public: /// 日志输出
    virtual void Ouput(const std::string& log) = 0;
};

class LogConsoleOutput : public LogOutput
{
    void Ouput(const std::string& log) override
    {
        std::cout << log << std::endl;
    }
};

class Logger
{
public:
    Logger()
    {
        std::cout << "Create Logger\n";
    }
    ~Logger()
    {
        delete output_;
        output_ = nullptr;
        std::cout << "Drop Logger\n";
    }

    void Write(const std::string& log)
    {
        output_->Ouput(log);
    }

    /// 设置委托
    void SetOuput(LogOutput* out)
    {
        output_ = out;
    }

private:
    LogOutput* output_{ nullptr };
};

class LogFac /// 日志工程 单件模式
{
public:
    static LogFac& Instance()
    {
        static LogFac fac;
        return fac;
    }
    ~LogFac()
    {
        std::cout << "Drop LogFac\n";
    }
    enum class LogTarget
    {
        FILE,
        CONSOLE,
        CONSOLE_FILE,
        CONSOLE_FILE_MSVC,
        MSVC,
        MSVC_FILE
    };

public:
    void ResetLogger(LogTarget target, std::string_view logPath = {})
    {
        /// 等级判断。。。
        logger_.SetOuput(new LogConsoleOutput());
    }

    void Debug(const std::string& msg)
    {
        logger_.Write(msg);
    }

private: /// 单件构造放私有
    LogFac()
    {
        std::cout << "Create LogFac\n";
    }
    Logger logger_;
};

/// 设置等级之类的
#define MInfo(msg) LogFac::Instance().Debug(msg)

int main()
{
    Logger logger;
    /// 委托
    logger.SetOuput(new LogConsoleOutput());
    logger.Write("test console log");

    LogFac::Instance().ResetLogger(LogFac::LogTarget::CONSOLE);
    MInfo("test console log2222");
}
