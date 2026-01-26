#include "CutProgressBar.h"

#include "ParameterValue.h"
#include "XFile.h"
#include "XExec.h"

#include <iostream>
#include <regex>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>

class CutProgressBar::PImpl
{
public:
    PImpl(CutProgressBar *owner);
    ~PImpl() = default;

public:
    /// 剪切任务进度显示
    auto showCutProgressImpl(XExec &exec, const std::string_view &srcPath, const std::string_view &dstPath,
                             double startTime, double clipDuration) -> void;

    /// 解析剪切参数
    auto parseCutParams(const std::map<std::string, ParameterValue> &params, double &startTime, double &clipDuration,
                        std::string &timeRangeStr) -> bool;

    /// 视频时长估算
    auto estimateVideoDuration(const std::string_view &srcPath) const -> double;

    /// 时间字符串转秒数
    auto parseTimeToSeconds(const std::string &timeStr) const -> double;

    /// 秒数转时间字符串
    auto secondsToTimeString(double seconds, bool showMilliseconds = false) const -> std::string;

    /// 格式化时间范围
    auto formatTimeRange(double startTime, double endTime) const -> std::string;

    /// 计算剪切进度
    auto calculateCutProgress(double currentTime, double startTime, double clipDuration) const -> float;

    /// 解析FFmpeg输出
    auto parseFFmpegOutputLine(const std::string &line, double &outTime, std::string &displayTime,
                               std::string &speed) const -> bool;

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

auto CutProgressBar::PImpl::parseTimeToSeconds(const std::string &timeStr) const -> double
{
    if (timeStr.empty())
        return 0.0;

    // 1. 检查是否是纯数字（秒数）
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

    // 2. 检查时间格式 HH:MM:SS 或 HH:MM:SS.sss
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
            // 补全到3位
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

auto CutProgressBar::PImpl::secondsToTimeString(double seconds, bool showMilliseconds) const -> std::string
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

auto CutProgressBar::PImpl::formatTimeRange(double startTime, double endTime) const -> std::string
{
    std::stringstream ss;
    ss << secondsToTimeString(startTime) << " - " << secondsToTimeString(endTime);
    return ss.str();
}

auto CutProgressBar::PImpl::parseCutParams(const std::map<std::string, ParameterValue> &params, double &startTime,
                                           double &clipDuration, std::string &timeRangeStr) -> bool
{
    // 解析开始时间
    startTime = 0.0;
    if (params.contains("--start"))
    {
        startTime = parseTimeToSeconds(params.at("--start").asString());
    }

    // 解析持续时间
    clipDuration = 0.0;
    if (params.contains("--duration"))
    {
        clipDuration = parseTimeToSeconds(params.at("--duration").asString());
    }
    else if (params.contains("--end"))
    {
        double endTime = parseTimeToSeconds(params.at("--end").asString());
        clipDuration   = endTime - startTime;
        if (clipDuration < 0)
            clipDuration = 0;
    }

    // 生成时间范围字符串
    if (clipDuration > 0)
    {
        double endTime = startTime + clipDuration;
        timeRangeStr   = formatTimeRange(startTime, endTime);
    }
    else
    {
        timeRangeStr = "从 " + secondsToTimeString(startTime) + " 开始";
    }

    return (clipDuration > 0);
}

auto CutProgressBar::PImpl::estimateVideoDuration(const std::string_view &srcPath) const -> double
{
    std::string ffprobeCmd = std::string(FFPROBE_PATH) +
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

auto CutProgressBar::PImpl::calculateCutProgress(double currentTime, double startTime, double clipDuration) const
        -> float
{
    if (clipDuration <= 0)
        return 0.0f;

    // 计算相对时间（从剪切开始点算起）
    double relativeTime = currentTime - startTime;
    if (relativeTime < 0)
        relativeTime = 0;

    // 计算百分比
    float percent = (relativeTime / clipDuration) * 100.0f;

    // 限制在0-100%之间
    percent = std::min(99.5f, std::max(0.0f, percent));

    return percent;
}

auto CutProgressBar::PImpl::parseFFmpegOutputLine(const std::string &line, double &outTime, std::string &displayTime,
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

auto CutProgressBar::PImpl::showCutProgressImpl(XExec &exec, const std::string_view &srcPath,
                                                const std::string_view &dstPath, double startTime, double clipDuration)
        -> void
{
    /// 获取视频总时长（用于显示参考）
    double totalDuration = estimateVideoDuration(srcPath);

    /// 设置剪切信息
    startTime_    = startTime;
    clipDuration_ = clipDuration;
    sourceFile_   = std::string(srcPath);

    /// 显示剪切信息
    std::cout << "\n=== 视频剪切信息 ===" << std::endl;
    std::cout << "源文件: " << srcPath << std::endl;
    std::cout << "目标文件: " << dstPath << std::endl;
    if (totalDuration > 0)
    {
        std::cout << "视频总时长: " << secondsToTimeString(totalDuration) << std::endl;
    }
    if (clipDuration > 0)
    {
        std::cout << "剪切范围: " << timeRangeStr_ << " (" << std::fixed << std::setprecision(1) << clipDuration
                  << "秒)" << std::endl;
        std::cout << "剪切比例: " << std::fixed << std::setprecision(1) << (clipDuration / totalDuration * 100.0) << "%"
                  << std::endl;
    }
    std::cout << "===================\n" << std::endl;

    /// 创建进度状态共享对象
    auto progressState           = std::make_shared<ProgressState>();
    progressState->clipDuration  = clipDuration;
    progressState->totalDuration = totalDuration;

    if (clipDuration > 0)
    {
        progressState->timeRange = formatTimeRange(startTime, startTime + clipDuration);
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
    owner_->setProgress(0.0f);
    owner_->setMessage("准备开始剪切...");
    owner_->updateDisplay();

    /// 主进度监控循环
    float       lastPercent = 0.0f;
    std::string lastSpeed;
    std::string lastTime;
    auto        startTimePoint   = std::chrono::steady_clock::now();
    auto        lastProgressTime = startTimePoint;

    int  progressCount = 0;
    bool showClipInfo  = true;

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

        if (hasProgress && clipDuration_ > 0)
        {
            /// 计算剪切进度
            float actualPercent = calculateCutProgress(currentTime, startTime_, clipDuration_);

            if (actualPercent > lastPercent || displayTime != lastTime || speed != lastSpeed)
            {
                progressCount++;

                /// 更新进度条
                owner_->setProgress(actualPercent);
                lastPercent      = actualPercent;
                lastTime         = displayTime;
                lastSpeed        = speed;
                lastProgressTime = now;

                /// 构建显示信息
                std::stringstream postfix;
                postfix << std::fixed << std::setprecision(1) << actualPercent << "%";

                if (!displayTime.empty())
                {
                    /// 显示相对于剪切开始的时间
                    double relativeTime = currentTime - startTime_;
                    if (relativeTime >= 0)
                    {
                        postfix << " | 已处理: " << secondsToTimeString(relativeTime, true);
                    }

                    if (clipDuration_ > 0)
                    {
                        postfix << " / " << secondsToTimeString(clipDuration_);
                    }
                }

                if (!speed.empty())
                {
                    postfix << " | " << speed;
                }

                /// 显示剩余时间估算（如果可能）
                if (actualPercent > 5.0 && !speed.empty())
                {
                    try
                    {
                        /// 解析速度倍数
                        std::string speedNum = speed;
                        std::erase(speedNum, 'x');
                        double speedFactor = std::stod(speedNum);

                        if (speedFactor > 0)
                        {
                            double remainingTime = (clipDuration_ - (currentTime - startTime_)) / speedFactor;
                            if (remainingTime > 0 && remainingTime < 3600) /// 小于1小时才显示
                            {
                                int remainingSecs = static_cast<int>(remainingTime);
                                int mins          = remainingSecs / 60;
                                int secs          = remainingSecs % 60;

                                if (mins > 0)
                                {
                                    postfix << " | 剩余: " << mins << "分" << secs << "秒";
                                }
                                else
                                {
                                    postfix << " | 剩余: " << secs << "秒";
                                }
                            }
                        }
                    }
                    catch (...)
                    {
                        /// 速度解析失败
                    }
                }

                owner_->setMessage(postfix.str());
                owner_->updateDisplay();

                /// 每100次更新显示一次状态（可选）
                if (progressCount % 100 == 0 && showClipInfo)
                {
                    std::cout << "\n[剪切状态] 进度: " << actualPercent << "%, "
                              << "速度: " << (speed.empty() ? "N/A" : speed) << ", 已运行: " << elapsed.count() << "秒"
                              << std::endl;
                }
            }
        }
        else if (hasProgress && !displayTime.empty())
        {
            /// 没有指定剪切时长，但有时问信息
            progressCount++;

            /// 轻微增加进度（模拟）
            float newPercent = std::min(lastPercent + 0.2f, 99.0f);
            if (newPercent > lastPercent)
            {
                owner_->setProgress(newPercent);
                lastPercent = newPercent;
            }

            /// 构建显示信息
            std::stringstream postfix;
            postfix << std::fixed << std::setprecision(1) << lastPercent << "%";

            if (!displayTime.empty())
            {
                postfix << " | 当前: " << displayTime;
            }

            if (!speed.empty() && speed != lastSpeed)
            {
                postfix << " | " << speed;
                lastSpeed = speed;
            }

            owner_->setMessage(postfix.str());
            owner_->updateDisplay();
        }
        else
        {
            /// 没有进度信息，检查是否长时间没有更新
            auto timeSinceLastProgress = std::chrono::duration_cast<std::chrono::seconds>(now - lastProgressTime);

            if (timeSinceLastProgress.count() > 3)
            {
                // 长时间没有进度更新
                std::stringstream postfix;
                postfix << "处理中... 已运行 " << elapsed.count() << " 秒";

                if (clipDuration_ > 0 && lastPercent < 99.0f)
                {
                    // 轻微增加进度（避免卡住的感觉）
                    float newPercent = std::min(lastPercent + 0.1f, 99.0f);
                    if (newPercent > lastPercent)
                    {
                        owner_->setProgress(newPercent);
                        lastPercent = newPercent;
                    }
                }

                owner_->setMessage(postfix.str());
                owner_->updateDisplay();
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(80));
    }

    // 任务完成
    auto endTimePoint = std::chrono::steady_clock::now();
    auto totalElapsed = std::chrono::duration_cast<std::chrono::seconds>(endTimePoint - startTimePoint);

    std::cout << "\n=== 剪切完成 ===" << std::endl;
    std::cout << "总运行时间: " << totalElapsed.count() << " 秒" << std::endl;
    std::cout << "进度更新次数: " << progressCount << std::endl;
    std::cout << "输出文件: " << dstPath << std::endl;
    std::cout << "文件大小: " << XFile::formatFileSize(std::filesystem::file_size(dstPath)) << std::endl;
    std::cout << "================\n" << std::endl;

    // 标记进度条完成
    owner_->markAsCompleted("剪切完成 ✓");
}

CutProgressBar::CutProgressBar(const ProgressBarConfig::Ptr &config) :
    TaskProgressBar(config), impl_(std::make_unique<PImpl>(this))
{
}

CutProgressBar::CutProgressBar(ProgressBarStyle style) : TaskProgressBar(style), impl_(std::make_unique<PImpl>(this))
{
}

CutProgressBar::CutProgressBar(const std::string_view &configName) :
    TaskProgressBar(configName), impl_(std::make_unique<PImpl>(this))
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

        // 解析剪切参数
        if (impl_->parseCutParams(inputParams, startTime, clipDuration, timeRangeStr))
        {
            impl_->timeRangeStr_ = timeRangeStr;
        }

        // 执行剪切进度显示
        impl_->showCutProgressImpl(exec, srcPath, dstPath, startTime, clipDuration);
    }
    else
    {
        // 参数不完整，使用默认进度显示
        TaskProgressBar::updateProgress(exec, taskName, inputParams);
    }
}

auto CutProgressBar::markAsCompleted(const std::string_view &message) -> void
{
    std::string finalMessage = std::string{ message };
    if (!impl_->timeRangeStr_.empty())
    {
        finalMessage += " [" + impl_->timeRangeStr_ + "]";
    }

    TaskProgressBar::markAsCompleted(finalMessage);
}

auto CutProgressBar::markAsFailed(const std::string_view &message) -> void
{
    std::string finalMessage = "剪切失败: " + std::string(message);
    if (!impl_->timeRangeStr_.empty())
    {
        finalMessage += " [" + impl_->timeRangeStr_ + "]";
    }

    TaskProgressBar::markAsFailed(finalMessage);
}

auto CutProgressBar::setTimeRange(double startTime, double endTime) -> void
{
    impl_->startTime_    = startTime;
    impl_->clipDuration_ = endTime - startTime;
    if (impl_->clipDuration_ > 0)
    {
        impl_->timeRangeStr_ = impl_->formatTimeRange(startTime, endTime);
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
