#include "CommandParser.h"

#include <ranges>

auto CommandParser::ParsedCommand::hasOption(const std::string &key) const -> bool
{
    return options.contains(key);
}

auto CommandParser::ParsedCommand::getOption(const std::string &key) const -> std::optional<std::string>
{
    auto it = options.find(key);
    if (it != options.end())
        return it->second;
    return std::nullopt;
}

auto CommandParser::ParsedCommand::hasArgs() const -> bool
{
    return !args.empty();
}

auto CommandParser::ParsedCommand::argCount() const -> size_t
{
    return args.size();
}

auto CommandParser::parse(const std::string_view &input) -> CommandParser::ParsedCommand
{
    ParsedCommand result;

    if (input.empty())
        throw std::invalid_argument("Input cannot be empty");

    std::string_view remaining = input;
    remaining                  = trim(remaining);

    /// 解析命令
    size_t pos = remaining.find_first_of(" \t");
    if (pos == std::string_view::npos)
    {
        result.command = std::string(remaining);
        return result;
    }

    result.command = std::string(remaining.substr(0, pos));
    remaining      = remaining.substr(pos);
    remaining      = trim(remaining);

    /// 解析参数和选项
    while (!remaining.empty())
    {
        if (remaining[0] == '-')
        {
            /// 解析选项
            auto optionResult = parseOption(remaining);
            result.options.insert(optionResult.begin(), optionResult.end());
        }
        else
        {
            /// 解析参数
            pos = remaining.find_first_of(" \t");
            if (pos == std::string_view::npos)
            {
                result.args.emplace_back(remaining);
                break;
            }
            result.args.emplace_back(remaining.substr(0, pos));
            remaining = remaining.substr(pos);
            remaining = trim(remaining);
        }
    }

    return result;
}

auto CommandParser::validate(const ParsedCommand &cmd) -> bool
{
    if (cmd.command.empty())
        return false;

    /// 检查命令格式（只允许字母、数字、下划线）
    for (const char c : cmd.command)
    {
        if (!std::isalnum(c) && c != '_')
            return false;
    }

    /// 检查选项格式
    for (const auto &key : cmd.options | std::views::keys)
    {
        if (key.empty() || key[0] != '-')
            return false;
    }

    return true;
}

auto CommandParser::trim(std::string_view str) -> std::string_view
{
    size_t start = str.find_first_not_of(" \t");
    if (start == std::string_view::npos)
        return "";
    size_t end = str.find_last_not_of(" \t");
    return str.substr(start, end - start + 1);
}

auto CommandParser::parseOption(std::string_view &remaining) -> std::map<std::string, std::string>
{
    std::map<std::string, std::string> options;

    size_t      pos = remaining.find_first_of(" \t=");
    std::string key = std::string(remaining.substr(0, pos));

    remaining = remaining.substr(pos);
    remaining = trim(remaining);

    if (!remaining.empty() && remaining[0] == '=')
    {
        /// 有值的选项: --option=value
        remaining = remaining.substr(1);
        remaining = trim(remaining);

        pos = remaining.find_first_of(" \t");
        std::string value;
        if (pos == std::string_view::npos)
        {
            value     = std::string(remaining);
            remaining = "";
        }
        else
        {
            value     = std::string(remaining.substr(0, pos));
            remaining = remaining.substr(pos);
            remaining = trim(remaining);
        }
        options[key] = value;
    }
    else if (!remaining.empty() && remaining[0] != '-')
    {
        /// 有值的选项: --option value
        pos = remaining.find_first_of(" \t");
        std::string value;
        if (pos == std::string_view::npos)
        {
            value     = std::string(remaining);
            remaining = "";
        }
        else
        {
            value     = std::string(remaining.substr(0, pos));
            remaining = remaining.substr(pos);
            remaining = trim(remaining);
        }
        options[key] = value;
    }
    else
    {
        /// 无值选项: --flag
        options[key] = "";
    }

    return options;
}
