/// VideoFileValidator.h
#pragma once
#include <string>
#include <filesystem>
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

    static bool isVideoFileByExtension(const std::string& filePath, std::string& errorMsg);
    static bool isVideoFileByMagicNumber(const std::string& filePath, std::string& errorMsg);
    static bool isVideoFileByFFmpeg(const std::string& filePath, std::string& errorMsg);

private:
    static const std::set<std::string>& getVideoExtensions();
};
