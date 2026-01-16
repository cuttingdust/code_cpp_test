#pragma once

#include "XExec.h"
#include "XTask.h"

class CVTask : public XTask
{
    DECLARE_CREATE_DEFAULT(CVTask)

public:
    CVTask();
    CVTask(const std::string_view &name, const TaskFunc &func, const std::string_view &desc = "");
    ~CVTask() override;

public:
    auto execute(const std::map<std::string, std::string> &inputParams, std::string &errorMsg) const -> bool override;

    auto executeFFmpegCommand(const std::string &command, const std::string &dstPath, std::string &errorMsg) const
            -> bool;

    auto showProgress(XExec &exec) const -> void;

    auto parseAndDisplayProgress(const std::string &output) const -> void;

    auto validatePaths(const std::string &srcPath, const std::string &dstPath, std::string &errorMsg) const -> bool;

    auto isVideoFile(const std::string &filePath, std::string &errorMsg) const -> bool;

    auto buildFFmpegCommand(const std::string &ffmpegPath, const std::string &srcPath, const std::string &dstPath,
                            const std::map<std::string, std::string> &params) const -> std::string;

private:
    class PImpl;
    std::unique_ptr<PImpl> impl_;
};
