#include "CutProgressBar.h"
#include "ParameterValue.h"
#include "XFile.h"
#include "XExec.h"

#include <algorithm>
#include <iostream>

class CutProgressBar::PImpl
{
public:
    PImpl(CutProgressBar *owner);
    ~PImpl() = default;

    auto parseCutParams(const std::map<std::string, ParameterValue> &params, double &startTime, double &clipDuration,
                        std::string &timeRangeStr) -> bool;

public:
    CutProgressBar *owner_        = nullptr;
    double          startTime_    = 0.0; ///< 剪切开始时间
    double          clipDuration_ = 0.0; ///< 剪切时长
    std::string     sourceFile_;         ///< 源文件路径
    std::string     timeRangeStr_;       ///< 时间范围显示字符串
};

CutProgressBar::PImpl::PImpl(CutProgressBar *owner) : owner_(owner)
{
}

auto CutProgressBar::PImpl::parseCutParams(const std::map<std::string, ParameterValue> &params, double &startTime,
                                           double &clipDuration, std::string &timeRangeStr) -> bool
{
    /// 解析开始时间
    startTime = 0.0;
    if (params.contains("--start"))
    {
        startTime = owner_->parseTimeToSeconds(params.at("--start").asString());
    }

    /// 解析持续时间
    clipDuration = 0.0;
    if (params.contains("--duration"))
    {
        clipDuration = owner_->parseTimeToSeconds(params.at("--duration").asString());
    }
    else if (params.contains("--end"))
    {
        double endTime = owner_->parseTimeToSeconds(params.at("--end").asString());
        clipDuration   = endTime - startTime;
        clipDuration   = std::max<double>(clipDuration, 0);
    }

    /// 生成时间范围字符串
    if (clipDuration > 0)
    {
        double endTime = startTime + clipDuration;
        timeRangeStr   = owner_->formatTimeRange(startTime, endTime);
    }
    else
    {
        timeRangeStr = "从 " + owner_->secondsToTimeString(startTime) + " 开始";
    }

    return (clipDuration > 0);
}

CutProgressBar::CutProgressBar(const ProgressBarConfig::Ptr &config) :
    AVProgressBar(config), impl_(std::make_unique<PImpl>(this))
{
}

CutProgressBar::CutProgressBar(ProgressBarStyle style) : AVProgressBar(style), impl_(std::make_unique<PImpl>(this))
{
}

CutProgressBar::CutProgressBar(const std::string_view &configName) :
    AVProgressBar(configName), impl_(std::make_unique<PImpl>(this))
{
}

CutProgressBar::~CutProgressBar() = default;

auto CutProgressBar::updateProgress(XExec &exec, const std::string_view &taskName,
                                    const std::map<std::string, ParameterValue> &inputParams) -> void
{
    if (inputParams.contains("--input") && inputParams.contains("--output"))
    {
        auto srcPath = inputParams.at("--input").asString();
        auto dstPath = inputParams.at("--output").asString();

        double      startTime    = 0.0;
        double      clipDuration = 0.0;
        std::string timeRangeStr;

        /// 解析剪切参数
        if (impl_->parseCutParams(inputParams, startTime, clipDuration, timeRangeStr))
        {
            impl_->timeRangeStr_ = timeRangeStr;
            impl_->startTime_    = startTime;
            impl_->clipDuration_ = clipDuration;
            impl_->sourceFile_   = srcPath;
        }

        /// 创建进度状态
        auto progressState = std::make_shared<AVProgressState>();
        setProgressState(progressState, startTime, clipDuration, timeRangeStr);

        /// 开始进度监控
        startProgressMonitoring(exec, progressState, srcPath, dstPath);
    }
}

auto CutProgressBar::markAsCompleted(const std::string_view &message) -> void
{
    std::string finalMessage = std::string{ message };
    if (!impl_->timeRangeStr_.empty())
    {
        finalMessage += " [" + impl_->timeRangeStr_ + "]";
    }

    AVProgressBar::markAsCompleted(finalMessage);
}

auto CutProgressBar::markAsFailed(const std::string_view &message) -> void
{
    std::string finalMessage = "剪切失败: " + std::string(message);
    if (!impl_->timeRangeStr_.empty())
    {
        finalMessage += " [" + impl_->timeRangeStr_ + "]";
    }

    AVProgressBar::markAsFailed(finalMessage);
}

auto CutProgressBar::setTimeRange(double startTime, double endTime) -> void
{
    impl_->startTime_    = startTime;
    impl_->clipDuration_ = endTime - startTime;
    if (impl_->clipDuration_ > 0)
    {
        impl_->timeRangeStr_ = formatTimeRange(startTime, endTime);
    }
}

auto CutProgressBar::setClipDuration(double duration) -> void
{
    impl_->clipDuration_ = duration;
}

auto CutProgressBar::setSourceFile(const std::string &filePath) -> void
{
    impl_->sourceFile_ = filePath;
}
