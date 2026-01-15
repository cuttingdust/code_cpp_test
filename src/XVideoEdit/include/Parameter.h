#pragma once

#include "XConst.h"

/// 参数定义类
class Parameter
{
public:
    enum class Type
    {
        String,
        Int,
        Double,
        Bool
    };

    Parameter(const std::string_view& name, Type type = Type::String, const std::string_view& desc = "",
              bool required = false);
    ~Parameter();

    /// 允许移动
    Parameter(Parameter&&) noexcept;
    Parameter& operator=(Parameter&&) noexcept;

public:
    auto getName() const -> const std::string&;

    auto getType() const -> Type;

    auto getDescription() const -> const std::string&;

    auto isRequired() const -> bool;

    /// 获取类型名称
    auto getTypeName() const -> std::string;

private:
    class PImpl;
    std::unique_ptr<PImpl> impl_;
};
