#include "XUserInput.h"
#include "TaskFacory.h"

#include <replxx.hxx>

#include <filesystem>
#include <ranges>
#include <vector>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <thread>
#include <chrono>
#include <corecrt_io.h>

namespace fs = std::filesystem;

// 内部实现类
class XUserInput::Impl
{
public:
    Impl();
    ~Impl();

    void start();
    void stop();

    XTask& registerTask(const std::string& name, const XTask::TaskFunc& func, const std::string& description);

    void handleCommand(const std::string& input);
    void processCommand(const std::string& input);
    void processTaskCommand(const std::string& input);
    void fallbackSimpleInput();

    void printHelp() const;
    void listTasks() const;
    void printTaskUsage(const std::string& taskName) const;

private:
    // 辅助函数
    static std::vector<std::string> split(const std::string& input, char delimiter = ' ', bool trimWhitespace = true);
    static std::vector<std::string> smartSplit(const std::string& input);
    static bool                     isPathInput(const std::string& input);
    static bool                     lastTokenIsPathParam(const std::vector<std::string>& tokens);

    // 补全相关函数
    replxx::Replxx::completions_t completionHook(const std::string& input, int& contextLen);
    replxx::Replxx::hints_t       hintHook(const std::string& input, int& contextLen, replxx::Replxx::Color& color);
    std::string                   formatFileSize(uintmax_t size);
    void completePath(const std::string& partialPath, replxx::Replxx::completions_t& completions);

    // 新增：检查终端是否交互式
    bool isInteractiveTerminal();

private:
    struct PathCompletionData
    {
        std::vector<std::string> directories;
        std::vector<std::string> files;
        std::vector<std::string> executables;
    };

    PathCompletionData collectPathCompletions(const fs::path& dir, const std::string& prefix);
    std::string        getCommonPrefix(const std::vector<std::string>& strings);
    bool               isExecutable(const fs::path& path);
    bool               shouldShowHiddenFiles() const;

    // 新增辅助函数：规范化路径显示
    std::string normalizeCompletionPath(const std::string& inputPath, const std::string& completion);
    // 新增：智能路径补全
    void completePathSmart(const std::string& partialPath, replxx::Replxx::completions_t& completions);

    std::string separator() const;

    std::string extractPathPart(const std::string& input) const;

private:
    std::unique_ptr<replxx::Replxx>   rx_;
    bool                              isRunning_ = true;
    std::map<std::string, XTask::Ptr> tasks_;
    std::string                       prompt_ = "\x1b[1;32m>>\x1b[0m "; // 使用带颜色的提示
};

// 构造函数和析构函数
XUserInput::Impl::Impl()
{
    rx_ = std::make_unique<replxx::Replxx>();
}

XUserInput::Impl::~Impl() = default;

bool XUserInput::Impl::isInteractiveTerminal()
{
#ifdef _WIN32
    return _isatty(_fileno(stdin)) && _isatty(_fileno(stdout));
#else
    return isatty(fileno(stdin)) && isatty(fileno(stdout));
#endif
}

