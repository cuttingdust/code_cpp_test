#include "AVTask.h"
#include "XExec.h"
#include "VideoFileValidator.h"

#include <iostream>
#include <chrono>
#include <sstream>

class AVTask::PImpl
{
public:
    PImpl(AVTask* owner);
    ~PImpl() = default;

public:
    auto isVideoFile(const std::string& filePath, std::string& errorMsg) const -> bool;

    auto validatePaths(const std::string& srcPath, const std::string& dstPath, std::string& errorMsg) const -> bool;

public:
    AVTask* owner_ = nullptr;
};

AVTask::PImpl::PImpl(AVTask* owner) : owner_(owner)
{
    owner_->setTaskType(TaskType::TT_AV);
}

auto AVTask::PImpl::isVideoFile(const std::string& filePath, std::string& errorMsg) const -> bool
{
    return VideoFileValidator::isVideoFile(filePath, errorMsg);
}

auto AVTask::PImpl::validatePaths(const std::string& srcPath, const std::string& dstPath, std::string& errorMsg) const
        -> bool
{
    /// 检查源文件
    if (!fs::exists(srcPath))
    {
        errorMsg = "源文件不存在: " + srcPath;
        return false;
    }

    if (!fs::is_regular_file(srcPath))
    {
        errorMsg = "源文件不是普通文件: " + srcPath;
        return false;
    }

    /// 检查文件大小（可选）
    try
    {
        auto fileSize = fs::file_size(srcPath);
        if (fileSize == 0)
        {
            errorMsg = "源文件大小为0";
            return false;
        }

        constexpr uintmax_t MAX_FILE_SIZE = 10ULL * 1024 * 1024 * 1024; /// 10GB
        if (fileSize > MAX_FILE_SIZE)
        {
            errorMsg = "源文件过大（超过10GB）";
            return false;
        }
    }
    catch (const fs::filesystem_error& e)
    {
        std::cout << "警告: 无法获取文件大小: " << e.what() << std::endl;
    }

    if (!dstPath.empty())
    {
        /// 创建目标目录
        fs::path dstFilePath(dstPath);
        fs::path dstDir = dstFilePath.parent_path();

        if (!dstDir.empty() && !fs::exists(dstDir))
        {
            try
            {
                fs::create_directories(dstDir);
            }
            catch (const fs::filesystem_error& e)
            {
                errorMsg = "无法创建目标目录: " + std::string(e.what());
                return false;
            }
        }
    }

    return true;
}


AVTask::AVTask()
{
    impl_ = std::make_unique<AVTask::PImpl>(this);
}

AVTask::AVTask(const std::string_view& name, const TaskFunc& func, const std::string_view& desc) :
    XTask(name, func, desc), impl_(std::make_unique<AVTask::PImpl>(this))
{
}


AVTask::~AVTask() = default;

auto AVTask::validateCommon(const std::map<std::string, ParameterValue>& inputParams, std::string& errorMsg) -> bool
{
    /// 2. 获取基本参数
    std::string srcPath, dstPath;
    srcPath = getRequiredParam(inputParams, "--input", errorMsg).asString();
    if (srcPath.empty())
    {
        return false;
    }
    if (hasParameter("--output"))
    {
        dstPath = getRequiredParam(inputParams, "--output", errorMsg).asString();
        if (srcPath.empty())
        {
            return false;
        }
    }

    /// 3. 文件系统检查
    if (!impl_->validatePaths(srcPath, dstPath, errorMsg))
    {
        return false;
    }

    /// 4. 检查是否为视频文件
    if (!impl_->isVideoFile(srcPath, errorMsg))
    {
        return false;
    }
    return true;
}

auto AVTask::execute(const std::string& command, const std::map<std::string, ParameterValue>& inputParams,
                     std::string& errorMsg, std::string& resultMsg) -> bool
{
    XExec exec;

    /// 启动命令
    if (!exec.start(command, true)) /// 合并 stderr 到 stdout
    {
        errorMsg = "启动FFmpeg命令失败";
        return false;
    }

    /// 显示进度条（使用FFmpeg特定的进度监控）
    updateProgress(exec, getName(), inputParams);
    if (!waitProgress(exec, inputParams, errorMsg))
    {
        return false;
    }
    resultMsg = exec.getOutput();

    return true;
}


IMPLEMENT_CREATE_DEFAULT(AVTask)
template auto AVTask::create(const std::string_view&, const TaskFunc&, const std::string_view&) -> AVTask::Ptr;
