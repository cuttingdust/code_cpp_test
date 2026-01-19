#pragma once
#include "XTask.h"
#include <memory>
#include <map>
#include <string>
#include <functional>
#include <mutex>

class XExec;

class CVTask : public XTask
{
    DECLARE_CREATE_DEFAULT(XTask)
public:
    CVTask();
    CVTask(const std::string_view& name, const TaskFunc& func, const std::string_view& desc);
    ~CVTask() override;

public:
    bool execute(const std::map<std::string, std::string>& inputParams, std::string& errorMsg) const override;

private:
    class PImpl;
    std::unique_ptr<PImpl> impl_;
};
