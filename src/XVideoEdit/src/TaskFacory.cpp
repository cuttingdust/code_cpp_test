#include "TaskFacory.h"

#include "CVTask.h"

auto TackFactory::createTask(const std::string_view &name, const XTask::TaskFunc &func,
                             const std::string_view &description) -> XTask::Ptr
{
    XTask::Ptr task = nullptr;
    if (name == "cv")
    {
        task = CVTask::create(name, func, description);
    }
    else
    {
        task = XTask::create(name, func, description);
    }

    /// 根据名字不同创建不同的实例
    return task;
}
