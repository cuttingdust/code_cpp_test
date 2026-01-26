#pragma once

#ifndef CUT_COMMAND_BUILDER_H
#define CUT_COMMAND_BUILDER_H

#include "AVTask.h"

class CutCommandBuilder : public AVTask::ICommandBuilder
{
    DECLARE_CREATE(CutCommandBuilder)
public:
    auto build(const std::map<std::string, ParameterValue> &params) const -> std::string override;
    auto validate(const std::map<std::string, ParameterValue> &params, std::string &errorMsg) const -> bool override;
    auto getTitle(const std::map<std::string, ParameterValue> &params) const -> std::string override;

private:
    enum class TimeSpec
    {
        DURATION,
        END_TIME
    };

    struct CutOptions
    {
        std::string input;
        std::string output;
        std::string start_time = "00:00:00";
        std::string time_value;
        TimeSpec    time_spec     = TimeSpec::DURATION;
        bool        use_copy      = true;
        bool        reencode      = false;
        bool        accurate_seek = false;
    };

    auto parseOptions(const std::map<std::string, ParameterValue> &params) const -> CutOptions;
    auto normalizeTime(const std::string &time, std::string &errorMsg) const -> std::string;
    auto validateTimeFormat(const std::string &time, std::string &errorMsg) const -> bool;
    auto timeToSeconds(const std::string &time) const -> std::string;
};

#endif // CUT_COMMAND_BUILDER_H
