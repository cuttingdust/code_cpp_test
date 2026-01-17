#pragma once

#include "XTask.h"


class XUserInput
{
public:
    using Parameter      = ::Parameter;
    using ParameterValue = ::ParameterValue;

    XUserInput();
    ~XUserInput();

public:
    void start();
    void stop();

    XTask& registerTask(const std::string& name, const XTask::TaskFunc& func, const std::string& description);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
