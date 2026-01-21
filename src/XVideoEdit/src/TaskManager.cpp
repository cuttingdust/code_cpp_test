#include "TaskManager.h"
#include "AVTask.h"
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
    TaskManager*           owenr_ = nullptr;
    TaskInstanceInfo::List taskInstances_;   ///< 任务实例
    TaskTypeConfig::List   taskTypeConfigs_; ///< 任务类型配置
    mutable std::mutex     mtx_;
};

TaskManager::PImpl::PImpl(TaskManager* owenr) : owenr_(owenr)
{
}

TaskManager::TaskManager() : impl_(std::make_unique<TaskManager::PImpl>(this))
{
    registerDefaultTaskTypes();
}

TaskManager::~TaskManager() = default;

void TaskManager::registerDefaultTaskTypes()
{
    std::lock_guard<std::mutex> lock(impl_->mtx_);

    /// 注册默认任务类型
    impl_->taskTypeConfigs_["default"] =
            TaskTypeConfig{ .taskCreator = [](const std::string_view& name, const XTask::TaskFunc& func,
                                              const std::string_view& desc) -> XTask::Ptr
                            { return XTask::create(name, func, desc); },
                            .progressBarCreator = nullptr,
                            .description        = "通用任务" };

    statistics_.totalTaskTypes = impl_->taskTypeConfigs_.size();
}

auto TaskManager::registerTaskType(const std::string_view& typeName, const TaskCreator& creator,
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
        statistics_.totalTaskTypes++;
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

std::optional<TaskManager::TaskTypeConfig> TaskManager::getTaskTypeConfig(const std::string& typeName) const
{
    std::lock_guard<std::mutex> lock(impl_->mtx_);

    auto it = impl_->taskTypeConfigs_.find(typeName);
    if (it != impl_->taskTypeConfigs_.end())
    {
        return it->second;
    }

    return std::nullopt;
}

bool TaskManager::removeTaskType(const std::string& typeName)
{
    std::lock_guard<std::mutex> lock(impl_->mtx_);

    /// 不能移除默认类型
    if (typeName == "default")
    {
        return false;
    }

    if (impl_->taskTypeConfigs_.erase(typeName) > 0)
    {
        statistics_.totalTaskTypes--;
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
            task->setTaskProgressBar(progressBar);
        }
    }

    TaskInstanceInfo info{ .name             = std::string{ taskName },
                           .typeName         = std::string{ typeName },
                           .task             = task,
                           .createdTime      = std::chrono::system_clock::now(),
                           .lastExecutedTime = std::chrono::system_clock::now() };
    /// 注册任务实例

    impl_->taskInstances_[std::string{ taskName }] = std::move(info);
    statistics_.totalTaskInstances++;

    return task;
}

XTask& TaskManager::registerTask(const std::string_view& name, const XTask::TaskFunc& func,
                                 const std::string_view& description)
{
    auto task = createSimpleTask(name, func, description);
    if (!task)
    {
        throw std::runtime_error("创建任务失败: " + std::string{ name });
    }
    return *task;
}

bool TaskManager::registerTaskInstance(const std::string& name, XTask::Ptr task, const std::string& typeName)
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

    TaskInstanceInfo info{ .name             = name,
                           .typeName         = typeName.empty() ? "default" : std::string{ typeName },
                           .task             = std::move(task),
                           .createdTime      = std::chrono::system_clock::now(),
                           .lastExecutedTime = std::chrono::system_clock::now() };

    impl_->taskInstances_[name] = std::move(info);
    statistics_.totalTaskInstances++;

    return true;
}

bool TaskManager::hasTaskInstance(const std::string& name) const
{
    std::lock_guard<std::mutex> lock(impl_->mtx_);
    return impl_->taskInstances_.contains(name);
}

XTask::Ptr TaskManager::getTaskInstance(const std::string& name) const
{
    std::lock_guard<std::mutex> lock(impl_->mtx_);

    auto it = impl_->taskInstances_.find(name);
    if (it != impl_->taskInstances_.end())
    {
        return it->second.task;
    }

    return nullptr;
}

bool TaskManager::executeTask(const std::string& name, const std::map<std::string, std::string>& params,
                              std::string& error)
{
    std::lock_guard<std::mutex> lock(impl_->mtx_);

    auto it = impl_->taskInstances_.find(name);
    if (it == impl_->taskInstances_.end())
    {
        error = "任务不存在: " + name;
        return false;
    }

    auto& taskInfo = it->second;
    auto  task     = taskInfo.task;

    if (!task)
    {
        error = "任务对象无效: " + name;
        return false;
    }

    try
    {
        auto now                  = std::chrono::system_clock::now();
        taskInfo.lastExecutedTime = now;
        taskInfo.executionCount++;

        statistics_.totalExecutions++;

        bool success = task->execute(params, error);

        // 更新统计信息
        updateStatistics(success);

        // 更新任务实例统计
        if (success)
        {
            taskInfo.successCount++;
        }
        else
        {
            taskInfo.failureCount++;
        }

        // 记录执行历史
        std::stringstream ss;
        auto              time_t_now = std::chrono::system_clock::to_time_t(now);
        ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S") << " - "
           << (success ? "成功" : "失败: " + error);
        addExecutionHistory(name, ss.str());

        return success;
    }
    catch (const std::exception& e)
    {
        error = std::string("执行异常: ") + e.what();
        taskInfo.failureCount++;
        updateStatistics(false);

        std::stringstream ss;
        auto              now        = std::chrono::system_clock::now();
        auto              time_t_now = std::chrono::system_clock::to_time_t(now);
        ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S") << " - 异常: " << e.what();
        addExecutionHistory(name, ss.str());

        return false;
    }
}

