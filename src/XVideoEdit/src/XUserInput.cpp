#include "XUserInput.h"
#include "ReplxxConfigurator.h"
#include "XTool.h"
#include <iostream>
#include <stdexcept>
#include <algorithm>

/// 私有实现类
class XUserInput::PImpl
{
public:
    explicit PImpl(UIConfig config);
    ~PImpl();

public:
    auto registerCommandHandler(const std::string_view& command, ParsedCallback handler) -> void;

    auto registerBuiltinCommands() -> void;

    auto handleBuiltinCommand(const ParsedCommand& cmd) -> void;

public:
    std::map<std::string, ParsedCallback> commandHandlers_; ///< 自定义命令处理器

    /// 事件回调
    CommandCallback onCommandStart_;
    CommandCallback onCommandComplete_;
    CommandCallback onError_;

    /// 统计
    size_t commandCount_ = 0;

public:
    /// 公共接口实现
    auto start() -> void;

    auto stop() -> void;

    auto registerTask(const std::string_view& name, const XTask::TaskFunc& func, const std::string_view& description)
            -> XTask&;

    auto registerTask(const std::string_view& taskName, const std::string_view& typeName, const XTask::TaskFunc& func,
                      const std::string_view& description) -> XTask&;

public:
    TaskManager&       getTaskManager();
    CompletionManager& getCompletionManager();

    bool                     isRunning() const;
    InputStateMachine::State getState() const;
    std::string              getStateString() const;

    void            setConfig(const UIConfig& config);
    const UIConfig& getConfig() const;

    void                     clearHistory();
    std::vector<std::string> getHistory() const;

    size_t getTaskCount() const;
    size_t getCommandCount() const;

    auto setOnCommandStart(CommandCallback callback) -> void;
    auto setOnCommandComplete(CommandCallback callback) -> void;
    auto setOnError(CommandCallback callback) -> void;

private:
    /// 初始化与清理
    void initialize();
    auto initializeREPL() -> void;
    void initializeSimpleMode();
    void cleanup();

public:
    /// 主循环
    auto runREPLLoop() -> void;
    void runSimpleLoop();

public:
    /// 命令处理
    void handleCommand(const std::string& input);
    void handleTaskCommand(const CommandParser::ParsedCommand& cmd);

public:
    /// 辅助方法
    auto showWelcomeMessage() const -> void;
    auto showGoodbyeMessage() const -> void;
    void showStatus() const;
    void showHelp() const;

public:
    std::string getPrompt() const;
    auto        shouldUseREPL() const -> bool;

    // 错误处理
    void handleError(const std::exception& e);
    void handleUnknownError();

private:
    UIConfig config_;

    /// 核心组件
    std::unique_ptr<replxx::Replxx>    rx_;
    std::unique_ptr<InputStateMachine> stateMachine_;
    std::unique_ptr<CommandParser>     commandParser_;
    std::unique_ptr<HistoryManager>    historyManager_;
    std::unique_ptr<TaskManager>       taskManager_;
    std::unique_ptr<CompletionManager> completionManager_;


    size_t successCount_ = 0;
    size_t errorCount_   = 0;
};

/// =============== 实现 ===============

XUserInput::PImpl::PImpl(UIConfig config) :
    config_(std::move(config)), stateMachine_(std::make_unique<InputStateMachine>()),
    commandParser_(std::make_unique<CommandParser>())
{
    try
    {
        initialize();
        stateMachine_->transitionTo(InputStateMachine::State::Running);
    }
    catch (const std::exception& e)
    {
        stateMachine_->transitionTo(InputStateMachine::State::Error);
        if (onError_)
            onError_(e.what());
        throw std::runtime_error("Failed to initialize XUserInput: " + std::string(e.what()));
    }
}

XUserInput::PImpl::~PImpl()
{
    try
    {
        cleanup();
    }
    catch (...)
    {
        // 析构函数中不抛出异常
    }
}

auto XUserInput::PImpl::initialize() -> void
{
    /// 循环代理
    rx_ = std::make_unique<replxx::Replxx>();

    /// 初始化任务管理器
    taskManager_ = std::make_unique<TaskManager>();

    /// 初始化历史管理器
    historyManager_ = std::make_unique<HistoryManager>(rx_, config_);

    /// 初始化补全管理器
    completionManager_ = std::make_unique<CompletionManager>(taskManager_->getTasks());

    /// 注册内置命令（这会同时注册到补全管理器）
    registerBuiltinCommands();

    /// 加载历史记录
    historyManager_->loadHistory();
}

