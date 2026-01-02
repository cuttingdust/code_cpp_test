#include <iostream>
#include <string>
#include <functional>


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
        // (this->*func_)("para Call in Class auto call");
        func_(*this, "para Call in Class auto call use std::function");
        /// 这样调用的话 不如直接使用函数指针 明确
    }
    int Test(std::string str)
    {
        std::cout << "MyClass::Test(" << str << ")" << std::endl;
        return 0;
    }

private:
    // Func func_{ nullptr };
    std::function<int(MyClass &, std::string)> func_;
};

/// 声明成员函数指针类型
using CFun = int (MyClass::*)(std::string);

int main(int argc, char *argv[])
{
    int (*fun_ptr)(std::string); ///声明函数指针
    fun_ptr = TestFuncPtr;
    fun_ptr("no using/typedef");

    FuncType ft = TestFuncPtr;
    ft("use using/typedef");

    std::function<int(std::string)> fun_template = TestFuncPtr;
    fun_template("using std::function");

    //////////////////////////////////////////////////////////////////
    std::cout << "==========================================" << std::endl;


    /// 成员函数取地址要加&
    auto    cfun = &MyClass::Test;
    MyClass my_class;
    (my_class.*cfun)("para auto cfun");

    CFun cfun2 = &MyClass::Test;
    (my_class.*cfun2)("para auto cfun with using");

    std::function<int(MyClass &, std::string)> cfunction = &MyClass::Test;
    cfunction(my_class, "para auto cfun with std::function");

    my_class.Call();

    return 0;
}
