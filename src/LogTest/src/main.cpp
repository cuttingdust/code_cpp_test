#include <iostream>
#include <string>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <iomanip>

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

class LogFileOutput : public LogOutput
{
public:
    bool Open(const std::string& file)
    {
        /// 追加写入
        ofs_.open(file, std::ios::app);
        if (ofs_.is_open())
            return true;
        return false;
    }

    void Ouput(const std::string& log) override
    {
        ofs_ << log << "\n";
    }

private:
    std::ofstream ofs_;
};


class LogFormat
{
public:
    virtual std::string Format(const std::string& level, const std::string& log, const std::string& file, int line) = 0;
};

static std::string GetNow(const char* fmt = "%Y-%m-%d %H:%M:%S", int time_zone = 8)
{
    std::time_t      unix_sec = std::time(nullptr);
    std::tm          tm;
    static const int kHoursInDay        = 24;
    static const int kMinutesInHour     = 60;
    static const int kDaysFromUnixTime  = 2472632;
    static const int kDaysFromYear      = 153;
    static const int kMagicUnkonwnFirst = 146097;
    static const int kMagicUnkonwnSec   = 1461;
    tm.tm_sec                           = unix_sec % kMinutesInHour;
    int i                               = (unix_sec / kMinutesInHour);
    tm.tm_min                           = i % kMinutesInHour; //nn
    i /= kMinutesInHour;
    tm.tm_hour = (i + time_zone) % kHoursInDay; // hh
    tm.tm_mday = (i + time_zone) / kHoursInDay;
    int a      = tm.tm_mday + kDaysFromUnixTime;
    int b      = (a * 4 + 3) / kMagicUnkonwnFirst;
    int c      = (-b * kMagicUnkonwnFirst) / 4 + a;
    int d      = ((c * 4 + 3) / kMagicUnkonwnSec);
    int e      = -d * kMagicUnkonwnSec;
    e          = e / 4 + c;
    int m      = (5 * e + 2) / kDaysFromYear;
    tm.tm_mday = -(kDaysFromYear * m + 2) / 5 + e + 1;
    tm.tm_mon  = (-m / 10) * 12 + m + 2;
    tm.tm_year = b * 100 + d - 6700 + (m / 10);
    std::stringstream ss;
    ss << std::put_time(&tm, fmt); //#include <iomanip>
    return ss.str();
}

class XLogFormat : public LogFormat
{
public:
    std::string Format(const std::string& level, const std::string& log, const std::string& file, int line) override
    {
        std::stringstream ss;
        ss << GetNow() << " " << level << " " << log << " " << file << ":" << line;
        return ss.str();
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
        std::cout << "Drop Logger\n";
    }
    enum class XLog
    { //日志级别
        DEBUG,
        INFO,
        ERROR,
        FATAL
    };

public:
    void Write(const std::string& log, const XLog& level, const char* file, int line) const
    {
        // auto format_log = formater_->Format(log_level_, log, file, line);

        output_->Ouput(log);
    }

    void Write(const std::string& log) const
    {
        output_->Ouput(log);
    }

    void SetFormat(std::unique_ptr<LogFormat> f)
    {
        formater_ = std::move(f);
    }

    void SetLevel(XLog level)
    {
        log_level_ = level;
    }

    /// 设置委托
    void SetOutput(std::unique_ptr<LogOutput> o)
    {
        output_ = std::move(o);
    }

private:
    std::unique_ptr<LogOutput> output_;
    std::unique_ptr<LogFormat> formater_;
    /// 最低日志级别
    XLog log_level_{ XLog::DEBUG };
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
        logger_.SetFormat(std::make_unique<XLogFormat>());

        /// 配置文件 等级设置
        // XConfig conf;
        // bool    re        = conf.Read(con_file);
        // string  log_type  = "console";
        // string  log_file  = LOGFILE;
        // string  log_level = "debug";
        // if (re)
        // {
        //     log_type  = conf.Get("log_type");
        //     log_file  = conf.Get("log_file");
        //     log_level = conf.Get("log_level");
        // }
        // if (log_level == "info")
        // {
        //     logger_.SetLevel(XLog::INFO);
        // }
        // else if (log_level == "error")
        // {
        //     logger_.SetLevel(XLog::ERROR);
        // }
        // else if (log_level == "fatal")
        // {
        //     logger_.SetLevel(XLog::ERROR);
        // }

        // if (log_type == "file")
        // {
        //     if (log_file.empty())
        //     {
        //         log_file = LOGFILE;
        //     }
        //     auto fout = std::make_unique<LogFileOutput>(); //new LogFileOutput();
        //     if (fout->Open(log_file))
        //     {
        //         std::cerr << "open file failed " << log_file << std::endl;
        //     }
        //     logger_.SetOutput(std::move(fout));
        // }
        // else
        // {
        //     logger_.SetOutput(std::make_unique<LogConsoleOutput>());
        // }

        logger_.SetOutput(std::make_unique<LogConsoleOutput>());
    }
    Logger& logger()
    {
        return logger_;
    }

private: /// 单件构造放私有
    LogFac()
    {
        std::cout << "Create LogFac\n";
    }
    Logger logger_;
};

/// 设置等级之类的
#define XLOGOUT(l, s) LogFac::Instance().logger().Write(s, l, __FILE__, __LINE__)
#define LOGDEBUG(s)   XLOGOUT(Logger::XLog::DEBUG, s)
#define LOGINFO(s)    XLOGOUT(Logger::XLog::INFO, s)
#define LOGERROR(s)   XLOGOUT(Logger::XLog::ERROR, s)
#define LOGFATAL(s)   XLOGOUT(Logger::XLog::FATAL, s)

int main()
{
    Logger logger;
    /// 委托
    logger.SetOutput(std::make_unique<LogConsoleOutput>());
    logger.Write("test console log");

    LogFac::Instance().ResetLogger(LogFac::LogTarget::CONSOLE);
    LOGDEBUG("test console log2222");
}
