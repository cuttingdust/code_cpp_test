#pragma once
#include "XTask.h"
#include <memory>
#include <map>
#include <string>
#include <functional>
#include <mutex>

class XExec;

class CVTask : public XTask
{
    DECLARE_CREATE_DEFAULT(XTask)
public:
    CVTask();
    CVTask(const std::string_view& name, const TaskFunc& func, const std::string_view& desc);
    ~CVTask() override;

    struct ProgressState
    {
        std::mutex  mutex;
        float       percent = 0.0f;
        std::string timeInfo;
        std::string speed;
    };

public:
    bool execute(const std::map<std::string, std::string>& inputParams, std::string& errorMsg) const override;


private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    /// 私有方法
    bool        isVideoFile(const std::string& filePath, std::string& errorMsg) const;
    std::string buildFFmpegCommand(const std::string& ffmpegPath, const std::string& srcPath,
                                   const std::string& dstPath, const std::map<std::string, std::string>& params) const;
    bool        executeFFmpegCommand(const std::string& command, const std::string& srcPath, const std::string& dstPath,
                                     std::string& errorMsg) const;
    bool        validatePaths(const std::string& srcPath, const std::string& dstPath, std::string& errorMsg) const;
};
