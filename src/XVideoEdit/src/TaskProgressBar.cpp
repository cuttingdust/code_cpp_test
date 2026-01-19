#include "TaskProgressBar.h"
#include "XExec.h"

#include <indicators/progress_bar.hpp>
#include <indicators/cursor_control.hpp>

#include <filesystem>
#include <iostream>
#include <thread>
#include <regex>
#include <chrono>
#include <sstream>
#include <iomanip>

using namespace indicators;

class TaskProgressBar::Impl
{
public:
    Impl();
    ~Impl();

public:
    void setupProgressBar(const std::string& title);
    void showWithFfmpegImpl(XExec& exec, const std::string& srcPath);
    void showGenericImpl(XExec& exec, const std::string& taskName);
    void updateProgressImpl(float percent, const std::string& message);
    void markAsCompletedImpl(const std::string& message);
    void markAsFailedImpl(const std::string& message);

private:
    ProgressBar        bar_;
    std::atomic<bool>  isActive_{ false };
    std::atomic<float> currentPercent_{ 0.0f };
    std::string        currentMessage_;
    std::mutex         mutex_;

    float       estimateTotalDuration(const std::string& srcPath) const;
    std::string formatDuration(float seconds) const;
};

TaskProgressBar::Impl::Impl()
{
    /// 使用简洁的配置方式初始化进度条
    bar_.set_option(option::BarWidth{ 50 });
    bar_.set_option(option::Start{ "[" });
    bar_.set_option(option::Fill{ "=" });
    bar_.set_option(option::Lead{ ">" });
    bar_.set_option(option::Remainder{ " " });
    bar_.set_option(option::End{ "]" });
    bar_.set_option(option::ShowPercentage{ true });
    bar_.set_option(option::ShowElapsedTime{ true });
    bar_.set_option(option::ShowRemainingTime{ true });
    bar_.set_option(option::ForegroundColor{ Color::magenta });
    bar_.set_option(option::FontStyles{ std::vector<FontStyle>{ FontStyle::bold } });
}

TaskProgressBar::Impl::~Impl()
{
    if (isActive_)
    {
        /// 确保光标恢复正常
        show_console_cursor(true);
    }
}

auto TaskProgressBar::Impl::setupProgressBar(const std::string& title) -> void
{
    bar_.set_option(option::PostfixText{ title });
    currentPercent_ = 0.0f;
    isActive_       = true;

    /// 隐藏光标以获得更流畅的显示
    show_console_cursor(false);
}

