// CompletionManager.cpp
#include "CompletionManager.h"
#include <algorithm>
#include <ranges>
#include <set>

CompletionManager::CompletionManager(std::map<std::string, std::shared_ptr<XTask>, std::less<>>& tasks) : tasks_(tasks)
{
}

CompletionManager::CompletionContext CompletionManager::analyzeInput(std::string_view input) const
{
    CompletionContext ctx;

    if (input.empty())
    {
        ctx.pathPart         = "";
        ctx.prefix           = "";
        ctx.isPathCompletion = false;
        return ctx;
    }

    size_t lastSpace = input.find_last_of(' ');

    if (lastSpace != std::string_view::npos)
    {
        ctx.prefix           = std::string(input.substr(0, lastSpace + 1));
        ctx.pathPart         = std::string(input.substr(lastSpace + 1));
        ctx.isPathCompletion = shouldCompletePath(ctx.pathPart);
    }
    else
    {
        ctx.pathPart         = std::string(input);
        ctx.isPathCompletion = shouldCompletePath(ctx.pathPart);
    }

    // 处理 task 命令的特殊情况
    if (input.starts_with("task "))
    {
        auto tokens = XTool::split(std::string(input));
        if (tokens.size() >= 3)
        {
            const std::string& lastToken = tokens.back();
            if (shouldCompletePath(lastToken))
            {
                size_t pos = input.rfind(lastToken);
                if (pos != std::string_view::npos)
                {
                    ctx.prefix           = std::string(input.substr(0, pos));
                    ctx.pathPart         = lastToken;
                    ctx.isPathCompletion = true;
                }
            }
        }
    }

    return ctx;
}

bool CompletionManager::shouldCompletePath(const std::string& lastPart) const
{
    return XFile::isPathInput(lastPart) || lastPart.find('/') != std::string::npos ||
            lastPart.find('\\') != std::string::npos || lastPart.find('.') != std::string::npos;
}

replxx::Replxx::completions_t CompletionManager::completionHook(const std::string& input, int& contextLen)
{
    replxx::Replxx::completions_t completions;
    contextLen = static_cast<int>(input.length());

    // 特殊处理空输入
    if (input.empty())
    {
        contextLen                                            = 0;
        static const std::vector<std::string> builtinCommands = { "exit", "help", "list" };
        for (const auto& cmd : builtinCommands)
        {
            completions.emplace_back(cmd, replxx::Replxx::Color::BRIGHTGREEN);
        }
        completions.emplace_back("task", replxx::Replxx::Color::YELLOW);
        return completions;
    }

    // 分析输入上下文
    CompletionContext ctx = analyzeInput(input);

    // 路径补全优先级最高
    if (ctx.isPathCompletion && !ctx.pathPart.empty())
    {
        handlePathCompletion(ctx, completions, contextLen);
        if (!completions.empty())
        {
            return completions;
        }
    }

    // Task命令补全
    if (input.starts_with("task "))
    {
        handleTaskCompletion(input, ctx, completions, contextLen);
    }
    else
    {
        handleCommandCompletion(input, ctx, completions);
    }

    return completions;
}

