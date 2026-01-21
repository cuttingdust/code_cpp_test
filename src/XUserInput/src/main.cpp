#include <algorithm>
#include <iostream>
#include <ranges>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <functional>
#include <memory>
#include <cctype>
#include <stdexcept>

/// \brief 分割字符串为子字符串向量
static auto split(const std::string& input, char delimiter = ' ', bool trimWhitespace = true)
        -> std::vector<std::string>
{
    std::vector<std::string> ret;
    if (input.empty())
        return ret;

    std::string        tmp;
    std::istringstream iss(input);

    while (std::getline(iss, tmp, delimiter))
    {
        if (trimWhitespace)
        {
            tmp.erase(tmp.begin(), std::ranges::find_if(tmp, [](unsigned char ch) { return !std::isspace(ch); }));
            tmp.erase(std::ranges::find_if(std::ranges::reverse_view(tmp),
                                           [](unsigned char ch) { return !std::isspace(ch); })
                              .base(),
                      tmp.end());
        }

        if (!tmp.empty())
        {
            ret.push_back(tmp);
        }
    }

    return ret;
}

/// 重载版本：支持字符串作为分隔符
static auto split(const std::string& input, const std::string& delimiter, bool trimWhitespace = true)
        -> std::vector<std::string>
{
    std::vector<std::string> ret;
    if (input.empty() || delimiter.empty())
    {
        ret.push_back(input);
        return ret;
    }

    size_t start = 0;
    size_t end   = input.find(delimiter);

    while (end != std::string::npos)
    {
        std::string token = input.substr(start, end - start);

        if (trimWhitespace)
        {
            token.erase(token.begin(), std::ranges::find_if(token, [](unsigned char ch) { return !std::isspace(ch); }));
            token.erase(std::ranges::find_if(std::ranges::reverse_view(token),
                                             [](unsigned char ch) { return !std::isspace(ch); })
                                .base(),
                        token.end());
        }

        if (!token.empty())
        {
            ret.push_back(token);
        }

        start = end + delimiter.length();
        end   = input.find(delimiter, start);
    }

    std::string lastToken = input.substr(start);
    if (trimWhitespace)
    {
        lastToken.erase(lastToken.begin(),
                        std::ranges::find_if(lastToken, [](unsigned char ch) { return !std::isspace(ch); }));
        lastToken.erase(std::ranges::find_if(std::ranges::reverse_view(lastToken),
                                             [](unsigned char ch) { return !std::isspace(ch); })
                                .base(),
                        lastToken.end());
    }

    if (!lastToken.empty())
    {
        ret.push_back(lastToken);
    }

    return ret;
}

class XUserInput
{
public:
    /// 参数值包装类 - 运行时类型转换
    class ParameterValue
    {
        std::string value_;

    public:
        ParameterValue() = default;
        ParameterValue(const std::string_view& value) : value_(value)
        {
        }
        ParameterValue(const char* value) : value_(value ? value : "")
        {
        }

        /// 类型转换接口
        const std::string& asString() const
        {
            return value_;
        }

        int asInt() const
        {
            try
            {
                return std::stoi(value_);
            }
            catch (...)
            {
                throw std::runtime_error("无法将 '" + value_ + "' 转换为整数");
            }
        }

        double asDouble() const
        {
            try
            {
                return std::stod(value_);
            }
            catch (...)
            {
                throw std::runtime_error("无法将 '" + value_ + "' 转换为浮点数");
            }
        }

        bool asBool() const
        {
            if (value_.empty())
                return false;
            std::string lower = value_;
            std::ranges::transform(lower, lower.begin(), ::tolower);
            return lower == "true" || lower == "1" || lower == "yes" || lower == "on" || lower == "enabled";
        }

        /// 隐式转换到string
        operator std::string() const
        {
            return value_;
        }

        /// 检查是否有值
        bool empty() const
        {
            return value_.empty();
        }

        /// 原始值访问
        const std::string& raw() const
        {
            return value_;
        }
    };

    //////////////////////////////////////////////////////////////////

public:
    /// 参数定义类
    class Parameter
    {
    public:
        enum class Type
        {
            String,
            Int,
            Double,
            Bool
        };

        Parameter(const std::string_view& name, Type type = Type::String, const std::string_view& desc = "",
                  bool required = false) : name_(name), type_(type), description_(desc), required_(required)
        {
        }

