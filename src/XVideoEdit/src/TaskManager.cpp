#include "TaskManager.h"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <ranges>
#include <stdexcept>
#include <utility>

class TaskManager::PImpl
{
public:
    PImpl(TaskManager* owenr);
    ~PImpl() = default;

public:
    auto registerDefaultTaskTypes() -> void;

    auto addExecutionHistory(const std::string_view& taskName, const std::string_view& result) -> void;

    auto updateStatistics(bool success) -> void;

public:
    TaskManager*           owenr_ = nullptr;
    TaskInstanceInfo::List taskInstances_;   ///< 任务实例
    TaskTypeConfig::List   taskTypeConfigs_; ///< 任务类型配置
    mutable std::mutex     mtx_;
    mutable Statistics     statistics_;       ///< 统计信息
    TaskHistoryList        executionHistory_; ///< 执行历史
};

TaskManager::PImpl::PImpl(TaskManager* owenr) : owenr_(owenr)
{
}

auto TaskManager::PImpl::registerDefaultTaskTypes() -> void
{
    std::lock_guard<std::mutex> lock(mtx_);

    /// 注册默认任务类型
    taskTypeConfigs_["default"] =
            TaskTypeConfig{ .taskCreator = [](const std::string_view& name, const XTask::TaskFunc& func,
                                              const std::string_view& desc) -> XTask::Ptr
                            { return XTask::create(name, func, desc); },
                            .progressBarCreator = nullptr,
                            .description        = "通用任务" };

    statistics_.totalTaskTypes = taskTypeConfigs_.size();
}

auto TaskManager::PImpl::addExecutionHistory(const std::string_view& taskName, const std::string_view& result) -> void
{
    auto& history = executionHistory_[std::string{ taskName }];
    history.emplace_back(result);

    /// 保持历史记录数量不超过100条
    constexpr size_t MAX_HISTORY = 100;
    if (history.size() > MAX_HISTORY)
    {
        history.erase(history.begin(), history.begin() + (history.size() - MAX_HISTORY));
    }
}

auto TaskManager::PImpl::updateStatistics(bool success) -> void
{
    (success ? statistics_.successExecutions : statistics_.failedExecutions)++;
}

TaskManager::TaskManager() : impl_(std::make_unique<TaskManager::PImpl>(this))
{
    impl_->registerDefaultTaskTypes();
}

TaskManager::~TaskManager() = default;

auto TaskManager::registerType(const std::string_view& typeName, const TaskCreator& creator,
                               const ProgressBarCreator& progressBarCreator, const std::string_view& description)
        -> void
{
    /// Xtask的工厂
    if (typeName.empty())
    {
        throw std::invalid_argument("任务类型名称不能为空");
    }

    if (!creator)
    {
        throw std::invalid_argument("任务创建器不能为空");
    }

    std::lock_guard<std::mutex> lock(impl_->mtx_);

    bool isNewType = !impl_->taskTypeConfigs_.contains(typeName);
    impl_->taskTypeConfigs_[std::string{ typeName }] =
            TaskTypeConfig{ .taskCreator        = creator,
                            .progressBarCreator = progressBarCreator,
                            .description        = description.empty() ? "自定义任务类型: " + std::string(typeName)
                                                                      : std::string{ description } };

    if (isNewType)
    {
        impl_->statistics_.totalTaskTypes++;
    }
}

auto TaskManager::getTaskTypes() const -> std::vector<std::string>
{
    std::lock_guard<std::mutex> lock(impl_->mtx_);

    std::vector<std::string> types;
    types.reserve(impl_->taskTypeConfigs_.size());

    for (const auto& typeName : impl_->taskTypeConfigs_ | std::views::keys)
    {
        types.push_back(typeName);
    }

    std::ranges::sort(types);
    return types;
}

auto TaskManager::hasTaskType(const std::string_view& typeName) const -> bool
{
    std::lock_guard<std::mutex> lock(impl_->mtx_);
    return impl_->taskTypeConfigs_.contains(typeName);
}

auto TaskManager::getTaskTypeDescription(const std::string_view& typeName) const -> std::string
{
    std::lock_guard<std::mutex> lock(impl_->mtx_);

    if (const auto it = impl_->taskTypeConfigs_.find(typeName); it != impl_->taskTypeConfigs_.end())
    {
        return it->second.description;
    }

    return "未知任务类型: " + std::string{ typeName };
}

auto TaskManager::getTaskTypeConfig(const std::string_view& typeName) const -> TaskTypeConfig::Option
{
    std::lock_guard<std::mutex> lock(impl_->mtx_);

    auto it = impl_->taskTypeConfigs_.find(typeName);
    if (it != impl_->taskTypeConfigs_.end())
    {
        return it->second;
    }

    return std::nullopt;
}

