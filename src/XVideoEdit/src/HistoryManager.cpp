#include "HistoryManager.h"
#include <fstream>
#include <algorithm>

using SmartReplxx = std::unique_ptr<replxx::Replxx>;

class HistoryManager::PImpl
{
public:
    PImpl(HistoryManager* owenr, SmartReplxx& rx, const UIConfig& config);
    ~PImpl() = default;

public:
    auto loadHistory() -> void;

    auto saveHistory() -> void;

    auto addToHistory(const std::string_view& input) -> void;

    auto clearHistory() -> void;

    auto updateHistoryCache() -> void;

public:
    HistoryManager* owenr_ = nullptr;
    SmartReplxx&    rx_;
    const UIConfig& config_;

public:
    std::vector<std::string> historyCache_;
    bool                     cacheDirty_ = true;
};

HistoryManager::PImpl::PImpl(HistoryManager* owenr, SmartReplxx& rx, const UIConfig& config) :
    owenr_(owenr), rx_(rx), config_(config)
{
}

auto HistoryManager::PImpl::loadHistory() -> void
{
    if (fs::exists(config_.historyPath))
    {
        rx_->history_load(config_.historyPath.string());
        cacheDirty_ = true; /// 标记缓存需要更新
    }
}

auto HistoryManager::PImpl::saveHistory() -> void
{
    /// 创建历史文件目录（如果不存在）
    auto historyDir = config_.historyPath.parent_path();
    if (!historyDir.empty() && !fs::exists(historyDir))
    {
        fs::create_directories(historyDir);
    }

    rx_->history_save(config_.historyPath.string());
}

auto HistoryManager::PImpl::addToHistory(const std::string_view& input) -> void
{
    if (!input.empty())
    {
        rx_->history_add(std::string{ input });
        cacheDirty_ = true; /// 标记缓存需要更新
    }
}

auto HistoryManager::PImpl::clearHistory() -> void
{
    rx_->history_clear();
    historyCache_.clear();
    cacheDirty_ = false;

    /// 删除历史文件
    if (fs::exists(config_.historyPath))
    {
        fs::remove(config_.historyPath);
    }
}

auto HistoryManager::PImpl::updateHistoryCache() -> void
{
    if (!cacheDirty_)
        return;

    /// Replxx 没有直接获取历史记录的方法
    /// 我们需要自己维护缓存或从文件读取

    historyCache_.clear();

    /// 尝试从文件读取历史记录
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

HistoryManager::HistoryManager(SmartReplxx& rx, const UIConfig& config) :
    impl_(std::make_unique<HistoryManager::PImpl>(this, rx, config))
{
    loadHistory();
}

HistoryManager::~HistoryManager()
{
    saveHistory();
}

auto HistoryManager::loadHistory() -> void
{
    impl_->loadHistory();
}

auto HistoryManager::saveHistory() -> void
{
    impl_->saveHistory();
}

auto HistoryManager::addToHistory(const std::string_view& input) -> void
{
    impl_->addToHistory(input);
}

auto HistoryManager::clearHistory() -> void
{
    impl_->clearHistory();
}

auto HistoryManager::getHistory() -> const std::vector<std::string>&
{
    impl_->updateHistoryCache();
    return impl_->historyCache_;
}

auto HistoryManager::searchHistory(const std::string& prefix) -> std::vector<std::string>
{
    impl_->updateHistoryCache();

    std::vector<std::string> results;

    for (const auto& entry : impl_->historyCache_)
    {
        if (entry.starts_with(prefix)) /// 以prefix开头
        {
            results.push_back(entry);
        }
    }

    return results;
}

auto HistoryManager::getRecentHistory(int count) -> std::vector<std::string>
{
    impl_->updateHistoryCache();

    if (count <= 0 || impl_->historyCache_.empty())
        return {};

    count = std::min(count, static_cast<int>(impl_->historyCache_.size()));

    /// 返回最近的历史记录（最新的在最后）
    std::vector<std::string> recent;
    auto                     start = impl_->historyCache_.rbegin();
    for (int i = 0; i < count && start != impl_->historyCache_.rend(); ++i, ++start)
    {
        recent.push_back(*start);
    }

    return recent;
}
