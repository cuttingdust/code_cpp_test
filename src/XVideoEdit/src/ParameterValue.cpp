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