std::vector<std::string> TaskManager::getTaskInstanceNames() const
{
    std::lock_guard<std::mutex> lock(impl_->mtx_);

    std::vector<std::string> names;
    names.reserve(impl_->taskInstances_.size());

    for (const auto& [name, _] : impl_->taskInstances_)
    {
        names.push_back(name);
    }

    std::sort(names.begin(), names.end());
    return names;
}

size_t TaskManager::getTaskInstanceCount() const
{
    std::lock_guard<std::mutex> lock(impl_->mtx_);
    return impl_->taskInstances_.size();
}

auto TaskManager::getTaskInstanceInfo(const std::string& name) const -> TaskInstanceInfo::Option
{
    std::lock_guard<std::mutex> lock(impl_->mtx_);
    if (const auto it = impl_->taskInstances_.find(name); it != impl_->taskInstances_.end())
    {
        return it->second;
    }

    return std::nullopt;
}


bool TaskManager::removeTaskInstance(const std::string& name)
{
    std::lock_guard<std::mutex> lock(impl_->mtx_);

    if (impl_->taskInstances_.erase(name) > 0)
    {
        statistics_.totalTaskInstances--;
        executionHistory_.erase(name);
        return true;
    }

    return false;
}

void TaskManager::clearAllTaskInstances()
{
    std::lock_guard<std::mutex> lock(impl_->mtx_);

    impl_->taskInstances_.clear();
    executionHistory_.clear();
    statistics_.totalTaskInstances = 0;
}

TaskManager::Statistics TaskManager::getStatistics() const
{
    std::lock_guard<std::mutex> lock(impl_->mtx_);
    return statistics_;
}

std::vector<std::string> TaskManager::getTaskExecutionHistory(const std::string& taskName) const
{
    std::lock_guard<std::mutex> lock(impl_->mtx_);

    auto it = executionHistory_.find(taskName);
    if (it != executionHistory_.end())
    {
        return it->second;
    }

    return {};
}

std::map<std::string, std::vector<std::string>> TaskManager::getAllExecutionHistory() const
{
    std::lock_guard<std::mutex> lock(impl_->mtx_);
    return executionHistory_;
}

void TaskManager::clearTaskHistory(const std::string& taskName)
{
    std::lock_guard<std::mutex> lock(impl_->mtx_);
    executionHistory_.erase(taskName);
}

void TaskManager::clearAllHistory()
{
    std::lock_guard<std::mutex> lock(impl_->mtx_);
    executionHistory_.clear();
}

XTask::Ptr TaskManager::createSimpleTask(const std::string_view& taskName, const XTask::TaskFunc& func,
                                         const std::string_view& description)
{
    return createAndRegisterTask(taskName, "default", func, description);
}

std::map<std::string, std::string> TaskManager::getTaskInfo() const
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

std::map<std::string, XTask::Ptr, std::less<>>& TaskManager::getTasks()
{
    std::lock_guard<std::mutex> lock(impl_->mtx_);

    static std::map<std::string, XTask::Ptr, std::less<>> tasks;
    tasks.clear();

    for (const auto& [name, taskInfo] : impl_->taskInstances_)
    {
        tasks[name] = taskInfo.task;
    }

    return tasks;
}

const std::map<std::string, XTask::Ptr, std::less<>>& TaskManager::getTasks() const
{
    std::lock_guard<std::mutex> lock(impl_->mtx_);

    static std::map<std::string, XTask::Ptr, std::less<>> tasks;
    tasks.clear();

    for (const auto& [name, taskInfo] : impl_->taskInstances_)
    {
        tasks[name] = taskInfo.task;
    }

    return tasks;
}

// =============== 私有辅助方法 ===============

void TaskManager::addExecutionHistory(const std::string& taskName, const std::string& result)
{
    auto& history = executionHistory_[taskName];
    history.push_back(result);

    // 保持历史记录数量不超过100条
    constexpr size_t MAX_HISTORY = 100;
    if (history.size() > MAX_HISTORY)
    {
        history.erase(history.begin(), history.begin() + (history.size() - MAX_HISTORY));
    }
}

void TaskManager::updateStatistics(bool success)
{
    if (success)
    {
        statistics_.successExecutions++;
    }
    else
    {
        statistics_.failedExecutions++;
    }
}
