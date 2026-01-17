#include "XTask.h"

#include "ITask.h"

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
};

XTask::PImpl::PImpl(XTask *owenr, const std::string_view &name, TaskFunc func, const std::string_view &desc) :
    owenr_(owenr), name_(name), func_(std::move(func)), description_(desc)
{
}

XTask::XTask()
{
}

XTask::XTask(const std::string_view &name, TaskFunc func, const std::string_view &desc) :
    impl_(std::make_unique<XTask::PImpl>(this, name, func, desc))
{
}

XTask::~XTask()                            = default;
XTask::XTask(XTask &&) noexcept            = default;
XTask &XTask::operator=(XTask &&) noexcept = default;

XTask &XTask::addParameter(const std::string &paramName, Parameter::Type type, const std::string &desc, bool required,
                           Parameter::CompletionFunc completor)
{
    auto param = Parameter(paramName, type, desc, required);
    if (completor)
    {
        param.setCompletions(std::move(completor));
    }
    impl_->parameters_.push_back(std::move(param));
    return *this;
}

XTask &XTask::addStringParam(const std::string &paramName, const std::string &desc, bool required,
                             Parameter::CompletionFunc completor)
{
    return addParameter(paramName, Parameter::Type::String, desc, required, std::move(completor));
}

XTask &XTask::addIntParam(const std::string &paramName, const std::string &desc, bool required,
                          Parameter::CompletionFunc completor)
{
    return addParameter(paramName, Parameter::Type::Int, desc, required, std::move(completor));
}

XTask &XTask::addDoubleParam(const std::string &paramName, const std::string &desc, bool required,
                             Parameter::CompletionFunc completor)
{
    return addParameter(paramName, Parameter::Type::Double, desc, required, std::move(completor));
}

XTask &XTask::addBoolParam(const std::string &paramName, const std::string &desc, bool required,
                           Parameter::CompletionFunc completor)
{
    return addParameter(paramName, Parameter::Type::Bool, desc, required, std::move(completor));
}

auto XTask::addFileParam(const std::string &paramName, const std::string &desc, bool required,
        Parameter::CompletionFunc completor) -> XTask &
{
    return addParameter(paramName, Parameter::Type::File, desc, required, std::move(completor));
}

auto XTask::addDirectoryParam(const std::string &paramName, const std::string &desc, bool required,
        Parameter::CompletionFunc completor) -> XTask &
{
    return addParameter(paramName, Parameter::Type::Directory, desc, required, std::move(completor));
}

auto XTask::execute(const std::map<std::string, std::string> &inputParams, std::string &errorMsg) const -> bool
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
    impl_->parameterList_.clear(); // 清空之前的结果
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

    /// 3. 执行任务
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

auto XTask::getFFmpegPath() const -> std::string
{
#ifdef FFMPEG_PATH
    return FFMPEG_PATH;
#else
    return "ffmpeg";
#endif
}

auto XTask::setName(const std::string_view &name) const -> void
{
    impl_->name_ = name;
}

auto XTask::setDescription(const std::string_view &desc) const -> void
{
    impl_->description_ = desc;
}


IMPLEMENT_CREATE_DEFAULT(XTask)
template auto XTask::create(const std::string_view &, const TaskFunc &, const std::string_view &) -> XTask::Ptr;
