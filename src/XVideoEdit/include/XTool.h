#pragma once
#include <string>
#include <vector>

class XTool
{
public:
    /// \brief 检查终端是否交互式
    /// \return
    static auto isInteractiveTerminal() -> bool;

    static auto split(const std::string_view &input, char delimiter = ' ', bool trimWhitespace = true)
            -> std::vector<std::string>;

    static auto smartSplit(const std::string &input) -> std::vector<std::string>;

    static auto getFFmpegPath() -> std::string;

    static auto getFFprobePath() -> std::string;

    static auto getFFPlayPath() -> std::string;
};