void CompletionManager::handleTaskCompletion(const std::string& input, const CompletionContext& ctx,
                                             replxx::Replxx::completions_t& completions, int& contextLen)
{
    auto tokens = XTool::split(input);

    // 计算要替换的部分长度
    size_t lastSpace = input.find_last_of(' ');
    if (lastSpace != std::string::npos)
    {
        contextLen = static_cast<int>(input.length() - lastSpace - 1);
    }

    // =============== 特殊处理输入以空格结尾的情况 ===============
    if (input.back() == ' ')
    {
        contextLen = 0; // 在光标位置插入

        // 输入以空格结尾，分析已完成的部分
        if (tokens.size() == 1) // 例如: "task "
        {
            handleEmptyTaskInput(completions);
        }
        else if (tokens.size() == 2) // 例如: "task calculate "
        {
            handleTaskNameCompletion(tokens, completions);
        }
        else if (tokens.size() >= 3) // 例如: "task calculate -x 2.0 "
        {
            std::string taskName = tokens[1];
            auto        taskIt   = tasks_.find(taskName);
            if (taskIt != tasks_.end())
            {
                // 传入 input 参数
                handleTaskParamCompletion(tokens, taskIt->second, input, completions, contextLen);
            }
        }
        return; // 直接返回，不再执行后续逻辑
    }
    // =============== 特殊处理结束 ===============

    if (tokens.size() == 1) // 只有 "task"
    {
        handleEmptyTaskInput(completions);
    }
    else if (tokens.size() == 2) // task <任务名>
    {
        handleTaskNameCompletion(tokens, completions);
    }
    else if (tokens.size() >= 3) // task <任务名> <参数...>
    {
        std::string taskName = tokens[1];
        auto        taskIt   = tasks_.find(taskName);
        if (taskIt != tasks_.end())
        {
            // 传入 input 参数
            handleTaskParamCompletion(tokens, taskIt->second, input, completions, contextLen);
        }
    }
}

void CompletionManager::handleEmptyTaskInput(replxx::Replxx::completions_t& completions)
{
    for (const auto& [name, _] : tasks_)
    {
        completions.emplace_back(name, replxx::Replxx::Color::BRIGHTBLUE);
    }
}

void CompletionManager::handleTaskNameCompletion(const std::vector<std::string>& tokens,
                                                 replxx::Replxx::completions_t&  completions)
{
    std::string taskPart      = tokens[1];
    bool        hasExactMatch = false;

    for (const auto& [name, _] : tasks_)
    {
        if (name == taskPart)
        {
            hasExactMatch = true;
            break;
        }
    }

    if (!hasExactMatch)
    {
        for (const auto& [name, _] : tasks_)
        {
            if (name.starts_with(taskPart))
            {
                completions.emplace_back(name, replxx::Replxx::Color::BRIGHTBLUE);
            }
        }
    }
    else
    {
        auto taskIt = tasks_.find(taskPart);
        if (taskIt != tasks_.end())
        {
            for (const auto& param : taskIt->second->getParameters())
            {
                completions.emplace_back(param.getName(), replxx::Replxx::Color::CYAN);
            }
        }
    }
}

