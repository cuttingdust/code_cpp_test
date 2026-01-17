// CommandProcessor.cpp
#include "CommandProcessor.h"

CommandProcessor::CommandProcessor(std::map<std::string, std::shared_ptr<XTask>, std::less<>>& tasks) : tasks_(tasks)
{
}

void CommandProcessor::processCommand(std::string_view input)
{
    if (input == "exit")
    {
        std::cout << "再见。\n";
        isRunning_ = false;
    }
    else if (input == "help")
    {
        printHelp();
    }
    else if (input == "list")
    {
        listTasks();
    }
    else if (input.starts_with("task "))
    {
        processTaskCommand(input);
    }
    else if (!input.empty())
    {
        std::cout << "未知命令。输入 'help' 查看可用命令。\n";
    }
}

void CommandProcessor::processTaskCommand(std::string_view input)
{
    auto tokens = XTool::split(std::string(input));

    if (tokens.size() < 2)
    {
        std::cout << "格式: task <任务名> [-参数 值]...\n";
        return;
    }

    const std::string& taskName = tokens[1];
    auto               it       = tasks_.find(taskName);
    if (it == tasks_.end())
    {
        std::cout << "未知任务: " << taskName << "\n";
        return;
    }

    std::map<std::string, std::string> params;
    for (size_t i = 2; i < tokens.size(); ++i)
    {
        if (tokens[i][0] == '-')
        {
            std::string paramName        = tokens[i];
            std::string paramValue       = (i + 1 < tokens.size() && tokens[i + 1][0] != '-') ? tokens[++i] : "";
            params[std::move(paramName)] = std::move(paramValue);
        }
    }

    std::string error;
    if (it->second->execute(params, error))
    {
        std::cout << "任务 '" << taskName << "' 执行成功\n";
    }
    else
    {
        std::cout << "任务执行失败: " << error << "\n";
        printTaskUsage(taskName);
    }
}

void CommandProcessor::printHelp() const
{
    std::cout << "\n=== 任务处理器帮助 ===\n"
              << "任务命令格式: task <任务名> [-参数1 值1] [-参数2 值2] ...\n"
              << "支持的类型: 字符串、整数、浮点数、布尔值(true/1/yes/on)\n"
              << "示例:\n"
              << "  task cv -input ./video.mp4 -output ./output.mp4\n"
              << "\n特殊命令:\n"
              << "  exit  - 退出程序\n"
              << "  help  - 显示此帮助\n"
              << "  list  - 列出所有注册的任务\n"
              << "================================\n\n";
}

void CommandProcessor::listTasks() const
{
    std::cout << "\n已注册的任务 (" << tasks_.size() << "):\n";
    for (const auto& [name, task] : tasks_)
    {
        std::cout << "  - " << name;
        if (!task->getDescription().empty())
        {
            std::cout << ": " << task->getDescription();
        }
        std::cout << "\n";
    }
}

void CommandProcessor::printTaskUsage(std::string_view taskName) const
{
    auto it = tasks_.find(std::string(taskName));
    if (it == tasks_.end())
        return;

    const auto& task = it->second;
    std::cout << "\n用法: task " << taskName;

    for (const auto& param : task->getParameters())
    {
        std::cout << " " << param.getName();
        if (!param.isRequired())
            std::cout << " [值]";
        else
            std::cout << " <值>";
    }
    std::cout << "\n";

    if (!task->getDescription().empty())
    {
        std::cout << "描述: " << task->getDescription() << "\n";
    }

    if (!task->getParameters().empty())
    {
        std::cout << "参数:\n";
        for (const auto& param : task->getParameters())
        {
            std::cout << "  " << param.getName() << " - " << param.getDescription() << " [" << param.getTypeName()
                      << "]";
            if (param.isRequired())
                std::cout << " (必需)";
            std::cout << "\n";
        }
    }
}
