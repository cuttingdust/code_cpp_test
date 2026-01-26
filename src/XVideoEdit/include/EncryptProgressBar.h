#pragma once

#ifndef ENCRYPT_PROGRESS_BAR_H
#define ENCRYPT_PROGRESS_BAR_H

#include "AVProgressBar.h"

class XExec;

class EncryptProgressBar : public AVProgressBar
{
    DECLARE_CREATE_DEFAULT(EncryptProgressBar)
public:
    EncryptProgressBar(const ProgressBarConfig::Ptr &config = nullptr);
    EncryptProgressBar(ProgressBarStyle style);
    EncryptProgressBar(const std::string_view &configName);
    ~EncryptProgressBar() override;

public:
    auto updateProgress(XExec &exec, const std::string_view &taskName,
                        const std::map<std::string, ParameterValue> &inputParams) -> void override;

    auto markAsCompleted(const std::string_view &message) -> void override;
    auto markAsFailed(const std::string_view &message) -> void override;

    /// 加密特定方法
    auto showEncryptionInfo(const std::string &key, const std::string &method) -> void;
    auto showDecryptionInstructions() -> void;

private:
    class PImpl;
    std::unique_ptr<PImpl> impl_;
};

IMPLEMENT_CREATE_DEFAULT(EncryptProgressBar)

#endif // ENCRYPT_PROGRESS_BAR_H
