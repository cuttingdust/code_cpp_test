#pragma once

#include "XTask.h"

class TaskProgressBar;

class TaskManager
{
public:
    using TaskCreator = std::function<XTask::Ptr(const std::string_view& name, const XTask::TaskFunc& func,
                                                 const std::string_view& description)>;

    using ProgressBarCreator = std::function<TaskProgressBar::Ptr(const std::string_view& name)>;

    /// 配置
    struct TaskTypeConfig
    {
        using List = std::map<std::string, TaskTypeConfig, std::less<>>;
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

    /// 指针调用
    auto createSimpleTask(const std::string_view& taskName, const XTask::TaskFunc& func,
                          const std::string_view& description = "") -> XTask::Ptr;

    /// 类型注册
    auto createAndRegisterTask(const std::string_view& taskName, const std::string_view& typeName,
                               const XTask::TaskFunc& func, const std::string_view& description = "") -> XTask::Ptr;

    /// 类型注册创建函数
    auto registerTaskType(const std::string_view& typeName, const TaskCreator& creator,
                          const ProgressBarCreator& progressBarCreator = nullptr,
                          const std::string_view&   description        = "") -> void;

    /// 获取所有注册的任务类型
    auto getTaskTypes() const -> std::vector<std::string>;

    /// 检查任务类型是否存在
    auto hasTaskType(const std::string_view& typeName) const -> bool;

    /// 获取任务类型描述
    auto getTaskTypeDescription(const std::string_view& typeName) const -> std::string;

public:
    /// 获取任务类型配置
    std::optional<TaskTypeConfig> getTaskTypeConfig(const std::string& typeName) const;

    /// 移除任务类型（不能移除默认类型）
    bool removeTaskType(const std::string& typeName);

public:
    /// =============== 任务实例管理（新接口） ===============

    /// 直接注册任务实例
    bool registerTaskInstance(const std::string& name, XTask::Ptr task, const std::string& typeName = "default");

    /// 检查任务实例是否存在
    bool hasTaskInstance(const std::string& name) const;

    /// 获取任务实例
    XTask::Ptr getTaskInstance(const std::string& name) const;

    /// 执行任务
    bool executeTask(const std::string& name, const std::map<std::string, std::string>& params, std::string& error);

    /// 获取所有任务实例名称
    std::vector<std::string> getTaskInstanceNames() const;

    /// 获取任务实例数量
    size_t getTaskInstanceCount() const;

    /// 获取任务实例信息
    auto getTaskInstanceInfo(const std::string& name) const -> TaskInstanceInfo::Option;

    /// 移除任务实例
    bool removeTaskInstance(const std::string& name);

    /// 清除所有任务实例
    void clearAllTaskInstances();

    // =============== 统计与信息 ===============

    /// 获取任务统计信息
    Statistics getStatistics() const;

    /// 获取任务执行历史
    std::vector<std::string> getTaskExecutionHistory(const std::string& taskName) const;

    /// 获取所有任务的执行历史
    std::map<std::string, std::vector<std::string>> getAllExecutionHistory() const;

    /// 清除任务执行历史
    void clearTaskHistory(const std::string& taskName);

    /// 清除所有执行历史
    void clearAllHistory();

public:
    /// =============== 快捷方法 ===============

    /// 注册默认任务类型
    template <typename TaskType>
    void registerDefaultTaskType(const std::string& typeName, const std::string& description = "");

    /// 快捷创建任务（使用默认任务类型）


    /// 获取支持的任务及其描述
    std::map<std::string, std::string> getTaskInfo() const;

public:
    /// 兼容原有 TaskManager::getTasks 接口
    std::map<std::string, XTask::Ptr, std::less<>>&       getTasks();
    const std::map<std::string, XTask::Ptr, std::less<>>& getTasks() const;

private:
    void registerDefaultTaskTypes();
    void addExecutionHistory(const std::string& taskName, const std::string& result);
    void updateStatistics(bool success);

private:
    /// 执行历史
    std::map<std::string, std::vector<std::string>> executionHistory_;

    /// 统计信息
    mutable Statistics statistics_;

private:
    class PImpl;
    std::unique_ptr<PImpl> impl_;
};

template <typename TaskType>
void TaskManager::registerDefaultTaskType(const std::string& typeName, const std::string& description)
{
    /// 任务注册 提前注册任务创建
    registerTaskType(
            typeName, [](const std::string& name, const XTask::TaskFunc& func, const std::string& desc)
            { return TaskType::create(name, func, desc); }, nullptr, description);
}
