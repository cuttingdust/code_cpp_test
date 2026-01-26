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
    auto validateCommon(const std::map<std::string, ParameterValue>& inputParams, std::string& errorMsg)
            -> bool override;

    auto execute(const std::string& command, const std::map<std::string, ParameterValue>& inputParams,
                 std::string& errorMsg, std::string& resultMsg) -> bool override;


private:
    class PImpl;
    std::unique_ptr<PImpl> impl_;
};