void CompletionManager::handleTaskParamCompletion(const std::vector<std::string>& tokens, std::shared_ptr<XTask> task,
                                                  const std::string&             originalInput,
                                                  replxx::Replxx::completions_t& completions, int& contextLen)
{
    const auto& taskParams = task->getParameters();

    // =============== 首先检查输入是否以空格结尾 ===============
    bool inputEndsWithSpace = !originalInput.empty() && originalInput.back() == ' ';

    // 收集已使用的参数名
    std::set<std::string> usedParams;

    for (size_t i = 2; i < tokens.size(); i++)
    {
        if (tokens[i].starts_with('-'))
        {
            usedParams.insert(tokens[i]);
        }
    }

    // 如果输入以空格结尾，直接显示未使用的参数
    if (inputEndsWithSpace)
    {
        for (const auto& param : taskParams)
        {
            if (!usedParams.contains(param.getName()))
            {
                completions.emplace_back(param.getName(), replxx::Replxx::Color::CYAN);
            }
        }
        return; // 直接返回
    }

    // 继续处理非空格结尾的情况
    std::string lastToken = tokens.back();

    // 收集参数值的配对信息
    std::set<std::string> paramsWithValue;
    std::set<std::string> fileParams; // 文件参数
    std::set<std::string> dirParams;  // 目录参数

    for (size_t i = 2; i < tokens.size(); i++)
    {
        if (tokens[i].starts_with('-'))
        {
            // 查找参数定义
            auto paramIt =
                    std::ranges::find_if(taskParams, [&tokens, i](const auto& p) { return p.getName() == tokens[i]; });

            if (paramIt != taskParams.end())
            {
                // 检查参数类型
                if (paramIt->getType() == Parameter::Type::File)
                {
                    fileParams.insert(tokens[i]);
                }
                else if (paramIt->getType() == Parameter::Type::Directory)
                {
                    dirParams.insert(tokens[i]);
                }
            }

            // 如果下一个token存在且不是参数名，则这个参数已经有值了
            if (i + 1 < tokens.size() && !tokens[i + 1].starts_with('-'))
            {
                paramsWithValue.insert(tokens[i]);
                i++; // 跳过值
            }
        }
    }

    // 检查倒数第二个token是否是参数名
    bool            secondLastIsParam = false;
    std::string     secondLastToken;
    Parameter::Type secondLastParamType = Parameter::Type::String;

    if (tokens.size() >= 3)
    {
        secondLastToken = tokens[tokens.size() - 2];
        if (secondLastToken.starts_with('-'))
        {
            secondLastIsParam = true;

            // 查找这个参数的类型
            auto paramIt = std::ranges::find_if(taskParams, [&secondLastToken](const auto& p)
                                                { return p.getName() == secondLastToken; });

            if (paramIt != taskParams.end())
            {
                secondLastParamType = paramIt->getType();
            }
        }
    }

    // =============== 处理路径类型参数的特殊逻辑 ===============
    if (secondLastIsParam &&
        (secondLastParamType == Parameter::Type::File || secondLastParamType == Parameter::Type::Directory))
    {
        // 当前正在输入文件/目录路径
        // 让路径补全系统处理
        if (shouldCompletePath(lastToken))
        {
            // 路径补全
            return;
        }
        // 如果不是路径，应该显示参数值的补全
    }

    // 如果最后一个token以'-'开头，检查它是否已经配对了一个值
    if (lastToken.starts_with('-'))
    {
        // 检查这个参数是否已经有值
        if (paramsWithValue.contains(lastToken))
        {
            // 这个参数已经有值了，应该补全新参数
            for (const auto& param : taskParams)
            {
                if (!usedParams.contains(param.getName()))
                {
                    completions.emplace_back(param.getName(), replxx::Replxx::Color::CYAN);
                }
            }
        }
        else
        {
            // 检查这个参数名是否完全匹配
            bool isExactParam = false;
            for (const auto& param : taskParams)
            {
                if (param.getName() == lastToken)
                {
                    isExactParam = true;
                    break;
                }
            }

            if (isExactParam)
            {
                // 参数名完全匹配，应该显示参数值的补全
                auto paramIt = std::ranges::find_if(taskParams,
                                                    [&lastToken](const auto& p) { return p.getName() == lastToken; });

                if (paramIt != taskParams.end())
                {
                    auto paramCompletions = paramIt->getCompletions("");
                    for (const auto& comp : paramCompletions)
                    {
                        replxx::Replxx::Color color = replxx::Replxx::Color::MAGENTA;
                        if (paramIt->getType() == Parameter::Type::Int || paramIt->getType() == Parameter::Type::Double)
                        {
                            color = replxx::Replxx::Color::BRIGHTCYAN;
                        }
                        completions.emplace_back(comp, color);
                    }
                }
            }
            else
            {
                // 参数名部分匹配
                for (const auto& param : taskParams)
                {
                    if (!usedParams.contains(param.getName()) && param.getName().starts_with(lastToken))
                    {
                        completions.emplace_back(param.getName(), replxx::Replxx::Color::CYAN);
                    }
                }
            }
        }
    }
    else // 最后一个token不是以'-'开头
    {
        // 检查倒数第二个token是否是参数名
        if (tokens.size() >= 3)
        {
            std::string secondLastToken = tokens[tokens.size() - 2];
            if (secondLastToken.starts_with('-'))
            {
                // 检查这个参数是否已经有值（防止重复值）
                if (paramsWithValue.contains(secondLastToken))
                {
                    // 这个参数已经有值了，应该补全新参数
                    for (const auto& param : taskParams)
                    {
                        if (!usedParams.contains(param.getName()))
                        {
                            completions.emplace_back(param.getName(), replxx::Replxx::Color::CYAN);
                        }
                    }
                }
                else
                {
                    // 倒数第二个是参数名，当前是参数值
                    auto paramIt = std::ranges::find_if(taskParams, [&secondLastToken](const auto& p)
                                                        { return p.getName() == secondLastToken; });

                    if (paramIt != taskParams.end())
                    {
                        auto paramCompletions = paramIt->getCompletions(lastToken);

                        // 如果是字符串参数且需要路径补全
                        if (paramIt->getType() == Parameter::Type::String && shouldCompletePath(lastToken))
                        {
                            int                           tempContextLen = static_cast<int>(lastToken.length());
                            replxx::Replxx::completions_t pathCompletions;
                            completePathSmart(lastToken, pathCompletions, tempContextLen);
                            for (const auto& pc : pathCompletions)
                            {
                                completions.emplace_back(pc);
                            }
                        }

                        for (const auto& comp : paramCompletions)
                        {
                            replxx::Replxx::Color color = replxx::Replxx::Color::MAGENTA;
                            if (paramIt->getType() == Parameter::Type::Int ||
                                paramIt->getType() == Parameter::Type::Double)
                            {
                                color = replxx::Replxx::Color::BRIGHTCYAN;
                            }
                            completions.emplace_back(comp, color);
                        }
                    }
                }
            }
            else
            {
                // 倒数第二个token也不是参数名，说明这个token可能是一个错误的参数值
                // 或者用户可能想输入新参数
                for (const auto& param : taskParams)
                {
                    if (!usedParams.contains(param.getName()))
                    {
                        completions.emplace_back(param.getName(), replxx::Replxx::Color::CYAN);
                    }
                }
            }
        }
        else
        {
            // tokens.size() < 3，说明只有任务名和可能的参数值
            // 直接补全所有参数
            for (const auto& param : taskParams)
            {
                if (!usedParams.contains(param.getName()))
                {
                    completions.emplace_back(param.getName(), replxx::Replxx::Color::CYAN);
                }
            }
        }
    }
}

