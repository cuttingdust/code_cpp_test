#pragma once

#ifndef DECRYPT_PROGRESS_BAR_H
#define DECRYPT_PROGRESS_BAR_H

#include "AVProgressBar.h"

class XExec;

class DecryptProgressBar : public AVProgressBar
{
    DECLARE_CREATE_DEFAULT(DecryptProgressBar)
public:
    DecryptProgressBar(const ProgressBarConfig::Ptr &config = nullptr);
    DecryptProgressBar(ProgressBarStyle style);
    DecryptProgressBar(const std::string_view &configName);
    ~DecryptProgressBar() override;

public:
    auto updateProgress(XExec &exec, const std::string_view &taskName,
                        const std::map<std::string, ParameterValue> &inputParams) -> void override;

    auto markAsCompleted(const std::string_view &message) -> void override;
    auto markAsFailed(const std::string_view &message) -> void override;

    /// 解密特定方法
    auto showDecryptionInfo(const std::string &key, const std::string &method) -> void;
    auto showSecurityReminder() -> void;

private:
    class PImpl;
    std::unique_ptr<PImpl> impl_;
};

IMPLEMENT_CREATE_DEFAULT(DecryptProgressBar)

#endif // DECRYPT_PROGRESS_BAR_H
