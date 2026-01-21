#pragma once

#include "ITask.h"
#include "Parameter.h"
#include "ParameterValue.h"
#include "TaskProgressBar.h"

class TaskProgressBar;

/// 任务定义类
class XTask : public ITask
{
    DECLARE_CREATE_DEFAULT(XTask)
public:
    using List             = std::map<std::string, XTask::Ptr, std::less<>>;
    using Container        = std::vector<XTask::Ptr>;
    using TaskFunc         = std::function<void(const std::map<std::string, ParameterValue>&)>;
    using ProgressCallback = std::function<void(float percent, const std::string& timeInfo)>;
    explicit XTask();
    explicit XTask(const std::string_view& name, const TaskFunc& func, const std::string_view& desc = "");
    ~XTask() override;

    XTask(XTask&&) noexcept;
    XTask& operator=(XTask&&) noexcept;

public:
    /// 为任务添加参数定义（支持指定类型）
    auto addParameter(const std::string& paramName, Parameter::Type type = Parameter::Type::String,
                      const std::string& desc = "", bool required = false,
                      Parameter::CompletionFunc completor = nullptr) -> XTask&;

    /// 便捷方法：添加字符串参数
    auto addStringParam(const std::string& paramName, const std::string& desc = "", bool required = false,
                        Parameter::CompletionFunc completor = nullptr) -> XTask&;

    /// 便捷方法：添加整数参数
    auto addIntParam(const std::string& paramName, const std::string& desc = "", bool required = false,
                     Parameter::CompletionFunc completor = nullptr) -> XTask&;

    /// 便捷方法：添加浮点数参数
    auto addDoubleParam(const std::string& paramName, const std::string& desc = "", bool required = false,
                        Parameter::CompletionFunc completor = nullptr) -> XTask&;

    /// 便捷方法：添加布尔参数
    auto addBoolParam(const std::string& paramName, const std::string& desc = "", bool required = false,
                      Parameter::CompletionFunc completor = nullptr) -> XTask&;

    auto addFileParam(const std::string& paramName, const std::string& desc, bool required,
                      Parameter::CompletionFunc completor = nullptr) -> XTask&;

    auto addDirectoryParam(const std::string& paramName, const std::string& desc, bool required,
                           Parameter::CompletionFunc completor = nullptr) -> XTask&;

public:
    /// 执行任务（带参数验证和类型检查）
    auto execute(const std::map<std::string, std::string>& inputParams, std::string& errorMsg) const -> bool override;

    auto getName() const -> const std::string&;

    auto getDescription() const -> const std::string&;

    auto getParameters() const -> const Parameter::Container&;

    auto hasParameter(const Parameter& parameter) const -> bool;

    auto hasParameter(const std::string_view& parameter) const -> bool;

    auto getRequiredParam(const std::map<std::string, std::string>& params, const std::string& key,
                          std::string& errorMsg) const -> std::string;

    auto getParameter(const std::string& key, std::string& errorMsg) const -> ParameterValue;

    auto setTaskProgressBar(const TaskProgressBar::Ptr& bar) -> void;

    auto getTaskProgressBar() const -> TaskProgressBar::Ptr;

    void setProgressCallback(ProgressCallback callback) const;

    auto getProgressCallback() const -> ProgressCallback;

public:
    auto setTitle(const std::string_view& name) -> void;

    auto setName(const std::string_view& name) const -> void;

    auto setDescription(const std::string_view& desc) const -> void;

    auto updateProgress(XExec& exec, const std::string_view& taskName,
                        const std::map<std::string, std::string>& inputParams) -> void;

private:
    class PImpl;
    std::unique_ptr<PImpl> impl_;
};
