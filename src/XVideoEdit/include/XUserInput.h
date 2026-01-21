#pragma once

#include "XTask.h"
#include "UIConfig.h"
#include "InputStateMachine.h"
#include "CommandParser.h"
#include "CompletionManager.h"
#include "HistoryManager.h"
#include "TaskManager.h"

/// \class XUserInput
/// \brief 基于REPL的命令行用户输入处理器
/// \
/// \提供：
/// \1. 交互式命令行界面
/// \2. 任务注册和执行系统
/// \3. 智能补全（支持路径补全）
/// \4. 历史记录管理
/// \5. 内置命令和自定义命令支持

class XUserInput
{
public:
    explicit XUserInput(const UIConfig& config = UIConfig::Default());

    /// 删除拷贝构造函数和赋值运算符
    XUserInput(const XUserInput&)            = delete;
    XUserInput& operator=(const XUserInput&) = delete;

    /// 支持移动语义
    XUserInput(XUserInput&&) noexcept;
    XUserInput& operator=(XUserInput&&) noexcept;

    ~XUserInput();

    using TaskFunc        = XTask::TaskFunc;
    using ParameterValue  = ::ParameterValue;
    using ParsedCommand   = CommandParser::ParsedCommand;
    using CommandCallback = std::function<void(const std::string&)>;
    using ParsedCallback  = std::function<void(const ParsedCommand&)>;

public:
    /// \brief 注册自定义命令处理器
    /// \param command 命令名称
    /// \param handler 命令处理函数
    auto registerCommandHandler(const std::string_view& command, ParsedCallback handler) -> void;

public:
    /// \brief 启动用户输入处理器
    ///
    /// \如果终端支持交互式输入，则启动REPL界面
    /// \否则退回到简单输入模式
    auto start() -> void;

    /// \brief
    auto stop() -> void;

    /// \brief 注册任务
    /// \param name 任务名称
    /// \param func 任务执行函数
    /// \param description 任务描述
    /// \return XTask& 任务引用，可用于链式调用添加参数
    auto registerTask(const std::string_view& name, const TaskFunc& func, const std::string_view& description = "")
            -> XTask&;

    auto registerTask(const std::string_view& name, const std::string& typeName, const TaskFunc& func,
                      const std::string_view& description = "") -> XTask&;


    /// \brief 获取任务管理器引用
    /// \return TaskManager& 任务管理器
    TaskManager& getTaskManager();


    /// \brief 获取补全管理器引用
    /// \return CompletionManager& 补全管理器
    CompletionManager& getCompletionManager();

    /// 状态查询
    bool                     isRunning() const;
    InputStateMachine::State getState() const;
    std::string              getStateString() const;

    /// 配置管理
    void            setConfig(const UIConfig& config);
    const UIConfig& getConfig() const;

    /// 历史记录管理
    void                     clearHistory();
    std::vector<std::string> getHistory() const;

    /// 统计信息
    size_t getTaskCount() const;
    size_t getCommandCount() const;

public:
    /// 事件回调
    auto setOnCommandStart(CommandCallback callback) -> void;
    auto setOnCommandComplete(CommandCallback callback) -> void;
    auto setOnError(CommandCallback callback) -> void;

private:
    class PImpl;
    std::unique_ptr<PImpl> impl_;
};
