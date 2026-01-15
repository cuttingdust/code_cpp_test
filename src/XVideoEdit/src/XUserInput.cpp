#include "XUserInput.h"

#include "linenoise.hpp"
#include "TaskFacory.h"

#include <ranges>
#include <vector>
#include <sstream>
#include <iostream>


/// \brief 分割字符串为子字符串向量
static auto split(const std::string &input, char delimiter = ' ', bool trimWhitespace = true)
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
static auto split(const std::string &input, const std::string &delimiter, bool trimWhitespace = true)
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

static bool                              gIsRunning = true;
static std::map<std::string, XTask::Ptr> gTasks;

//////////////////////////////////////////////////////////////////

/// 补全回调函数的实现
void completionCallback(const char *prefix, std::vector<std::string> &completions)
{
    std::string input(prefix);

    /// 1. 所有可能的完整命令
    std::vector<std::string> allCommands;

    /// 内置命令
    allCommands.insert(allCommands.end(), { "exit", "help", "list" });

    /// 任务命令（带 "task " 前缀）
    for (const auto &taskName : gTasks | std::views::keys)
    {
        allCommands.push_back("task " + taskName);
    }

    /// 2. 过滤：命令是否以用户输入开头？
    for (const auto &cmd : allCommands)
    {
        if (cmd.starts_with(input))
        {
            completions.push_back(cmd);
        }
    }

    /// 3. 特殊情况：如果用户输入正好是 "task"，额外建议 "task "（带空格）
    if (input == "task")
    {
        completions.emplace_back("task ");
    }
}

//////////////////////////////////////////////////////////////////


auto XUserInput::start() -> void
{
    /// 1. 设置补全回调
    linenoise::SetCompletionCallback(completionCallback);

    /// 2. (可选) 加载历史记录
    linenoise::LoadHistory(".command_history");

    std::cout << "任务处理器已启动。输入 'exit' 退出，'help' 查看帮助，'list' 列出任务。" << std::endl;

    while (gIsRunning)
    {
        std::string line;
        /// 使用linenoise读取输入，提示符为 ">> "
        bool quit = linenoise::Readline(">> ", line);

        if (quit)
        { /// 用户可能按下了Ctrl+D/C
            std::cout << "\ngoodbye." << std::endl;
            break;
        }

        if (line.empty())
        {
            continue;
        }

        /// 3. 将输入添加到历史记录（可优化为只添加成功命令）
        linenoise::AddHistory(line.c_str());

        /// 4. 统一处理命令
        handleCommand(line);
    }

    /// (可选) 退出前保存历史
    linenoise::SaveHistory(".command_history");
}

auto XUserInput::stop() -> void
{
    gIsRunning = false;
}

auto XUserInput::registerTask(const std::string &name, const XTask::TaskFunc &func, const std::string &description)
        -> XTask &
{
    auto  task    = TaskFac->createTask(name, func, description);
    auto &taskRef = *task;
    gTasks[name]  = std::move(task);
    return taskRef;
}

auto XUserInput::handleCommand(const std::string &input) -> void
{
    if (input == "exit")
    {
        std::cout << "goodbye." << std::endl;
        gIsRunning = false;
    }
    else if (input == "help")
    {
        printHelp();
    }
    else if (input == "list")
    {
        listTasks();
    }
    else
    {
        /// 其他命令（主要是task命令）交给原来的processCommand
        processCommand(input);
    }
}

auto XUserInput::processCommand(const std::string &input) -> void
{
    if (input.starts_with("task "))
    {
        processTaskCommand(input);
    }
    else if (!input.empty())
    {
        /// 只有非内置、非task命令才会到达这里
        std::cout << "未知命令。输入 'help' 查看可用命令。" << std::endl;
    }
}

auto XUserInput::processTaskCommand(const std::string &input) -> void
{
    auto tokens = split(input);
    if (tokens.size() < 2)
    {
        std::cout << "格式: task <任务名> [-参数 值]..." << std::endl;
        return;
    }

    const std::string &taskName = tokens[1];
    if (!gTasks.contains(taskName))
    {
        std::cout << "未知任务: " << taskName << std::endl;
        return;
    }

    std::map<std::string, std::string> params;
    for (size_t i = 2; i < tokens.size(); ++i)
    {
        if (tokens[i][0] == '-')
        {
            const std::string &paramName  = tokens[i];
            std::string        paramValue = (i + 1 < tokens.size() && tokens[i + 1][0] != '-') ? tokens[++i] : "";
            params[paramName]             = paramValue;
        }
    }

    std::string error;
    if (gTasks[taskName]->execute(params, error))
    {
        std::cout << "任务 '" << taskName << "' 执行成功" << std::endl;
    }
    else
    {
        std::cout << "任务执行失败: " << error << std::endl;
        printTaskUsage(taskName);
    }
}

auto XUserInput::printTaskUsage(const std::string &taskName) const -> void
{
    if (!gTasks.contains(taskName))
        return;

    const auto &task = gTasks.at(taskName);
    std::cout << "\n用法: task " << taskName;

    for (const auto &param : task->getParameters())
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
        for (const auto &param : task->getParameters())
        {
            std::cout << "  " << param.getName() << " - " << param.getDescription();
            std::cout << " [" << param.getTypeName() << "]";
            if (param.isRequired())
                std::cout << " (必需)";
            std::cout << std::endl;
        }
    }
}

auto XUserInput::printHelp() const -> void
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

auto XUserInput::listTasks() const -> void
{
    std::cout << "\n已注册的任务 (" << gTasks.size() << "):" << std::endl;
    for (const auto &[name, task] : gTasks)
    {
        std::cout << "  - " << name;
        if (!task->getDescription().empty())
        {
            std::cout << ": " << task->getDescription();
        }
        std::cout << std::endl;
    }
}
