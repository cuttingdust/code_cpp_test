#include "CompletionManager.h"
#include "XFile.h"
#include "XTool.h"

#include <algorithm>
#include <ranges>
#include <set>

class CompletionManager::PImpl
{
public:
    PImpl(CompletionManager* owenr);
    PImpl(CompletionManager* owenr, XTask::List tasks);
    ~PImpl() = default;

public:
    auto handleBuiltCommand(Completions& completions) -> void;

    /// \brief 路径补全
    /// \param[in] ctx 上下文
    /// \param[out] completions
    /// \param[out] contextLen
    auto handlePathCompletion(const CompletionContext& ctx, Completions& completions, int& contextLen) -> void;

    auto handleCommandCompletion(const std::string_view& input, const CompletionContext& ctx, Completions& completions)
            -> void;

    auto handleTaskCompletion(const std::string_view& input, const CompletionContext& ctx, Completions& completions,
                              int& contextLen) -> void;

public:
    /// 路径补全
    auto completePathSmart(std::string_view partialPath, Completions& completions, int& contextLen) -> void;

    /// task 相关补全
    auto handleEmptyTaskInput(Completions& completions) -> void;

    auto handleTaskNameCompletion(const std::vector<std::string>& tokens, Completions& completions) -> void;

    auto handleTaskParamCompletion(const std::vector<std::string>& tokens, XTask::Ptr task,
                                   const std::string_view& originalInput, Completions& completions, int& contextLen)
            -> void;

public:
    auto collectCompletions(const fs::path& basePath, const std::string& matchPrefix, Completions& completions) -> void;

    auto addCompletion(const fs::directory_entry& entry, const std::string_view& name, Completions& completions)
            -> void;

    auto sortCompletions(Completions& completions) -> void;

    auto createContext(const std::string_view& input) const -> CompletionContext;

    auto shouldCompletePath(const std::string_view& lastPart) const -> bool;

    auto extractPathPartForHint(const std::string_view& input) const -> std::string;

public:
    CompletionManager*       owenr_ = nullptr;
    XTask::List              tasks_;
    std::vector<std::string> builtinCommands_;
};

CompletionManager::PImpl::PImpl(CompletionManager* owenr) : owenr_(owenr)
{
}

CompletionManager::PImpl::PImpl(CompletionManager* owenr, XTask::List tasks) : owenr_(owenr), tasks_(std::move(tasks))
{
}


auto CompletionManager::PImpl::handleBuiltCommand(Completions& completions) -> void
{
    for (const auto& cmd : builtinCommands_)
    {
        completions.emplace_back(cmd, replxx::Replxx::Color::BRIGHTGREEN);
    }
    completions.emplace_back("task", replxx::Replxx::Color::YELLOW);
}

auto CompletionManager::PImpl::handlePathCompletion(const CompletionContext& ctx, Completions& completions,
                                                    int& contextLen) -> void
{
    contextLen         = static_cast<int>(ctx.pathPart.length());
    int tempContextLen = contextLen;
    completePathSmart(ctx.pathPart, completions, tempContextLen);
    contextLen = tempContextLen;
}

