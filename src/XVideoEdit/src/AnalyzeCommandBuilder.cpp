#include "AnalyzeCommandBuilder.h"
#include "XTool.h"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <filesystem>

auto AnalyzeCommandBuilder::parseOptions(const std::map<std::string, ParameterValue>& params) const
        -> AnalyzeCommandBuilder::AnalyzeOptions
{
    AnalyzeOptions options;

    /// 必需参数
    options.input = params.at("--input").asString();

    /// 可选参数
    if (params.contains("--json"))
    {
        std::string val = params.at("--json").asString();
        std::ranges::transform(val, val.begin(), ::tolower);
        options.json_output = (val == "true" || val == "1" || val == "yes" || val == "on");
    }

    if (params.contains("--show-format"))
    {
        std::string val = params.at("--show-format").asString();
        std::ranges::transform(val, val.begin(), ::tolower);
        options.show_format = (val == "true" || val == "1" || val == "yes" || val == "on");
    }

    if (params.contains("--show-streams"))
    {
        std::string val = params.at("--show-streams").asString();
        std::ranges::transform(val, val.begin(), ::tolower);
        options.show_streams = (val == "true" || val == "1" || val == "yes" || val == "on");
    }

    if (params.contains("--show-frames"))
    {
        std::string val = params.at("--show-frames").asString();
        std::ranges::transform(val, val.begin(), ::tolower);
        options.show_frames = (val == "true" || val == "1" || val == "yes" || val == "on");
    }

    if (params.contains("--show-programs"))
    {
        std::string val = params.at("--show-programs").asString();
        std::ranges::transform(val, val.begin(), ::tolower);
        options.show_programs = (val == "true" || val == "1" || val == "yes" || val == "on");
    }

    if (params.contains("--show-chapters"))
    {
        std::string val = params.at("--show-chapters").asString();
        std::ranges::transform(val, val.begin(), ::tolower);
        options.show_chapters = (val == "true" || val == "1" || val == "yes" || val == "on");
    }

    if (params.contains("--show-error"))
    {
        std::string val = params.at("--show-error").asString();
        std::ranges::transform(val, val.begin(), ::tolower);
        options.show_error = (val == "true" || val == "1" || val == "yes" || val == "on");
    }

    return options;
}

auto AnalyzeCommandBuilder::validate(const std::map<std::string, ParameterValue>& params, std::string& errorMsg) const
        -> bool
{
    /// 检查必需参数
    if (!params.contains("--input") || params.at("--input").empty())
    {
        errorMsg = "缺少输入文件参数(--input)";
        return false;
    }

    /// 检查文件是否存在
    std::string inputPath = params.at("--input").asString();
    if (!std::filesystem::exists(inputPath))
    {
        errorMsg = "输入文件不存在: " + inputPath;
        return false;
    }

    /// 检查是否为视频/音频文件（通过扩展名）
    std::filesystem::path path(inputPath);
    std::string           extension = path.extension().string();
    std::ranges::transform(extension, extension.begin(), ::tolower);

    static const std::vector<std::string> videoExtensions = { ".mp4", ".avi", ".mov",  ".mkv", ".wmv", ".flv",  ".webm",
                                                              ".m4v", ".mpg", ".mpeg", ".ts",  ".mts", ".m2ts", ".vob",
                                                              ".ogv", ".3gp", ".3g2",  ".f4v", ".rm",  ".rmvb" };

    static const std::vector<std::string> audioExtensions = { ".mp3", ".wav", ".aac",  ".flac", ".ogg",
                                                              ".wma", ".m4a", ".opus", ".ac3",  ".dts" };

    bool isVideoOrAudio =
            std::ranges::any_of(videoExtensions, [&extension](const std::string& ext) { return extension == ext; }) ||
            std::ranges::any_of(audioExtensions, [&extension](const std::string& ext) { return extension == ext; });

    if (!isVideoOrAudio && !params.contains("--force"))
    {
        errorMsg = "文件扩展名不支持，请确认是否为视频/音频文件。使用 --force 参数强制分析。";
        return false;
    }

    return true;
}

auto AnalyzeCommandBuilder::build(const std::map<std::string, ParameterValue>& params) const -> std::string
{
    AnalyzeOptions options = parseOptions(params);

    std::stringstream cmd;

#ifdef _WIN32
    /// Windows系统：使用完整路径并处理管道
    cmd << "cmd /c \"\"" << XTool::getFFprobePath() << "\" ";
#else
    /// Linux/macOS系统
    cmd << "\"" << XTool::getFFprobePath() << "\" ";
#endif

    /// 基本参数
    cmd << "-hide_banner ";

    /// JSON格式输出
    if (options.json_output)
    {
        cmd << "-print_format json ";
    }
    else
    {
        cmd << "-print_format default ";
    }

    /// 显示选项
    if (options.show_format && options.show_streams)
    {
        cmd << "-show_format -show_streams ";
    }
    else if (options.show_format)
    {
        cmd << "-show_format ";
    }
    else if (options.show_streams)
    {
        cmd << "-show_streams ";
    }

    if (options.show_frames)
    {
        cmd << "-show_frames ";
        if (params.contains("--select-streams"))
        {
            cmd << "-select_streams " << params.at("--select-streams").asString() << " ";
        }
    }

    if (options.show_programs)
    {
        cmd << "-show_programs ";
    }

    if (options.show_chapters)
    {
        cmd << "-show_chapters ";
    }

    if (options.show_error)
    {
        cmd << "-show_error ";
    }

    /// 输入文件
    cmd << "-i \"" << options.input << "\" ";

    /// 额外选项
    if (params.contains("--count-frames"))
    {
        cmd << "-count_frames ";
    }

    if (params.contains("--count-packets"))
    {
        cmd << "-count_packets ";
    }

#ifdef _WIN32
    /// Windows：处理管道
    if (params.contains("--pretty") && options.json_output)
    {
        cmd << "2>nul | python -m json.tool\"";
    }
    else
    {
        cmd << "\"";
    }
#else
    /// Linux/macOS：处理管道
    if (params.contains("--pretty") && options.json_output)
    {
        cmd << " 2>/dev/null | python -m json.tool";
    }
#endif

    return cmd.str();
}

auto AnalyzeCommandBuilder::getTitle(const std::map<std::string, ParameterValue>& params) const -> std::string
{
    std::string           input = params.at("--input").asString();
    std::filesystem::path inputPath(input);

    std::string title = "分析: " + inputPath.filename().string();

    if (params.contains("--json"))
    {
        title += " (JSON格式)";
    }

    return title;
}

IMPLEMENT_CREATE(AnalyzeCommandBuilder);
