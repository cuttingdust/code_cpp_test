#include "AVProgressBar.h"
#include "ParameterValue.h"
#include "XFile.h"
#include "XExec.h"
#include "XTool.h"

#include <iostream>
#include <regex>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <filesystem>

class AVProgressBar::PImpl
{
public:
    PImpl(AVProgressBar *owner);
    ~PImpl() = default;

public:
    /// 主进度监控循环
    auto runProgressLoop(XExec &exec, const std::shared_ptr<AVProgressState> &progressState,
                         const std::string_view &dstPath) -> void;

    /// 计算剩余时间
    auto calculateRemainingTime(double currentTime, double startTime, double totalDuration,
                                const std::string &speed) const -> std::string;

public:
    AVProgressBar *owner_ = nullptr;
    std::string    sourceFile_;   ///< 源文件路径（缓存）
    std::string    timeRangeStr_; ///< 时间范围字符串
};

AVProgressBar::PImpl::PImpl(AVProgressBar *owner) : owner_(owner)
{
}

auto AVProgressBar::PImpl::runProgressLoop(XExec &exec, const std::shared_ptr<AVProgressState> &progressState,
                                           const std::string_view &dstPath) -> void
{
    /// 主进度监控循环
    float       lastPercent = 0.0f;
    std::string lastSpeed;
    std::string lastTime;
    auto        startTimePoint   = std::chrono::steady_clock::now();
    auto        lastProgressTime = startTimePoint;
    int         progressCount    = 0;
    bool        hasTotalDuration = (progressState->clipDuration > 0);

    while (exec.isRunning())
    {
        /// 获取进度信息
        double      currentTime = 0.0;
        std::string displayTime;
        std::string speed;
        bool        hasProgress = false;

        {
            std::lock_guard<std::mutex> lock(progressState->mutex);
            currentTime                = progressState->currentTime;
            displayTime                = progressState->displayTime;
            speed                      = progressState->speed;
            hasProgress                = progressState->hasProgress;
            progressState->hasProgress = false;
        }

        /// 计算运行时间
        auto now     = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - startTimePoint);

        if (hasProgress && (hasTotalDuration || !displayTime.empty()))
        {
            /// 计算进度百分比
            float progressPercent = 0.0f;
            if (hasTotalDuration)
            {
                progressPercent =
                        owner_->calculateProgress(currentTime, progressState->startTime, progressState->clipDuration);
            }
            else if (!displayTime.empty())
            {
                /// 没有总时长，模拟进度
                progressPercent = std::min(lastPercent + 0.2f, 99.0f);
            }

            if (progressPercent > lastPercent || displayTime != lastTime || speed != lastSpeed)
            {
                progressCount++;
                lastPercent      = progressPercent;
                lastTime         = displayTime;
                lastSpeed        = speed;
                lastProgressTime = now;

                /// 构建显示信息
                std::string progressInfo =
                        owner_->getProgressInfo(currentTime, progressState->startTime, progressState->clipDuration,
                                                displayTime, speed, progressPercent);

                /// 更新进度条
                owner_->setProgress(progressPercent);
                owner_->setMessage(progressInfo);
                owner_->updateDisplay();

                /// 定期显示详细状态
                if (progressCount % 100 == 0)
                {
                    std::cout << "\n[处理状态] 进度: " << progressPercent << "%, "
                              << "速度: " << (speed.empty() ? "N/A" : speed) << ", 已运行: " << elapsed.count() << "秒"
                              << std::endl;
                }
            }
        }
        else
        {
            /// 检查是否长时间没有进度更新
            auto timeSinceLastProgress = std::chrono::duration_cast<std::chrono::seconds>(now - lastProgressTime);

            if (timeSinceLastProgress.count() > 3)
            {
                std::stringstream message;
                message << "处理中... 已运行 " << elapsed.count() << " 秒";

                if (hasTotalDuration && lastPercent < 99.0f)
                {
                    /// 轻微增加进度（避免卡住的感觉）
                    float newPercent = std::min(lastPercent + 0.1f, 99.0f);
                    if (newPercent > lastPercent)
                    {
                        owner_->setProgress(newPercent);
                        lastPercent = newPercent;
                    }
                }

                owner_->setMessage(message.str());
                owner_->updateDisplay();
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(80));
    }

    /// 任务完成
    auto endTimePoint = std::chrono::steady_clock::now();
    auto totalElapsed = std::chrono::duration_cast<std::chrono::seconds>(endTimePoint - startTimePoint);

    owner_->markAsCompleted();
    owner_->showCompletionInfo(dstPath, totalElapsed);
}

