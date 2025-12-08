#include <iostream>

class Object
{
public:
    Object() : m_ptr(new int(10))
    {
        std::cout << "Object created\n";
    }
    ~Object()
    {
        delete m_ptr;
        std::cout << "Object destroyed\n";
    }
    /// 方法1 深拷贝构造函数
    Object(const Object &other) : m_ptr(new int(*(other.m_ptr)))
    {
        std::cout << "Object copied\n";
    } /// 效率低但安全的深拷贝

    Object &operator=(const Object &other)
    {
        if (this != &other)
        {
            *m_ptr = *(other.m_ptr); /// 复制值，而非指针
        }
        return *this;
    }

    /// 方法2 移动构造函数
    Object(Object &&other) noexcept : m_ptr(other.m_ptr)
    {
        other.m_ptr = nullptr; /// *** 避免双重删除
        std::cout << "Object moved\n";
    } /// 移动构造函数

    Object &operator=(Object &&other) noexcept
    {
        /// std::move赋值运算符
        if (this != &other)
        {
            delete m_ptr;
            m_ptr       = other.m_ptr;
            other.m_ptr = nullptr;
        }
        return *this;
    }

private:
    int *m_ptr = nullptr;
};

Object get_object(bool flag)
{
    Object A;
    Object B;
    if (flag)
    {
        return A;
    }
    else
    {
        return B;
    }
}

int main(int argc, char *argv[])
{
    /// 多次删除 /// 默认浅拷贝
    Object myObj = get_object(true);

    std::cout << "hello world" << std::endl;

    return 0;
}