void TaskProgressBar::Impl::showWithFfmpegImpl(XExec& exec, const std::string& srcPath)
{
    // 设置进度条标题
    std::string title = "转码: " + std::filesystem::path(srcPath).filename().string();
    setupProgressBar(title);

    // 获取视频总时长
    float totalDuration    = estimateTotalDuration(srcPath);
    bool  hasTotalDuration = (totalDuration > 0);

    std::cout << "\n开始" << title << std::endl;
    if (hasTotalDuration)
    {
        std::cout << "视频总时长: " << formatDuration(totalDuration) << std::endl;
    }
    else
    {
        std::cout << "警告: 无法获取视频总时长，进度估算可能不准确" << std::endl;
    }

    // 显示初始进度条
    bar_.print_progress();

    // 共享进度状态 - 简化版本
    struct ProgressState
    {
        std::mutex  mutex;
        double      currentTime = 0.0; // 当前时间（秒）
        std::string displayTime;       // 显示时间
        std::string speed;
        bool        hasProgress = false;
    };

    auto progressState = std::make_shared<ProgressState>();

    // 设置FFmpeg输出回调 - 简化处理
    exec.setOutputCallback(
            [progressState](const std::string& line, bool isStderr)
            {
                if (!isStderr)
                {
                    // 调试：显示所有进度行
                    // if (line.find("progress=") != std::string::npos || line.find("out_time=") != std::string::npos ||
                    //     line.find("speed=") != std::string::npos)
                    // {
                    //     std::cout << "进度行: " << line << std::endl;
                    // }

                    // 查找 out_time= 和 speed=
                    std::string timeStr;
                    std::string speedStr;

                    // 解析时间
                    size_t timePos = line.find("out_time=");
                    if (timePos != std::string::npos)
                    {
                        size_t end = line.find(' ', timePos);
                        if (end == std::string::npos)
                            end = line.length();
                        timeStr = line.substr(timePos + 9, end - timePos - 9);
                    }

                    // 解析速度
                    size_t speedPos = line.find("speed=");
                    if (speedPos != std::string::npos)
                    {
                        size_t end = line.find(' ', speedPos);
                        if (end == std::string::npos)
                            end = line.length();
                        std::string rawSpeed = line.substr(speedPos + 6, end - speedPos - 6);

                        // 清理速度字符串（移除可能的空格）
                        rawSpeed.erase(std::remove(rawSpeed.begin(), rawSpeed.end(), ' '), rawSpeed.end());
                        if (!rawSpeed.empty() && rawSpeed.back() == 'x')
                        {
                            speedStr = rawSpeed;
                        }
                        else
                        {
                            speedStr = rawSpeed + "x";
                        }
                    }

                    if (!timeStr.empty() || !speedStr.empty())
                    {
                        std::lock_guard<std::mutex> lock(progressState->mutex);

                        if (!timeStr.empty())
                        {
                            progressState->displayTime = timeStr;

                            // 将时间字符串转换为秒数
                            try
                            {
                                // 格式: HH:MM:SS.mmmmmm
                                std::regex  timeRegex(R"((\d{2}):(\d{2}):(\d{2})\.(\d+))");
                                std::smatch matches;

                                if (std::regex_search(timeStr, matches, timeRegex) && matches.size() >= 5)
                                {
                                    int         hours    = std::stoi(matches[1].str());
                                    int         minutes  = std::stoi(matches[2].str());
                                    int         seconds  = std::stoi(matches[3].str());
                                    std::string microStr = matches[4].str();

                                    // 取前2位作为百分秒
                                    int microLen     = std::min(2, static_cast<int>(microStr.length()));
                                    int centiseconds = std::stoi(microStr.substr(0, microLen));

                                    progressState->currentTime =
                                            hours * 3600 + minutes * 60 + seconds + centiseconds / 100.0;
                                }
                            }
                            catch (...)
                            {
                                // 转换失败
                            }
                        }

                        if (!speedStr.empty())
                        {
                            progressState->speed = speedStr;
                        }

                        progressState->hasProgress = true;
                    }
                }
            });

    // 主循环
    float       lastPercent = 0.0f;
    std::string lastSpeed;
    std::string lastTime;
    auto        startTime        = std::chrono::steady_clock::now();
    auto        lastProgressTime = startTime;

    // 进度计数器
    int progressCount = 0;

    while (exec.isRunning())
    {
        // 获取进度信息
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
            progressState->hasProgress = false; // 重置标志
        }

        // 计算运行时间
        auto now     = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - startTime);

        if (hasTotalDuration && currentTime > 0)
        {
            // 计算实际进度百分比
            float actualPercent = (currentTime / totalDuration) * 100.0f;
            actualPercent       = std::min(99.5f, std::max(0.0f, actualPercent));

            if (actualPercent > lastPercent || displayTime != lastTime || speed != lastSpeed)
            {
                progressCount++;

                // 更新进度
                bar_.set_progress(actualPercent);
                lastPercent      = actualPercent;
                lastTime         = displayTime;
                lastSpeed        = speed;
                lastProgressTime = now;

                // 构建显示信息
                std::stringstream postfix;
                postfix << std::fixed << std::setprecision(1) << actualPercent << "%";

                if (!displayTime.empty())
                {
                    // 简化时间显示（去掉多余的0）
                    std::string cleanTime = displayTime;

                    // 移除尾随的0
                    size_t dotPos = cleanTime.find('.');
                    if (dotPos != std::string::npos)
                    {
                        // 移除末尾的0
                        while (cleanTime.length() > dotPos + 1 && (cleanTime.back() == '0' || cleanTime.back() == '.'))
                        {
                            if (cleanTime.back() == '.')
                            {
                                cleanTime.pop_back();
                                break;
                            }
                            cleanTime.pop_back();
                        }
                    }

                    postfix << " | " << cleanTime;
                }

                if (!speed.empty())
                {
                    postfix << " | " << speed;
                }

                bar_.set_option(option::PostfixText{ postfix.str() });
                bar_.print_progress();

                if (progressCount % 1000 == 0)
                {
                    std::cout << "\n进度更新 #" << progressCount << ": " << actualPercent << "%" << std::endl;
                }
            }
        }
        else if (hasProgress && !displayTime.empty())
        {
            // 没有总时长，但有时问信息
            progressCount++;

            // 轻微增加进度（模拟）
            float newPercent = std::min(lastPercent + 0.5f, 99.0f);
            if (newPercent > lastPercent)
            {
                bar_.set_progress(newPercent);
                lastPercent = newPercent;
            }

            // 构建显示信息
            std::stringstream postfix;
            postfix << std::fixed << std::setprecision(1) << lastPercent << "%";

            if (!displayTime.empty())
            {
                postfix << " | " << displayTime;
            }

            if (!speed.empty() && speed != lastSpeed)
            {
                postfix << " | " << speed;
                lastSpeed = speed;
            }

            bar_.set_option(option::PostfixText{ postfix.str() });
            bar_.print_progress();
        }
        else
        {
            // 没有进度信息，检查是否长时间没有更新
            auto timeSinceLastProgress = std::chrono::duration_cast<std::chrono::seconds>(now - lastProgressTime);

            if (timeSinceLastProgress.count() > 2) // 超过2秒没有进度更新
            {
                std::stringstream postfix;
                postfix << "等待FFmpeg输出... 已运行 " << elapsed.count() << " 秒";
                bar_.set_option(option::PostfixText{ postfix.str() });

                // 轻微增加进度（避免卡住的感觉）
                float newPercent = std::min(lastPercent + 0.05f, 99.0f);
                if (newPercent > lastPercent)
                {
                    bar_.set_progress(newPercent);
                    lastPercent = newPercent;
                }

                bar_.print_progress();
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // 完成
    markAsCompletedImpl("转码完成 ✓");

    // 恢复光标
    show_console_cursor(true);

    // 显示统计信息
    std::cout << "\n转码完成统计:" << std::endl;
    std::cout << "- 总进度更新次数: " << progressCount << std::endl;
    std::cout << "- 最终文件: " << std::filesystem::path(srcPath).filename().string() << std::endl;
}

void TaskProgressBar::Impl::showGenericImpl(XExec& exec, const std::string& taskName)
{
    setupProgressBar(taskName);

    std::cout << "\n开始" << taskName << std::endl;
    bar_.print_progress();

    auto  startTime = std::chrono::steady_clock::now();
    float progress  = 0.0f;

    // 简单的主循环
    while (exec.isRunning())
    {
        auto now     = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - startTime);

        // 简单模拟进度
        progress = std::min(progress + 0.2f, 99.0f);


        bar_.set_progress(progress);

        std::stringstream postfix;
        postfix << "运行中... 已运行 " << elapsed.count() << " 秒";
        bar_.set_option(option::PostfixText{ postfix.str() });

        bar_.print_progress();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    markAsCompletedImpl("任务完成 ✓");
    show_console_cursor(true);
    std::cout << std::endl;
}

void TaskProgressBar::Impl::updateProgressImpl(float percent, const std::string& message)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!isActive_)
        return;

    currentPercent_ = std::min(100.0f, std::max(0.0f, percent));
    bar_.set_progress(currentPercent_);

    if (!message.empty())
    {
        currentMessage_ = message;
        bar_.set_option(option::PostfixText{ message });
    }

    bar_.print_progress();
}

