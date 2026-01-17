#include "Parameter.h"

#include <algorithm>
#include <string>

class Parameter::PImpl
{
public:
    PImpl(Parameter *owner);
    PImpl(Parameter *owner, const std::string_view &name, Type type, const std::string_view &desc, bool required);
    ~PImpl() = default;

public:
    Parameter     *owner_ = nullptr;
    std::string    name_;
    Type           type_;
    std::string    description_;
    bool           required_;
    CompletionFunc completor_; // 补全函数
};

Parameter::PImpl::PImpl(Parameter *owner) : owner_(owner)
{
}

Parameter::PImpl::PImpl(Parameter *owner, const std::string_view &name, Type type, const std::string_view &desc,
                        bool required) :
    owner_(owner), name_(name), type_(type), description_(desc), required_(required)
{
}

Parameter::Parameter(const std::string_view &name, Type type, const std::string_view &desc, bool required) :
    impl_(std::make_unique<PImpl>(this, name, type, desc, required))
{
}

Parameter::~Parameter() = default;

Parameter::Parameter(Parameter &&) noexcept                     = default;
auto Parameter::operator=(Parameter &&) noexcept -> Parameter & = default;

/// 拷贝构造函数实现
Parameter::Parameter(const Parameter &other) : impl_(other.impl_ ? std::make_unique<PImpl>(*other.impl_) : nullptr)
{
    /// 更新owner_指针指向当前对象
    if (impl_)
    {
        impl_->owner_ = this;
        // 需要单独拷贝补全函数，因为它是可调用对象
        if (other.impl_)
        {
            impl_->completor_ = other.impl_->completor_;
        }
    }
}

/// 拷贝赋值运算符
auto Parameter::operator=(const Parameter &other) -> Parameter &
{
    if (this != &other)
    {
        if (other.impl_)
        {
            impl_         = std::make_unique<PImpl>(*other.impl_);
            impl_->owner_ = this; /// 更新owner_指针
            // 拷贝补全函数
            impl_->completor_ = other.impl_->completor_;
        }
        else
        {
            impl_.reset();
        }
    }
    return *this;
}

auto Parameter::operator==(const Parameter &other) const -> bool
{
    return getName() == other.getName();
}

auto Parameter::operator==(const std::string_view &name) const -> bool
{
    return getName() == name;
}

auto Parameter::getName() const -> const std::string &
{
    return impl_->name_;
}

auto Parameter::getType() const -> Type
{
    return impl_->type_;
}

auto Parameter::getDescription() const -> const std::string &
{
    return impl_->description_;
}

auto Parameter::isRequired() const -> bool
{
    return impl_->required_;
}

auto Parameter::getTypeName() const -> std::string
{
    switch (impl_->type_)
    {
        case Type::String:
            return "字符串";
        case Type::Int:
            return "整数";
        case Type::Double:
            return "浮点数";
        case Type::Bool:
            return "布尔值";
        case Type::File:
            return "文件路径";
        case Type::Directory:
            return "目录路径";
        default:
            return "未知";
    }
}

Parameter &Parameter::setCompletions(CompletionFunc completor)
{
    impl_->completor_ = std::move(completor);
    return *this;
}

std::vector<std::string> Parameter::getCompletions(std::string_view partial) const
{
    if (impl_->completor_)
    {
        // 使用自定义补全函数
        return impl_->completor_(partial);
    }
    // 使用默认补全逻辑
    return getDefaultCompletions(impl_->type_, impl_->name_, partial);
}

