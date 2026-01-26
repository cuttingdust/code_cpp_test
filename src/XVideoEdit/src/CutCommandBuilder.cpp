#include "CutCommandBuilder.h"
#include "XTool.h"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <regex>

auto CutCommandBuilder::parseOptions(const std::map<std::string, ParameterValue>& params) const
        -> CutCommandBuilder::CutOptions
{
    CutOptions options;

    /// 必需参数
    options.input  = params.at("--input").asString();
    options.output = params.at("--output").asString();

    /// 开始时间
    if (params.contains("--start"))
    {
        options.start_time = params.at("--start").asString();
    }

    /// 持续时间或结束时间
    if (params.contains("--duration"))
    {
        options.time_value = params.at("--duration").asString();
        options.time_spec  = TimeSpec::DURATION;
    }
    else if (params.contains("--end"))
    {
        options.time_value = params.at("--end").asString();
        options.time_spec  = TimeSpec::END_TIME;
    }

    /// 布尔参数处理
    auto parseBool = [](const std::string& val) -> bool
    {
        std::string lower = val;
        std::ranges::transform(lower, lower.begin(), ::tolower);
        return (lower == "true" || lower == "1" || lower == "yes" || lower == "on");
    };

    if (params.contains("--copy"))
    {
        options.use_copy = parseBool(params.at("--copy"));
    }

    if (params.contains("--reencode"))
    {
        options.reencode = parseBool(params.at("--reencode"));
    }

    if (params.contains("--accurate"))
    {
        options.accurate_seek = parseBool(params.at("--accurate"));
    }

    return options;
}

auto CutCommandBuilder::validateTimeFormat(const std::string& time, std::string& errorMsg) const -> bool
{
    /// 支持格式:
    /// 1. 秒数 (如 "30", "120.5")
    /// 2. HH:MM:SS (如 "01:30:00")
    /// 3. HH:MM:SS.sss (如 "01:30:00.500")

    /// 检查是否是纯数字（秒数）
    static std::regex numberRegex(R"(^\d+(\.\d+)?$)");
    if (std::regex_match(time, numberRegex))
    {
        return true;
    }

    /// 检查时间格式 HH:MM:SS 或 HH:MM:SS.sss
    static std::regex timeRegex(R"(^(\d{1,2}):([0-5]?\d):([0-5]?\d)(\.\d{1,3})?$)");
    if (std::regex_match(time, timeRegex))
    {
        return true;
    }

    errorMsg = "无效的时间格式: " + time + " (支持格式: 秒数、HH:MM:SS、HH:MM:SS.sss)";
    return false;
}

auto CutCommandBuilder::timeToSeconds(const std::string& time) const -> std::string
{
    /// 如果是数字，直接返回
    static std::regex numberRegex(R"(^\d+(\.\d+)?$)");
    if (std::regex_match(time, numberRegex))
    {
        return time;
    }

    /// 解析时间格式 HH:MM:SS.sss
    static std::regex timeRegex(R"(^(\d{1,2}):([0-5]?\d):([0-5]?\d)(?:\.(\d{1,3}))?$)");
    std::smatch       match;

    if (std::regex_match(time, match, timeRegex))
    {
        int    hours   = std::stoi(match[1].str());
        int    minutes = std::stoi(match[2].str());
        int    seconds = std::stoi(match[3].str());
        double total   = hours * 3600 + minutes * 60 + seconds;

        if (match[4].matched)
        {
            double milliseconds = std::stod("0." + match[4].str());
            total += milliseconds;
        }

        return std::to_string(total);
    }

    return time; /// 如果无法转换，返回原始值
}

auto CutCommandBuilder::normalizeTime(const std::string& time, std::string& errorMsg) const -> std::string
{
    if (!validateTimeFormat(time, errorMsg))
    {
        return "";
    }
    return timeToSeconds(time);
}

