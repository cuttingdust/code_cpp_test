#pragma once

#include "TaskProgressBar.h"

class XExec;

class CVProgressBar : public TaskProgressBar
{
    DECLARE_CREATE_UN_DEFAULT(CVProgressBar)
public:
    struct ProgressState
    {
        std::mutex  mutex;
        double      currentTime = 0.0; /// 当前时间（秒）
        std::string displayTime;       /// 显示时间
        std::string speed;
        bool        hasProgress = false;
    };

    CVProgressBar(const ProgressBarConfig::Ptr &config = nullptr);
    CVProgressBar(ProgressBarStyle style);
    CVProgressBar(const std::string_view &configName);
    ~CVProgressBar() override;

public:
    auto updateProgress(XExec &exec, const std::string_view &taskName,
                        const std::map<std::string, std::string> &inputParams) -> void override;

    auto markAsCompleted(const std::string_view &message) -> void override;

private:
    class PImpl;
    std::unique_ptr<PImpl> impl_;
};
IMPLEMENT_CREATE_UN_DEFAULT(CVProgressBar)
