#pragma once

#ifndef ANALYZE_COMMAND_BUILDER_H
#define ANALYZE_COMMAND_BUILDER_H

#include "AVTask.h"

class AnalyzeCommandBuilder : public AVTask::ICommandBuilder
{
    DECLARE_CREATE(AnalyzeCommandBuilder)

public:
    auto build(const std::map<std::string, ParameterValue> &params) const -> std::string override;
    auto validate(const std::map<std::string, ParameterValue> &params, std::string &errorMsg) const -> bool override;
    auto getTitle(const std::map<std::string, ParameterValue> &params) const -> std::string override;

private:
    struct AnalyzeOptions
    {
        std::string input;
        bool        json_output   = false; /// JSON格式输出
        bool        show_format   = true;  /// 显示格式信息
        bool        show_streams  = true;  /// 显示流信息
        bool        show_frames   = false; /// 显示帧信息（详细模式）
        bool        show_programs = false; /// 显示节目信息
        bool        show_chapters = false; /// 显示章节信息
        bool        show_error    = false; /// 显示错误信息
    };

    auto parseOptions(const std::map<std::string, ParameterValue> &params) const -> AnalyzeOptions;
};

#endif // ANALYZE_COMMAND_BUILDER_H
