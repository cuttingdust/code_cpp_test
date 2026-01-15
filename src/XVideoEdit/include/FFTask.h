#pragma once

#include "XTask.h"

class FFTask : public XTask
{
    DECLARE_CREATE_DEFAULT(FFTask)

public:
    FFTask();
    FFTask(const std::string_view &name, const TaskFunc &func, const std::string_view &desc = "");
    ~FFTask() override;

public:
    auto execute(const std::map<std::string, std::string> &inputParams, std::string &errorMsg) const -> bool override;

    virtual auto process(int percent) -> void;

private:
    class PImpl;
    std::unique_ptr<PImpl> impl_;
};
