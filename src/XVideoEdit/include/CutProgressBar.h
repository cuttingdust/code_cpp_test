#pragma once

#ifndef CUT_PROGRESS_BAR_H
#define CUT_PROGRESS_BAR_H

#include "AVProgressBar.h"

class XExec;

class CutProgressBar : public AVProgressBar
{
    DECLARE_CREATE_DEFAULT(CutProgressBar)
public:
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

#endif // CUT_PROGRESS_BAR_H
