#include "XTask.h"

#include <utility>


class XTask::PImpl
{
public:
    PImpl(XTask *owenr, const std::string_view &name, TaskFunc func, const std::string_view &desc);
    ~PImpl() = default;

public:
    XTask                 *owenr_ = nullptr;
    std::string            name_;
    TaskFunc               func_;
    std::string            description_;
    std::vector<Parameter> parameters_;
};

XTask::PImpl::PImpl(XTask *owenr, const std::string_view &name, TaskFunc func, const std::string_view &desc) :
    owenr_(owenr), name_(name), func_(std::move(func)), description_(desc)
{
}

XTask::XTask(const std::string_view &name, TaskFunc func, const std::string_view &desc) :
    impl_(std::make_unique<XTask::PImpl>(this, name, func, desc))
{
}

XTask::~XTask()                            = default;
XTask::XTask(XTask &&) noexcept            = default;
XTask &XTask::operator=(XTask &&) noexcept = default;

auto XTask::addParameter(const std::string &paramName, Parameter::Type type, const std::string &desc, bool required)
        -> XTask &
{
    impl_->parameters_.emplace_back(paramName, type, desc, required);
    return *this;
}

auto XTask::addStringParam(const std::string &paramName, const std::string &desc, bool required) -> XTask &
{
    return addParameter(paramName, Parameter::Type::String, desc, required);
}

auto XTask::addIntParam(const std::string &paramName, const std::string &desc, bool required) -> XTask &
{
    return addParameter(paramName, Parameter::Type::Int, desc, required);
}

auto XTask::addDoubleParam(const std::string &paramName, const std::string &desc, bool required) -> XTask &
{
    return addParameter(paramName, Parameter::Type::Double, desc, required);
}

auto XTask::addBoolParam(const std::string &paramName, const std::string &desc, bool required) -> XTask &
{
    return addParameter(paramName, Parameter::Type::Bool, desc, required);
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
    std::map<std::string, ParameterValue> typedParams;
    for (const auto &[key, strValue] : inputParams)
    {
        /// 查找参数定义
        auto paramIt =
                std::ranges::find_if(impl_->parameters_, [&key](const Parameter &p) { return p.getName() == key; });

        if (paramIt != impl_->parameters_.end())
        {
            /// 有定义的类型参数，创建ParameterValue
            typedParams[key] = ParameterValue(strValue);

            /// 类型验证（简单版，实际执行时转换）
            try
            {
                switch (paramIt->getType())
                {
                    case Parameter::Type::Int:
                        return typedParams[key].asInt(); /// 测试转换
                    case Parameter::Type::Double:
                        return typedParams[key].asDouble(); /// 测试转换
                    case Parameter::Type::Bool:
                        return typedParams[key].asBool(); /// 测试转换
                    case Parameter::Type::String:
                    default:
                        break; /// 字符串无需特殊验证
                }
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
            typedParams[key] = ParameterValue(strValue);
        }
    }

    /// 3. 执行任务
    try
    {
        impl_->func_(typedParams);
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

auto XTask::getParameters() const -> const std::vector<Parameter> &
{
    return impl_->parameters_;
}
