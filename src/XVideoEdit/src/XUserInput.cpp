#include "XUserInput.h"
#include "TaskFacory.h"
#include "XFile.h"
#include "XTool.h"

#include <replxx.hxx>

#include <iostream>
#include <algorithm>
#include <ranges>

class XUserInput::Impl
{
public:
    Impl();
    ~Impl();

public:
    auto start() -> void;
    auto stop() -> void;

    auto registerTask(const std::string& name, const XTask::TaskFunc& func, const std::string& description) -> XTask&;
    auto handleCommand(const std::string& input) -> void;
    void processCommand(const std::string& input);
    void processTaskCommand(const std::string& input);
    void fallbackSimpleInput();

    auto printHelp() const -> void;
    auto listTasks() const -> void;
    auto printTaskUsage(const std::string& taskName) const -> void;

private:
    /// 补全相关函数
    replxx::Replxx::completions_t completionHook(const std::string& input, int& contextLen);
    replxx::Replxx::hints_t       hintHook(const std::string& input, int& contextLen, replxx::Replxx::Color& color);

private:
    void completePathSmart(const std::string& partialPath, replxx::Replxx::completions_t& completions);

private:
    std::unique_ptr<replxx::Replxx>   rx_;
    bool                              isRunning_ = true;
    std::map<std::string, XTask::Ptr> tasks_;
    std::string                       prompt_ = "\x1b[1;32m>>\x1b[0m "; /// 使用带颜色的提示
};

XUserInput::Impl::Impl()
{
    rx_ = std::make_unique<replxx::Replxx>();
}

XUserInput::Impl::~Impl() = default;


