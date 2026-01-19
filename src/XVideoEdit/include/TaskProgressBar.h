#pragma once
#include <string>
#include <memory>
#include <functional>
#include <atomic>
#include <mutex>

class XExec;

class TaskProgressBar
{
public:
    using ProgressCallback = std::function<void(float percent, const std::string& timeInfo)>;

    TaskProgressBar();
    ~TaskProgressBar();

    // 设置外部回调
    void setProgressCallback(ProgressCallback callback);

    // 设置进度条标题
    void setTitle(const std::string& title);

    // 显示进度条并监控FFmpeg进度
    void showWithFfmpeg(XExec& exec, const std::string& srcPath);

    // 通用进度显示（不依赖FFmpeg）
    void showGeneric(XExec& exec, const std::string& taskName);

    // 手动更新进度
    void updateProgress(float percent, const std::string& message = "");

    // 标记为完成
    void markAsCompleted(const std::string& message = "完成 ✓");

    // 标记为失败
    void markAsFailed(const std::string& message = "失败 ✗");

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