auto CompletionManager::PImpl::handleCommandCompletion(const std::string_view& input, const CompletionContext& ctx,
                                                       Completions& completions) -> void
{
    /// 使用动态获取的内置命令列表
    for (const auto& cmd : builtinCommands_)
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

auto CompletionManager::PImpl::handleTaskCompletion(const std::string_view& input, const CompletionContext& ctx,
                                                    Completions& completions, int& contextLen) -> void
{
    auto tokens = XTool::split(input);

    /// 计算要替换的部分长度
    size_t lastSpace = input.find_last_of(' ');
    if (lastSpace != std::string::npos)
    {
        contextLen = static_cast<int>(input.length() - lastSpace - 1);
    }

    /// 特殊处理输入以空格结尾的情况
    if (input.back() == ' ')
    {
        contextLen = 0; /// 在光标位置插入

        if (tokens.size() == 1) /// 例如: "task "
        {
            handleEmptyTaskInput(completions);
        }
        else if (tokens.size() == 2) /// 例如: "task calculate "
        {
            handleTaskNameCompletion(tokens, completions);
        }
        else if (tokens.size() >= 3) /// 例如: "task calculate -x 2.0 "
        {
            std::string taskName = tokens[1];
            auto        taskIt   = tasks_.find(taskName);
            if (taskIt != tasks_.end())
            {
                handleTaskParamCompletion(tokens, taskIt->second, input, completions, contextLen);
            }
        }
        return;
    }

    if (tokens.size() == 1) /// 只有 "task"
    {
        handleEmptyTaskInput(completions);
    }
    else if (tokens.size() == 2) /// task <任务名>
    {
        handleTaskNameCompletion(tokens, completions);
    }
    else if (tokens.size() >= 3) /// task <任务名> <参数...>
    {
        std::string taskName = tokens[1];
        auto        taskIt   = tasks_.find(taskName);
        if (taskIt != tasks_.end())
        {
            handleTaskParamCompletion(tokens, taskIt->second, input, completions, contextLen);
        }
    }
}

auto CompletionManager::PImpl::completePathSmart(std::string_view partialPath, Completions& completions,
                                                 int& contextLen) -> void
{
    try
    {
        int      originalContextLen = contextLen;
        fs::path inputPath(partialPath);

        if (partialPath.empty())
        {
            inputPath  = ".";
            contextLen = 0;
        }

        fs::path    basePath;
        std::string matchPrefix;

        if (fs::exists(inputPath) && fs::is_directory(inputPath))
        {
            basePath    = inputPath;
            matchPrefix = "";
            contextLen  = 0;
        }
        else
        {
            basePath    = inputPath.parent_path();
            matchPrefix = inputPath.filename().string();

            if (basePath.empty())
            {
                basePath = ".";
            }

            fs::path testPath = basePath / matchPrefix;
            if (fs::exists(testPath) && fs::is_regular_file(testPath))
            {
                contextLen = 0;
                return;
            }

            contextLen = static_cast<int>(matchPrefix.length());
        }

        basePath = fs::absolute(basePath);

        if (!fs::exists(basePath) || !fs::is_directory(basePath))
        {
            contextLen = originalContextLen;
            return;
        }

        collectCompletions(basePath, matchPrefix, completions);

        if (completions.size() == 1)
        {
            const std::string& completionText = completions[0].text();
            if (completionText == matchPrefix || completionText == matchPrefix + XFile::separator())
            {
                contextLen = static_cast<int>(matchPrefix.length());
            }
        }

        sortCompletions(completions);
    }
    catch (...)
    {
        contextLen = static_cast<int>(partialPath.length());
    }
}

auto CompletionManager::PImpl::handleEmptyTaskInput(Completions& completions) -> void
{
    for (const auto& name : tasks_ | std::views::keys)
    {
        completions.emplace_back(name, replxx::Replxx::Color::BRIGHTBLUE);
    }
}

auto CompletionManager::PImpl::handleTaskNameCompletion(const std::vector<std::string>& tokens,
                                                        Completions&                    completions) -> void
{
    const std::string& taskPart      = tokens[1];
    bool               hasExactMatch = false;

    for (const auto& name : tasks_ | std::views::keys)
    {
        if (name == taskPart)
        {
            hasExactMatch = true;
            break;
        }
    }

    if (!hasExactMatch)
    {
        for (const auto& name : tasks_ | std::views::keys)
        {
            if (name.starts_with(taskPart))
            {
                completions.emplace_back(name, replxx::Replxx::Color::BRIGHTBLUE);
            }
        }
    }
    else
    {
        if (const auto taskIt = tasks_.find(taskPart); taskIt != tasks_.end())
        {
            for (const auto& param : taskIt->second->getParameters())
            {
                completions.emplace_back(param.getName(), replxx::Replxx::Color::CYAN);
            }
        }
    }
}

auto CompletionManager::PImpl::handleTaskParamCompletion(const std::vector<std::string>& tokens, XTask::Ptr task,
                                                         const std::string_view& originalInput,
                                                         Completions& completions, int& contextLen) -> void
{
    const auto& taskParams = task->getParameters();

    /// 检查输入是否以空格结尾
    bool inputEndsWithSpace = !originalInput.empty() && originalInput.back() == ' ';

    /// 收集已使用的参数名
    std::set<std::string> usedParams;
    for (size_t i = 2; i < tokens.size(); i++)
    {
        if (tokens[i].starts_with('-'))
        {
            usedParams.insert(tokens[i]);
        }
    }

    /// 如果输入以空格结尾，显示未使用的参数
    if (inputEndsWithSpace)
    {
        for (const auto& param : taskParams)
        {
            if (!usedParams.contains(param.getName()))
            {
                completions.emplace_back(param.getName(), replxx::Replxx::Color::CYAN);
            }
        }
        return;
    }

    std::string lastToken = tokens.back();

    /// 收集参数值的配对信息
    std::set<std::string> paramsWithValue;
    for (size_t i = 2; i < tokens.size(); i++)
    {
        if (tokens[i].starts_with('-'))
        {
            if (i + 1 < tokens.size() && !tokens[i + 1].starts_with('-'))
            {
                paramsWithValue.insert(tokens[i]);
                i++; /// 跳过值
            }
        }
    }

    /// 检查倒数第二个token是否是参数名
    bool            secondLastIsParam = false;
    std::string     secondLastToken;
    Parameter::Type secondLastParamType = Parameter::Type::String;

    if (tokens.size() >= 3)
    {
        secondLastToken = tokens[tokens.size() - 2];
        if (secondLastToken.starts_with('-'))
        {
            secondLastIsParam = true;
            auto paramIt      = std::ranges::find_if(taskParams, [&secondLastToken](const auto& p)
                                                     { return p.getName() == secondLastToken; });

            if (paramIt != taskParams.end())
            {
                secondLastParamType = paramIt->getType();
            }
        }
    }

    /// 处理路径类型参数的特殊逻辑
    if (secondLastIsParam &&
        (secondLastParamType == Parameter::Type::File || secondLastParamType == Parameter::Type::Directory))
    {
        if (shouldCompletePath(lastToken))
        {
            /// 路径补全
            return;
        }
    }

    /// 如果最后一个token以'-'开头
    if (lastToken.starts_with('-'))
    {
        /// 检查这个参数是否已经有值
        if (paramsWithValue.contains(lastToken))
        {
            /// 补全新参数
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
            /// 检查这个参数名是否完全匹配
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
                /// 参数名完全匹配，显示参数值的补全
                auto paramIt = std::ranges::find_if(taskParams,
                                                    [&lastToken](const auto& p) { return p.getName() == lastToken; });

                if (paramIt != taskParams.end())
                {
                    for (const auto& comp : paramIt->getCompletions(""))
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
                // 检查这个参数是否已经有值
                if (paramsWithValue.contains(secondLastToken))
                {
                    // 补全新参数
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
                        // 使用参数自身的补全建议
                        auto paramCompletions = paramIt->getCompletions(lastToken);

                        // 如果是文件/目录类型且输入看起来像路径，让路径补全处理
                        if ((paramIt->getType() == Parameter::Type::File ||
                             paramIt->getType() == Parameter::Type::Directory ||
                             paramIt->getType() == Parameter::Type::String) &&
                            shouldCompletePath(lastToken))
                        {
                            int         tempContextLen = static_cast<int>(lastToken.length());
                            Completions pathCompletions;
                            completePathSmart(lastToken, pathCompletions, tempContextLen);
                            for (const auto& pc : pathCompletions)
                            {
                                completions.emplace_back(pc);
                            }
                        }
                    }
                }
            }
            else
            {
                // 补全新参数
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

auto CompletionManager::PImpl::collectCompletions(const fs::path& basePath, const std::string& matchPrefix,
                                                  Completions& completions) -> void
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

auto CompletionManager::PImpl::addCompletion(const fs::directory_entry& entry, const std::string_view& name,
                                             Completions& completions) -> void
{
    std::string           completion;
    replxx::Replxx::Color color = replxx::Replxx::Color::DEFAULT;

    if (fs::is_directory(entry.path()))
    {
        completion = std::string{ name } + XFile::separator();
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

auto CompletionManager::PImpl::sortCompletions(Completions& completions) -> void
{
    std::ranges::sort(completions,
                      [](const auto& a, const auto& b)
                      {
                          bool aIsDir = !a.text().empty() && (a.text().back() == '/' || a.text().back() == '\\');
                          bool bIsDir = !b.text().empty() && (b.text().back() == '/' || b.text().back() == '\\');

                          if (aIsDir != bIsDir)
                              return aIsDir > bIsDir;
                          return a.text() < b.text();
                      });
}

auto CompletionManager::PImpl::createContext(const std::string_view& input) const -> CompletionContext
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
        ctx.prefix           = input.substr(0, lastSpace + 1);
        ctx.pathPart         = input.substr(lastSpace + 1);
        ctx.isPathCompletion = shouldCompletePath(ctx.pathPart);
    }
    else
    {
        ctx.pathPart         = input;
        ctx.isPathCompletion = shouldCompletePath(ctx.pathPart);
    }

    /// 处理 task 命令的特殊情况
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
                    ctx.prefix           = input.substr(0, pos);
                    ctx.pathPart         = lastToken;
                    ctx.isPathCompletion = true;
                }
            }
        }
    }

    return ctx;
}

