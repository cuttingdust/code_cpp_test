#include "XTask.h"

#include "ITask.h"
#include "TaskProgressBar.h"

#include <iostream>
#include <stdexcept>
#include <utility>


class XTask::PImpl
{
public:
    PImpl(XTask *owenr, const std::string_view &name, TaskFunc func, const std::string_view &desc);
    ~PImpl() = default;

public:
    XTask                                *owenr_ = nullptr;
    std::string                           name_;
    TaskFunc                              func_;
    std::string                           description_;
    std::vector<Parameter>                parameters_;
    std::map<std::string, ParameterValue> parameterList_;
    mutable ProgressCallback              progressCallback_;
    TaskProgressBar::Ptr                  progressBar_ = nullptr;
    ICommandBuilder::Ptr                  builder_     = nullptr;
};


XTask::PImpl::PImpl(XTask *owenr, const std::string_view &name, TaskFunc func, const std::string_view &desc) :
    owenr_(owenr), name_(name), func_(std::move(func)), description_(desc)
{
}

XTask::XTask()
{
}

XTask::XTask(const std::string_view &name, const TaskFunc &func, const std::string_view &desc) :
    impl_(std::make_unique<XTask::PImpl>(this, name, func, desc))
{
}

XTask::~XTask()                            = default;
XTask::XTask(XTask &&) noexcept            = default;
XTask &XTask::operator=(XTask &&) noexcept = default;


auto XTask::addParameter(const std::string_view &paramName, Type type, const std::string_view &desc, bool required,
                         const CompletionFunc &completor) -> XTask &
{
    auto param = Parameter(paramName, type, desc, required);
    if (completor)
    {
        param.setCompletions(completor);
    }
    impl_->parameters_.push_back(std::move(param));
    return *this;
}

auto XTask::addStringParam(const std::string_view &paramName, const std::string_view &desc, bool required,
                           const CompletionFunc &completor) -> XTask &
{
    return addParameter(paramName, Type::String, desc, required, completor);
}

auto XTask::addIntParam(const std::string_view &paramName, const std::string_view &desc, bool required,
                        const CompletionFunc &completor) -> XTask &
{
    return addParameter(paramName, Type::Int, desc, required, completor);
}

auto XTask::addDoubleParam(const std::string_view &paramName, const std::string_view &desc, bool required,
                           const CompletionFunc &completor) -> XTask &
{
    return addParameter(paramName, Type::Double, desc, required, completor);
}

auto XTask::addBoolParam(const std::string_view &paramName, const std::string_view &desc, bool required,
                         const CompletionFunc &completor) -> XTask &
{
    return addParameter(paramName, Type::Bool, desc, required, completor);
}

auto XTask::addFileParam(const std::string_view &paramName, const std::string_view &desc, bool required,
                         const CompletionFunc &completor) -> XTask &
{
    return addParameter(paramName, Type::File, desc, required, completor);
}

auto XTask::addDirectoryParam(const std::string_view &paramName, const std::string_view &desc, bool required,
                              const CompletionFunc &completor) -> XTask &
{
    return addParameter(paramName, Type::Directory, desc, required, completor);
}

auto XTask::doExecute(const std::map<std::string, std::string> &inputParams, std::string &errorMsg) -> bool
{
    /// 1. 验证必需参数
    for (const auto &param : impl_->parameters_)
    {
        if (param.isRequired() && !inputParams.contains(param.getName()))
        {
            errorMsg = "缺少必需参数: " + param.getName();
            return false;
        }
    }

    /// 2. 类型检查和转换
    impl_->parameterList_.clear(); /// 清空之前的结果
    for (const auto &[key, strValue] : inputParams)
    {
        /// 查找参数定义
        auto paramIt =
                std::ranges::find_if(impl_->parameters_, [&key](const Parameter &p) { return p.getName() == key; });

        if (paramIt != impl_->parameters_.end())
        {
            /// 有定义的类型参数，创建ParameterValue
            ParameterValue typedValue(strValue);

            /// 类型验证（简单版，实际执行时转换）
            try
            {
                switch (paramIt->getType())
                {
                    case Parameter::Type::Int:
                        typedValue.asInt(); /// 测试转换
                        break;
                    case Parameter::Type::Double:
                        typedValue.asDouble(); /// 测试转换
                        break;
                    case Parameter::Type::Bool:
                        typedValue.asBool(); /// 测试转换
                        break;
                    case Parameter::Type::String:
                    default:
                        break; /// 字符串无需特殊验证
                }
                impl_->parameterList_[key] = typedValue;
            }
            catch (const std::exception &e)
            {
                errorMsg = "参数 '" + key + "' 类型错误: " + e.what() + " (期望类型: " + paramIt->getTypeName() + ")";
                return false;
            }
        }
        else
        {
            /// 未定义类型的参数，按字符串处理
            impl_->parameterList_[key] = ParameterValue(strValue);
        }
    }

    /// 3. 公共验证（文件存在、路径等）
    if (!validateCommon(inputParams, errorMsg))
    {
        return false;
    }

    /// 4. 构建任务命令（如果有构建器）
    std::string command;
    if (impl_->builder_)
    {
        ///  (1). 特定任务验证
        if (!impl_->builder_->validate(inputParams, errorMsg))
        {
            return false;
        }

        /// (2). 设置任务标题
        std::string title = impl_->builder_->getTitle(inputParams);
        setTitle(title);

        /// (3). 构建命令
        command = impl_->builder_->build(inputParams);
        std::cout << "执行命令: " << command << std::endl;
    }

    /// 4. 执行一些任务
    if (!execute(command, inputParams, errorMsg))
    {
        return false;
    }

    /// 5. 执行任务
    try
    {
        impl_->func_(impl_->parameterList_);
        return true;
    }
    catch (const std::exception &e)
    {
        errorMsg = "执行错误: " + std::string(e.what());
        return false;
    }
}

