#ifndef ISINGLE_H
#define ISINGLE_H

#include <type_traits>

template <typename T>
class ISingleton
{
protected:
    ISingleton()                   = default;
    virtual ~ISingleton() noexcept = default;

public:
    static T *getInstance() noexcept(std::is_nothrow_constructible<T>::value)
    {
        static T instance;
        return &instance;
    }

    /// 变参模板版本 - 支持带参数的构造函数
    template <typename... Args>
    static T *getInstance(Args &&...args) noexcept(std::is_nothrow_constructible<T, Args...>::value)
    {
        static T instance(std::forward<Args>(args)...);
        return &instance;
    }

    ISingleton(const ISingleton &)            = delete;
    ISingleton &operator=(const ISingleton &) = delete;

    ISingleton(ISingleton &&)            = delete;
    ISingleton &operator=(ISingleton &&) = delete;
};

#endif // ISINGLE_H
