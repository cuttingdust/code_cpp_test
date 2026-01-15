#pragma once

#include "XTask.h"
#include "ISingleton.hpp"

#define TaskFac TackFactory::getInstance()
class TackFactory : public ISingleton<TackFactory>
{
public:
    auto createTask(const std::string_view& name, const XTask::TaskFunc& func, const std::string_view& description)
            -> XTask::Ptr;
};
