#pragma once

#include "XConst.h"
#include "ParameterValue.h"

class ITask
{
public:
    enum class TaskType
    {
        TT_DEFAULT,
        TT_AV
    };

    virtual ~ITask() = default;

    virtual auto execute(const std::string& command, const std::map<std::string, ParameterValue>& inputParams,
                         std::string& errorMsg, std::string& resultMsg) -> bool = 0;

    virtual auto validateCommon(const std::map<std::string, ParameterValue>& inputParams, std::string& errorMsg)
            -> bool = 0;

    virtual auto validateSuccess(const std::map<std::string, ParameterValue>& inputParams, std::string& errorMsg)
            -> bool = 0;

    auto setTaskType(const TaskType& tt) -> void
    {
        tt_ = tt;
    }

    auto getTaskType() const -> TaskType
    {
        return tt_;
    }

protected:
    TaskType tt_ = TaskType::TT_DEFAULT;
};