        const std::string& getName() const
        {
            return name_;
        }
        Type getType() const
        {
            return type_;
        }
        const std::string& getDescription() const
        {
            return description_;
        }
        bool isRequired() const
        {
            return required_;
        }

        /// 获取类型名称
        std::string getTypeName() const
        {
            switch (type_)
            {
                case Type::String:
                    return "字符串";
                case Type::Int:
                    return "整数";
                case Type::Double:
                    return "浮点数";
                case Type::Bool:
                    return "布尔值";
                default:
                    return "未知";
            }
        }

    private:
        std::string name_;
        Type        type_;
        std::string description_;
        bool        required_;
    };

    //////////////////////////////////////////////////////////////////

public:
    /// 任务定义类
    class Task
    {
    public:
        using TaskFunc = std::function<void(const std::map<std::string, ParameterValue>&)>;

        Task(const std::string_view& name, TaskFunc func, const std::string_view& desc = "") :
            name_(name), func_(std::move(func)), description_(desc)
        {
        }

        /// 为任务添加参数定义（支持指定类型）
        Task& addParameter(const std::string& paramName, Parameter::Type type = Parameter::Type::String,
                           const std::string& desc = "", bool required = false)
        {
            parameters_.emplace_back(paramName, type, desc, required);
            return *this;
        }

        /// 便捷方法：添加字符串参数
        Task& addStringParam(const std::string& paramName, const std::string& desc = "", bool required = false)
        {
            return addParameter(paramName, Parameter::Type::String, desc, required);
        }

        /// 便捷方法：添加整数参数
        Task& addIntParam(const std::string& paramName, const std::string& desc = "", bool required = false)
        {
            return addParameter(paramName, Parameter::Type::Int, desc, required);
        }

        /// 便捷方法：添加浮点数参数
        Task& addDoubleParam(const std::string& paramName, const std::string& desc = "", bool required = false)
        {
            return addParameter(paramName, Parameter::Type::Double, desc, required);
        }

        /// 便捷方法：添加布尔参数
        Task& addBoolParam(const std::string& paramName, const std::string& desc = "", bool required = false)
        {
            return addParameter(paramName, Parameter::Type::Bool, desc, required);
        }

        /// 执行任务（带参数验证和类型检查）
        bool execute(const std::map<std::string, std::string>& inputParams, std::string& errorMsg) const
        {
            /// 1. 验证必需参数
            for (const auto& param : parameters_)
            {
                if (param.isRequired() && !inputParams.contains(param.getName()))
                {
                    errorMsg = "缺少必需参数: " + param.getName();
                    return false;
                }
            }

            /// 2. 类型检查和转换
            std::map<std::string, ParameterValue> typedParams;
            for (const auto& [key, strValue] : inputParams)
            {
                /// 查找参数定义
                auto paramIt =
                        std::ranges::find_if(parameters_, [&key](const Parameter& p) { return p.getName() == key; });

                if (paramIt != parameters_.end())
                {
                    /// 有定义的类型参数，创建ParameterValue
                    typedParams[key] = ParameterValue(strValue);

                    /// 类型验证（简单版，实际执行时转换）
                    try
                    {
                        switch (paramIt->getType())
                        {
                            case Parameter::Type::Int:
                                return typedParams[key].asInt(); /// 测试转换
                            case Parameter::Type::Double:
                                return typedParams[key].asDouble(); /// 测试转换
                            case Parameter::Type::Bool:
                                return typedParams[key].asBool(); /// 测试转换
                            case Parameter::Type::String:
                            default:
                                break; /// 字符串无需特殊验证
                        }
                    }
                    catch (const std::exception& e)
                    {
                        errorMsg = "参数 '" + key + "' 类型错误: " + e.what() +
                                " (期望类型: " + paramIt->getTypeName() + ")";
                        return false;
                    }
                }
                else
                {
                    /// 未定义类型的参数，按字符串处理
                    typedParams[key] = ParameterValue(strValue);
                }
            }

            /// 3. 执行任务
            try
            {
                func_(typedParams);
                return true;
            }
            catch (const std::exception& e)
            {
                errorMsg = "执行错误: " + std::string(e.what());
                return false;
            }
        }

        const std::string& getName() const
        {
            return name_;
        }
        const std::string& getDescription() const
        {
            return description_;
        }
        const std::vector<Parameter>& getParameters() const
        {
            return parameters_;
        }

