#include "FFTask.h"

class FFTask::PImpl
{
public:
    PImpl(FFTask *owenr);
    ~PImpl() = default;

public:
    FFTask *owenr_ = nullptr;
};

FFTask::PImpl::PImpl(FFTask *owenr) : owenr_(owenr)
{
}

FFTask::FFTask()
{
    this->setTaskType(TaskType::TT_VIDEO);
}

FFTask::FFTask(const std::string_view &name, const TaskFunc &func, const std::string_view &desc) :
    XTask(name, func, desc)
{
}

FFTask::~FFTask()
{
}

auto FFTask::execute(const std::map<std::string, std::string> &inputParams, std::string &errorMsg) const -> bool
{
    /// 能拿到 src 与 dst


    return true;
}

auto FFTask::process(int percent) -> void
{
}

IMPLEMENT_CREATE_DEFAULT(FFTask)
template auto FFTask::create(const std::string_view &, const TaskFunc &, const std::string_view &) -> FFTask::Ptr;
