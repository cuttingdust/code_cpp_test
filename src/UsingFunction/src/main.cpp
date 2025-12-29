#include <iostream>
#include <string>


int TestFuncPtr(std::string str)
{
    std::cout << "call TestFuncPtr " << str << std::endl;
    return 0;
}

/// 函数指针声明
// typedef int (*FuncType)(std::string);
using FuncType = int (*)(std::string);

class MyClass
{
public:
    // typedef int (MyClass::*Func)(std::string);
    using Func = int (MyClass::*)(std::string);
    MyClass()
    {
        func_ = &MyClass::Test;
    }
    void Call()
    {
        (this->*func_)("para Call in Class auto call");
    }
    int Test(std::string str)
    {
        std::cout << "MyClass::Test(" << str << ")" << std::endl;
        return 0;
    }

private:
    Func func_{ nullptr };
};

/// 声明成员函数指针类型
using CFun = int (MyClass::*)(std::string);

int main(int argc, char *argv[])
{
    int (*fun_ptr)(std::string); ///声明函数指针
    fun_ptr = TestFuncPtr;
    fun_ptr("no using/typedef");

    FuncType ft = TestFuncPtr;
    ft("test");

    //////////////////////////////////////////////////////////////////
    std::cout << "==========================================" << std::endl;


    /// 成员函数取地址要加&
    auto    cfun = &MyClass::Test;
    MyClass my_class;
    (my_class.*cfun)("para auto cfun");

    CFun cfun2 = &MyClass::Test;
    (my_class.*cfun2)("para auto cfun with using");

    my_class.Call();

    return 0;
}
