#pragma once

#ifndef AV_PROGRESS_BAR_H
#define AV_PROGRESS_BAR_H

#include "TaskProgressBar.h"

class XExec;

/// \class AVProgressBar
/// \brief 音视频处理的通用进度条基类
/// \提供通用的音视频处理进度显示功能，包括时间解析、进度计算等
class AVProgressBar : public TaskProgressBar
{
protected:
    /// 进度状态结构，用于线程间共享
    struct AVProgressState
    {
        std::mutex  mutex;
        double      currentTime  = 0.0;  ///< 当前处理时间（秒）
        double      clipDuration = 0.0;  ///< 剪切片段时长（秒），0表示使用总时长
        double      startTime    = 0.0;  ///< 开始时间（用于剪切）
        std::string displayTime;         ///< 显示时间字符串
        std::string speed;               ///< 处理速度
        std::string timeRange;           ///< 时间范围显示
        bool        hasProgress = false; ///< 是否有新的进度信息
    };

public:
    DECLARE_CREATE_DEFAULT(AVProgressBar)

    AVProgressBar(const ProgressBarConfig::Ptr &config = nullptr);
    AVProgressBar(ProgressBarStyle style);
    AVProgressBar(const std::string_view &configName);
    ~AVProgressBar() override;

public:
    /// 从子类调用的核心方法
    auto updateProgress(XExec &exec, const std::string_view &taskName,
                        const std::map<std::string, ParameterValue> &inputParams) -> void override;

    /// 供子类调用的辅助方法
protected:
    /// 开始进度监控
    auto startProgressMonitoring(XExec &exec, const std::shared_ptr<AVProgressState> &progressState,
                                 const std::string_view &srcPath = "", const std::string_view &dstPath = "") -> void;

    /// 解析FFmpeg输出行
    auto parseFFmpegOutputLine(const std::string &line, double &outTime, std::string &displayTime,
                               std::string &speed) const -> bool;

    /// 时间字符串转秒数
    auto parseTimeToSeconds(const std::string &timeStr) const -> double;

    /// 秒数转时间字符串
    auto secondsToTimeString(double seconds, bool showMilliseconds = false) const -> std::string;

    /// 格式化时间范围
    auto formatTimeRange(double startTime, double endTime) const -> std::string;

    /// 计算进度百分比
    auto calculateProgress(double currentTime, double startTime = 0.0, double totalDuration = 0.0) const -> float;

    /// 估算视频总时长
    auto estimateTotalDuration(const std::string_view &srcPath) const -> double;

    /// 获取进度信息字符串
    auto getProgressInfo(double currentTime, double startTime, double totalDuration, const std::string &displayTime,
                         const std::string &speed, float progressPercent) const -> std::string;

    /// 显示任务信息
    auto showTaskInfo(const std::string_view &srcPath, const std::string_view &dstPath, double totalDuration = 0.0,
                      const std::string &timeRange = "") const -> void;

    /// 显示完成信息
    auto showCompletionInfo(const std::string_view &dstPath, const std::chrono::seconds &elapsedTime) const -> void;

    /// 设置进度状态
    auto setProgressState(const std::shared_ptr<AVProgressState> &state, double startTime = 0.0,
                          double clipDuration = 0.0, const std::string &timeRange = "") -> void;

private:
    class PImpl;
    std::unique_ptr<PImpl> impl_;
};

IMPLEMENT_CREATE_DEFAULT(AVProgressBar)

#endif // AV_PROGRESS_BAR_H
