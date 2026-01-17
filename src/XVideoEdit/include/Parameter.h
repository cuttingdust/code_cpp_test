#pragma once

#include "XConst.h"

#include <functional>
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
        Bool,
        File,     /// 新增：文件路径类型
        Directory /// 新增：目录路径类型
    };
    using Container      = std::vector<Parameter>;
    using Optional       = std::optional<Parameter>;
    using CompletionFunc = std::function<std::vector<std::string>(std::string_view partial)>;

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

    /// 设置补全函数
    Parameter& setCompletions(CompletionFunc completor);

    /// 获取补全建议
    std::vector<std::string> getCompletions(std::string_view partial) const;

    /// 根据参数类型自动生成补全建议
    static std::vector<std::string> getDefaultCompletions(Type type, const std::string& name, std::string_view partial);

private:
    class PImpl;
    std::unique_ptr<PImpl> impl_;
};
