#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <optional>
#include <stdexcept>

class CommandParser
{
public:
    struct ParsedCommand
    {
        std::string                        command;
        std::vector<std::string>           args;
        std::map<std::string, std::string> options;

        auto hasOption(const std::string& key) const -> bool;

        auto getOption(const std::string& key) const -> std::optional<std::string>;

        auto hasArgs() const -> bool;

        auto argCount() const -> size_t;
    };

public:
    auto parse(const std::string_view& input) -> ParsedCommand;

    auto validate(const ParsedCommand& cmd) -> bool;

private:
    static auto trim(std::string_view str) -> std::string_view;

    static auto parseOption(std::string_view& remaining) -> std::map<std::string, std::string>;
};
