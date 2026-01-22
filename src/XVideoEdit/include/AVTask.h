#pragma once
#include "XTask.h"

class XExec;
class AVTask : public XTask
{
    DECLARE_CREATE_DEFAULT(XTask)
public:
    AVTask();
    AVTask(const std::string_view& name, const TaskFunc& func, const std::string_view& desc);
    ~AVTask() override;

public:
    auto execute(const std::map<std::string, std::string>& inputParams, std::string& errorMsg) const -> bool override;

private:
    class PImpl;
    std::unique_ptr<PImpl> impl_;
};
