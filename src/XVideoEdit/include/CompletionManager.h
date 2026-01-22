#pragma once

#include "XTask.h"
#include <replxx.hxx>

class CompletionManager
{
public:
    struct CompletionContext
    {
        std::string prefix;
        std::string pathPart;
        bool        isPathCompletion = false;
    };
    CompletionManager();
    CompletionManager(const XTask::List& tasks);
    ~CompletionManager();
    using Completions = replxx::Replxx::completions_t;
    using Hints       = replxx::Replxx::hints_t;
    using Color       = replxx::Replxx::Color;

public:
    auto registerBuiltinCommand(const std::string_view& command) -> void;

    auto getBuiltinCommands() const -> const std::vector<std::string>&;

    auto completionHook(const std::string_view& input, int& contextLen) -> Completions;

    auto hintHook(const std::string_view& input, int& contextLen, Color& color) -> Hints;

    auto setTaskList(const XTask::List& tasks) -> void;

    auto registerTaskCommand(const std::string_view& command, const XTask::Ptr& task) -> void;

private:
    class PImpl;
    std::unique_ptr<PImpl> impl_;
};