auto XUserInput::PImpl::registerBuiltinCommands() -> void
{
    /// exit 命令
    registerCommandHandler(config_.exitCommand,
                           [this](const ParsedCommand&)
                           {
                               stateMachine_->transitionTo(InputStateMachine::State::ShuttingDown);
                               showGoodbyeMessage();
                           });

    /// help 命令
    registerCommandHandler(config_.helpCommand, [this](const ParsedCommand&) { showHelp(); });

    /// status 命令
    registerCommandHandler("status", [this](const ParsedCommand&) { showStatus(); });

    /// clear 命令
    registerCommandHandler("clear",
                           [](const ParsedCommand&)
                           {
#ifdef _WIN32
                               system("cls");
#else
                              system("clear");
#endif
                           });

    /// stats 命令
    registerCommandHandler("stats",
                           [this](const CommandParser::ParsedCommand&)
                           {
                               auto stats = taskManager_->getStatistics();
                               std::cout << "\n=== 任务统计信息 ===\n"
                                         << "任务类型数: " << stats.totalTaskTypes << "\n"
                                         << "任务实例数: " << stats.totalTaskInstances << "\n"
                                         << "总执行次数: " << stats.totalExecutions << "\n"
                                         << "成功次数: " << stats.successExecutions << "\n"
                                         << "失败次数: " << stats.failedExecutions << "\n"
                                         << "成功率: "
                                         << (stats.totalExecutions > 0
                                                     ? (stats.successExecutions * 100.0 / stats.totalExecutions)
                                                     : 0.0)
                                         << "%\n"
                                         << "=====================\n";
                           });

    registerCommandHandler("list",
                           [this](const ParsedCommand&)
                           {
                               std::cout << "\n已注册的任务实例 (" << taskManager_->getTaskInstanceCount() << "个):\n";

                               auto taskInfos = taskManager_->getTaskInstanceNames();
                               for (const auto& name : taskInfos)
                               {
                                   std::cout << "  " << name;
                                   auto task = taskManager_->getTaskInstance(name);
                                   if (task && !task->getDescription().empty())
                                   {
                                       std::cout << " - " << task->getDescription();
                                   }

                                   /// 显示统计信息
                                   if (auto taskInfo = taskManager_->getTaskInstanceInfo(name))
                                   {
                                       std::cout << " [" << taskInfo->typeName << "]";
                                       if (taskInfo->executionCount > 0)
                                       {
                                           std::cout << " (执行" << taskInfo->executionCount << "次, ";
                                           std::cout << "成功" << taskInfo->successCount << "次)";
                                       }
                                   }
                                   std::cout << "\n";
                               }

                               /// 显示可用任务类型
                               std::cout << "\n可用任务类型:\n";
                               auto taskTypes = taskManager_->getTaskTypes();
                               for (const auto& type : taskTypes)
                               {
                                   std::cout << "  " << type << " - " << taskManager_->getTaskTypeDescription(type)
                                             << "\n";
                               }
                           });
}

auto XUserInput::PImpl::initializeREPL() -> void
{
    if (!rx_)
    {
        rx_ = std::make_unique<replxx::Replxx>();
    }

    rx_->install_window_change_handler();

    /// 配置REPL
    ReplxxConfigurator::configure(
            rx_, [this](const std::string& input, int& contextLen)
            { return completionManager_->completionHook(input, contextLen); },
            [this](const std::string& input, int& contextLen, replxx::Replxx::Color& color)
            { return completionManager_->hintHook(input, contextLen, color); });

    completionManager_->setTaskList(taskManager_->getTasks());
}

void XUserInput::PImpl::initializeSimpleMode()
{
    /// 简单模式不需要特殊初始化
    /// 可以在这里设置一些简单模式特有的配置
    std::cout << "使用简单输入模式（无智能补全功能）\n";
}

void XUserInput::PImpl::cleanup()
{
    if (historyManager_)
    {
        historyManager_->saveHistory();
    }

    if (stateMachine_ && stateMachine_->isRunning())
    {
        stateMachine_->transitionTo(InputStateMachine::State::ShuttingDown);
    }
}

