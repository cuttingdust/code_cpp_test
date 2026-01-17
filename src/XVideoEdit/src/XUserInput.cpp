#include "XUserInput.h"
#include "CompletionManager.h"
#include "CommandProcessor.h"
#include "ReplxxConfigurator.h"
#include "TaskFacory.h"

#include <replxx.hxx>
#include <iostream>
#include <algorithm>
#include <ranges>
#include <set>

class XUserInput::Impl
{
public:
    Impl();
    ~Impl() = default;

    void start();
    void stop();

    XTask& registerTask(std::string_view name, const XTask::TaskFunc& func, std::string_view description);

private:
    void runMainLoop();
    void fallbackSimpleInput();
    void handleReplxxError(const std::exception& e);
    void handleUnknownError();
    void handleCommand(std::string_view input);

    void loadHistory();
    void saveHistory();

private:
    std::unique_ptr<replxx::Replxx>    rx_;
    std::unique_ptr<CompletionManager> completionManager_;
    std::unique_ptr<CommandProcessor>  commandProcessor_;

    std::map<std::string, std::shared_ptr<XTask>, std::less<>> tasks_;
    bool                                                       isRunning_ = true;

    static constexpr std::string_view prompt_      = "\x1b[1;32m>>\x1b[0m ";
    static constexpr std::string_view historyFile_ = ".command_history";
};

// =============== 构造函数和生命周期管理 ===============

XUserInput::Impl::Impl() :
    rx_(std::make_unique<replxx::Replxx>()), completionManager_(std::make_unique<CompletionManager>(tasks_)),
    commandProcessor_(std::make_unique<CommandProcessor>(tasks_))
{
}

void XUserInput::Impl::start()
{
    if (!XTool::isInteractiveTerminal())
    {
        fallbackSimpleInput();
        return;
    }

    try
    {
        rx_->install_window_change_handler();
        loadHistory();

        // 配置 replxx
        ReplxxConfigurator::configure(
                rx_, [this](const std::string& input, int& contextLen)
                { return completionManager_->completionHook(input, contextLen); },
                [this](const std::string& input, int& contextLen, replxx::Replxx::Color& color)
                { return completionManager_->hintHook(input, contextLen, color); });

        // 主循环
        runMainLoop();
        saveHistory();
    }
    catch (const std::exception& e)
    {
        handleReplxxError(e);
    }
    catch (...)
    {
        handleUnknownError();
    }
}

void XUserInput::Impl::stop()
{
    isRunning_ = false;
}

XTask& XUserInput::Impl::registerTask(std::string_view name, const XTask::TaskFunc& func, std::string_view description)
{
    auto  task                = TaskFac->createTask(std::string(name), func, std::string(description));
    auto& taskRef             = *task;
    tasks_[std::string(name)] = std::move(task);
    return taskRef;
}

void XUserInput::Impl::runMainLoop()
{
    std::cout << "任务处理器已启动。\n"
              << "提示：按 Tab 键补全，Ctrl+L 清屏。\n"
              << "输入 'help' 查看帮助，'exit' 退出程序。\n\n";

    while (isRunning_)
    {
        char const* line = nullptr;

        do
        {
            line = rx_->input(std::string(prompt_));
        }
        while (line == nullptr && errno == EAGAIN);

        if (line == nullptr)
        {
            std::cout << "\n再见。\n";
            break;
        }

        std::string input(line);
        if (!input.empty())
        {
            rx_->history_add(input);
            handleCommand(input);
        }
    }
}

void XUserInput::Impl::fallbackSimpleInput()
{
    std::cout << "使用简单输入模式...\n"
              << "提示：输入 'help' 查看帮助，'exit' 退出程序。\n";

    while (isRunning_)
    {
        std::cout << ">> ";
        std::cout.flush();

        std::string input;
        if (!std::getline(std::cin, input))
        {
            std::cout << "\n再见。\n";
            break;
        }

        if (!input.empty())
        {
            handleCommand(input);
        }
    }
}

void XUserInput::Impl::handleCommand(std::string_view input)
{
    commandProcessor_->processCommand(input);

    // 如果需要退出
    if (!commandProcessor_->isRunning())
    {
        isRunning_ = false;
    }
}

void XUserInput::Impl::loadHistory()
{
    if (fs::exists(historyFile_))
    {
        rx_->history_load(std::string(historyFile_));
    }
}

void XUserInput::Impl::saveHistory()
{
    rx_->history_save(std::string(historyFile_));
}

void XUserInput::Impl::handleReplxxError(const std::exception& e)
{
    std::cerr << "Replxx初始化失败: " << e.what() << "\n"
              << "切换到简单输入模式...\n";
    fallbackSimpleInput();
}

void XUserInput::Impl::handleUnknownError()
{
    std::cerr << "未知错误初始化Replxx\n";
    fallbackSimpleInput();
}

// =============== XUserInput 公共接口 ===============

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
