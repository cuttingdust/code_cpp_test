#pragma once
#include "XConst.h"
#include "ProgressBarConfig.h"

#include <mutex>

class XExec;

class TaskProgressBar
{
    DECLARE_CREATE_DEFAULT(TaskProgressBar)
public:
    explicit TaskProgressBar(const ProgressBarConfig::Ptr& config = nullptr);
    explicit TaskProgressBar(ProgressBarStyle style);
    explicit TaskProgressBar(const std::string_view& configName);
    virtual ~TaskProgressBar();

public:
    /// 配置相关
    auto setConfig(const ProgressBarConfig::Ptr& config) const -> void;

    auto applyConfig() -> void;

    auto getConfig() -> ProgressBarConfig::Ptr const;

public:
    /// 设置进度条标题
    virtual auto setTitle(const std::string_view& title) const -> void;

    /// 显示进度条并监控FFmpeg进度
    virtual auto updateProgress(XExec& exec, const std::string_view& taskName,
                                const std::map<std::string, std::string>& inputParams) -> void;

    /// 手动更新进度
    virtual auto setProgress(float percent = 0.0f, const std::string& message = "") -> void;

    /// 标记为完成
    virtual auto markAsCompleted(const std::string_view& message = "完成 ✓") -> void;

    /// 标记为失败
    virtual auto markAsFailed(const std::string_view& message = "失败 ✗") -> void;

public:
    /// \brief 设置进度
    /// \param percent
    auto setValue(float percent) -> void;

    /// \brief 设置文字
    /// \param text
    auto setMessage(const std::string_view& text) -> void;

    /// \brief 刷新显示
    auto updateDisplay() -> void;

private:
    class PImpl;
    std::unique_ptr<PImpl> impl_;
};
IMPLEMENT_CREATE_DEFAULT(TaskProgressBar)