void XUserInput::PImpl::start()
{
    if (stateMachine_->isShuttingDown() || stateMachine_->isError())
    {
        throw std::runtime_error("Cannot start in state: " + getStateString());
    }

    showWelcomeMessage();

    if (shouldUseREPL())
    {
        try
        {
            initializeREPL();
            runREPLLoop();
        }
        catch (const std::exception& e)
        {
            handleError(e);
            runSimpleLoop();
        }
    }
    else
    {
        runSimpleLoop();
    }

    cleanup();
}

void XUserInput::PImpl::stop()
{
    stateMachine_->transitionTo(InputStateMachine::State::ShuttingDown);
}

auto XUserInput::PImpl::runREPLLoop() -> void
{
    while (stateMachine_->isRunning())
    {
        const char* line = nullptr;

        do
        {
            line = rx_->input(getPrompt());
        }
        while (line == nullptr && errno == EAGAIN);

        if (line == nullptr)
        {
            break;
        }

        std::string input(line);
        handleCommand(input);
    }
}

void XUserInput::PImpl::runSimpleLoop()
{
    std::cout << "\n使用简单输入模式...\n";

    while (stateMachine_->isRunning())
    {
        std::cout << getPrompt();
        std::cout.flush();

        std::string input;
        if (!std::getline(std::cin, input))
        {
            break;
        }

        handleCommand(input);
    }
}

void XUserInput::PImpl::handleCommand(const std::string& input)
{
    if (input.empty())
        return;

    commandCount_++;

    try
    {
        if (onCommandStart_)
            onCommandStart_(input);

        stateMachine_->transitionTo(InputStateMachine::State::ProcessingCommand);

        /// 解析命令
        auto parsed = commandParser_->parse(input);
        if (!commandParser_->validate(parsed))
        {
            throw std::runtime_error("Invalid command format");
        }

        // 添加到历史记录
        historyManager_->addToHistory(input);

        // 分发命令处理
        if (parsed.command == "task")
        {
            handleTaskCommand(parsed);
        }
        else if (commandHandlers_.contains(parsed.command))
        {
            handleBuiltinCommand(parsed);
        }

        successCount_++;
        if (onCommandComplete_)
            onCommandComplete_(input);
    }
    catch (const std::exception& e)
    {
        errorCount_++;
        handleError(e);
        if (onError_)
            onError_(e.what());
    }

    /// 恢复运行状态
    if (!stateMachine_->isShuttingDown())
    {
        stateMachine_->transitionTo(InputStateMachine::State::Running);
    }
}

void XUserInput::PImpl::handleTaskCommand(const CommandParser::ParsedCommand& cmd)
{
    if (cmd.args.empty())
    {
        throw std::runtime_error("Task command requires a task name");
    }

    std::string taskName = cmd.args[0];
    if (!taskManager_->hasTaskInstance(taskName))
    {
        throw std::runtime_error("Unknown task: " + taskName);
    }

    /// 转换参数格式
    std::map<std::string, std::string> params;
    for (const auto& [key, value] : cmd.options)
    {
        params[key] = value;
    }

    /// 添加位置参数
    for (size_t i = 1; i < cmd.args.size(); i++)
    {
        params["arg" + std::to_string(i)] = cmd.args[i];
    }

    /// 执行任务
    std::string error;
    if (!taskManager_->executeTask(taskName, params, error))
    {
        throw std::runtime_error("Task execution failed: " + error);
    }
}

auto XUserInput::PImpl::handleBuiltinCommand(const ParsedCommand& cmd) -> void
{
    auto handler = commandHandlers_[cmd.command];
    handler(cmd);
}

auto XUserInput::PImpl::registerTask(const std::string_view& name, const XTask::TaskFunc& func,
                                     const std::string_view& description) -> XTask&
{
    return taskManager_->registerTask(name, func, description);
}

auto XUserInput::PImpl::registerTask(const std::string_view& taskName, const std::string_view& typeName,
                                     const XTask::TaskFunc& func, const std::string_view& description) -> XTask&
{
    return *taskManager_->createAndRegisterTask(taskName, typeName, func, description);
}

auto XUserInput::PImpl::registerCommandHandler(const std::string_view& command, ParsedCallback handler) -> void
{
    commandHandlers_[std::string(command)] = std::move(handler);
    completionManager_->registerBuiltinCommand(command);
}

