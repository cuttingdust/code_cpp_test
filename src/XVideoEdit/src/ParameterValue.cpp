#include "ParameterValue.h"

#include <algorithm>
#include <stdexcept>

class ParameterValue::PImpl
{
public:
    PImpl(ParameterValue *owenr);
    PImpl(ParameterValue *owenr, const std::string_view &v);
    PImpl(ParameterValue *owenr, const char *v);

    ~PImpl() = default;

public:
    ParameterValue *owenr_ = nullptr;
    std::string     value_;
};

ParameterValue::PImpl::PImpl(ParameterValue *owenr) : owenr_(owenr)
{
}

ParameterValue::PImpl::PImpl(ParameterValue *owenr, const std::string_view &v) : owenr_(owenr), value_(v)
{
}

ParameterValue::PImpl::PImpl(ParameterValue *owenr, const char *v) : owenr_(owenr), value_(v)
{
}

ParameterValue::ParameterValue() : impl_(std::make_unique<PImpl>(this))
{
}

ParameterValue::ParameterValue(const std::string_view &value) : impl_(std::make_unique<PImpl>(this, value))
{
}

ParameterValue::ParameterValue(const char *value) : impl_(std::make_unique<PImpl>(this, value))
{
}

ParameterValue::~ParameterValue()                                     = default;
ParameterValue::ParameterValue(ParameterValue &&) noexcept            = default;
ParameterValue &ParameterValue::operator=(ParameterValue &&) noexcept = default;


/// 拷贝构造函数实现
ParameterValue::ParameterValue(const ParameterValue &other) :
    impl_(other.impl_ ? std::make_unique<PImpl>(*other.impl_) : nullptr)
{
    /// 更新owner_指针指向当前对象
    if (impl_)
    {
        impl_->owenr_ = this;
    }
}

/// 拷贝赋值运算符
auto ParameterValue::operator=(const ParameterValue &other) -> ParameterValue &
{
    if (this != &other)
    {
        if (other.impl_)
        {
            impl_         = std::make_unique<PImpl>(*other.impl_);
            impl_->owenr_ = this; /// 更新owner_指针
        }
        else
        {
            impl_.reset();
        }
    }
    return *this;
}

auto ParameterValue::asString() const -> const std::string &
{
    return impl_->value_;
}

auto ParameterValue::asInt() const -> int
{
    try
    {
        return std::stoi(impl_->value_);
    }
    catch (...)
    {
        throw std::runtime_error("无法将 '" + impl_->value_ + "' 转换为整数");
    }
}

auto ParameterValue::asDouble() const -> double
{
    try
    {
        return std::stod(impl_->value_);
    }
    catch (...)
    {
        throw std::runtime_error("无法将 '" + impl_->value_ + "' 转换为浮点数");
    }
}

auto ParameterValue::asBool() const -> bool
{
    if (impl_->value_.empty())
        return false;
    std::string lower = impl_->value_;
    std::ranges::transform(lower, lower.begin(), ::tolower);
    return lower == "true" || lower == "1" || lower == "yes" || lower == "on" || lower == "enabled";
}

auto ParameterValue::asPath() const -> fs::path
{
    if (impl_->value_.empty())
    {
        throw std::runtime_error("文件路径为空");
    }

    fs::path filePath(impl_->value_);

    /// 验证路径是否存在且是文件
    if (!fs::exists(filePath))
    {
        throw std::runtime_error("文件不存在: " + impl_->value_);
    }

    if (!fs::is_regular_file(filePath))
    {
        throw std::runtime_error("路径不是普通文件: " + impl_->value_);
    }

    /// 检查文件是否可访问（可选）
    try
    {
        /// 尝试获取文件状态以验证可访问性
        fs::status(filePath);
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error("无法访问文件: " + std::string(e.what()));
    }

    return filePath;
}

ParameterValue::operator std::string() const
{
    return impl_->value_;
}

auto ParameterValue::empty() const -> bool
{
    return impl_->value_.empty();
}

auto ParameterValue::raw() const -> const std::string &
{
    return impl_->value_;
}
