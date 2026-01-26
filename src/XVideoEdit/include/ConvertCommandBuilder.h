#pragma once

#ifndef CONVERT_COMMAND_BUILDER_H
#define CONVERT_COMMAND_BUILDER_H

#include "AVTask.h"

class ConvertCommandBuilder : public AVTask::ICommandBuilder
{
    DECLARE_CREATE(ConvertCommandBuilder)

public:
    auto build(const std::map<std::string, ParameterValue> &params) const -> std::string override;
    auto validate(const std::map<std::string, ParameterValue> &params, std::string &errorMsg) const -> bool override;
    auto getTitle(const std::map<std::string, ParameterValue> &params) const -> std::string override;

private:
    struct ConvertOptions
    {
        std::string input;
        std::string output;
        std::string video_codec   = "libx264";
        std::string audio_codec   = "aac";
        std::string video_bitrate = "2000k";
        std::string audio_bitrate;
        std::string resolution;
        std::string fps;
        std::string preset;
        std::string crf;
        bool        faststart = false; /// MP4快速启动
        bool        overwrite = true;  /// 覆盖输出文件
    };

    auto parseOptions(const std::map<std::string, ParameterValue> &params) const -> ConvertOptions;
    auto buildVideoFilters(const ConvertOptions &options) const -> std::string;
};

#endif // CONVERT_COMMAND_BUILDER_H