auto AVProgressBar::PImpl::calculateRemainingTime(double currentTime, double startTime, double totalDuration,
                                                  const std::string &speed) const -> std::string
{
    if (speed.empty() || totalDuration <= 0)
        return "";

    try
    {
        /// 解析速度倍数
        std::string speedNum = speed;
        std::erase(speedNum, 'x');
        double speedFactor = std::stod(speedNum);

        if (speedFactor > 0)
        {
            double elapsedTime   = currentTime - startTime;
            double remainingTime = (totalDuration - elapsedTime) / speedFactor;

            if (remainingTime > 0 && remainingTime < 3600) /// 小于1小时才显示
            {
                int remainingSecs = static_cast<int>(remainingTime);
                int mins          = remainingSecs / 60;
                int secs          = remainingSecs % 60;

                std::stringstream ss;
                if (mins > 0)
                {
                    ss << "剩余: " << mins << "分" << secs << "秒";
                }
                else
                {
                    ss << "剩余: " << secs << "秒";
                }
                return ss.str();
            }
        }
    }
    catch (...)
    {
        /// 速度解析失败
    }

    return "";
}

/// ==================== AVProgressBar 实现 ====================

AVProgressBar::AVProgressBar(const ProgressBarConfig::Ptr &config) :
    TaskProgressBar(config), impl_(std::make_unique<PImpl>(this))
{
}

AVProgressBar::AVProgressBar(ProgressBarStyle style) : TaskProgressBar(style), impl_(std::make_unique<PImpl>(this))
{
}

AVProgressBar::AVProgressBar(const std::string_view &configName) :
    TaskProgressBar(configName), impl_(std::make_unique<PImpl>(this))
{
}

AVProgressBar::~AVProgressBar() = default;


auto AVProgressBar::updateProgress(XExec &exec, const std::string_view &taskName,
                                   const std::map<std::string, ParameterValue> &inputParams) -> void
{
    /// 基类实现，子类应该重写
    TaskProgressBar::updateProgress(exec, taskName, inputParams);
}


auto AVProgressBar::startProgressMonitoring(XExec &exec, const std::shared_ptr<AVProgressState> &progressState,
                                            const std::string_view &srcPath, const std::string_view &dstPath) -> void
{
    /// 显示任务信息
    if (!srcPath.empty() && !dstPath.empty())
    {
        showTaskInfo(srcPath, dstPath, progressState->clipDuration, progressState->timeRange);
    }

    /// 设置FFmpeg输出回调
    exec.setOutputCallback(
            [progressState, this](const std::string_view &line, bool isStderr)
            {
                if (!isStderr)
                {
                    double      outTime = 0.0;
                    std::string displayTime;
                    std::string speed;

                    if (parseFFmpegOutputLine(std::string(line), outTime, displayTime, speed))
                    {
                        std::lock_guard<std::mutex> lock(progressState->mutex);

                        progressState->currentTime = outTime;
                        if (!displayTime.empty())
                        {
                            progressState->displayTime = displayTime;
                        }
                        if (!speed.empty())
                        {
                            progressState->speed = speed;
                        }

                        progressState->hasProgress = true;
                    }
                }
            });

    /// 显示初始进度条
    setProgress(0.0f);
    setMessage("准备开始处理...");
    updateDisplay();

    /// 启动进度监控循环
    impl_->runProgressLoop(exec, progressState, dstPath);
}

