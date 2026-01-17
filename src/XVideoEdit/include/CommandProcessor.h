// CommandProcessor.h
#pragma once

#include "XTask.h"
#include "XTool.h"
#include <string>
#include <map>
#include <iostream>
#include <memory>

class CommandProcessor
{
public:
    CommandProcessor(std::map<std::string, std::shared_ptr<XTask>, std::less<>>& tasks);

    void processCommand(std::string_view input);
    void printHelp() const;
    void listTasks() const;
    void printTaskUsage(std::string_view taskName) const;

    bool isRunning() const
    {
        return isRunning_;
    }
    void stop()
    {
        isRunning_ = false;
    }

private:
    void processTaskCommand(std::string_view input);
    void processBuiltinCommand(std::string_view command);
    bool parseTaskParameters(const std::vector<std::string>& tokens, std::map<std::string, std::string>& params);

private:
    std::map<std::string, std::shared_ptr<XTask>, std::less<>>& tasks_;
    bool                                                        isRunning_ = true;
};
