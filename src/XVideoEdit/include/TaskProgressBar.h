#pragma once
#include <map>
#include <string>
#include <memory>
#include <mutex>

class XExec;

class TaskProgressBar
{
public:
    TaskProgressBar();
    ~TaskProgressBar();

public:
    /// 设置进度条标题
    auto setTitle(const std::string& title) -> void;

    /// 显示进度条并监控FFmpeg进度
    virtual auto updateProgress(XExec& exec, const std::map<std::string, std::string>& inputParams) -> void;

    /// 通用进度显示（不依赖FFmpeg）
    void showGeneric(XExec& exec, const std::string& taskName);

    /// 手动更新进度
    virtual void setProgress(float percent, const std::string& message = "");

    /// 标记为完成
    virtual void markAsCompleted(const std::string& message = "完成 ✓");

    /// 标记为失败
    virtual void markAsFailed(const std::string& message = "失败 ✗");

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