auto AVProgressBar::parseFFmpegOutputLine(const std::string &line, double &outTime, std::string &displayTime,
                                          std::string &speed) const -> bool
{
    bool hasProgress = false;

    /// 查找 out_time=
    size_t timePos = line.find("out_time=");
    if (timePos != std::string::npos)
    {
        size_t end = line.find(' ', timePos);
        if (end == std::string::npos)
            end = line.length();

        displayTime = line.substr(timePos + 9, end - timePos - 9);

        /// 将时间字符串转换为秒数
        try
        {
            /// 格式: HH:MM:SS.mmmmmm
            std::regex  timeRegex(R"((\d{2}):(\d{2}):(\d{2})\.(\d+))");
            std::smatch matches;

            if (std::regex_search(displayTime, matches, timeRegex) && matches.size() >= 5)
            {
                int         hours    = std::stoi(matches[1].str());
                int         minutes  = std::stoi(matches[2].str());
                int         seconds  = std::stoi(matches[3].str());
                std::string microStr = matches[4].str();

                /// 取前3位作为毫秒
                int microLen     = std::min(3, static_cast<int>(microStr.length()));
                int milliseconds = std::stoi(microStr.substr(0, microLen));

                outTime     = hours * 3600 + minutes * 60 + seconds + milliseconds / 1000.0;
                hasProgress = true;
            }
        }
        catch (...)
        {
            /// 转换失败
        }
    }

    /// 查找 speed=
    size_t speedPos = line.find("speed=");
    if (speedPos != std::string::npos)
    {
        size_t end = line.find(' ', speedPos);
        if (end == std::string::npos)
            end = line.length();

        speed = line.substr(speedPos + 6, end - speedPos - 6);

        /// 清理速度字符串
        std::erase(speed, ' ');
        if (!speed.empty() && speed.back() != 'x')
        {
            speed += "x";
        }
    }

    return hasProgress;
}

auto AVProgressBar::parseTimeToSeconds(const std::string &timeStr) const -> double
{
    if (timeStr.empty())
        return 0.0;

    /// 1. 检查是否是纯数字（秒数）
    static std::regex numberRegex(R"(^\d+(\.\d+)?$)");
    if (std::regex_match(timeStr, numberRegex))
    {
        try
        {
            return std::stod(timeStr);
        }
        catch (...)
        {
            return 0.0;
        }
    }

    /// 2. 检查时间格式 HH:MM:SS 或 HH:MM:SS.sss
    static std::regex timeRegex(R"(^(\d{1,2}):([0-5]?\d):([0-5]?\d)(?:\.(\d{1,3}))?$)");
    std::smatch       matches;

    if (std::regex_match(timeStr, matches, timeRegex))
    {
        int    hours   = std::stoi(matches[1].str());
        int    minutes = std::stoi(matches[2].str());
        int    seconds = std::stoi(matches[3].str());
        double total   = hours * 3600.0 + minutes * 60.0 + seconds;

        if (matches[4].matched)
        {
            std::string millisStr = matches[4].str();
            /// 补全到3位
            while (millisStr.length() < 3)
            {
                millisStr += "0";
            }
            double milliseconds = std::stod(millisStr) / 1000.0;
            total += milliseconds;
        }

        return total;
    }

    return 0.0;
}

auto AVProgressBar::secondsToTimeString(double seconds, bool showMilliseconds) const -> std::string
{
    int totalSeconds = static_cast<int>(seconds);
    int hours        = totalSeconds / 3600;
    int minutes      = (totalSeconds % 3600) / 60;
    int secs         = totalSeconds % 60;
    int millis       = static_cast<int>((seconds - totalSeconds) * 1000);

    std::stringstream ss;
    if (hours > 0)
    {
        ss << std::setw(2) << std::setfill('0') << hours << ":";
    }
    ss << std::setw(2) << std::setfill('0') << minutes << ":" << std::setw(2) << std::setfill('0') << secs;

    if (showMilliseconds && millis > 0)
    {
        ss << "." << std::setw(3) << std::setfill('0') << millis;
    }

    return ss.str();
}

auto AVProgressBar::formatTimeRange(double startTime, double endTime) const -> std::string
{
    std::stringstream ss;
    ss << secondsToTimeString(startTime) << " - " << secondsToTimeString(endTime);
    return ss.str();
}

auto AVProgressBar::calculateProgress(double currentTime, double startTime, double totalDuration) const -> float
{
    if (totalDuration <= 0)
        return 0.0f;

    /// 计算相对时间（从开始点算起）
    double relativeTime = currentTime - startTime;
    relativeTime        = std::max<double>(relativeTime, 0);

    /// 计算百分比
    float percent = relativeTime / totalDuration * 100.0f;

    /// 限制在0-100%之间
    percent = std::min(99.5f, std::max(0.0f, percent));

    return percent;
}

