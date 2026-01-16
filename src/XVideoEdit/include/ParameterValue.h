#pragma once

#include "XConst.h"

/// 参数值包装类 - 运行时类型转换
class ParameterValue
{
public:
    ParameterValue();
    ParameterValue(const std::string_view& value);

    ParameterValue(const char* value);
    ~ParameterValue();

    /// 允许拷贝
    ParameterValue(const ParameterValue& other);
    auto operator=(const ParameterValue& other) -> ParameterValue&;

    /// 允许移动
    ParameterValue(ParameterValue&&) noexcept;
    ParameterValue& operator=(ParameterValue&&) noexcept;

public:
    /// 类型转换接口
    auto asString() const -> const std::string&;

    auto asInt() const -> int;

    auto asDouble() const -> double;

    auto asBool() const -> bool;

    /// 隐式转换到string
    operator std::string() const;

    /// 检查是否有值
    auto empty() const -> bool;

    /// 原始值访问
    auto raw() const -> const std::string&;

private:
    class PImpl;
    std::unique_ptr<PImpl> impl_;
};
