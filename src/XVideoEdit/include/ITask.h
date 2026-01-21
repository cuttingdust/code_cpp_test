#pragma once

#include "XConst.h"

class ITask
{
public:
    enum class TaskType
    {
        TT_NORMAL,
        TT_VIDEO
    };

    virtual ~ITask() = default;

    virtual auto execute(const std::map<std::string, std::string>& inputParams, std::string& errorMsg) const
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
    TaskType tt_ = TaskType::TT_NORMAL;
};
