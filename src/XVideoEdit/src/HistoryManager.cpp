#include "HistoryManager.h"
#include <fstream>
#include <algorithm>

HistoryManager::HistoryManager(std::unique_ptr<replxx::Replxx>& rx, const UIConfig& config) : rx_(rx), config_(config)
{
    loadHistory();
}

HistoryManager::~HistoryManager()
{
    saveHistory();
}

auto HistoryManager::loadHistory() -> void
{
    if (fs::exists(config_.historyPath))
    {
        rx_->history_load(config_.historyPath.string());
        cacheDirty_ = true; // 标记缓存需要更新
    }
}

auto HistoryManager::saveHistory() -> void
{
    // 创建历史文件目录（如果不存在）
    auto historyDir = config_.historyPath.parent_path();
    if (!historyDir.empty() && !fs::exists(historyDir))
    {
        fs::create_directories(historyDir);
    }

    rx_->history_save(config_.historyPath.string());
}

auto HistoryManager::addToHistory(const std::string& input) -> void
{
    if (!input.empty())
    {
        rx_->history_add(input);
        cacheDirty_ = true; // 标记缓存需要更新
    }
}

auto HistoryManager::clearHistory() -> void
{
    rx_->history_clear();
    historyCache_.clear();
    cacheDirty_ = false;

    // 删除历史文件
    if (fs::exists(config_.historyPath))
    {
        fs::remove(config_.historyPath);
    }
}

auto HistoryManager::updateHistoryCache() -> void
{
    if (!cacheDirty_)
        return;

    // Replxx 没有直接获取历史记录的方法
    // 我们需要自己维护缓存或从文件读取

    historyCache_.clear();

    // 尝试从文件读取历史记录
    if (fs::exists(config_.historyPath))
    {
        std::ifstream file(config_.historyPath);
        std::string   line;

        while (std::getline(file, line))
        {
            if (!line.empty())
            {
                historyCache_.push_back(line);
            }
        }
    }

    cacheDirty_ = false;
}

auto HistoryManager::getHistory() -> const std::vector<std::string>&
{
    updateHistoryCache();
    return historyCache_;
}

auto HistoryManager::searchHistory(const std::string& prefix) -> std::vector<std::string>
{
    updateHistoryCache();

    std::vector<std::string> results;

    for (const auto& entry : historyCache_)
    {
        if (entry.find(prefix) == 0) // 以prefix开头
        {
            results.push_back(entry);
        }
    }

    return results;
}

auto HistoryManager::getRecentHistory(int count) -> std::vector<std::string>
{
    updateHistoryCache();

    if (count <= 0 || historyCache_.empty())
        return {};

    count = std::min(count, static_cast<int>(historyCache_.size()));

    /// 返回最近的历史记录（最新的在最后）
    std::vector<std::string> recent;
    auto                     start = historyCache_.rbegin();
    for (int i = 0; i < count && start != historyCache_.rend(); ++i, ++start)
    {
        recent.push_back(*start);
    }

    return recent;
}
