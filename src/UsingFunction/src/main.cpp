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
        func_  = &MyClass::Test;
        bfunc_ = [this](auto &&PH1) { return Test(std::forward<decltype(PH1)>(PH1)); };
        bfunc_ = std::bind(&MyClass::Test, this, std::placeholders::_1);
    }
    void Call()
    {
        // (this->*func_)("para Call in Class auto call");
        func_(*this, "para Call in Class auto call use std::function");
        bfunc_("para Call in Class auto call use std::bind");
        /// 这样调用的话 不如直接使用函数指针 明确
    }

    void SetFunc(const std::function<int(std::string)> &f)
    {
        bfunc_ = f;
    }

    int Test(std::string str)
    {
        std::cout << "MyClass::Test(" << str << ")" << std::endl;
        return 0;
    }

private:
    // Func func_{ nullptr };
    std::function<int(MyClass &, std::string)> func_;
    std::function<int(std::string)>            bfunc_;
};

/// 声明成员函数指针类型
using CFun = int (MyClass::*)(std::string);


int TestBind(int x, int y, std::string str, int count)
{
    std::cout << x << ":" << y << " " << str << " " << count << std::endl;
    return 0;
}

int main(int argc, char *argv[])
{
    auto bfun = std::bind(TestBind, 100, 200, std::placeholders::_1, std::placeholders::_2);

    bfun("test bind 2", 999);

    auto bfun2 = std::bind(TestBind, 100, 200, std::placeholders::_1, 888);
    bfun2("test bind 2");
    bfun2("test bind 2", 3333); /// 多出来的参数无用 因为占位符就提供了一个

    auto bfun3 = std::bind(TestBind, 100, 200, std::placeholders::_1, std::placeholders::_2);
    // bfun3("test bind 2");       /// 编译报错
    bfun3("test bind 2", 3333); /// 多出来的参数无用 因为占位符就提供了一个

    auto bfun4 = std::bind(TestBind, 100, 200, std::placeholders::_2, std::placeholders::_1);
    bfun4(777, "test bind 3");

    /// 成员函数转换为普通函数
    MyClass data;
    auto    cbfun = std::bind(&MyClass::Test, &data, std::placeholders::_1);
    cbfun("bind MyClass::Test");
    data.Call();

    /// 普通函数转换为成员函数
    auto bfun6 = std::bind(TestBind, 100, 200, std::placeholders::_1, 666);
    data.SetFunc(bfun6); /// 随让这样回调 感觉还可以 但是非常隐士 难怪后面都使用lamba;
    data.Call();

    data.SetFunc(cbfun);
    data.Call();

    //////////////////////////////////////////////////////////////////

    std::cout << "==========================================" << std::endl;

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

    getchar();
    return 0;
}