auto TaskManager::removeTaskType(const std::string_view& typeName) -> bool
{
    std::lock_guard<std::mutex> lock(impl_->mtx_);

    /// 不能移除默认类型
    if (typeName == "default")
    {
        return false;
    }

    if (impl_->taskTypeConfigs_.erase(typeName) > 0)
    {
        impl_->statistics_.totalTaskTypes--;
        return true;
    }

    return false;
}

auto TaskManager::createAndRegisterTask(const std::string_view& taskName, const std::string_view& typeName,
                                        const XTask::TaskFunc& func, const std::string_view& description) -> XTask::Ptr
{
    if (taskName.empty())
    {
        throw std::invalid_argument("任务名称不能为空");
    }

    if (!func)
    {
        throw std::invalid_argument("任务函数不能为空");
    }

    std::lock_guard<std::mutex> lock(impl_->mtx_);

    /// 检查任务是否已存在
    if (impl_->taskInstances_.contains(taskName))
    {
        throw std::runtime_error("任务已存在: " + std::string{ taskName });
    }

    /// 查找任务类型配置
    auto typeIt = impl_->taskTypeConfigs_.find(typeName);
    if (typeIt == impl_->taskTypeConfigs_.end())
    {
        /// 回退到默认类型
        typeIt = impl_->taskTypeConfigs_.find("default");
        if (typeIt == impl_->taskTypeConfigs_.end())
        {
            throw std::runtime_error("未找到任务类型: " + std::string{ typeName });
        }
    }

    const auto& config = typeIt->second;

    /// 确定任务描述
    std::string taskDescription;
    if (!description.empty())
    {
        taskDescription = description;
    }
    else if (!config.description.empty())
    {
        taskDescription = config.description;
    }
    else
    {
        taskDescription = taskName;
    }

    /// 创建任务
    auto task = config.taskCreator(taskName, func, taskDescription);

    /// 设置进度条
    if (config.progressBarCreator)
    {
        if (auto progressBar = config.progressBarCreator(taskName))
        {
            task->setProgressBar(progressBar);
        }
    }

    TaskInstanceInfo info{ .name             = std::string{ taskName },
                           .typeName         = std::string{ typeName },
                           .task             = task,
                           .createdTime      = std::chrono::system_clock::now(),
                           .lastExecutedTime = std::chrono::system_clock::now() };
    /// 注册任务实例

    impl_->taskInstances_[std::string{ taskName }] = std::move(info);
    impl_->statistics_.totalTaskInstances++;

    return task;
}

auto TaskManager::registerTask(const std::string_view& name, const XTask::TaskFunc& func,
                               const std::string_view& description) -> XTask&
{
    auto task = createSimpleTask(name, func, description);
    if (!task)
    {
        throw std::runtime_error("创建任务失败: " + std::string{ name });
    }
    return *task;
}

auto TaskManager::registerTaskInstance(const std::string_view& name, const XTask::Ptr& task,
                                       const std::string_view& typeName) -> bool
{
    if (name.empty() || !task)
    {
        return false;
    }

    std::lock_guard<std::mutex> lock(impl_->mtx_);

    if (impl_->taskInstances_.contains(name))
    {
        return false;
    }

    TaskInstanceInfo info{ .name             = std::string{ name },
                           .typeName         = typeName.empty() ? "default" : std::string{ typeName },
                           .task             = task,
                           .createdTime      = std::chrono::system_clock::now(),
                           .lastExecutedTime = std::chrono::system_clock::now() };

    impl_->taskInstances_[std::string{ name }] = std::move(info);
    impl_->statistics_.totalTaskInstances++;

    return true;
}

auto TaskManager::hasTaskInstance(const std::string_view& name) const -> bool
{
    std::lock_guard<std::mutex> lock(impl_->mtx_);
    return impl_->taskInstances_.contains(name);
}

auto TaskManager::getTaskInstance(const std::string_view& name) const -> XTask::Ptr
{
    std::lock_guard<std::mutex> lock(impl_->mtx_);

    auto it = impl_->taskInstances_.find(name);
    if (it != impl_->taskInstances_.end())
    {
        return it->second.task;
    }

    return nullptr;
}

