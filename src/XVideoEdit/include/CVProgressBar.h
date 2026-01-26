#pragma once

#ifndef CV_PROGRESS_BAR_H
#define CV_PROGRESS_BAR_H

#include "AVProgressBar.h"

class XExec;

class CVProgressBar : public AVProgressBar
{
    DECLARE_CREATE_DEFAULT(CVProgressBar)
public:
    CVProgressBar(const ProgressBarConfig::Ptr &config = nullptr);
    CVProgressBar(ProgressBarStyle style);
    CVProgressBar(const std::string_view &configName);
    ~CVProgressBar() override;

public:
    auto updateProgress(XExec &exec, const std::string_view &taskName,
                        const std::map<std::string, ParameterValue> &inputParams) -> void override;

    auto markAsCompleted(const std::string_view &message) -> void override;

private:
    class PImpl;
    std::unique_ptr<PImpl> impl_;
};

IMPLEMENT_CREATE_DEFAULT(CVProgressBar)

#endif // CV_PROGRESS_BAR_H
