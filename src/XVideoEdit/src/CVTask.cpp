#include "CVTask.h"
#include "XExec.h"
#include "VideoFileValidator.h"

#include <filesystem>
#include <iostream>
#include <thread>

class CVTask::PImpl
{
public:
    PImpl(CVTask *owenr);
    ~PImpl() = default;

public:
    CVTask *owenr_ = nullptr;
};

CVTask::PImpl::PImpl(CVTask *owenr) : owenr_(owenr)
{
}


CVTask::CVTask()
{
    this->setTaskType(TaskType::TT_VIDEO);
}

CVTask::CVTask(const std::string_view &name, const TaskFunc &func, const std::string_view &desc) :
    XTask(name, func, desc), impl_(std::make_unique<CVTask::PImpl>(this))
{
}

CVTask::~CVTask() = default;

auto CVTask::execute(const std::map<std::string, std::string> &inputParams, std::string &errorMsg) const -> bool
{
    /// 1. 参数验证
    if (!XTask::execute(inputParams, errorMsg))
    {
        return false;
    }

    /// 2. 获取基本参数
    std::string ffmpegPath = getFFmpegPath();
    std::string srcPath    = getRequiredParam(inputParams, "--input", errorMsg);
    std::string dstPath    = getRequiredParam(inputParams, "--output", errorMsg);

    if (srcPath.empty() || dstPath.empty())
    {
        return false;
    }
    /// 3. 文件系统检查
    if (!validatePaths(srcPath, dstPath, errorMsg))
    {
        return false;
    }

    /// 4. 检查是否为视频文件
    if (!isVideoFile(srcPath, errorMsg))
    {
        return false;
    }

    /// 5. 构建FFmpeg命令
    std::string command = buildFFmpegCommand(ffmpegPath, srcPath, dstPath, inputParams);
    std::cout << "执行命令: " << command << std::endl;

    /// 6. 执行命令
    return executeFFmpegCommand(command, dstPath, errorMsg);
}

/// 检查是否为视频文件
auto CVTask::isVideoFile(const std::string &filePath, std::string &errorMsg) const -> bool
{
    return VideoFileValidator::isVideoFile(filePath, errorMsg);
}

auto CVTask::buildFFmpegCommand(const std::string &ffmpegPath, const std::string &srcPath, const std::string &dstPath,
                                const std::map<std::string, std::string> &params) const -> std::string
{
    std::stringstream cmd;
    cmd << "\"" << ffmpegPath << "\" -y -i \"" << srcPath << "\" ";

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

    cmd << "\"" << dstPath << "\"";

    return cmd.str();
}

auto CVTask::executeFFmpegCommand(const std::string &command, const std::string &dstPath, std::string &errorMsg) const
        -> bool
{
    XExec exec;

    if (!exec.start(command.c_str()))
    {
        errorMsg = "启动FFmpeg命令失败";
        return false;
    }

    /// 显示进度
    showProgress(exec);

    // 等待完成
    bool success = exec.wait();

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
        errorMsg = "转码过程失败";
        return false;
    }
}

auto CVTask::showProgress(XExec &exec) const -> void
{
    std::cout << "转码进度:" << std::endl;

    while (exec.isRunning())
    {
        std::string output;
        if (exec.getOutput(output))
        {
            /// 解析并显示进度
            parseAndDisplayProgress(output);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

auto CVTask::parseAndDisplayProgress(const std::string &output) const -> void
{
    /// 简单的进度解析
    if (output.find("time=") != std::string::npos)
    {
        size_t timePos = output.find("time=");
        size_t endPos  = output.find(" bitrate=");

        if (timePos != std::string::npos && endPos != std::string::npos && endPos > timePos)
        {
            std::string timeInfo = output.substr(timePos + 5, endPos - timePos - 5);
            std::cout << "\r已处理: " << timeInfo << std::flush;
        }
    }
}

auto CVTask::validatePaths(const std::string &srcPath, const std::string &dstPath, std::string &errorMsg) const -> bool
{
    namespace fs = std::filesystem;

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

        constexpr uintmax_t MAX_FILE_SIZE = 10ULL * 1024 * 1024 * 1024; // 10GB
        if (fileSize > MAX_FILE_SIZE)
        {
            errorMsg = "源文件过大（超过10GB）";
            return false;
        }
    }
    catch (const fs::filesystem_error &e)
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
        catch (const fs::filesystem_error &e)
        {
            errorMsg = "无法创建目标目录: " + std::string(e.what());
            return false;
        }
    }

    return true;
}

IMPLEMENT_CREATE_DEFAULT(CVTask)
template auto CVTask::create(const std::string_view &, const TaskFunc &, const std::string_view &) -> CVTask::Ptr;