auto TaskManager::executeTask(const std::string_view& name, const std::map<std::string, std::string>& params,
                              std::string& error) -> bool
{
    std::lock_guard<std::mutex> lock(impl_->mtx_);

    const auto it = impl_->taskInstances_.find(name);
    if (it == impl_->taskInstances_.end())
    {
        error = "任务不存在: " + std::string{ name };
        return false;
    }

    auto& taskInfo = it->second;
    auto  task     = taskInfo.task;

    if (!task)
    {
        error = "任务对象无效: " + std::string{ name };
        return false;
    }

    try
    {
        auto now                  = std::chrono::system_clock::now();
        taskInfo.lastExecutedTime = now;
        taskInfo.executionCount++;

        impl_->statistics_.totalExecutions++;

        bool success = task->doExecute(params, error);

        /// 更新统计信息
        impl_->updateStatistics(success);

        /// 更新任务实例统计
        if (success)
        {
            taskInfo.successCount++;
        }
        else
        {
            taskInfo.failureCount++;
        }

        /// 记录执行历史
        std::stringstream ss;
        auto              time_t_now = std::chrono::system_clock::to_time_t(now);
        ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S") << " - "
           << (success ? "成功" : "失败: " + error);
        impl_->addExecutionHistory(name, ss.str());

        return success;
    }
    catch (const std::exception& e)
    {
        error = std::string("执行异常: ") + e.what();
        taskInfo.failureCount++;
        impl_->updateStatistics(false);

        std::stringstream ss;
        auto              now        = std::chrono::system_clock::now();
        auto              time_t_now = std::chrono::system_clock::to_time_t(now);
        ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S") << " - 异常: " << e.what();
        impl_->addExecutionHistory(name, ss.str());

        return false;
    }
}

auto TaskManager::getTaskInstanceNames() const -> std::vector<std::string>
{
    std::lock_guard<std::mutex> lock(impl_->mtx_);

    std::vector<std::string> names;
    names.reserve(impl_->taskInstances_.size());

    for (const auto& name : impl_->taskInstances_ | std::views::keys)
    {
        names.push_back(name);
    }

    std::ranges::sort(names);
    return names;
}

auto TaskManager::getTaskInstanceCount() const -> size_t
{
    std::lock_guard<std::mutex> lock(impl_->mtx_);
    return impl_->taskInstances_.size();
}

auto TaskManager::getTaskInstanceInfo(const std::string_view& name) const -> TaskInstanceInfo::Option
{
    std::lock_guard<std::mutex> lock(impl_->mtx_);
    if (const auto it = impl_->taskInstances_.find(name); it != impl_->taskInstances_.end())
    {
        return it->second;
    }

    return std::nullopt;
}


auto TaskManager::removeTaskInstance(const std::string_view& name) -> bool
{
    std::lock_guard<std::mutex> lock(impl_->mtx_);

    if (impl_->taskInstances_.erase(name) > 0)
    {
        impl_->statistics_.totalTaskInstances--;
        impl_->executionHistory_.erase(std::string{ name });
        return true;
    }

    return false;
}

auto TaskManager::clearAllTaskInstances() -> void
{
    std::lock_guard<std::mutex> lock(impl_->mtx_);

    impl_->taskInstances_.clear();
    impl_->executionHistory_.clear();
    impl_->statistics_.totalTaskInstances = 0;
}

auto TaskManager::getStatistics() const -> TaskManager::Statistics
{
    std::lock_guard<std::mutex> lock(impl_->mtx_);
    return impl_->statistics_;
}

auto TaskManager::getAllExecutionHistory() const -> TaskHistoryList
{
    std::lock_guard<std::mutex> lock(impl_->mtx_);
    return impl_->executionHistory_;
}

auto TaskManager::getTaskExecutionHistory(const std::string_view& taskName) const -> TypeHistoryList
{
    std::lock_guard<std::mutex> lock(impl_->mtx_);

    auto it = impl_->executionHistory_.find(std::string{ taskName });
    if (it != impl_->executionHistory_.end())
    {
        return it->second;
    }

    return {};
}


auto TaskManager::clearTaskHistory(const std::string_view& taskName) -> void
{
    std::lock_guard<std::mutex> lock(impl_->mtx_);
    impl_->executionHistory_.erase(std::string{ taskName });
}

auto TaskManager::clearAllHistory() -> void
{
    std::lock_guard<std::mutex> lock(impl_->mtx_);
    impl_->executionHistory_.clear();
}

auto TaskManager::createSimpleTask(const std::string_view& taskName, const XTask::TaskFunc& func,
                                   const std::string_view& description) -> XTask::Ptr
{
    return createAndRegisterTask(taskName, "default", func, description);
}

auto TaskManager::getTaskInfo() const -> std::map<std::string, std::string>
{
    std::lock_guard<std::mutex> lock(impl_->mtx_);

    std::map<std::string, std::string> info;
    for (const auto& [taskName, taskInfo] : impl_->taskInstances_)
    {
        auto taskDesc  = taskInfo.task->getDescription();
        info[taskName] = taskDesc.empty() ? "无描述" : taskDesc;
    }

    return info;
}

auto TaskManager::getTaskInstances() const -> XTask::List
{
    std::lock_guard<std::mutex> lock(impl_->mtx_);

    static XTask::List tasks;
    tasks.clear();

    for (const auto& [name, taskInfo] : impl_->taskInstances_)
    {
        tasks[name] = taskInfo.task;
    }

    return tasks;
}
