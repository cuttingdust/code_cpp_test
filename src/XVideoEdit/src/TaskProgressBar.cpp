#include "TaskProgressBar.h"

#include "ProgressBarConfigManager.h"
#include "XExec.h"

#include <indicators/progress_bar.hpp>
#include <indicators/cursor_control.hpp>

#include <filesystem>
#include <iostream>
#include <thread>
#include <chrono>
#include <sstream>

using namespace indicators;

class TaskProgressBar::PImpl
{
public:
    PImpl(TaskProgressBar* owner_, const ProgressBarConfig::Ptr& config);
    ~PImpl();

public:
    auto setupProgressBar(const std::string_view& title) -> void;

    void showGenericImpl(XExec& exec, const std::string_view& taskName);

    void updateProgressImpl(float percent, const std::string& message);

    auto markAsCompletedImpl(const std::string_view& message) -> void;

    auto markAsFailedImpl(const std::string& message) -> void;

public:
    TaskProgressBar*       owner_  = nullptr;
    ProgressBarConfig::Ptr config_ = nullptr;
    ProgressBar            bar_;
    std::atomic<bool>      isActive_{ false };
    std::atomic<float>     currentPercent_{ 0.0f };
    std::string            currentMessage_;
    std::mutex             mutex_;
};

TaskProgressBar::PImpl::PImpl(TaskProgressBar* owner_, const ProgressBarConfig::Ptr& config) :
    owner_(owner_), config_(config)
{
    /// 使用简洁的配置方式初始化进度条
    // bar_.set_option(option::BarWidth{ 50 });
    // bar_.set_option(option::Start{ "[" });
    // bar_.set_option(option::Fill{ "=" });
    // bar_.set_option(option::Lead{ ">" });
    // bar_.set_option(option::Remainder{ " " });
    // bar_.set_option(option::End{ "]" });
    // bar_.set_option(option::ShowPercentage{ true });
    // bar_.set_option(option::ShowElapsedTime{ true });
    // bar_.set_option(option::ShowRemainingTime{ true });
    // bar_.set_option(option::ForegroundColor{ Color::magenta });
    // bar_.set_option(option::FontStyles{ std::vector<FontStyle>{ FontStyle::bold } });

    ProgressBarConfigManager::getInstance()->applyConfig(bar_, config_);
}

TaskProgressBar::PImpl::~PImpl()
{
    if (isActive_)
    {
        /// 确保光标恢复正常
        show_console_cursor(true);
    }
}

auto TaskProgressBar::PImpl::setupProgressBar(const std::string_view& title) -> void
{
    bar_.set_option(option::PostfixText{ title });
    currentPercent_ = 0.0f;
    isActive_       = true;

    /// 隐藏光标以获得更流畅的显示
    show_console_cursor(false);
}

void TaskProgressBar::PImpl::showGenericImpl(XExec& exec, const std::string_view& taskName)
{
    owner_->setTitle(taskName);

    std::cout << "\n开始" << taskName << std::endl;
    owner_->updateDisplay();

    auto  startTime = std::chrono::steady_clock::now();
    float progress  = 0.0f;

    /// 简单的主循环
    while (exec.isRunning())
    {
        auto now     = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - startTime);

        /// 简单模拟进度
        progress = std::min(progress + 0.2f, 99.0f);
        owner_->setValue(progress);

        std::stringstream postfix;
        postfix << "运行中... 已运行 " << elapsed.count() << " 秒";
        owner_->setMessage(postfix.str());

        owner_->updateDisplay();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    markAsCompletedImpl("任务完成 ✓");
    show_console_cursor(true);
    std::cout << std::endl;
}

void TaskProgressBar::PImpl::updateProgressImpl(float percent, const std::string& message)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!isActive_)
    {
        return;
    }

    currentPercent_ = std::min(100.0f, std::max(0.0f, percent));
    if (currentPercent_ > 0.0f)
    {
        owner_->setValue(currentPercent_);
    }

    if (!message.empty())
    {
        currentMessage_ = message;
        owner_->setMessage(message);
    }

    owner_->updateDisplay();
}

void TaskProgressBar::PImpl::markAsCompletedImpl(const std::string_view& message)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!isActive_)
        return;

    bar_.set_option(option::PostfixText{ message });
    bar_.set_option(option::ForegroundColor{ Color::green });
    bar_.set_progress(100.f);
    isActive_ = false;
}

auto TaskProgressBar::PImpl::markAsFailedImpl(const std::string& message) -> void
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!isActive_)
        return;

    bar_.set_option(option::PostfixText{ message });
    bar_.set_option(option::ForegroundColor{ Color::red });
    bar_.print_progress();
    isActive_ = false;
}


TaskProgressBar::TaskProgressBar(const ProgressBarConfig::Ptr& config)
{
    auto cg = config ? config : ProgressBarConfigManager::getInstance()->getConfig("default");
    impl_   = std::make_unique<TaskProgressBar::PImpl>(this, cg);
}

TaskProgressBar::TaskProgressBar(ProgressBarStyle style)
{
    auto cg = ProgressBarConfigManager::getInstance()->getConfig(style);
    impl_   = std::make_unique<TaskProgressBar::PImpl>(this, cg);
}

TaskProgressBar::TaskProgressBar(const std::string_view& configName)
{
    auto cg = ProgressBarConfigManager::getInstance()->getConfig(configName);
    impl_   = std::make_unique<TaskProgressBar::PImpl>(this, cg);
}

TaskProgressBar::~TaskProgressBar() = default;

auto TaskProgressBar::setConfig(const ProgressBarConfig::Ptr& config) const -> void
{
    impl_->config_ = config;
}

auto TaskProgressBar::setTitle(const std::string_view& title) const -> void
{
    impl_->setupProgressBar(title);
}

auto TaskProgressBar::updateProgress(XExec& exec, const std::string_view& taskName,
                                     const std::map<std::string, std::string>& inputParams) -> void
{
    impl_->showGenericImpl(exec, taskName);
}

auto TaskProgressBar::setProgress(float percent, const std::string& message) -> void
{
    impl_->updateProgressImpl(percent, message);
}

auto TaskProgressBar::markAsCompleted(const std::string_view& message) -> void
{
    impl_->markAsCompletedImpl(message);

    /// 恢复光标显示
    show_console_cursor(true);
}

auto TaskProgressBar::markAsFailed(const std::string& message) -> void
{
    impl_->markAsFailedImpl(message);

    /// 恢复光标显示
    show_console_cursor(true);
}


auto TaskProgressBar::applyConfig() -> void
{
    ProgressBarConfigManager::getInstance()->applyConfig(impl_->bar_, impl_->config_);
}

auto TaskProgressBar::getConfig() -> ProgressBarConfig::Ptr const
{
    return impl_->config_;
}

auto TaskProgressBar::setValue(float percent) -> void
{
    impl_->bar_.set_progress(percent);
}

auto TaskProgressBar::setMessage(const std::string_view& text) -> void
{
    impl_->bar_.set_option(option::PostfixText{ std::string(text) });
}

auto TaskProgressBar::updateDisplay() -> void
{
    impl_->bar_.print_progress();
}
