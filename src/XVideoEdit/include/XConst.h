#pragma once

#include <string>
#include <memory>

#define DECLARE_CREATE(ClassName)           \
public:                                     \
    using Ptr = std::shared_ptr<ClassName>; \
    static auto create() -> ClassName::Ptr;

#define DECLARE_CREATE_DEFAULT(ClassName)   \
public:                                     \
    using Ptr = std::shared_ptr<ClassName>; \
    static auto create() -> ClassName::Ptr; \
    template <typename... Args>             \
    static auto create(Args &&...args) -> ClassName::Ptr;


#define IMPLEMENT_CREATE(ClassName)            \
    auto ClassName::create() -> ClassName::Ptr \
    {                                          \
        return std::make_shared<ClassName>();  \
    }

#define IMPLEMENT_CREATE_DEFAULT(ClassName)                              \
    auto ClassName::create() -> ClassName::Ptr                           \
    {                                                                    \
        return std::make_shared<ClassName>();                            \
    }                                                                    \
    template <typename... Args>                                          \
    auto ClassName::create(Args &&...args) -> ClassName::Ptr             \
    {                                                                    \
        return std::make_shared<ClassName>(std::forward<Args>(args)...); \
    }
