#pragma once

#include "Parameter.h"
#include "ParameterValue.h"
#include "XTask.h"

class XUserInput
{
public:
    using Parameter      = ::Parameter;
    using ParameterValue = ::ParameterValue;

public:
    auto start() -> void;

    auto stop() -> void;

    /// 注册任务（返回Task引用以便链式添加参数）
    auto registerTask(const std::string& name, const XTask::TaskFunc& func, const std::string& description = "")
            -> XTask&;

private:
    auto handleCommand(const std::string& input) -> void;

    /// 主命令处理函数
    auto processCommand(const std::string& input) -> void;

    /// 解析并执行任务命令
    auto processTaskCommand(const std::string& input) -> void;

    auto printTaskUsage(const std::string& taskName) const -> void;

    auto printHelp() const -> void;

    auto listTasks() const -> void;
};