TaskManager& XUserInput::PImpl::getTaskManager()
{
    return *taskManager_;
}


CompletionManager& XUserInput::PImpl::getCompletionManager()
{
    return *completionManager_;
}

bool XUserInput::PImpl::isRunning() const
{
    return stateMachine_->isRunning();
}

InputStateMachine::State XUserInput::PImpl::getState() const
{
    return stateMachine_->getCurrentState();
}

std::string XUserInput::PImpl::getStateString() const
{
    return InputStateMachine::stateToString(getState());
}

void XUserInput::PImpl::setConfig(const UIConfig& config)
{
    config_ = config;
}

const UIConfig& XUserInput::PImpl::getConfig() const
{
    return config_;
}

void XUserInput::PImpl::clearHistory()
{
    historyManager_->clearHistory();
}

std::vector<std::string> XUserInput::PImpl::getHistory() const
{
    return historyManager_->getHistory();
}

size_t XUserInput::PImpl::getTaskCount() const
{
    return taskManager_->getTaskInstanceCount();
}

size_t XUserInput::PImpl::getCommandCount() const
{
    return commandCount_;
}

auto XUserInput::PImpl::setOnCommandStart(CommandCallback callback) -> void
{
    onCommandStart_ = std::move(callback);
}

auto XUserInput::PImpl::setOnCommandComplete(std::function<void(const std::string&)> callback) -> void
{
    onCommandComplete_ = std::move(callback);
}

auto XUserInput::PImpl::setOnError(std::function<void(const std::string&)> callback) -> void
{
    onError_ = std::move(callback);
}

auto XUserInput::PImpl::showWelcomeMessage() const -> void
{
    if (config_.enableColor)
    {
        std::cout << "\x1b[1;36m=== 增强型任务处理器 ===\x1b[0m\n";
    }
    else
    {
        std::cout << "=== 增强型任务处理器 ===\n";
    }

    std::cout << "提示：按 Tab 键智能补全，Ctrl+L 清屏。\n"
              << "输入 '" << config_.helpCommand << "' 查看帮助，'" << config_.exitCommand << "' 退出程序。\n\n";
}

auto XUserInput::PImpl::showGoodbyeMessage() const -> void
{
    std::cout << "\n\x1b[1;32m处理统计：\x1b[0m\n"
              << "  总命令数: " << commandCount_ << "\n"
              << "  成功: " << successCount_ << "\n"
              << "  失败: " << errorCount_ << "\n"
              << "\n\x1b[1;33m再见！\x1b[0m\n";
}

void XUserInput::PImpl::showStatus() const
{
    std::cout << "\n=== 系统状态 ===\n"
              << "状态: " << getStateString() << "\n"
              << "任务数: " << getTaskCount() << "\n"
              << "命令数: " << commandCount_ << "\n"
              << "模式: " << (shouldUseREPL() ? "交互式(REPL)" : "简单模式") << "\n"
              << "================\n";
}

void XUserInput::PImpl::showHelp() const
{
    std::cout << "\n=== 任务处理器帮助 ===\n"
              << "任务命令格式: task <任务名> [-参数1 值1] [-参数2 值2] ...\n"
              << "\n可用任务类型:\n";

    /// 显示可用的任务类型
    auto taskTypes = taskManager_->getTaskTypes();
    for (const auto& type : taskTypes)
    {
        std::cout << "  " << type << " - " << taskManager_->getTaskTypeDescription(type) << "\n";
    }

    std::cout << "\n内置命令:\n";

    /// 从补全管理器获取内置命令列表
    const auto builtin_cmds = completionManager_->getBuiltinCommands();
    for (const auto& cmd : builtin_cmds)
    {
        if (cmd == "exit")
            std::cout << "  exit     - 退出程序\n";
        else if (cmd == "help")
            std::cout << "  help     - 显示此帮助\n";
        else if (cmd == "status")
            std::cout << "  status   - 显示系统状态\n";
        else if (cmd == "clear")
            std::cout << "  clear    - 清屏\n";
        else if (cmd == "list")
            std::cout << "  list     - 列出所有任务\n";
        else if (cmd == "stats")
            std::cout << "  stats    - 显示任务统计信息\n";
    }

    std::cout << "\n示例:\n"
              << "  task copy -s file.txt -d backup/\n"
              << "  task cv --input video.mp4 --output video.avi\n"
              << "  task start -host localhost -port 8080\n"
              << "\n智能补全功能:\n"
              << "  - 按 Tab 键补全命令、参数、路径\n"
              << "  - 参数值支持智能补全\n"
              << "  - 路径补全支持文件和目录\n"
              << "================================\n";

    if (taskManager_->getTaskInstanceCount() > 0)
    {
        std::cout << "\n已注册的任务实例:\n";
        for (const auto& name : taskManager_->getTaskInstanceNames())
        {
            std::cout << "  " << name;
            auto task = taskManager_->getTaskInstance(name);
            if (task && !task->getDescription().empty())
            {
                std::cout << " - " << task->getDescription();
            }

            /// 显示任务类型
            if (auto taskInfo = taskManager_->getTaskInstanceInfo(name))
            {
                std::cout << " [" << taskInfo->typeName << "]";
            }

            std::cout << "\n";
        }
    }
}