    private:
        std::string            name_;
        TaskFunc               func_;
        std::string            description_;
        std::vector<Parameter> parameters_;
    };

public:
    void start()
    {
        std::cout << "任务处理器已启动。输入 'exit' 退出，'help' 查看帮助，'list' 列出任务。" << std::endl;

        while (is_running_)
        {
            std::cout << "\n>> " << std::flush;
            std::string input;
            std::getline(std::cin, input);

            if (input.empty())
                continue;

            if (input == "exit")
            {
                std::cout << "goodbye." << std::endl;
                is_running_ = false;
                continue;
            }

            if (input == "help")
            {
                printHelp();
                continue;
            }

            if (input == "list")
            {
                listTasks();
                continue;
            }

            processCommand(input);
        }
    }

    auto stop() -> void
    {
        is_running_ = false;
    }

    /// 注册任务（返回Task引用以便链式添加参数）
    Task& registerTask(const std::string& name, const Task::TaskFunc& func, const std::string& description = "")
    {
        auto  task    = std::make_unique<Task>(name, func, description);
        auto& taskRef = *task;
        tasks_[name]  = std::move(task);
        return taskRef;
    }

private:
    /// 主命令处理函数
    void processCommand(const std::string& input)
    {
        if (input.starts_with("task "))
        {
            processTaskCommand(input);
        }
        else
        {
            std::cout << "未知命令，任务命令请以 'task' 开头" << std::endl;
        }
    }

    /// 解析并执行任务命令
    void processTaskCommand(const std::string& input)
    {
        auto tokens = split(input);
        if (tokens.size() < 2)
        {
            std::cout << "格式: task <任务名> [-参数 值]..." << std::endl;
            return;
        }

        const std::string& taskName = tokens[1];
        if (!tasks_.contains(taskName))
        {
            std::cout << "未知任务: " << taskName << std::endl;
            return;
        }

        std::map<std::string, std::string> params;
        for (size_t i = 2; i < tokens.size(); ++i)
        {
            if (tokens[i][0] == '-')
            {
                const std::string& paramName  = tokens[i];
                std::string        paramValue = (i + 1 < tokens.size() && tokens[i + 1][0] != '-') ? tokens[++i] : "";
                params[paramName]             = paramValue;
            }
        }

        std::string error;
        if (tasks_[taskName]->execute(params, error))
        {
            std::cout << "任务 '" << taskName << "' 执行成功" << std::endl;
        }
        else
        {
            std::cout << "任务执行失败: " << error << std::endl;
            printTaskUsage(taskName);
        }
    }

    void printTaskUsage(const std::string& taskName) const
    {
        if (!tasks_.contains(taskName))
            return;

        const auto& task = tasks_.at(taskName);
        std::cout << "\n用法: task " << taskName;

        for (const auto& param : task->getParameters())
        {
            std::cout << " " << param.getName();
            if (!param.isRequired())
                std::cout << " [值]";
            else
                std::cout << " <值>";
        }
        std::cout << std::endl;

        if (!task->getDescription().empty())
            std::cout << "描述: " << task->getDescription() << std::endl;

        if (!task->getParameters().empty())
        {
            std::cout << "参数:" << std::endl;
            for (const auto& param : task->getParameters())
            {
                std::cout << "  " << param.getName() << " - " << param.getDescription();
                std::cout << " [" << param.getTypeName() << "]";
                if (param.isRequired())
                    std::cout << " (必需)";
                std::cout << std::endl;
            }
        }
    }

    void printHelp() const
    {
        std::cout << "\n=== 任务处理器帮助 ===" << std::endl;
        std::cout << "任务命令格式: task <任务名> [-参数1 值1] [-参数2 值2] ..." << std::endl;
        std::cout << "支持的类型: 字符串、整数、浮点数、布尔值(true/1/yes/on)" << std::endl;
        std::cout << "示例:" << std::endl;
        std::cout << "  task copy -s /home/file.txt -d /backup/" << std::endl;
        std::cout << "  task start -host 127.0.0.1 -port 8080 -debug true" << std::endl;
        std::cout << "\n特殊命令:" << std::endl;
        std::cout << "  exit  - 退出程序" << std::endl;
        std::cout << "  help  - 显示此帮助" << std::endl;
        std::cout << "  list  - 列出所有注册的任务" << std::endl;
        std::cout << "================================\n" << std::endl;
    }