void TaskProgressBar::Impl::markAsCompletedImpl(const std::string& message)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!isActive_)
        return;

    bar_.set_option(option::PostfixText{ message });
    bar_.set_option(option::ForegroundColor{ Color::green });
    bar_.set_progress(100.f);
    isActive_ = false;
}

void TaskProgressBar::Impl::markAsFailedImpl(const std::string& message)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!isActive_)
        return;

    bar_.set_option(option::PostfixText{ message });
    bar_.set_option(option::ForegroundColor{ Color::red });
    bar_.print_progress();
    isActive_ = false;
}


float TaskProgressBar::Impl::estimateTotalDuration(const std::string& srcPath) const
{
    std::string ffprobeCmd = std::string(FFPROBE_PATH) +
            " -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 \"" + srcPath + "\"";

    XExec exec;
    if (exec.start(ffprobeCmd, true))
    {
        exec.wait();
        std::string output = exec.getStdout();

        try
        {
            // 清理输出
            output.erase(std::remove(output.begin(), output.end(), '\n'), output.end());
            output.erase(std::remove(output.begin(), output.end(), '\r'), output.end());

            if (!output.empty())
            {
                return std::stof(output);
            }
        }
        catch (...)
        {
            // 解析失败
        }
    }

    return 0.0f;
}

std::string TaskProgressBar::Impl::formatDuration(float seconds) const
{
    int totalSeconds = static_cast<int>(seconds);
    int hours        = totalSeconds / 3600;
    int minutes      = (totalSeconds % 3600) / 60;
    int secs         = totalSeconds % 60;

    std::stringstream ss;
    if (hours > 0)
    {
        ss << hours << "小时";
    }
    if (minutes > 0)
    {
        ss << minutes << "分";
    }
    ss << secs << "秒";

    return ss.str();
}

// TaskProgressBar 公共接口实现
TaskProgressBar::TaskProgressBar() : impl_(std::make_unique<Impl>())
{
}

TaskProgressBar::~TaskProgressBar() = default;

auto TaskProgressBar::setTitle(const std::string& title) -> void
{
    impl_->setupProgressBar(title);
}

void TaskProgressBar::updateProgress(XExec& exec, const std::map<std::string, std::string>& inputParams)
{
    auto srcPath = inputParams.at("--input");
    impl_->showWithFfmpegImpl(exec, srcPath);
}

void TaskProgressBar::showGeneric(XExec& exec, const std::string& taskName)
{
    impl_->showGenericImpl(exec, taskName);
}

void TaskProgressBar::setProgress(float percent, const std::string& message)
{
    impl_->updateProgressImpl(percent, message);
}

void TaskProgressBar::markAsCompleted(const std::string& message)
{
    impl_->markAsCompletedImpl(message);
}

void TaskProgressBar::markAsFailed(const std::string& message)
{
    impl_->markAsFailedImpl(message);
}
