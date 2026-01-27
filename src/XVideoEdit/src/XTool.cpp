#include "XTool.h"

#include <corecrt_io.h>
#include <cstdio>
#include <ranges>
#include <sstream>

auto XTool::isInteractiveTerminal() -> bool
{
#ifdef _WIN32
    return _isatty(_fileno(stdin)) && _isatty(_fileno(stdout));
#else
    return isatty(fileno(stdin)) && isatty(fileno(stdout));
#endif
}

auto XTool::split(const std::string_view &input, char delimiter, bool trimWhitespace) -> std::vector<std::string>
{
    std::vector<std::string> ret;
    if (input.empty())
        return ret;

    std::string        tmp;
    std::istringstream iss((input.data()));

    while (std::getline(iss, tmp, delimiter))
    {
        if (trimWhitespace)
        {
            tmp.erase(tmp.begin(), std::ranges::find_if(tmp, [](unsigned char ch) { return !std::isspace(ch); }));
            tmp.erase(std::ranges::find_if(std::ranges::reverse_view(tmp),
                                           [](unsigned char ch) { return !std::isspace(ch); })
                              .base(),
                      tmp.end());
        }

        if (!tmp.empty())
        {
            ret.push_back(tmp);
        }
    }

    return ret;
}

auto XTool::smartSplit(const std::string &input) -> std::vector<std::string>
{
    std::vector<std::string> tokens;
    std::string              current;
    bool                     inQuotes  = false;
    char                     quoteChar = 0;

    for (size_t i = 0; i < input.length(); ++i)
    {
        char c = input[i];

        if (c == '\\' && i + 1 < input.length())
        {
            current += input[++i];
            continue;
        }

        if ((c == '"' || c == '\'') && !inQuotes)
        {
            inQuotes  = true;
            quoteChar = c;
            current += c;
        }
        else if (c == quoteChar && inQuotes)
        {
            inQuotes = false;
            current += c;
        }
        else if (c == ' ' && !inQuotes)
        {
            if (!current.empty())
            {
                tokens.push_back(current);
                current.clear();
            }
        }
        else
        {
            current += c;
        }
    }

    if (!current.empty())
    {
        tokens.push_back(current);
    }

    return tokens;
}

auto XTool::getFFmpegPath() -> std::string
{
#ifdef FFMPEG_PATH
    return FFMPEG_PATH;
#else
    return "ffmpeg";
#endif
}

auto XTool::getFFprobePath() -> std::string
{
#ifdef FFPROBE_PATH
    return FFPROBE_PATH;
#else
    return "ffprobe";
#endif
}

auto XTool::getFFplayPath() -> std::string
{
#ifdef FFPLAY_PATH
    return FFPLAY_PATH;
#else
    return "ffplay";
#endif
}