std::vector<std::string> Parameter::getDefaultCompletions(Type type, const std::string &name, std::string_view partial)
{
    std::vector<std::string> completions;

    auto addIfMatches = [&](const std::string &value)
    {
        if (value.starts_with(partial))
        {
            completions.push_back(value);
        }
    };

    switch (type)
    {
        case Type::File:
            {
                // 文件类型：提供一些常见的文件扩展名
                if (partial.empty() || partial.find('.') != std::string::npos)
                {
                    static const std::vector<std::string> commonFiles = { ".txt", ".mp4", ".avi", ".mov",  ".jpg",
                                                                          ".png", ".bmp", ".gif", ".json", ".xml",
                                                                          ".csv", ".pdf", ".zip", ".rar",  ".exe",
                                                                          ".dll", ".so" };
                    for (const auto &file : commonFiles)
                    {
                        addIfMatches(file);
                    }
                }
                // 如果输入看起来像路径，让路径补全系统处理
                break;
            }

        case Type::Directory:
            {
                // 目录类型：提供一些常见的目录名
                if (partial.empty())
                {
                    static const std::vector<std::string> commonDirs = { "./", "../", "~/", "C:/", "D:/", "E:/" };
                    for (const auto &dir : commonDirs)
                    {
                        completions.push_back(dir);
                    }
                }
                // 如果输入看起来像路径，让路径补全系统处理
                break;
            }

        case Type::Bool:
            {
                static const std::vector<std::string> boolValues = {
                    "true", "false", "1", "0", "yes", "no", "on", "off"
                };
                for (const auto &value : boolValues)
                {
                    addIfMatches(value);
                }
            }
            break;

        case Type::String:
            {
                if (name == "--format" || name == "-f")
                {
                    static const std::vector<std::string> formats = { "mp4", "avi", "mov",  "mkv", "webm", "jpg", "png",
                                                                      "bmp", "gif", "json", "xml", "txt",  "csv" };
                    for (const auto &format : formats)
                    {
                        addIfMatches(format);
                    }
                }
                else if (name == "--mode" || name == "-mode")
                {
                    static const std::vector<std::string> modes = { "fast", "normal", "slow",    "high", "medium",
                                                                    "low",  "debug",  "release", "test", "production" };
                    for (const auto &mode : modes)
                    {
                        addIfMatches(mode);
                    }
                }
            }
            break;

        case Type::Int:
            {
                if (name == "-port" || name == "--port")
                {
                    static const std::vector<std::string> commonPorts = { "80",   "443",  "8080", "3000", "5000",
                                                                          "3306", "5432", "6379", "27017" };
                    for (const auto &port : commonPorts)
                    {
                        addIfMatches(port);
                    }
                }
                else if (name == "-n" || name == "--count" || name == "--iterations" || name == "-iterations")
                {
                    static const std::vector<std::string> commonCounts = {
                        "1", "2", "3", "5", "10", "50", "100", "1000"
                    };
                    for (const auto &count : commonCounts)
                    {
                        addIfMatches(count);
                    }
                }
                else if (name == "--level" || name == "-level" || name == "--quality" || name == "-quality")
                {
                    for (int i = 1; i <= 10; ++i)
                    {
                        std::string level = std::to_string(i);
                        addIfMatches(level);
                    }
                }
                else if (partial.empty() || partial == "-" || partial == "+")
                {
                    static const std::vector<std::string> commonInts = { "0",  "1",   "2",    "3",  "5",
                                                                         "10", "100", "1000", "-1", "-10" };
                    for (const auto &value : commonInts)
                    {
                        completions.push_back(value);
                    }
                }
            }
            break;

        case Type::Double:
            {
                if (name == "-x" || name == "--ratio" || name == "--scale" || name == "-scale")
                {
                    static const std::vector<std::string> commonRatios = { "0.5", "0.75", "1.0", "1.5",
                                                                           "2.0", "2.5",  "3.0" };
                    for (const auto &ratio : commonRatios)
                    {
                        addIfMatches(ratio);
                    }
                }
                else if (name == "-timeout" || name == "--timeout" || name == "--delay" || name == "-delay")
                {
                    static const std::vector<std::string> commonTimeouts = { "0.1", "0.5",  "1.0",  "2.0",
                                                                             "5.0", "10.0", "30.0", "60.0" };
                    for (const auto &timeout : commonTimeouts)
                    {
                        addIfMatches(timeout);
                    }
                }
                else if (name == "--threshold" || name == "-threshold")
                {
                    static const std::vector<std::string> commonThresholds = { "0.1", "0.2", "0.3", "0.5",
                                                                               "0.7", "0.8", "0.9", "0.95" };
                    for (const auto &threshold : commonThresholds)
                    {
                        addIfMatches(threshold);
                    }
                }
                else if (partial.empty() || partial == "0" || partial == "1" || partial == "-" || partial == ".")
                {
                    static const std::vector<std::string> commonDoubles = { "0.0",  "0.5",  "1.0",   "2.0",
                                                                            "3.14", "10.0", "100.0", "-1.0",
                                                                            "-0.5", "0.25", "0.75" };
                    for (const auto &value : commonDoubles)
                    {
                        completions.push_back(value);
                    }
                }
            }
            break;

        default:
            // 对于其他类型，返回空列表
            break;
    }

    return completions;
}
