#include <iostream>

class Data
{
public:
    Data()
    {
        std::cout << "create data\n";
    }
    ~Data()
    {
        std::cout << "drop data\n";
    }
    void Print()
    {
        std::cout << "print data\n";
    }
};

class DataPtr
{
public:
    DataPtr() = default; /// 恢复默认构造
    DataPtr(Data* d)
    {
        d_ = d;
    }
    DataPtr(const DataPtr&) = delete;
    DataPtr(DataPtr&& dp) noexcept
    {
        d_    = dp.d_;
        dp.d_ = nullptr;
    }
    DataPtr& operator=(DataPtr&& dp) noexcept
    {
        if (this == &dp)
        {
            return *this;
        }
        delete d_;
        d_    = dp.d_;
        dp.d_ = nullptr;
        return *this;
    }
    DataPtr& operator=(const DataPtr&) = delete;
    ~DataPtr()
    {
        delete d_;
        d_ = nullptr;
    }
    Data* Get() const
    {
        return d_;
    }
    Data* Release()
    {
        auto d = d_;
        d_     = nullptr;
        return d;
    }
    void Reset(Data* d)
    {
        if (d == d_)
        {
            return;
        }
        delete d_;
        d_ = d;
    }

private:
    Data* d_{ nullptr };
};

int main(int argc, char* argv[])
{
    {
        DataPtr ptr1(new Data);
        /// 限制使用 拷贝构造
        /// DataPtr(const DataPtr&) = delete;
        // DataPtr ptr2 = ptr1; /// 拷贝构造
        /// 限制赋值
        /// DataPtr& operator= (const DataPtr &) = delete;
        // DataPtr ptr3 = ptr1;//赋值
        ptr1.Get()->Print();
        ptr1.Reset(new Data);
        auto p = ptr1.Release();
        p->Print();
        delete p;
        DataPtr ptr4 = std::move(ptr1);
        ptr4.Get()->Print();
        DataPtr ptr5;
        ptr5 = std::move(ptr4);
    }

    {
        /// 一 创建智能指针
        std::unique_ptr<Data> ptr1(new Data);
        /// c++14 支持，尽量用make_unique
        /// 1 编译器产生更小更快的代码
        /// 2 减少重复类型设置
        /// 3 内存安全
        auto ptr2 = std::make_unique<Data>();

        /// 二 智能指针使用
        ptr2->Print();
        (*ptr2).Print();
        auto ptr2_ptr = ptr2.get();
        ptr2_ptr->Print();

        /// 三 智能指针修改
        auto ptr3 = std::make_unique<Data>();
        ptr3.reset(new Data); /// 释放原有空间
        ptr3.reset(nullptr);  /// 释放空间
        ptr3 = nullptr;       /// 释放空间
        {
            auto ptr4 = std::make_unique<Data>();
            /// 释放控制权
            auto ptr4_ptr = ptr4.release();
            delete ptr4_ptr;
        }
        /// 智能指针移动
        auto                  ptr5 = std::make_unique<Data>();
        std::unique_ptr<Data> ptr6 = std::move(ptr5);
        std::unique_ptr<Data> ptr7;
        ptr7 = std::move(ptr6);

        std::cout << "-----------------------" << std::endl;
    }

    getchar();
    return 0;
}
