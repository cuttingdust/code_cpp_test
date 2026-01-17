// CompletionManager.h
#pragma once

#include "XTask.h"
#include "XFile.h"
#include "XTool.h"
#include <replxx.hxx>
#include <filesystem>
#include <map>
#include <memory>

namespace fs = std::filesystem;

class CompletionManager
{
public:
    struct CompletionContext
    {
        std::string prefix;
        std::string pathPart;
        bool        isPathCompletion = false;
    };

    CompletionManager(std::map<std::string, std::shared_ptr<XTask>, std::less<>>& tasks);

    replxx::Replxx::completions_t completionHook(const std::string& input, int& contextLen);
    replxx::Replxx::hints_t       hintHook(const std::string& input, int& contextLen, replxx::Replxx::Color& color);

private:
    // 补全分析
    CompletionContext analyzeInput(std::string_view input) const;
    bool              shouldCompletePath(const std::string& lastPart) const;

    // 不同类型的补全处理
    void handleTaskCompletion(const std::string& input, const CompletionContext& ctx,
                              replxx::Replxx::completions_t& completions, int& contextLen);
    void handleCommandCompletion(const std::string& input, const CompletionContext& ctx,
                                 replxx::Replxx::completions_t& completions);
    void handlePathCompletion(const CompletionContext& ctx, replxx::Replxx::completions_t& completions,
                              int& contextLen);

    // Task命令补全的辅助方法
    void handleEmptyTaskInput(replxx::Replxx::completions_t& completions);
    void handleTaskNameCompletion(const std::vector<std::string>& tokens, replxx::Replxx::completions_t& completions);

    // 修改：添加 originalInput 参数
    void handleTaskParamCompletion(const std::vector<std::string>& tokens, std::shared_ptr<XTask> task,
                                   const std::string& originalInput, replxx::Replxx::completions_t& completions,
                                   int& contextLen);

    // 路径补全
    void completePathSmart(std::string_view partialPath, replxx::Replxx::completions_t& completions, int& contextLen);
    void collectCompletions(const fs::path& basePath, const std::string& matchPrefix,
                            replxx::Replxx::completions_t& completions);
    void addCompletion(const fs::directory_entry& entry, const std::string& name,
                       replxx::Replxx::completions_t& completions);
    void sortCompletions(replxx::Replxx::completions_t& completions);

    // 提示信息
    std::string extractPathPartForHint(const std::string& input) const;

private:
    std::map<std::string, std::shared_ptr<XTask>, std::less<>>& tasks_;
};
