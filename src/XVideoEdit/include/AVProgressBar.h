#pragma once

#include "TaskProgressBar.h"

class XExec;

class AVProgressBar : public TaskProgressBar
{
    DECLARE_CREATE_DEFAULT(AVProgressBar)
public:
    struct ProgressState
    {
        std::mutex  mutex;
        double      currentTime = 0.0; /// 当前时间（秒）
        std::string displayTime;       /// 显示时间
        std::string speed;
        bool        hasProgress = false;
    };

    AVProgressBar(const ProgressBarConfig::Ptr &config = nullptr);
    AVProgressBar(ProgressBarStyle style);
    AVProgressBar(const std::string_view &configName);
    ~AVProgressBar() override;

public:
    auto updateProgress(XExec &exec, const std::string_view &taskName,
                        const std::map<std::string, std::string> &inputParams) -> void override;

    auto markAsCompleted(const std::string_view &message) -> void override;

    auto markAsFailed(const std::string_view &message) -> void override;

private:
    class PImpl;
    std::unique_ptr<PImpl> impl_;
};
IMPLEMENT_CREATE_DEFAULT(AVProgressBar)
