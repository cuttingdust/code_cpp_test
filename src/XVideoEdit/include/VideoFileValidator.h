/// VideoFileValidator.h
#pragma once
#include "XConst.h"

#include <set>

class VideoFileValidator
{
public:
    enum class ValidationLevel
    {
        ExtensionOnly, ///< 仅检查扩展名
        MagicNumber,   ///< 检查文件头魔数
        FFmpegProbe    ///< 使用FFmpeg探测（最准确）
    };

    static auto isVideoFile(const std::string& filePath, std::string& errorMsg,
                            ValidationLevel level = ValidationLevel::FFmpegProbe) -> bool;

    static auto isVideoFileByExtension(const std::string& filePath, std::string& errorMsg) -> bool;
    static auto isVideoFileByMagicNumber(const std::string& filePath, std::string& errorMsg) -> bool;
    static auto isVideoFileByFFmpeg(const std::string& filePath, std::string& errorMsg) -> bool;

private:
    static auto getVideoExtensions() -> const std::set<std::string>&;
};