auto CutCommandBuilder::validate(const std::map<std::string, ParameterValue>& params, std::string& errorMsg) const
        -> bool
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

    /// 验证时间参数
    std::string startTime = params.contains("--start") ? params.at("--start") : "00:00:00";
    if (!validateTimeFormat(startTime, errorMsg))
    {
        errorMsg = "开始时间格式错误: " + errorMsg;
        return false;
    }

    /// 必须有持续时间或结束时间
    bool hasDuration = params.contains("--duration") && !params.at("--duration").empty();
    bool hasEndTime  = params.contains("--end") && !params.at("--end").empty();

    if (!hasDuration && !hasEndTime)
    {
        errorMsg = "必须指定 --duration 或 --end 参数之一";
        return false;
    }

    if (hasDuration && hasEndTime)
    {
        errorMsg = "不能同时指定 --duration 和 --end 参数";
        return false;
    }

    /// 验证时间值
    if (hasDuration)
    {
        if (!validateTimeFormat(params.at("--duration"), errorMsg))
        {
            errorMsg = "持续时间格式错误: " + errorMsg;
            return false;
        }
    }
    else if (hasEndTime)
    {
        if (!validateTimeFormat(params.at("--end"), errorMsg))
        {
            errorMsg = "结束时间格式错误: " + errorMsg;
            return false;
        }
    }

    /// 检查编码选项冲突
    if (params.contains("--copy") && params.contains("--reencode"))
    {
        bool copy = false, reencode = false;

        auto parseBool = [](const std::string& val) -> bool
        {
            std::string lower = val;
            std::ranges::transform(lower, lower.begin(), ::tolower);
            return (lower == "true" || lower == "1" || lower == "yes" || lower == "on");
        };

        if (params.contains("--copy"))
            copy = parseBool(params.at("--copy"));
        if (params.contains("--reencode"))
            reencode = parseBool(params.at("--reencode"));

        if (copy && reencode)
        {
            errorMsg = "参数冲突: --copy 和 --reencode 不能同时为 true";
            return false;
        }
    }

    return true;
}

auto CutCommandBuilder::build(const std::map<std::string, ParameterValue>& params) const -> std::string
{
    CutOptions options = parseOptions(params);

    std::stringstream cmd;
    cmd << "\"" << XTool::getFFmpegPath() << "\" ";

    /// 基本参数
    cmd << "-hide_banner -progress pipe:1 -nostats -loglevel error ";
    cmd << "-y "; /// 覆盖输出文件

    /// 准确搜索（如果启用）
    if (options.accurate_seek)
    {
        cmd << "-ss " << options.start_time << " ";
        cmd << "-i \"" << options.input << "\" ";
    }
    else
    {
        cmd << "-i \"" << options.input << "\" ";
        cmd << "-ss " << options.start_time << " ";
    }

    /// 持续时间或结束时间
    if (options.time_spec == TimeSpec::DURATION)
    {
        cmd << "-t " << options.time_value << " ";
    }
    else
    {
        cmd << "-to " << options.time_value << " ";
    }

    /// 编码选项
    if (options.use_copy && !options.reencode)
    {
        /// 流复制模式（快速）
        cmd << "-c copy ";
        cmd << "-avoid_negative_ts make_zero ";
    }
    else if (options.reencode && !options.use_copy)
    {
        /// 重新编码模式（精确）
        cmd << "-c:v libx264 -crf 23 -preset fast ";
        cmd << "-c:a aac -b:a 128k ";
    }
    else
    {
        /// 默认使用流复制
        cmd << "-c copy ";
        cmd << "-avoid_negative_ts make_zero ";
    }

    /// 输出文件
    cmd << "\"" << options.output << "\"";

    return cmd.str();
}

auto CutCommandBuilder::getTitle(const std::map<std::string, ParameterValue>& params) const -> std::string
{
    std::string input     = params.at("--input");
    std::string output    = params.at("--output");
    std::string startTime = params.contains("--start") ? params.at("--start") : "00:00:00";

    std::filesystem::path inputPath(input);
    std::filesystem::path outputPath(output);

    std::string timeInfo;
    if (params.contains("--duration"))
    {
        timeInfo = "从 " + startTime + " 开始，持续 " + params.at("--duration").asString();
    }
    else if (params.contains("--end"))
    {
        timeInfo = "从 " + startTime + " 到 " + params.at("--end").asString();
    }

    return "剪切: " + inputPath.filename().string() + " (" + timeInfo + ")";
}

IMPLEMENT_CREATE(CutCommandBuilder)