auto XTask::execute(const std::string &command, const std::map<std::string, std::string> &inputParams,
                    std::string &errorMsg) -> bool
{
    return true;
}

auto XTask::validateCommon(const std::map<std::string, std::string> &inputParams, std::string &errorMsg) -> bool

{
    /// 目标是true，表示验证通过
    return true;
}

auto XTask::getName() const -> const std::string &
{
    return impl_->name_;
}

auto XTask::getDescription() const -> const std::string &
{
    return impl_->description_;
}

auto XTask::getParameters() const -> const Parameter::Container &
{
    return impl_->parameters_;
}

auto XTask::hasParameter(const Parameter &parameter) const -> bool
{
    /// 比较参数名称，因为参数名称应该是唯一的
    return std::ranges::find(impl_->parameters_, parameter) != impl_->parameters_.end();
}

auto XTask::hasParameter(const std::string_view &parameter) const -> bool
{
    return std::ranges::find(impl_->parameters_, parameter) != impl_->parameters_.end();
}

auto XTask::getRequiredParam(const std::map<std::string, std::string> &params, const std::string &key,
                             std::string &errorMsg) const -> std::string
{
    auto it = params.find(key);
    if (it == params.end())
    {
        errorMsg = "缺少必要参数: " + key;
        return "";
    }
    return it->second;
}

auto XTask::getParameter(const std::string &key, std::string &errorMsg) const -> ParameterValue
{
    auto it = impl_->parameterList_.find(key);
    if (it == impl_->parameterList_.end())
    {
        errorMsg = "缺少必要参数: " + key;
        return "";
    }
    return it->second;
}

auto XTask::setTaskProgressBar(const TaskProgressBar::Ptr &bar) -> void
{
    impl_->progressBar_ = bar;
}

auto XTask::getTaskProgressBar() const -> TaskProgressBar::Ptr
{
    return impl_->progressBar_;
}

auto XTask::setProgressCallback(ProgressCallback callback) const -> void
{
    impl_->progressCallback_ = std::move(callback);
}

auto XTask::getProgressCallback() const -> ProgressCallback
{
    return impl_->progressCallback_;
}

auto XTask::setBuilder(const ICommandBuilder::Ptr &builder) -> XTask &
{
    impl_->builder_ = builder;
    return *this;
}

auto XTask::builder() const -> ICommandBuilder::Ptr
{
    return impl_->builder_;
}

auto XTask::setTitle(const std::string_view &name) -> void
{
    if (impl_->progressBar_)
    {
        impl_->progressBar_->setTitle(name);
    }
}

auto XTask::setName(const std::string_view &name) const -> void
{
    impl_->name_ = name;
}

auto XTask::setDescription(const std::string_view &desc) const -> void
{
    impl_->description_ = desc;
}

auto XTask::updateProgress(XExec &exec, const std::string_view &taskName,
                           const std::map<std::string, std::string> &inputParams) -> void
{
    if (impl_->progressBar_)
    {
        impl_->progressBar_->updateProgress(exec, taskName, inputParams);
    }
}


IMPLEMENT_CREATE_DEFAULT(XTask)
template auto XTask::create(const std::string_view &, const TaskFunc &, const std::string_view &) -> XTask::Ptr;