void CompletionManager::handleCommandCompletion(const std::string& input, const CompletionContext& ctx,
                                                replxx::Replxx::completions_t& completions)
{
    static const std::vector<std::string> builtinCommands = { "exit", "help", "list" };
    for (const auto& cmd : builtinCommands)
    {
        if (cmd.starts_with(ctx.pathPart))
        {
            completions.emplace_back(cmd, replxx::Replxx::Color::BRIGHTGREEN);
        }
    }

    if (std::string("task").starts_with(ctx.pathPart))
    {
        completions.emplace_back("task", replxx::Replxx::Color::YELLOW);
    }
}

void CompletionManager::handlePathCompletion(const CompletionContext& ctx, replxx::Replxx::completions_t& completions,
                                             int& contextLen)
{
    contextLen         = static_cast<int>(ctx.pathPart.length());
    int tempContextLen = contextLen;
    completePathSmart(ctx.pathPart, completions, tempContextLen);
    contextLen = tempContextLen;
}

void CompletionManager::completePathSmart(std::string_view partialPath, replxx::Replxx::completions_t& completions,
                                          int& contextLen)
{
    try
    {
        // 保存原始上下文长度（用户要替换的部分）
        int originalContextLen = contextLen;

        fs::path inputPath(partialPath);

        if (partialPath.empty())
        {
            inputPath  = ".";
            contextLen = 0;
        }

        // 确定搜索路径和匹配前缀
        fs::path    basePath;
        std::string matchPrefix;

        if (fs::exists(inputPath) && fs::is_directory(inputPath))
        {
            basePath    = inputPath;
            matchPrefix = "";
            contextLen  = 0; // 在目录后面追加，不替换任何内容
        }
        else
        {
            basePath    = inputPath.parent_path();
            matchPrefix = inputPath.filename().string();

            // 关键修复：当输入已经是完整文件名时，contextLen应该为0
            if (basePath.empty())
            {
                basePath = ".";
            }

            // 检查这个文件名是否已经存在
            fs::path testPath = basePath / matchPrefix;
            if (fs::exists(testPath) && fs::is_regular_file(testPath))
            {
                // 文件已经存在，不需要替换，直接返回空
                contextLen = 0;
                return;
            }

            // 否则，替换匹配前缀
            contextLen = static_cast<int>(matchPrefix.length());
        }

        basePath = fs::absolute(basePath);

        if (!fs::exists(basePath) || !fs::is_directory(basePath))
        {
            contextLen = originalContextLen; // 恢复原始值
            return;
        }

        // 收集补全项
        collectCompletions(basePath, matchPrefix, completions);

        // 如果只有一个补全项且完全匹配，设置contextLen为文件名长度
        if (completions.size() == 1)
        {
            const std::string& completionText = completions[0].text();
            if (completionText == matchPrefix || completionText == matchPrefix + XFile::separator())
            {
                contextLen = static_cast<int>(matchPrefix.length());
            }
        }

        // 排序（目录优先）
        sortCompletions(completions);
    }
    catch (...)
    {
        // 忽略错误，但要恢复contextLen
        contextLen = static_cast<int>(partialPath.length());
    }
}

