#include "AVTask.h"
#include "XExec.h"
#include "VideoFileValidator.h"
#include "XFile.h"
#include "XTool.h"

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

    auto buildFFmpegCommand(const std::string& ffmpegPath, const std::string& srcPath, const std::string& dstPath,
                            const std::map<std::string, std::string>& params) const -> std::string;

    auto executeFFmpegCommand(const std::string& command, const std::map<std::string, std::string>& inputParams,
                              std::string& errorMsg) const -> bool;

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

    return true;
}

auto AVTask::PImpl::buildFFmpegCommand(const std::string& ffmpegPath, const std::string& srcPath,
                                       const std::string&                        dstPath,
                                       const std::map<std::string, std::string>& params) const -> std::string
{
    std::stringstream cmd;
    cmd << "\"" << ffmpegPath << "\" -hide_banner -progress pipe:1 -nostats -loglevel error -y -i \"" << srcPath
        << "\" ";

    /// 视频参数
    if (params.contains("video_codec"))
    {
        cmd << "-c:v " << params.at("video_codec") << " ";
    }
    else
    {
        cmd << "-c:v libx264 ";
    }

    if (params.contains("bitrate"))
    {
        cmd << "-b:v " << params.at("bitrate") << " ";
    }
    else
    {
        cmd << "-b:v 2000k ";
    }

    if (params.contains("resolution"))
    {
        cmd << "-s " << params.at("resolution") << " ";
    }

    if (params.contains("fps"))
    {
        cmd << "-r " << params.at("fps") << " ";
    }

    /// 音频参数
    if (params.contains("audio_codec"))
    {
        cmd << "-c:a " << params.at("audio_codec") << " ";
    }
    else
    {
        cmd << "-c:a aac ";
    }

    if (params.contains("audio_bitrate"))
    {
        cmd << "-b:a " << params.at("audio_bitrate") << " ";
    }

    /// 其他参数
    if (params.contains("preset"))
    {
        cmd << "-preset " << params.at("preset") << " ";
    }

    if (params.contains("crf"))
    {
        cmd << "-crf " << params.at("crf") << " ";
    }

    cmd << "\"" << dstPath << "\""; /// 将 stderr 重定向到 stdout

    return cmd.str();
}

auto AVTask::PImpl::executeFFmpegCommand(const std::string&                        command,
                                         const std::map<std::string, std::string>& inputParams,
                                         std::string&                              errorMsg) const -> bool
{
    XExec exec;

    std::string dstPath  = inputParams.at("--output");
    std::string fileName = std::filesystem::path(dstPath).filename().string();
    owner_->setTitle("转码: " + fileName);

    /// 启动命令
    if (!exec.start(command, true)) /// 合并 stderr 到 stdout
    {
        errorMsg = "启动FFmpeg命令失败";
        return false;
    }

    /// 显示进度条（使用FFmpeg特定的进度监控）
    owner_->updateProgress(exec, owner_->getName(), inputParams);

    /// 等待完成
    int  exitCode = exec.wait();
    bool success  = (exitCode == 0);

    /// 验证结果
    if (success)
    {
        namespace fs = std::filesystem;
        if (fs::exists(dstPath) && fs::file_size(dstPath) > 0)
        {
            return true;
        }
        else
        {
            errorMsg = "转码完成但输出文件无效";
            return false;
        }
    }
    else
    {
        errorMsg = "转码过程失败，退出码: " + std::to_string(exitCode);

        /// 获取错误输出
        std::string stderrOutput = exec.getOutError();
        if (!stderrOutput.empty())
        {
            errorMsg += "\n错误信息: " + stderrOutput;
        }

        return false;
    }
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

auto AVTask::execute(const std::map<std::string, std::string>& inputParams, std::string& errorMsg) const -> bool
{
    /// 1. 参数验证
    if (!XTask::execute(inputParams, errorMsg))
    {
        return false;
    }

    if (getName() == "cv") /// 音频转码
    {
        /// 2. 获取基本参数
        std::string ffmpegPath = XTool::getFFmpegPath();
        std::string srcPath    = getRequiredParam(inputParams, "--input", errorMsg);
        std::string dstPath    = getRequiredParam(inputParams, "--output", errorMsg);

        if (srcPath.empty() || dstPath.empty())
        {
            return false;
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

        /// 5. 构建FFmpeg命令
        std::string command = impl_->buildFFmpegCommand(ffmpegPath, srcPath, dstPath, inputParams);
        std::cout << "执行命令: " << command << std::endl;

        /// 6. 执行命令
        return impl_->executeFFmpegCommand(command, inputParams, errorMsg);
    }

    return true;
}


IMPLEMENT_CREATE_DEFAULT(AVTask)
template auto AVTask::create(const std::string_view&, const TaskFunc&, const std::string_view&) -> AVTask::Ptr;
