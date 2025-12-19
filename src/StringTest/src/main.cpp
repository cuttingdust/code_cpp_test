#include <iostream>
#include <vector>
#include <string>
#include <string>
class Data
{
public:
    Data()
    {
        std::cout << "Create Data" << std::endl;
    }
    Data(const Data& d)
    {
        std::cout << "Copy Data" << std::endl;
    }
    ~Data()
    {
        std::cout << "Drop Data" << std::endl;
    }
};

void TestData(const std::vector<Data>& d)
{
    std::cout << "In TestData " << d.size() << std::endl;
}

//////////////////////////////////////////////////////////////////

/// 自定义字符串类String
class String
{
public:
    void Clear()
    {
        delete str_;
        str_  = nullptr;
        size_ = 0;
    }
    ~String()
    {
        std::cout << "Drop String:" << size_ << std::endl;
        Clear();
    }
    String& operator=(const char* str)
    {
        if (str == nullptr)
        {
            return *this;
        }
        if (str == str_)
        {
            return *this;
        }
        std::cout << "operator=(const char* str) String:" << str << std::endl;
        size_ = strlen(str);
        if (size_ == 0)
        {
            return *this;
        }
        str_ = new char[size_ + 1]; /// +1存储\0  /// string 底层也是new出来的内存
        memcpy(str_, str, size_ + 1);
        return *this;
    }
    String(const char* str = "")
    {
        if (str == nullptr)
        {
            return;
        }
        std::cout << "Create String:" << str << std::endl;
        size_ = strlen(str);
        if (size_ == 0)
        {
            return;
        }
        str_ = new char[size_ + 1]; /// +1存储\0
        memcpy(str_, str, size_ + 1);
    }
    /// 拷贝构造函数
    String(const String& s)
    {
        std::cout << "Copy String:" << s.str_ << std::endl;
        size_ = s.size_;
        str_  = new char[size_ + 1];
        memcpy(str_, s.str_, size_ + 1);
    }
    /// 赋值操作符
    String& operator=(const String& s)
    {
        /// 对象原来空间处理
        if (this == &s)
        {
            return *this;
        }
        Clear();
        std::cout << "operator=(const String& s):" << s.str_ << std::endl;
        size_ = s.size_;
        str_  = new char[size_ + 1];
        memcpy(str_, s.str_, size_ + 1);
        return *this;
    }

    /// 移动构造函数
    String(String&& str) noexcept
    {
        std::cout << "Move String" << std::endl;
        str_      = str.str_;
        size_     = str.size_;
        str.str_  = nullptr;
        str.size_ = 0;
    }

    /// 移动赋值操作符
    String& operator=(String&& s) noexcept
    {
        /// 对象原来空间处理
        if (this == &s)
        {
            return *this;
        }
        Clear();

        std::cout << "operator=(String&& s) Move String" << std::endl;
        str_    = s.str_;
        size_   = s.size_;
        s.str_  = nullptr;
        s.size_ = 0;

        return *this;
    }
    const char* c_str()
    {
        if (!str_)
            return "";
        return str_;
    }

private:
    char* str_{ nullptr };
    int   size_{ 0 };
};


int main(int argc, char* argv[])
{
    {
        String str1("string str1");
        String str2("string str2");
        std::cout << "str1=" << str1.c_str() << std::endl;
        std::cout << "str2=" << str2.c_str() << std::endl;
        str2 = str1;
        std::cout << "str1=" << str1.c_str() << std::endl;
        std::cout << "str2=" << str2.c_str() << std::endl;
        str2 = str2;
        str1 = "opeator = string 001";

        str2 = std::move(str1);
        std::cout << "str1=" << str1.c_str() << std::endl;
        std::cout << "str2=" << str2.c_str() << std::endl;
        String str3(std::move(str2));
    }

    {
        String str1;
        // str1 = "test str1";
        String str2{ "test string 002" };
        // str1 = str2;
        Data d1;
        Data d2; /// d1的内存复制给d2，不调用copy构造
        Data d3(d2 = d1);
        Data d4{ d2.operator=(d1) };
        Data d5 = d2; /// 调用拷贝构造
        d1      = d3; /// 调用操作符=函数
    }

    {
        String str1("test string 001");
        String str2 = str1;
        std::cout << "str1 = " << str1.c_str() << std::endl;
        std::cout << "str2 = " << str2.c_str() << std::endl;
        String str3{ std::move(str2) }; //str2的堆内存移到str3
        std::cout << "str1 = " << str1.c_str() << std::endl;
        std::cout << "str2 = " << str2.c_str() << std::endl;
        std::cout << "str3 = " << str3.c_str() << std::endl;
        //String str4;
        //str4 = move(str3);
    }

    {
        std::vector<Data> datas(3); /// 创建三个Data对象
        TestData(std::move(datas)); /// move转为右值引用 (万能引用)
        std::cout << "after move " << datas.size() << std::endl;
    }
    getchar();
    return 0;
}
