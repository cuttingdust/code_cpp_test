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
    /// 命令构建器接口
    class ICommandBuilder
    {
    public:
        virtual ~ICommandBuilder() = default;
        using Ptr                  = std::shared_ptr<ICommandBuilder>;
        virtual auto build(const std::map<std::string, ParameterValue>& params) const -> std::string = 0;
        virtual auto validate(const std::map<std::string, ParameterValue>& params, std::string& errorMsg) const
                -> bool                                                                                 = 0;
        virtual auto getTitle(const std::map<std::string, ParameterValue>& params) const -> std::string = 0;
    };
    using SmartBuilder     = ICommandBuilder::Ptr;
    using List             = std::map<std::string, XTask::Ptr, std::less<>>;
    using Container        = std::vector<XTask::Ptr>;
    using TaskFunc         = std::function<void(const std::map<std::string, ParameterValue>&, const std::string&)>;
    using ProgressCallback = std::function<void(float percent, const std::string& timeInfo)>;
    using Type             = Parameter::Type;
    using CompletionFunc   = Parameter::CompletionFunc;
    explicit XTask();
    explicit XTask(const std::string_view& name, const TaskFunc& func, const std::string_view& desc = "");
    ~XTask() override;

    XTask(XTask&&) noexcept;
    XTask& operator=(XTask&&) noexcept;

public:
    /// 为任务添加参数定义（支持指定类型）
    auto addParameter(const std::string_view& paramName, Type type = Type::String, const std::string_view& desc = "",
                      bool required = false, const CompletionFunc& completor = nullptr) -> XTask&;

    /// 便捷方法：添加字符串参数
    auto addStringParam(const std::string_view& paramName, const std::string_view& desc = "", bool required = false,
                        const CompletionFunc& completor = nullptr) -> XTask&;

    /// 便捷方法：添加整数参数
    auto addIntParam(const std::string_view& paramName, const std::string_view& desc = "", bool required = false,
                     const CompletionFunc& completor = nullptr) -> XTask&;

    /// 便捷方法：添加浮点数参数
    auto addDoubleParam(const std::string_view& paramName, const std::string_view& desc = "", bool required = false,
                        const CompletionFunc& completor = nullptr) -> XTask&;

    /// 便捷方法：添加布尔参数
    auto addBoolParam(const std::string_view& paramName, const std::string_view& desc = "", bool required = false,
                      const CompletionFunc& completor = nullptr) -> XTask&;

    auto addFileParam(const std::string_view& paramName, const std::string_view& desc, bool required,
                      const CompletionFunc& completor = nullptr) -> XTask&;

    auto addDirectoryParam(const std::string_view& paramName, const std::string_view& desc, bool required,
                           const CompletionFunc& completor = nullptr) -> XTask&;

public:
    /// 执行任务（带参数验证和类型检查）
    auto doExecute(const std::map<std::string, std::string>& inputParams, std::string& errorMsg) -> bool;

    auto execute(const std::string& command, const std::map<std::string, ParameterValue>& inputParams,
                 std::string& errorMsg, std::string& resultMsg) -> bool override;

    auto validateCommon(const std::map<std::string, ParameterValue>& inputParams, std::string& errorMsg)
            -> bool override;

    auto validateSuccess(const std::map<std::string, ParameterValue>& inputParams, std::string& errorMsg)
            -> bool override;

    auto getName() const -> const std::string&;

    auto getDescription() const -> const std::string&;

    auto getParameters() const -> const Parameter::Container&;

    auto hasParameter(const Parameter& parameter) const -> bool;

    auto hasParameter(const std::string_view& parameter) const -> bool;

    auto getRequiredParam(const std::map<std::string, ParameterValue>& params, const std::string& key,
                          std::string& errorMsg) const -> ParameterValue;

    auto getParameter(const std::string& key, std::string& errorMsg) const -> ParameterValue;

    auto setProgressBar(const TaskProgressBar::Ptr& bar) -> XTask&;

    auto progressBar() const -> TaskProgressBar::Ptr;

    auto setProgressCallback(ProgressCallback callback) -> XTask&;

    auto getProgressCallback() const -> ProgressCallback;

    auto setBuilder(const ICommandBuilder::Ptr& builder) -> XTask&;

    auto builder() const -> ICommandBuilder::Ptr;

public:
    auto setTitle(const std::string_view& name) -> void;

    auto setName(const std::string_view& name) const -> void;

    auto setDescription(const std::string_view& desc) const -> void;

    auto updateProgress(XExec& exec, const std::string_view& taskName,
                        const std::map<std::string, ParameterValue>& inputParams) -> void;

    auto waitProgress(XExec& exec, const std::map<std::string, ParameterValue>& inputParams, std::string& errorMsg)
            -> bool;

private:
    class PImpl;
    std::unique_ptr<PImpl> impl_;
};
