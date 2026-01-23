#pragma once

#include "XConst.h"

class ITask
{
public:
    enum class TaskType
    {
        TT_DEFAULT,
        TT_AV
    };

    virtual ~ITask() = default;

    virtual auto execute(const std::string& command, const std::map<std::string, std::string>& inputParams,
                         std::string& errorMsg) -> bool = 0;

    virtual auto validateCommon(const std::map<std::string, std::string>& inputParams, std::string& errorMsg)
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
