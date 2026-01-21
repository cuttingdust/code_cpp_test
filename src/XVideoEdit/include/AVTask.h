#pragma once
#include "XTask.h"
#include <memory>
#include <map>
#include <string>
#include <functional>
#include <mutex>

class XExec;

class AVTask : public XTask
{
    DECLARE_CREATE_DEFAULT(XTask)
public:
    AVTask();
    AVTask(const std::string& name, const TaskFunc& func, const std::string& desc);
    ~AVTask() override;

public:
    bool execute(const std::map<std::string, std::string>& inputParams, std::string& errorMsg) const override;

private:
    class PImpl;
    std::unique_ptr<PImpl> impl_;
};