auto XUserInput::Impl::start() -> void
{
    /// 检查是否在交互式终端中
    if (!XTool::isInteractiveTerminal())
    {
        std::cout << "不在交互式终端中，使用简单模式" << std::endl;
        fallbackSimpleInput();
        return;
    }

    try
    {
        /// 安装窗口变化处理器
        rx_->install_window_change_handler();
        std::string historyFile = ".command_history";
        if (fs::exists(historyFile))
        {
            try
            {
                rx_->history_load(historyFile);
            }
            catch (...)
            {
                /// 忽略加载错误
            }
        }

        /// 配置replxx
        rx_->set_max_history_size(128);
        rx_->set_max_hint_rows(3);
        rx_->set_completion_count_cutoff(128);

        /// 设置单词分隔符，注意不要包含路径分隔符
#ifdef _WIN32
        rx_->set_word_break_characters(" \t\n-%!;:=*~^<>?|&()"); /// Windows路径包含反斜杠
#else
        rx_->set_word_break_characters(" \t\n-%!;:=*~^<>?|&()"); /// Unix路径包含斜杠
#endif

        rx_->set_complete_on_empty(false);
        rx_->set_beep_on_ambiguous_completion(false);
        rx_->set_no_color(false);
        rx_->set_double_tab_completion(false);
        rx_->set_indent_multiline(false);

        /// 设置回调函数
        rx_->set_completion_callback([this](const std::string& input, int& contextLen)
                                     { return this->completionHook(input, contextLen); });

        /// 启用提示
        rx_->set_hint_callback([this](const std::string& input, int& contextLen, replxx::Replxx::Color& color)
                               { return this->hintHook(input, contextLen, color); });

        // 设置提示延迟
        rx_->set_hint_delay(500); // 500毫秒

        // 绑定快捷键
        rx_->bind_key_internal(replxx::Replxx::KEY::control('L'), "clear_screen");
        rx_->bind_key_internal(replxx::Replxx::KEY::BACKSPACE, "delete_character_left_of_cursor");
        rx_->bind_key_internal(replxx::Replxx::KEY::DELETE, "delete_character_under_cursor");
        rx_->bind_key_internal(replxx::Replxx::KEY::control('D'), "send_eof");
        rx_->bind_key_internal(replxx::Replxx::KEY::control('C'), "abort_line");
        rx_->bind_key_internal(replxx::Replxx::KEY::TAB, "complete_line");

        std::cout << "任务处理器已启动。" << std::endl;
        std::cout << "提示：按 Tab 键补全，Ctrl+L 清屏。" << std::endl;
        std::cout << "输入 'help' 查看帮助，'exit' 退出程序。" << std::endl;
        std::cout << std::endl;

        /// 主循环
        while (isRunning_)
        {
            char const* line = nullptr;

            do
            {
                line = rx_->input(prompt_);
            }
            while ((line == nullptr) && (errno == EAGAIN));

            if (line == nullptr)
            {
                /// EOF或用户取消
                std::cout << "\n再见。" << std::endl;
                break;
            }

            std::string input(line);

            if (input.empty())
            {
                continue;
            }

            // 添加到历史
            rx_->history_add(input.c_str());

            // 处理命令
            handleCommand(input);
        }

        // 保存历史
        try
        {
            rx_->history_save(".command_history");
        }
        catch (...)
        {
            // 忽略保存错误
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Replxx初始化失败: " << e.what() << std::endl;
        std::cerr << "切换到简单输入模式..." << std::endl;
        fallbackSimpleInput();
    }
    catch (...)
    {
        std::cerr << "未知错误初始化Replxx" << std::endl;
        fallbackSimpleInput();
    }
}

void XUserInput::Impl::fallbackSimpleInput()
{
    std::cout << "使用简单输入模式..." << std::endl;
    std::cout << "提示：输入 'help' 查看帮助，'exit' 退出程序。" << std::endl;

    while (isRunning_)
    {
        std::cout << ">> ";
        std::cout.flush();

        std::string input;
        if (!std::getline(std::cin, input))
        {
            std::cout << "\n再见。" << std::endl;
            break;
        }

        if (input.empty())
        {
            continue;
        }

        handleCommand(input);
    }
}

auto XUserInput::Impl::stop() -> void
{
    isRunning_ = false;
}

/// 主补全函数
replxx::Replxx::completions_t XUserInput::Impl::completionHook(const std::string& input, int& contextLen)
{
    replxx::Replxx::completions_t completions;
    replxx::Replxx::completions_t commands;

    /// 上下文长度
    contextLen = static_cast<int>(input.length());
    if (input.empty())
    {
        return commands;
    }

    std::string pathPart = XFile::extractPathPart(input);
    std::string prePart  = input.substr(0, input.length() - pathPart.length());
    if (!pathPart.empty())
    {
        bool shouldCompletePath = false;

        /// 情况1：整个输入看起来像路径
        if (XFile::isPathInput(input))
        {
            shouldCompletePath = true;
        }
        /// 情况2：最后一部分是路径
        else
        {
            /// 查找最后一个空格
            size_t lastSpace = input.find_last_of(' ');
            if (lastSpace != std::string::npos)
            {
                std::string lastPart = input.substr(lastSpace + 1);

                /// 如果最后一部分是路径或包含路径特征
                if (XFile::isPathInput(lastPart) || lastPart.find('/') != std::string::npos ||
                    lastPart.find('\\') != std::string::npos || lastPart.find('.') != std::string::npos)
                {
                    shouldCompletePath = true;
                }
            }
            /// 情况3：没有空格，但输入包含路径分隔符
            else if (input.find('/') != std::string::npos || input.find('\\') != std::string::npos ||
                     input.find('.') != std::string::npos)
            {
                shouldCompletePath = true;
            }
        }

        if (shouldCompletePath)
        {
            completePathSmart(pathPart, commands);

            /// 如果有路径补全，直接返回
            if (!commands.empty())
            {
                for (auto completion : commands)
                {
                    completion = prePart + completion.text();
                    completions.emplace_back(completion);
                }

                return completions;
            }
        }
    }

    if (input == "e" || input == "ex" || input == "exi")
    {
        completions.emplace_back("exit");
    }
    else if (input == "h" || input == "he" || input == "hel")
    {
        completions.emplace_back("help");
    }
    else if (input == "l" || input == "li" || input == "lis")
    {
        completions.emplace_back("list");
    }
    // 检查是否是 task 命令
    else if (input.starts_with("task"))
    {
        auto tokens = XTool::split(input);

        if (tokens.empty())
        {
            /// 不应该发生
        }
        else if (tokens.size() == 1)
        {
            /// 只有 "task"，可能后面要输入任务名
            /// 这里我们不补全，让用户继续输入
        }
        else if (tokens.size() == 2)
        {
            /// "task 任务名" - 补全任务名
            std::string prefix = tokens[1];
            for (const auto& name : tasks_ | std::views::keys)
            {
                if (prefix.empty() || name.starts_with(prefix))
                {
                    completions.emplace_back(name);
                }
            }
        }
        else
        {
            // "task 任务名 参数" - 检查最后一个参数是否是路径
            const std::string& lastToken = tokens.back();

            // 如果最后一个token是路径，进行路径补全
            if (XFile::isPathInput(lastToken) || lastToken.find('/') != std::string::npos ||
                lastToken.find('\\') != std::string::npos || lastToken.find('.') != std::string::npos)
            {
                // 重新提取路径部分
                std::string newPathPart = XFile::extractPathPart(input);
                if (!newPathPart.empty())
                {
                    completePathSmart(newPathPart, completions);
                }
            }
        }
    }
    // 检查是否是单个任务名（没有task前缀）
    else
    {
        std::string prefix = input;
        for (const auto& name : tasks_ | std::views::keys)
        {
            if (name.starts_with(prefix))
            {
                completions.emplace_back(name);
            }
        }
    }

    return completions;
}

void XUserInput::Impl::completePathSmart(const std::string& partialPath, replxx::Replxx::completions_t& completions)
{
    try
    {
        fs::path inputPath(partialPath);

        /// 处理空输入
        if (partialPath.empty())
        {
            inputPath = ".";
        }

        /// 确定基路径和前缀
        fs::path    basePath;
        std::string prefix;

        if (fs::exists(inputPath) && fs::is_directory(inputPath))
        {
            basePath = inputPath;
            prefix   = "";
        }
        else
        {
            basePath = inputPath.parent_path();
            prefix   = inputPath.filename().string();

            if (basePath.empty())
            {
                basePath = ".";
            }
        }

        /// 转换为绝对路径
        basePath = fs::absolute(basePath);

        /// 检查基路径是否存在
        if (!fs::exists(basePath) || !fs::is_directory(basePath))
        {
            return;
        }

        /// 遍历目录
        for (const auto& entry : fs::directory_iterator(basePath, fs::directory_options::skip_permission_denied))
        {
            std::string name = entry.path().filename().string();

            /// 跳过不匹配前缀的文件
            if (!prefix.empty() && !name.starts_with(prefix))
            {
                continue;
            }

            /// 跳过隐藏文件
            if (!XFile::shouldShowHiddenFiles() && name[0] == '.')
            {
                continue;
            }

            /// 构建完整路径
            fs::path    fullPath = entry.path();
            std::string completion;

            if (fs::is_directory(fullPath))
            {
                /// 对于目录：构建相对于输入路径的完整路径
                if (partialPath.empty() || partialPath == ".")
                {
                    // 当前目录：显示 ./目录名/
                    completion = "./" + name + XFile::separator();
                }
                else if (partialPath.back() == fs::path::preferred_separator)
                {
                    // 输入以分隔符结尾：追加到当前路径
                    completion = partialPath + name + XFile::separator();
                }
                else
                {
                    // 部分路径输入：替换或追加
                    fs::path inputParent = fs::path(partialPath).parent_path();
                    if (inputParent.empty())
                    {
                        // 只有文件名，没有路径
                        completion = name + XFile::separator();
                    }
                    else
                    {
                        // 有父路径，构建完整路径
                        fs::path parentPath(inputParent);
                        fs::path resultPath = parentPath / name / "";
                        completion          = resultPath.string();
                    }
                }

                completions.emplace_back(completion, replxx::Replxx::Color::BRIGHTBLUE);
            }
            else if (fs::is_regular_file(fullPath))
            {
                // 对于文件：构建相对于输入路径的完整路径
                if (partialPath.empty() || partialPath == ".")
                {
                    // 当前目录：显示 ./文件名
                    completion = "./" + name;
                }
                else if (partialPath.back() == fs::path::preferred_separator)
                {
                    // 输入以分隔符结尾：追加到当前路径
                    completion = partialPath + name;
                }
                else
                {
                    // 部分路径输入：替换或追加
                    fs::path inputParent = fs::path(partialPath).parent_path();
                    if (inputParent.empty())
                    {
                        // 只有文件名，没有路径
                        completion = name;
                    }
                    else
                    {
                        // 有父路径，构建完整路径
                        fs::path parentPath(inputParent);
                        fs::path resultPath = parentPath / name;
                        completion          = resultPath.string();
                    }
                }

                // 根据文件类型设置颜色
                auto color = replxx::Replxx::Color::DEFAULT;
                if (XFile::isExecutable(fullPath.string()))
                {
                    color = replxx::Replxx::Color::BRIGHTGREEN;
                }

                completions.emplace_back(completion, color);
            }
        }

        // 排序补全项
        std::sort(completions.begin(), completions.end(),
                  [](const auto& a, const auto& b) { return a.text() < b.text(); });
    }
    catch (...)
    {
        // 忽略文件系统错误
    }
}

auto XUserInput::Impl::registerTask(const std::string& name, const XTask::TaskFunc& func,
                                    const std::string& description) -> XTask&
{
    auto  task    = TaskFac->createTask(name, func, description);
    auto& taskRef = *task;
    tasks_[name]  = std::move(task);
    return taskRef;
}

auto XUserInput::Impl::handleCommand(const std::string& input) -> void
{
    if (input == "exit")
    {
        std::cout << "再见。" << std::endl;
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
    else
    {
        processCommand(input);
    }
}

void XUserInput::Impl::processCommand(const std::string& input)
{
    if (input.starts_with("task "))
    {
        processTaskCommand(input);
    }
    else if (!input.empty())
    {
        std::cout << "未知命令。输入 'help' 查看可用命令。" << std::endl;
    }
}

void XUserInput::Impl::processTaskCommand(const std::string& input)
{
    auto tokens = XTool::split(input);
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

auto XUserInput::Impl::printHelp() const -> void
{
    std::cout << "\n=== 任务处理器帮助 ===" << std::endl;
    std::cout << "任务命令格式: task <任务名> [-参数1 值1] [-参数2 值2] ..." << std::endl;
    std::cout << "支持的类型: 字符串、整数、浮点数、布尔值(true/1/yes/on)" << std::endl;
    std::cout << "示例:" << std::endl;
    std::cout << "  task cv -input ./video.mp4 -output ./output.mp4" << std::endl;
    std::cout << "\n特殊命令:" << std::endl;
    std::cout << "  exit  - 退出程序" << std::endl;
    std::cout << "  help  - 显示此帮助" << std::endl;
    std::cout << "  list  - 列出所有注册的任务" << std::endl;
    std::cout << "================================\n" << std::endl;
}

auto XUserInput::Impl::listTasks() const -> void
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

auto XUserInput::Impl::printTaskUsage(const std::string& taskName) const -> void
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


replxx::Replxx::hints_t XUserInput::Impl::hintHook(const std::string& input, int& contextLen,
                                                   replxx::Replxx::Color& color)
{
    replxx::Replxx::hints_t hints;
    color = replxx::Replxx::Color::GRAY;

    if (input.empty())
    {
        return hints;
    }

    /// 使用相同的路径提取逻辑
    std::string pathPart = XFile::extractPathPart(input);

    if (pathPart.empty())
    {
        return hints;
    }

    // 为找到的路径部分显示提示
    try
    {
        fs::path p(pathPart);

        if (fs::exists(p))
        {
            if (fs::is_directory(p))
            {
                int count = 0;
                for (const auto& _ : fs::directory_iterator(p))
                    ++count;

                std::string hint = " [目录: " + std::to_string(count) + " 个项目]";
                hints.push_back(hint);
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
        else
        {
            /// 路径不存在，检查是否是部分路径
            if (!pathPart.empty() &&
                (pathPart.find('/') != std::string::npos || pathPart.find('\\') != std::string::npos ||
                 pathPart.find('.') != std::string::npos))
            {
                hints.emplace_back(" [按 Tab 键补全]");
            }
        }
    }
    catch (...)
    {
        /// 发生异常时显示通用提示
        if (!pathPart.empty())
        {
            hints.emplace_back(" [路径]");
        }
    }

    return hints;
}

XUserInput::XUserInput() : impl_(std::make_unique<Impl>())
{
}

XUserInput::~XUserInput() = default;

auto XUserInput::start() -> void
{
    impl_->start();
}

void XUserInput::stop()
{
    impl_->stop();
}

XTask& XUserInput::registerTask(const std::string& name, const XTask::TaskFunc& func, const std::string& description)
{
    return impl_->registerTask(name, func, description);
}
