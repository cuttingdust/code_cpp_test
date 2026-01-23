#pragma once

#include <filesystem>
#include <string_view>

namespace fs = std::filesystem;

struct UIConfig
{
    std::string prompt      = "\x1b[1;32m>>\x1b[0m ";
    std::string historyFile = ".command_history";
    fs::path    historyPath = ".command_history";
    std::string exitCommand = "exit";
    std::string helpCommand = "help";

    /// 功能开关
    bool enableColor      = true;
    bool enableREPL       = true;
    bool enableHistory    = true;
    bool enableCompletion = true;

    /// 配置参数
    int historySize = 1000;
    int maxHintRows = 3;
    int hintDelay   = 500;

    static auto Default() -> UIConfig
    {
        return UIConfig{};
    }

    static auto NoColor() -> UIConfig
    {
        auto config        = Default();
        config.enableColor = false;
        config.prompt      = ">> ";
        return config;
    }

    static auto SimpleMode() -> UIConfig
    {
        auto config        = Default();
        config.enableColor = false;
        config.enableREPL  = false;
        config.prompt      = ">> ";
        return config;
    }

    static auto WithCustomPrompt(const std::string_view &prompt) -> UIConfig
    {
        auto config   = Default();
        config.prompt = prompt;
        return config;
    }
};