void XUserInput::Impl::start()
{
    // 检查是否在交互式终端中
    if (!isInteractiveTerminal())
    {
        std::cout << "不在交互式终端中，使用简单模式" << std::endl;
        fallbackSimpleInput();
        return;
    }

    try
    {
        // 安装窗口变化处理器（重要！）
        rx_->install_window_change_handler();

        // 先加载历史（如果有）
        std::string historyFile = ".command_history";
        if (fs::exists(historyFile))
        {
            try
            {
                rx_->history_load(historyFile);
            }
            catch (...)
            {
                // 忽略加载错误
            }
        }

        // 配置replxx
        rx_->set_max_history_size(128);
        rx_->set_max_hint_rows(3);
        rx_->set_completion_count_cutoff(128);

        // 设置单词分隔符，注意不要包含路径分隔符
#ifdef _WIN32
        rx_->set_word_break_characters(" \t\n-%!;:=*~^<>?|&()"); // Windows路径包含反斜杠
#else
        rx_->set_word_break_characters(" \t\n-%!;:=*~^<>?|&()"); // Unix路径包含斜杠
#endif

        rx_->set_complete_on_empty(false);
        rx_->set_beep_on_ambiguous_completion(false);
        rx_->set_no_color(false);
        rx_->set_double_tab_completion(false);
        rx_->set_indent_multiline(false);

        // 设置回调函数
        rx_->set_completion_callback([this](const std::string& input, int& contextLen)
                                     { return this->completionHook(input, contextLen); });

        // 启用提示
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

        std::cout << "任务处理器已启动（使用 replxx）。" << std::endl;
        std::cout << "提示：按 Tab 键补全，Ctrl+L 清屏。" << std::endl;
        std::cout << "输入 'help' 查看帮助，'exit' 退出程序。" << std::endl;
        std::cout << std::endl;

        // 主循环
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
                // EOF或用户取消
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

void XUserInput::Impl::stop()
{
    isRunning_ = false;
}

// 主补全函数
replxx::Replxx::completions_t XUserInput::Impl::completionHook(const std::string& input, int& contextLen)
{
    replxx::Replxx::completions_t completions;
    replxx::Replxx::completions_t commands;

    // 上下文长度
    contextLen = static_cast<int>(input.length());

    if (input.empty())
    {
        return commands;
    }

    // 使用 extractPathPart 提取可能的路径部分
    std::string pathPart = extractPathPart(input);
    std::string prePart  = input.substr(0, input.length() - pathPart.length());

    // 调试信息（可选）
    // std::cout << "DEBUG: input='" << input << "', pathPart='" << pathPart << "'" << std::endl;

    // 如果有路径部分，进行路径补全
    if (!pathPart.empty())
    {
        // 检查是否应该进行路径补全
        // 规则：最后一个部分是路径，或者输入以路径分隔符结尾
        bool shouldCompletePath = false;

        // 情况1：整个输入看起来像路径
        if (isPathInput(input))
        {
            shouldCompletePath = true;
        }
        // 情况2：最后一部分是路径
        else
        {
            // 查找最后一个空格
            size_t lastSpace = input.find_last_of(' ');
            if (lastSpace != std::string::npos)
            {
                std::string lastPart = input.substr(lastSpace + 1);

                // 如果最后一部分是路径或包含路径特征
                if (isPathInput(lastPart) || lastPart.find('/') != std::string::npos ||
                    lastPart.find('\\') != std::string::npos || lastPart.find('.') != std::string::npos)
                {
                    shouldCompletePath = true;
                }
            }
            // 情况3：没有空格，但输入包含路径分隔符
            else if (input.find('/') != std::string::npos || input.find('\\') != std::string::npos ||
                     input.find('.') != std::string::npos)
            {
                shouldCompletePath = true;
            }
        }

        if (shouldCompletePath)
        {
            completePathSmart(pathPart, commands);

            // 如果有路径补全，直接返回
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

    // 如果没有路径补全，再检查其他情况

    // 内置命令补全
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
        auto tokens = split(input);

        if (tokens.size() == 0)
        {
            // 不应该发生
        }
        else if (tokens.size() == 1)
        {
            // 只有 "task"，可能后面要输入任务名
            // 这里我们不补全，让用户继续输入
        }
        else if (tokens.size() == 2)
        {
            // "task 任务名" - 补全任务名
            std::string prefix = tokens[1];
            for (const auto& [name, _] : tasks_)
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
            if (isPathInput(lastToken) || lastToken.find('/') != std::string::npos ||
                lastToken.find('\\') != std::string::npos || lastToken.find('.') != std::string::npos)
            {
                // 重新提取路径部分
                std::string newPathPart = extractPathPart(input);
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
        for (const auto& [name, _] : tasks_)
        {
            if (name.starts_with(prefix))
            {
                completions.emplace_back(name);
            }
        }
    }

    return completions;
}

// 智能路径补全函数
// 智能路径补全函数 - 修复版本
void XUserInput::Impl::completePathSmart(const std::string& partialPath, replxx::Replxx::completions_t& completions)
{
    try
    {
        fs::path inputPath(partialPath);

        // 处理空输入
        if (partialPath.empty())
        {
            inputPath = ".";
        }

        // 确定基路径和前缀
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

        // 转换为绝对路径
        basePath = fs::absolute(basePath);

        // 检查基路径是否存在
        if (!fs::exists(basePath) || !fs::is_directory(basePath))
        {
            return;
        }

        // 遍历目录
        for (const auto& entry : fs::directory_iterator(basePath, fs::directory_options::skip_permission_denied))
        {
            std::string name = entry.path().filename().string();

            // 跳过不匹配前缀的文件
            if (!prefix.empty() && !name.starts_with(prefix))
            {
                continue;
            }

            // 跳过隐藏文件
            if (!shouldShowHiddenFiles() && name[0] == '.')
            {
                continue;
            }

            // 构建完整路径
            fs::path    fullPath = entry.path();
            std::string completion;

            if (fs::is_directory(fullPath))
            {
                // 对于目录：构建相对于输入路径的完整路径
                if (partialPath.empty() || partialPath == ".")
                {
                    // 当前目录：显示 ./目录名/
                    completion = "./" + name + separator();
                }
                else if (partialPath.back() == fs::path::preferred_separator)
                {
                    // 输入以分隔符结尾：追加到当前路径
                    completion = partialPath + name + separator();
                }
                else
                {
                    // 部分路径输入：替换或追加
                    fs::path inputParent = fs::path(partialPath).parent_path();
                    if (inputParent.empty())
                    {
                        // 只有文件名，没有路径
                        completion = name + separator();
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
                if (isExecutable(fullPath))
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

std::string XUserInput::Impl::separator() const
{
    std::string dirPath;
#ifdef _WIN32
    dirPath += '\\';
#else
    dirPath += '/';
#endif

    return dirPath;
}

std::string XUserInput::Impl::extractPathPart(const std::string& input) const
{
    if (input.empty())
        return "";

    // 分割输入
    auto tokens = split(input);

    if (tokens.empty())
        return "";

    // 获取最后一个token
    const std::string& lastToken = tokens.back();

    // 情况1：最后一个token本身就是路径
    if (isPathInput(lastToken))
    {
        return lastToken;
    }

    // 情况2：检查输入字符串的最后一个字符
    if (!input.empty())
    {
        // 如果输入以空格结尾，用户可能正准备输入路径
        if (input.back() == ' ')
        {
            return "."; // 当前目录
        }

        // 如果最后一个字符是路径分隔符
        char lastChar = input.back();
        if (lastChar == '/' || lastChar == '\\')
        {
            // 查找从最后一个分隔符开始的路径部分
            size_t start = input.length() - 1;
            while (start > 0 && input[start - 1] != ' ')
            {
                --start;
            }
            return input.substr(start);
        }

        // 如果最后一个字符是点
        if (lastChar == '.')
        {
            // 检查是否是以点开头的相对路径
            size_t dotPos = input.find_last_of('.');
            if (dotPos != std::string::npos)
            {
                // 向前查找空格
                size_t start = dotPos;
                while (start > 0 && input[start - 1] != ' ')
                {
                    --start;
                }
                std::string candidate = input.substr(start);

                if (candidate == "." || candidate.starts_with("./") || candidate == ".." ||
                    candidate.starts_with("../") || candidate.starts_with(".\\") || candidate.starts_with("..\\"))
                {
                    return candidate;
                }
            }
        }
    }

    // 情况3：在最后一个token中查找路径特征
    if (lastToken.find('/') != std::string::npos || lastToken.find('\\') != std::string::npos ||
        lastToken.find('.') != std::string::npos)
    {
        // 这可能是一个部分输入的路径
        return lastToken;
    }

    // 情况4：检查整个输入是否包含路径分隔符
    size_t lastSlash = input.find_last_of("/\\");
    if (lastSlash != std::string::npos)
    {
        // 从最后一个斜杠开始，向前找空格或开头
        size_t start = lastSlash;
        while (start > 0 && input[start - 1] != ' ')
        {
            --start;
        }
        return input.substr(start);
    }

    return "";
}

// 简化版路径补全函数（备用）
void XUserInput::Impl::completePath(const std::string& partialPath, replxx::Replxx::completions_t& completions)
{
    try
    {
        fs::path inputPath(partialPath);

        if (partialPath.empty())
        {
            inputPath = ".";
        }

        // 确定搜索目录
        fs::path    searchDir;
        std::string prefix;

        if (fs::exists(inputPath) && fs::is_directory(inputPath))
        {
            searchDir = inputPath;
            prefix    = "";
        }
        else
        {
            searchDir = inputPath.parent_path();
            prefix    = inputPath.filename().string();

            if (searchDir.empty())
            {
                searchDir = ".";
            }
        }

        if (!fs::exists(searchDir) || !fs::is_directory(searchDir))
        {
            return;
        }

        // 遍历目录
        for (const auto& entry : fs::directory_iterator(searchDir, fs::directory_options::skip_permission_denied))
        {
            std::string name = entry.path().filename().string();

            if (!prefix.empty() && !name.starts_with(prefix))
                continue;

            if (!shouldShowHiddenFiles() && name[0] == '.')
                continue;

            // 构建补全项 - 直接使用相对于搜索目录的路径
            std::string completion = name;
            if (fs::is_directory(entry.path()))
            {
                completion += fs::path::preferred_separator;
                completions.emplace_back(completion, replxx::Replxx::Color::BRIGHTBLUE);
            }
            else if (fs::is_regular_file(entry.path()))
            {
                auto color = replxx::Replxx::Color::DEFAULT;
                if (isExecutable(entry.path()))
                {
                    color = replxx::Replxx::Color::BRIGHTGREEN;
                }
                completions.emplace_back(completion, color);
            }
        }

        // 排序
        std::sort(completions.begin(), completions.end(),
                  [](const auto& a, const auto& b) { return a.text() < b.text(); });
    }
    catch (...)
    {
        // 忽略错误
    }
}

// 收集路径补全数据（返回完整路径）
XUserInput::Impl::PathCompletionData XUserInput::Impl::collectPathCompletions(const fs::path&    dir,
                                                                              const std::string& prefix)
{
    PathCompletionData data;

    try
    {
        bool showHidden = shouldShowHiddenFiles();

        for (const auto& entry : fs::directory_iterator(dir, fs::directory_options::skip_permission_denied))
        {
            std::string name = entry.path().filename().string();

            if (!prefix.empty() && !name.starts_with(prefix))
                continue;

            if (!showHidden && name[0] == '.')
                continue;

            // 使用完整路径
            std::string fullPath = entry.path().string();

            if (fs::is_directory(entry.path()))
            {
                data.directories.push_back(fullPath + separator());
            }
            else if (fs::is_regular_file(entry.path()))
            {
                if (isExecutable(entry.path()))
                {
                    data.executables.push_back(fullPath);
                }
                else
                {
                    data.files.push_back(fullPath);
                }
            }
        }
    }
    catch (...)
    {
        // 忽略错误
    }

    return data;
}

// 规范化路径显示
std::string XUserInput::Impl::normalizeCompletionPath(const std::string& inputPath, const std::string& completion)
{
    if (fs::path(completion).is_absolute())
    {
        return completion;
    }

    if (fs::path(inputPath).is_absolute())
    {
        fs::path input = inputPath;
        if (fs::is_directory(input) && inputPath.back() != fs::path::preferred_separator)
        {
            input /= "";
        }

        fs::path parent = input.parent_path();
        if (parent.empty())
        {
            parent = fs::current_path();
        }

        fs::path fullPath = parent / completion;
        return fullPath.string();
    }

    return completion;
}

// 判断是否为可执行文件
bool XUserInput::Impl::isExecutable(const fs::path& path)
{
#ifdef _WIN32
    std::string ext = path.extension().string();
    std::ranges::transform(ext, ext.begin(), ::tolower);
    return ext == ".exe" || ext == ".bat" || ext == ".cmd" || ext == ".com";
#else
    try
    {
        fs::perms p = fs::status(path).permissions();
        return (p & fs::perms::owner_exec) != fs::perms::none || (p & fs::perms::group_exec) != fs::perms::none ||
                (p & fs::perms::others_exec) != fs::perms::none;
    }
    catch (...)
    {
        return false;
    }
#endif
}

// 是否显示隐藏文件
bool XUserInput::Impl::shouldShowHiddenFiles() const
{
    const char* env = std::getenv("SHOW_HIDDEN_FILES");
    return env != nullptr && std::string(env) == "1";
}

// 获取字符串列表的公共前缀
std::string XUserInput::Impl::getCommonPrefix(const std::vector<std::string>& strings)
{
    if (strings.empty())
        return "";

    std::string prefix = strings[0];

    for (size_t i = 1; i < strings.size(); ++i)
    {
        const std::string& str = strings[i];
        size_t             j   = 0;
        while (j < prefix.length() && j < str.length() && prefix[j] == str[j])
        {
            ++j;
        }
        prefix = prefix.substr(0, j);

        if (prefix.empty())
            break;
    }

    return prefix;
}

XTask& XUserInput::Impl::registerTask(const std::string& name, const XTask::TaskFunc& func,
                                      const std::string& description)
{
    auto  task    = TaskFac->createTask(name, func, description);
    auto& taskRef = *task;
    tasks_[name]  = std::move(task);
    return taskRef;
}

void XUserInput::Impl::handleCommand(const std::string& input)
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

void XUserInput::Impl::printHelp() const
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

void XUserInput::Impl::listTasks() const
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

void XUserInput::Impl::printTaskUsage(const std::string& taskName) const
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

// 工具函数实现
std::vector<std::string> XUserInput::Impl::split(const std::string& input, char delimiter, bool trimWhitespace)
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

std::vector<std::string> XUserInput::Impl::smartSplit(const std::string& input)
{
    std::vector<std::string> tokens;
    std::string              current;
    bool                     inQuotes  = false;
    char                     quoteChar = 0;

    for (size_t i = 0; i < input.length(); ++i)
    {
        char c = input[i];

        if (c == '\\' && i + 1 < input.length())
        {
            current += input[++i];
            continue;
        }

        if ((c == '"' || c == '\'') && !inQuotes)
        {
            inQuotes  = true;
            quoteChar = c;
            current += c;
        }
        else if (c == quoteChar && inQuotes)
        {
            inQuotes = false;
            current += c;
        }
        else if (c == ' ' && !inQuotes)
        {
            if (!current.empty())
            {
                tokens.push_back(current);
                current.clear();
            }
        }
        else
        {
            current += c;
        }
    }

    if (!current.empty())
    {
        tokens.push_back(current);
    }

    return tokens;
}

bool XUserInput::Impl::isPathInput(const std::string& input)
{
    if (input.empty())
        return false;

    // 检查常见路径模式
    if (input.find('/') != std::string::npos || input.find('\\') != std::string::npos)
    {
        return true;
    }

    // 相对路径
    if (input == "." || input == ".." || input.starts_with("./") || input.starts_with("../") ||
        input.starts_with(".\\") || input.starts_with("..\\"))
    {
        return true;
    }

    // Windows 驱动器路径
    if (input.length() >= 2 && input[1] == ':' &&
        ((input[0] >= 'A' && input[0] <= 'Z') || (input[0] >= 'a' && input[0] <= 'z')))
    {
        return true;
    }

    // 检查文件扩展名（可能是文件名）
    size_t dotPos = input.find_last_of('.');
    if (dotPos != std::string::npos && dotPos > 0 && dotPos < input.length() - 1)
    {
        // 点不在开头，也不在结尾，后面有字符
        std::string afterDot = input.substr(dotPos + 1);
        // 如果点后面的部分看起来像文件扩展名
        if (afterDot.find_first_of(" \t\n") == std::string::npos)
        {
            return true;
        }
    }

    return false;
}

bool XUserInput::Impl::lastTokenIsPathParam(const std::vector<std::string>& tokens)
{
    if (tokens.size() < 3)
        return false;

    const std::string& lastToken = tokens.back();

    if (lastToken.starts_with('-'))
        return false;

    if (tokens.size() >= 2)
    {
        const std::string& secondLast = tokens[tokens.size() - 2];
        if (secondLast.starts_with('-'))
        {
            return isPathInput(lastToken) || lastToken.find('.') != std::string::npos;
        }
    }

    return false;
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

    // 使用相同的路径提取逻辑
    std::string pathPart = extractPathPart(input);

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
                    hints.push_back(" [文件: " + formatFileSize(size) + "]");
                }
                catch (...)
                {
                    hints.push_back(" [文件]");
                }
            }
        }
        else
        {
            // 路径不存在，检查是否是部分路径
            if (!pathPart.empty() &&
                (pathPart.find('/') != std::string::npos || pathPart.find('\\') != std::string::npos ||
                 pathPart.find('.') != std::string::npos))
            {
                hints.push_back(" [按 Tab 键补全]");
            }
        }
    }
    catch (...)
    {
        // 发生异常时显示通用提示
        if (!pathPart.empty())
        {
            hints.push_back(" [路径]");
        }
    }

    return hints;
}

// 格式化文件大小
std::string XUserInput::Impl::formatFileSize(uintmax_t size)
{
    static const char* units[] = { "B", "KB", "MB", "GB", "TB" };
    int                unit    = 0;
    double             sz      = static_cast<double>(size);

    while (sz >= 1024.0 && unit < 4)
    {
        sz /= 1024.0;
        ++unit;
    }

    char buffer[32];
    if (unit == 0)
    {
        snprintf(buffer, sizeof(buffer), "%llu %s", static_cast<unsigned long long>(size), units[unit]);
    }
    else
    {
        snprintf(buffer, sizeof(buffer), "%.1f %s", sz, units[unit]);
    }

    return buffer;
}

// XUserInput的包装函数
XUserInput::XUserInput() : impl_(std::make_unique<Impl>())
{
}
XUserInput::~XUserInput() = default;

void XUserInput::start()
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

void XUserInput::handleCommand(const std::string& input)
{
    impl_->handleCommand(input);
}

void XUserInput::processCommand(const std::string& input)
{
    impl_->processCommand(input);
}

void XUserInput::processTaskCommand(const std::string& input)
{
    impl_->processTaskCommand(input);
}

void XUserInput::printHelp() const
{
    impl_->printHelp();
}

void XUserInput::listTasks() const
{
    impl_->listTasks();
}

void XUserInput::printTaskUsage(const std::string& taskName) const
{
    impl_->printTaskUsage(taskName);
}
