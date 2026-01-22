#pragma once

#include "XTask.h"

class TaskProgressBar;

class TaskManager
{
public:
    using TaskCreator = std::function<XTask::Ptr(const std::string_view& name, const XTask::TaskFunc& func,
                                                 const std::string_view& description)>;

    using ProgressBarCreator = std::function<TaskProgressBar::Ptr(const std::string_view& name)>;

    using TypeHistoryList = std::vector<std::string>;

    using TaskHistoryList = std::map<std::string, TypeHistoryList>; /// 类型-》所有记录

    /// 配置
    struct TaskTypeConfig
    {
        using List   = std::map<std::string, TaskTypeConfig, std::less<>>;
        using Option = std::optional<TaskTypeConfig>;
        TaskCreator        taskCreator;        ///< 任务创造者
        ProgressBarCreator progressBarCreator; ///< 任务进度条
        std::string        description;        ///< 任务描述
    };

    /// 数据/信息
    struct TaskInstanceInfo
    {
        using Option = std::optional<TaskInstanceInfo>;
        using List   = std::map<std::string, TaskInstanceInfo, std::less<>>;

        std::string                           name;
        std::string                           typeName;
        XTask::Ptr                            task;
        std::vector<std::string>              executionHistory;
        std::chrono::system_clock::time_point createdTime;
        std::chrono::system_clock::time_point lastExecutedTime;
        size_t                                executionCount = 0;
        size_t                                successCount   = 0;
        size_t                                failureCount   = 0;
    };

    struct Statistics
    {
        size_t totalTaskTypes     = 0;
        size_t totalTaskInstances = 0;
        size_t totalExecutions    = 0;
        size_t successExecutions  = 0;
        size_t failedExecutions   = 0;
    };

    TaskManager();
    virtual ~TaskManager();

public:
    /// 链式调用
    auto registerTask(const std::string_view& name, const XTask::TaskFunc& func,
                      const std::string_view& description = "") -> XTask&;

    auto registerTaskInstance(const std::string_view& name, const XTask::Ptr& task,
                              const std::string_view& typeName = "default") -> bool;

    /// 指针调用
    auto createSimpleTask(const std::string_view& taskName, const XTask::TaskFunc& func,
                          const std::string_view& description = "") -> XTask::Ptr;

    /// 类型注册
    auto createAndRegisterTask(const std::string_view& taskName, const std::string_view& typeName,
                               const XTask::TaskFunc& func, const std::string_view& description = "") -> XTask::Ptr;

    /// 获取任务实例数量
    auto getTaskInstanceCount() const -> size_t;

    /// 获取所有任务
    auto getTaskInstances() const -> XTask::List;

    /// 清除所有任务实例
    auto clearAllTaskInstances() -> void;

    /// 获取所有任务实例名称
    auto getTaskInstanceNames() const -> std::vector<std::string>;

    /// 检查任务实例是否存在
    auto hasTaskInstance(const std::string_view& name) const -> bool;

    /// 获取任务实例
    auto getTaskInstance(const std::string_view& name) const -> XTask::Ptr;

    /// 获取任务实例信息
    auto getTaskInstanceInfo(const std::string_view& name) const -> TaskInstanceInfo::Option;

    /// 移除任务实例
    auto removeTaskInstance(const std::string_view& name) -> bool;

    /// 执行任务
    auto executeTask(const std::string_view& name, const std::map<std::string, std::string>& params, std::string& error)
            -> bool;

public:
    /// 类型注册创建函数
    auto registerType(const std::string_view& typeName, const TaskCreator& creator,
                      const ProgressBarCreator& progressBarCreator = nullptr, const std::string_view& description = "")
            -> void;

    template <typename TaskType>
    void registerType(const std::string_view& typeName, const std::string_view& description = "");

    template <typename TaskType, typename BarType>
    void registerType(const std::string_view& typeName, const std::string_view& description = "");

    /// 获取所有注册的任务类型
    auto getTaskTypes() const -> std::vector<std::string>;

    /// 检查任务类型是否存在
    auto hasTaskType(const std::string_view& typeName) const -> bool;

    /// 获取任务类型描述
    auto getTaskTypeDescription(const std::string_view& typeName) const -> std::string;

    /// 移除任务类型（不能移除默认类型）
    bool removeTaskType(const std::string_view& typeName);

    /// 获取任务类型配置
    auto getTaskTypeConfig(const std::string_view& typeName) const -> TaskTypeConfig::Option;

public:
    /// 获取支持的任务及其描述
    auto getTaskInfo() const -> std::map<std::string, std::string>;

    /// 获取任务统计信息
    auto getStatistics() const -> Statistics;

    /// 获取所有任务的执行历史
    auto getAllExecutionHistory() const -> TaskHistoryList;

    /// 清除所有执行历史
    auto clearAllHistory() -> void;

    /// 获取任务执行历史
    auto getTaskExecutionHistory(const std::string_view& taskName) const -> TypeHistoryList;

    /// 清除任务执行历史
    auto clearTaskHistory(const std::string_view& taskName) -> void;

private:
    class PImpl;
    std::unique_ptr<PImpl> impl_;
};

template <typename TaskType>
void TaskManager::registerType(const std::string_view& typeName, const std::string_view& description)
{
    /// 任务注册 提前注册任务创建
    registerType(
            typeName, [](const std::string_view& name, const XTask::TaskFunc& func, const std::string_view& desc)
            { return TaskType::create(name, func, desc); }, nullptr, description);
}

template <typename TaskType, typename BarType>
void TaskManager::registerType(const std::string_view& typeName, const std::string_view& description)
{
    registerType(
            typeName, [](const std::string_view& name, const XTask::TaskFunc& func, const std::string_view& desc)
            { return TaskType::create(name, func, desc); },
            [](const std::string_view& name) { return BarType::create(name); }, description);
}
