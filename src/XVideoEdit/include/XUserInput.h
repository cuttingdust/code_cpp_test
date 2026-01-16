#pragma once

#include "XTask.h"

class XUserInput
{
public:
    XUserInput();
    ~XUserInput();

    using Parameter      = ::Parameter;
    using ParameterValue = ::ParameterValue;

public:
    auto start() -> void;
    auto stop() -> void;

    auto registerTask(const std::string &name, const XTask::TaskFunc &func, const std::string &description = "")
            -> XTask &;

private:
    auto handleCommand(const std::string &input) -> void;
    auto processCommand(const std::string &input) -> void;
    auto processTaskCommand(const std::string &input) -> void;

    auto printHelp() const -> void;
    auto listTasks() const -> void;
    auto printTaskUsage(const std::string &taskName) const -> void;

private:
    /// 内部实现类
    class Impl;
    std::unique_ptr<Impl> impl_;
};