auto CompletionManager::PImpl::shouldCompletePath(const std::string_view& lastPart) const -> bool
{
    return XFile::isPathInput(lastPart) || lastPart.find('.') != std::string::npos;
}

auto CompletionManager::PImpl::extractPathPartForHint(const std::string_view& input) const -> std::string
{
    return XFile::extractPathPart(input);
}

CompletionManager::CompletionManager() : impl_(std::make_unique<CompletionManager::PImpl>(this))
{
}

CompletionManager::CompletionManager(XTask::List& tasks) :
    impl_(std::make_unique<CompletionManager::PImpl>(this, tasks))
{
}

CompletionManager::~CompletionManager() = default;

auto CompletionManager::registerBuiltinCommand(const std::string_view& command) -> void
{
    if (std::ranges::find(impl_->builtinCommands_, command) == impl_->builtinCommands_.end())
    {
        impl_->builtinCommands_.emplace_back(command);
        std::ranges::sort(impl_->builtinCommands_);
        impl_->builtinCommands_.erase(std::ranges::unique(impl_->builtinCommands_).begin(),
                                      impl_->builtinCommands_.end());
    }
}

auto CompletionManager::getBuiltinCommands() const -> const std::vector<std::string>&
{
    return impl_->builtinCommands_;
}

auto CompletionManager::completionHook(const std::string_view& input, int& contextLen) -> Completions
{
    Completions completions;
    contextLen = static_cast<int>(input.length());

    /// 特殊处理空输入
    if (input.empty())
    {
        contextLen = 0;
        impl_->handleBuiltCommand(completions);
        return completions;
    }

    /// 分析输入上下文
    CompletionContext ctx = impl_->createContext(input);
    if (ctx.isPathCompletion && !ctx.pathPart.empty())
    {
        /// 路径补全优先级最高
        impl_->handlePathCompletion(ctx, completions, contextLen);
        if (!completions.empty())
        {
            return completions;
        }
    }

    /// Task命令补全
    if (input.starts_with("task "))
    {
        impl_->handleTaskCompletion(input, ctx, completions, contextLen);
    }
    else
    {
        impl_->handleCommandCompletion(input, ctx, completions);
    }

    return completions;
}

auto CompletionManager::hintHook(const std::string_view& input, int& contextLen, Color& color) -> Hints
{
    Hints hints;
    color = Color::GRAY;

    if (input.empty())
        return hints;

    std::string pathPart = impl_->extractPathPartForHint(input);
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
        else if (impl_->shouldCompletePath(pathPart))
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

auto CompletionManager::setTaskList(const XTask::List& tasks) -> void
{
    impl_->tasks_ = tasks;
}

auto CompletionManager::registerTaskCommand(const std::string_view& command, const XTask::Ptr& task) -> void
{
    if (!impl_->tasks_.contains(command) && task)
    {
        impl_->tasks_.emplace(command, task);
    }
}
