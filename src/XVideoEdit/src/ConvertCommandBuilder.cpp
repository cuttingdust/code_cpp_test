#include "ConvertCommandBuilder.h"
#include "XTool.h"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>

auto ConvertCommandBuilder::parseOptions(const std::map<std::string, std::string>& params) const
        -> ConvertCommandBuilder::ConvertOptions
{
    ConvertOptions options;

    /// 必需参数
    options.input  = params.at("--input");
    options.output = params.at("--output");

    /// 可选参数
    if (params.contains("--video_codec"))
        options.video_codec = params.at("--video_codec");
    if (params.contains("--audio_codec"))
        options.audio_codec = params.at("--audio_codec");
    if (params.contains("--bitrate") || params.contains("--video_bitrate"))
        options.video_bitrate = params.contains("--bitrate") ? params.at("--bitrate") : params.at("--video_bitrate");
    if (params.contains("--audio_bitrate"))
        options.audio_bitrate = params.at("--audio_bitrate");
    if (params.contains("--resolution"))
        options.resolution = params.at("--resolution");
    if (params.contains("--fps"))
        options.fps = params.at("--fps");
    if (params.contains("--preset"))
        options.preset = params.at("--preset");
    if (params.contains("--crf"))
        options.crf = params.at("--crf");

    /// 布尔参数
    if (params.contains("--faststart"))
    {
        std::string val = params.at("--faststart");
        std::ranges::transform(val, val.begin(), ::tolower);
        options.faststart = (val == "true" || val == "1" || val == "yes" || val == "on");
    }

    return options;
}

bool ConvertCommandBuilder::validate(const std::map<std::string, std::string>& params, std::string& errorMsg) const
{
    /// 检查必需参数
    if (!params.contains("--input") || params.at("--input").empty())
    {
        errorMsg = "缺少输入文件参数(--input)";
        return false;
    }

    if (!params.contains("--output") || params.at("--output").empty())
    {
        errorMsg = "缺少输出文件参数(--output)";
        return false;
    }

    /// 验证视频编解码器
    if (params.contains("--video_codec"))
    {
        const std::string&                    codec       = params.at("--video_codec");
        static const std::vector<std::string> validCodecs = {
            "libx264", "libx265", "h264", "hevc", "vp9", "vp8", "mpeg4", "mpeg2video", "libvpx", "libvpx-vp9"
        };

        bool found = std::ranges::any_of(validCodecs, [&codec](const std::string& valid)
                                         { return codec.find(valid) != std::string::npos; });

        if (!found)
        {
            errorMsg = "不支持的视频编解码器: " + codec;
            return false;
        }
    }

    /// 验证音频编解码器
    if (params.contains("--audio_codec"))
    {
        const std::string&                    codec       = params.at("--audio_codec");
        static const std::vector<std::string> validCodecs = { "aac", "mp3", "opus", "vorbis", "flac", "libopus" };

        bool found = std::any_of(validCodecs.begin(), validCodecs.end(),
                                 [&codec](const std::string& valid) { return codec.find(valid) != std::string::npos; });

        if (!found)
        {
            errorMsg = "不支持的音频编解码器: " + codec;
            return false;
        }
    }

    /// 验证CRF值（如果提供）
    if (params.contains("--crf"))
    {
        try
        {
            int crf = std::stoi(params.at("--crf"));
            if (crf < 0 || crf > 51)
            {
                errorMsg = "CRF值必须在0-51之间";
                return false;
            }
        }
        catch (const std::exception&)
        {
            errorMsg = "无效的CRF值";
            return false;
        }
    }

    return true;
}

auto ConvertCommandBuilder::buildVideoFilters(const ConvertOptions& options) const -> std::string
{
    std::stringstream filters;
    bool              hasFilter = false;

    /// 分辨率缩放
    if (!options.resolution.empty())
    {
        if (hasFilter)
            filters << ",";
        filters << "scale=" << options.resolution;
        hasFilter = true;
    }

    /// FPS调整
    if (!options.fps.empty())
    {
        if (hasFilter)
            filters << ",";
        filters << "fps=" << options.fps;
        hasFilter = true;
    }

    return filters.str();
}

auto ConvertCommandBuilder::build(const std::map<std::string, std::string>& params) const -> std::string
{
    ConvertOptions options = parseOptions(params);

    std::stringstream cmd;
    cmd << "\"" << XTool::getFFmpegPath() << "\" ";

    /// 基本参数
    cmd << "-hide_banner -progress pipe:1 -nostats -loglevel error ";
    cmd << "-y "; /// 覆盖输出文件

    /// 输入文件
    cmd << "-i \"" << options.input << "\" ";

    /// 视频参数
    cmd << "-c:v " << options.video_codec << " ";

    if (!options.video_bitrate.empty())
    {
        cmd << "-b:v " << options.video_bitrate << " ";
    }

    /// 视频滤镜
    std::string videoFilters = buildVideoFilters(options);
    if (!videoFilters.empty())
    {
        cmd << "-vf \"" << videoFilters << "\" ";
    }

    /// 编码预设
    if (!options.preset.empty())
    {
        cmd << "-preset " << options.preset << " ";
    }

    /// CRF质量
    if (!options.crf.empty())
    {
        cmd << "-crf " << options.crf << " ";
    }

    /// 音频参数
    cmd << "-c:a " << options.audio_codec << " ";

    if (!options.audio_bitrate.empty())
    {
        cmd << "-b:a " << options.audio_bitrate << " ";
    }
    else
    {
        cmd << "-b:a 128k "; /// 默认音频码率
    }

    /// 快速启动（针对MP4）
    if (options.faststart)
    {
        cmd << "-movflags +faststart ";
    }

    /// 输出文件
    cmd << "\"" << options.output << "\"";

    return cmd.str();
}

auto ConvertCommandBuilder::getTitle(const std::map<std::string, std::string>& params) const -> std::string
{
    std::string input  = params.at("--input");
    std::string output = params.at("--output");

    std::filesystem::path inputPath(input);
    std::filesystem::path outputPath(output);

    return "转码: " + inputPath.filename().string() + " → " + outputPath.filename().string();
}

IMPLEMENT_CREATE(ConvertCommandBuilder);