auto AVProgressBar::estimateTotalDuration(const std::string_view &srcPath) const -> double
{
    std::string ffprobeCmd = XTool::getFFprobePath() +
            " -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 \"" +
            std::string{ srcPath } + "\"";

    XExec exec;
    if (exec.start(ffprobeCmd, true))
    {
        exec.wait();
        std::string output = exec.getOutput();

        try
        {
            /// 清理输出
            std::erase(output, '\n');
            std::erase(output, '\r');

            if (!output.empty())
            {
                return std::stod(output);
            }
        }
        catch (...)
        {
            /// 解析失败
        }
    }

    return 0.0;
}

auto AVProgressBar::getProgressInfo(double currentTime, double startTime, double totalDuration,
                                    const std::string &displayTime, const std::string &speed,
                                    float progressPercent) const -> std::string
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1) << progressPercent << "%";

    if (totalDuration > 0)
    {
        /// 显示相对时间
        double relativeTime = currentTime - startTime;
        if (relativeTime >= 0)
        {
            ss << " | 已处理: " << secondsToTimeString(relativeTime, true);
            ss << " / " << secondsToTimeString(totalDuration);
        }
    }
    else if (!displayTime.empty())
    {
        /// 没有总时长，显示当前时间
        ss << " | 当前: " << displayTime;
    }

    if (!speed.empty())
    {
        ss << " | " << speed;
    }

    /// 计算剩余时间（如果有总时长和速度）
    if (totalDuration > 0 && !speed.empty())
    {
        std::string remainingTime = impl_->calculateRemainingTime(currentTime, startTime, totalDuration, speed);
        if (!remainingTime.empty())
        {
            ss << " | " << remainingTime;
        }
    }

    return ss.str();
}

auto AVProgressBar::showTaskInfo(const std::string_view &srcPath, const std::string_view &dstPath, double totalDuration,
                                 const std::string &timeRange) const -> void
{
    std::cout << "\n=== 音视频处理信息 ===" << std::endl;
    std::cout << "源文件: " << srcPath << std::endl;
    std::cout << "目标文件: " << dstPath << std::endl;

    if (totalDuration > 0)
    {
        std::cout << "处理时长: " << secondsToTimeString(totalDuration) << " (" << std::fixed << std::setprecision(1)
                  << totalDuration << "秒)" << std::endl;
    }

    if (!timeRange.empty())
    {
        std::cout << "处理范围: " << timeRange << std::endl;
    }

    /// 显示源文件大小
    try
    {
        if (std::filesystem::exists(srcPath))
        {
            auto fileSize = std::filesystem::file_size(srcPath);
            std::cout << "源文件大小: " << XFile::formatFileSize(fileSize) << std::endl;
        }
    }
    catch (...)
    {
        /// 忽略文件大小获取错误
    }

    std::cout << "===================\n" << std::endl;
}

auto AVProgressBar::showCompletionInfo(const std::string_view &dstPath, const std::chrono::seconds &elapsedTime) const
        -> void
{
    std::cout << "\n=== 处理完成 ===" << std::endl;
    std::cout << "总运行时间: " << elapsedTime.count() << " 秒" << std::endl;
    std::cout << "输出文件: " << dstPath << std::endl;

    /// 显示输出文件大小
    try
    {
        if (std::filesystem::exists(dstPath))
        {
            auto fileSize = std::filesystem::file_size(dstPath);
            std::cout << "输出文件大小: " << XFile::formatFileSize(fileSize) << std::endl;
        }
    }
    catch (...)
    {
        /// 忽略文件大小获取错误
    }

    std::cout << "================\n" << std::endl;
}

auto AVProgressBar::setProgressState(const std::shared_ptr<AVProgressState> &state, double startTime,
                                     double clipDuration, const std::string &timeRange) -> void
{
    impl_->sourceFile_   = "";
    impl_->timeRangeStr_ = timeRange;

    state->startTime    = startTime;
    state->clipDuration = clipDuration;
    state->timeRange    = timeRange;
}