void CompletionManager::collectCompletions(const fs::path& basePath, const std::string& matchPrefix,
                                           replxx::Replxx::completions_t& completions)
{
    for (const auto& entry : fs::directory_iterator(basePath, fs::directory_options::skip_permission_denied))
    {
        std::string name = entry.path().filename().string();

        if (!matchPrefix.empty() && !name.starts_with(matchPrefix))
        {
            continue;
        }

        if (!XFile::shouldShowHiddenFiles() && name[0] == '.')
        {
            continue;
        }

        addCompletion(entry, name, completions);
    }
}

void CompletionManager::addCompletion(const fs::directory_entry& entry, const std::string& name,
                                      replxx::Replxx::completions_t& completions)
{
    std::string           completion;
    replxx::Replxx::Color color = replxx::Replxx::Color::DEFAULT;

    if (fs::is_directory(entry.path()))
    {
        completion = name + XFile::separator();
        color      = replxx::Replxx::Color::BRIGHTBLUE;
    }
    else
    {
        completion = name;
        if (XFile::isExecutable(entry.path().string()))
        {
            color = replxx::Replxx::Color::BRIGHTGREEN;
        }
    }

    completions.emplace_back(completion, color);
}

void CompletionManager::sortCompletions(replxx::Replxx::completions_t& completions)
{
    std::sort(completions.begin(), completions.end(),
              [](const auto& a, const auto& b)
              {
                  bool aIsDir = !a.text().empty() && (a.text().back() == '/' || a.text().back() == '\\');
                  bool bIsDir = !b.text().empty() && (b.text().back() == '/' || b.text().back() == '\\');

                  if (aIsDir != bIsDir)
                      return aIsDir > bIsDir;
                  return a.text() < b.text();
              });
}

replxx::Replxx::hints_t CompletionManager::hintHook(const std::string& input, int& contextLen,
                                                    replxx::Replxx::Color& color)
{
    replxx::Replxx::hints_t hints;
    color = replxx::Replxx::Color::GRAY;

    if (input.empty())
        return hints;

    std::string pathPart = extractPathPartForHint(input);
    if (pathPart.empty())
        return hints;

    try
    {
        fs::path p(pathPart);

        if (fs::exists(p))
        {
            if (fs::is_directory(p))
            {
                int count = std::distance(fs::directory_iterator(p), fs::directory_iterator{});
                hints.push_back(" [目录: " + std::to_string(count) + " 个项目]");
            }
            else if (fs::is_regular_file(p))
            {
                try
                {
                    auto size = fs::file_size(p);
                    hints.push_back(" [文件: " + XFile::formatFileSize(size) + "]");
                }
                catch (...)
                {
                    hints.emplace_back(" [文件]");
                }
            }
        }
        else if (shouldCompletePath(pathPart))
        {
            hints.emplace_back(" [按 Tab 键补全]");
        }
    }
    catch (...)
    {
        if (!pathPart.empty())
        {
            hints.emplace_back(" [路径]");
        }
    }

    return hints;
}

std::string CompletionManager::extractPathPartForHint(const std::string& input) const
{
    return XFile::extractPathPart(input);
}
