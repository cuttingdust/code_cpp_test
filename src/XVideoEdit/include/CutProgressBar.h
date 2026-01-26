#pragma once

#include "TaskProgressBar.h"

class XExec;

class CutProgressBar : public TaskProgressBar
{
    DECLARE_CREATE_DEFAULT(CutProgressBar)
public:
    struct ProgressState
    {
        std::mutex  mutex;
        double      currentTime   = 0.0; /// 当前时间（秒）
        double      clipDuration  = 0.0; /// 剪切片段时长（秒）
        double      totalDuration = 0.0; /// 视频总时长（秒）
        std::string displayTime;         /// 显示时间
        std::string speed;               /// 处理速度
        std::string timeRange;           /// 时间范围显示
        bool        hasProgress = false;
    };

    CutProgressBar(const ProgressBarConfig::Ptr &config = nullptr);
    CutProgressBar(ProgressBarStyle style);
    CutProgressBar(const std::string_view &configName);
    ~CutProgressBar() override;

public:
    auto updateProgress(XExec &exec, const std::string_view &taskName,
                        const std::map<std::string, ParameterValue> &inputParams) -> void override;

    auto markAsCompleted(const std::string_view &message) -> void override;
    auto markAsFailed(const std::string_view &message) -> void override;

    /// 剪切特定方法
    auto setTimeRange(double startTime, double endTime) -> void;
    auto setClipDuration(double duration) -> void;
    auto setSourceFile(const std::string &filePath) -> void;

private:
    class PImpl;
    std::unique_ptr<PImpl> impl_;
};

IMPLEMENT_CREATE_DEFAULT(CutProgressBar)
