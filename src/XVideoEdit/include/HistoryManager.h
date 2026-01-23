#pragma once
#include "UIConfig.h"

#include <replxx.hxx>
#include <filesystem>
#include <memory>
#include <vector>
#include <string>

class HistoryManager
{
public:
    explicit HistoryManager(std::unique_ptr<replxx::Replxx>& rx, const UIConfig& config);
    ~HistoryManager();

public:
    /// 加载历史记录
    auto loadHistory() -> void;

    /// 保存历史记录
    auto saveHistory() -> void;

    /// 添加到历史记录
    auto addToHistory(const std::string_view& input) -> void;

    /// 清空历史记录
    auto clearHistory() -> void;

    /// 获取历史记录列表（需要缓存）
    auto getHistory() -> const std::vector<std::string>&;

    /// 搜索历史记录
    auto searchHistory(const std::string& prefix) -> std::vector<std::string>;

    /// 获取最近 N 条历史记录
    auto getRecentHistory(int count = 10) -> std::vector<std::string>;

private:
    class PImpl;
    std::unique_ptr<PImpl> impl_;
};