    void listTasks() const
    {
        std::cout << "\n已注册的任务 (" << tasks_.size() << "):" << std::endl;
        for (const auto& [name, task] : tasks_)
        {
            std::cout << "  - " << name;
            if (!task->getDescription().empty())
            {
                std::cout << ": " << task->getDescription();
            }
            std::cout << std::endl;
        }
    }

private:
    bool                                         is_running_ = true;
    std::map<std::string, std::unique_ptr<Task>> tasks_;
};

int main(int argc, char* argv[])
{
    setlocale(LC_ALL, "zh_CN.UTF-8");

    XUserInput user_input;

    /// 示例1：支持类型的copy任务
    user_input
            .registerTask(
                    "copy",
                    [](const std::map<std::string, XUserInput::ParameterValue>& params)
                    {
                        std::cout << "[复制操作]" << std::endl;
                        auto src = params.at("-s").asString();
                        auto dst = params.at("-d").asString();
                        std::cout << "  从 " << src << " 复制到 " << dst << std::endl;
                        /// 实际复制逻辑
                    },
                    "复制文件")
            .addStringParam("-s", "源文件路径", true)
            .addStringParam("-d", "目标路径", true);

    /// 示例2：数学计算任务（演示数值类型）
    user_input
            .registerTask(
                    "calculate",
                    [](const std::map<std::string, XUserInput::ParameterValue>& params)
                    {
                        std::cout << "[计算操作]" << std::endl;
                        double x       = params.at("-x").asDouble();
                        int    n       = params.at("-n").asInt();
                        bool   verbose = params.contains("-v") ? params.at("-v").asBool() : false;

                        double result = 1.0;
                        for (int i = 0; i < n; ++i)
                            result *= x;

                        std::cout << "  结果: " << x << " ^ " << n << " = " << result << std::endl;
                        if (verbose)
                            std::cout << "  详细模式: 计算完成" << std::endl;
                    },
                    "数学计算")
            .addDoubleParam("-x", "基数", true)
            .addIntParam("-n", "指数", true)
            .addBoolParam("-v", "详细模式", false);

    /// 示例3：服务器启动任务（演示多种类型）
    user_input
            .registerTask(
                    "start",
                    [](const std::map<std::string, XUserInput::ParameterValue>& params)
                    {
                        std::cout << "[启动服务器]" << std::endl;
                        std::string host  = params.at("-host").asString();
                        int         port  = params.at("-port").asInt();
                        bool        debug = params.contains("-debug") ? params.at("-debug").asBool() : false;

                        std::cout << "  主机: " << host << ":" << port << std::endl;
                        std::cout << "  调试模式: " << (debug ? "开启" : "关闭") << std::endl;

                        if (params.contains("-timeout"))
                        {
                            double timeout = params.at("-timeout").asDouble();
                            std::cout << "  超时设置: " << timeout << "秒" << std::endl;
                        }
                    },
                    "启动服务器")
            .addStringParam("-host", "主机地址", true)
            .addIntParam("-port", "端口号", true)
            .addBoolParam("-debug", "调试模式", false)
            .addDoubleParam("-timeout", "超时时间(秒)", false);

    /// 示例4：回显任务（保持简单）
    user_input
            .registerTask(
                    "echo",
                    [](const std::map<std::string, XUserInput::ParameterValue>& params)
                    {
                        if (params.contains("-m"))
                        {
                            std::cout << "回显: " << params.at("-m").asString() << std::endl;
                        }
                        else
                        {
                            std::cout << "(未指定消息，使用 -m 参数)" << std::endl;
                        }
                    },
                    "回显消息")
            .addStringParam("-m", "要回显的消息", false);

    std::cout << "=== 增强型任务处理器示例 ===" << std::endl;
    std::cout << "支持参数类型: 字符串、整数、浮点数、布尔值" << std::endl;
    std::cout << "可以尝试以下命令:" << std::endl;
    std::cout << "  1. task copy -s /home/file.txt -d /backup/" << std::endl;
    std::cout << "  2. task calculate -x 2.5 -n 3 -v true" << std::endl;
    std::cout << "  3. task start -host localhost -port 8080 -debug yes -timeout 30.5" << std::endl;
    std::cout << "  4. task echo -m \"Hello World\"" << std::endl;
    std::cout << "  5. help (查看帮助) 或 list (列出任务)\n" << std::endl;

    user_input.start();

    return 0;
}