std::string XUserInput::PImpl::getPrompt() const
{
    return std::string(config_.prompt);
}

auto XUserInput::PImpl::shouldUseREPL() const -> bool
{
    return XTool::isInteractiveTerminal() && config_.enableREPL;
}

void XUserInput::PImpl::handleError(const std::exception& e)
{
    if (config_.enableColor)
    {
        std::cerr << "\x1b[1;31m错误: " << e.what() << "\x1b[0m\n";
    }
    else
    {
        std::cerr << "错误: " << e.what() << "\n";
    }
}

void XUserInput::PImpl::handleUnknownError()
{
    if (config_.enableColor)
    {
        std::cerr << "\x1b[1;31m未知错误\x1b[0m\n";
    }
    else
    {
        std::cerr << "未知错误\n";
    }
}

/// =============== XUserInput 公共接口实现 ===============

XUserInput::XUserInput(const UIConfig& config) : impl_(std::make_unique<PImpl>(config))
{
}

XUserInput::XUserInput(XUserInput&&) noexcept            = default;
XUserInput& XUserInput::operator=(XUserInput&&) noexcept = default;
XUserInput::~XUserInput()                                = default;

auto XUserInput::start() -> void
{
    impl_->start();
}

auto XUserInput::stop() -> void
{
    impl_->stop();
}

auto XUserInput::registerTask(const std::string_view& name, const TaskFunc& func, const std::string_view& description)
        -> XTask&
{
    return impl_->registerTask(name, func, description);
}

auto XUserInput::registerTask(const std::string_view& name, const std::string& typeName, const TaskFunc& func,
                              const std::string_view& description) -> XTask&
{
    return impl_->registerTask(name, typeName, func, description);
}

auto XUserInput::registerCommandHandler(const std::string_view& command, ParsedCallback handler) -> void
{
    impl_->registerCommandHandler(command, std::move(handler));
}


TaskManager& XUserInput::getTaskManager()
{
    return impl_->getTaskManager();
}


CompletionManager& XUserInput::getCompletionManager()
{
    return impl_->getCompletionManager();
}

bool XUserInput::isRunning() const
{
    return impl_->isRunning();
}

InputStateMachine::State XUserInput::getState() const
{
    return impl_->getState();
}

std::string XUserInput::getStateString() const
{
    return impl_->getStateString();
}

void XUserInput::setConfig(const UIConfig& config)
{
    impl_->setConfig(config);
}

const UIConfig& XUserInput::getConfig() const
{
    return impl_->getConfig();
}

void XUserInput::clearHistory()
{
    impl_->clearHistory();
}

std::vector<std::string> XUserInput::getHistory() const
{
    return impl_->getHistory();
}

size_t XUserInput::getTaskCount() const
{
    return impl_->getTaskCount();
}

size_t XUserInput::getCommandCount() const
{
    return impl_->getCommandCount();
}

auto XUserInput::setOnCommandStart(CommandCallback callback) -> void
{
    impl_->setOnCommandStart(std::move(callback));
}

auto XUserInput::setOnCommandComplete(std::function<void(const std::string&)> callback) -> void
{
    impl_->setOnCommandComplete(std::move(callback));
}

auto XUserInput::setOnError(std::function<void(const std::string&)> callback) -> void
{
    impl_->setOnError(std::move(callback));
}
