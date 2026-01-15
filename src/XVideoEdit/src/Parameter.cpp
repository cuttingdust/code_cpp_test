#include "Parameter.h"

class Parameter::PImpl
{
public:
    PImpl(Parameter *owenr);
    PImpl(Parameter *owenr, const std::string_view &name, Type type, const std::string_view &desc, bool required);
    ~PImpl() = default;

public:
    Parameter  *owenr_ = nullptr;
    std::string name_;
    Type        type_;
    std::string description_;
    bool        required_;
};

Parameter::PImpl::PImpl(Parameter *owenr) : owenr_(owenr)
{
}

Parameter::PImpl::PImpl(Parameter *owenr, const std::string_view &name, Type type, const std::string_view &desc,
                        bool required) : owenr_(owenr), name_(name), type_(type), description_(desc)
{
}


Parameter::Parameter(const std::string_view &name, Type type, const std::string_view &desc, bool required) :
    impl_(std::make_unique<PImpl>(this, name, type, desc, required))
{
}

Parameter::~Parameter()                                = default;
Parameter::Parameter(Parameter &&) noexcept            = default;
Parameter &Parameter::operator=(Parameter &&) noexcept = default;

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
        default:
            return "未知";
    }
}
