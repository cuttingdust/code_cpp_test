#pragma once

#include "XConst.h"

#include <vector>
#include <optional>

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
    using Container = std::vector<Parameter>;
    using Optional  = std::optional<Parameter>;

    Parameter(const std::string_view& name, Type type = Type::String, const std::string_view& desc = "",
              bool required = false);
    ~Parameter();

    Parameter(const Parameter& other);
    auto operator=(const Parameter& other) -> Parameter&;

    /// 允许移动
    Parameter(Parameter&&) noexcept;
    auto operator=(Parameter&&) noexcept -> Parameter&;
    auto operator==(const Parameter& other) const -> bool;
    auto operator==(const std::string_view& name) const -> bool;

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
