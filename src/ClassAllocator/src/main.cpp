#include <iostream>
#include <cstdlib> // for rand()
#include <ctime>   // for time()

///容器的空间配置器，负责内存的分配和对象的构造与析构

template <typename T>
struct Allocator
{
    T* allocate(size_t size)
    {
        return (T*)malloc(sizeof(T) * size);
    }
    void deallocate(void* p)
    {
        free(p);
    }
    void construct(T* p, const T& value) /// 负责对象构造
    {
        new (p) T(value); // placement new
    }
    void destroy(T* p) /// 负责对象析构
    {
        p->~T();
    }
};


/// 容器底层内存开辟， 内存释放， 对象构造和析构的细节都交给 Allocator 来处理

template <typename T, typename Alloc = Allocator<T>>
class vector
{
public:
    explicit vector(int size = 10, const Alloc& alloc = Alloc()) // 加 explicit
        : _allocator(alloc)
    {
        /// 需要把内存开辟和对象构造分开处理
        // _first = new T[size];
        _first = _allocator.allocate(size);
        _last  = _first;
        _end   = _first + size;
    }

    ~vector()
    {
        /// 析构容器有效的元素， 然后释放_first指针指向的堆内存
        for (T* p = _first; p != _last; ++p)
        {
            _allocator.destroy(p); /// 把_first指针指向的堆内存中有效的元素析构掉
        }
        _allocator.deallocate(_first); /// 释放堆上的数组内存
        _first = _last = _end = nullptr;
    }

    vector(const vector<T>& other)
    {
        size_t size = other._end - other._first;
        // _first      = new T[size];
        _first = _allocator.allocate(size);
        _last  = _first + (other._last - other._first);
        _end   = _first + size;
        for (T *src = other._first, *dst = _first; src != other._last; ++src, ++dst)
        {
            _allocator.construct(dst, *src);
        }
    }

    vector<T>& operator=(const vector<T>& other)
    {
        if (this == &other)
            return *this;
        // delete[] _first;
        for (T* p = _first; p != _last; ++p)
        {
            _allocator.destroy(p);
        }
        _allocator.deallocate(_first);
        size_t size = other._end - other._first;
        _first      = _allocator.allocate(size);
        _last       = _first + (other._last - other._first);
        _end        = _first + size;
        for (T *src = other._first, *dst = _first; src != other._last; ++src, ++dst)
        {
            // *dst = *src;
            _allocator.construct(dst, *src);
        }
        return *this;
    }

    void push_back(const T& value)
    {
        if (full())
        {
            expand();
        }
        // *_last = value;
        _allocator.construct(_last, value);
        ++_last;
    }

    void pop_back()
    {
        if (empty())
            return;
        // --_last;
        _allocator.destroy(_last - 1);
        --_last;
    }

    T& back()
    {
        return *(_last - 1);
    }

    const T& back() const
    {
        return *(_last - 1);
    }

    bool full() const
    {
        return _last == _end;
    }

    bool empty() const
    {
        return _first == _last;
    }

    size_t size() const
    {
        return _last - _first;
    }

private:
    T*    _first = nullptr;
    T*    _last  = nullptr;
    T*    _end   = nullptr;
    Alloc _allocator; /// 定义容器的空间配置器对象

    void expand()
    {
        /// 关键修复2：先保存旧大小
        size_t old_size     = _last - _first;
        size_t old_capacity = _end - _first;
        size_t new_capacity = old_capacity * 2;

        // T* new_first = new T[new_capacity];
        T* new_first = _allocator.allocate(new_capacity);

        /// 复制元素
        for (size_t i = 0; i < old_size; ++i)
        {
            // new_first[i] = _first[i];
            _allocator.construct(new_first + i, _first[i]);
        }

        for (size_t i = 0; i < old_size; ++i)
        {
            _allocator.destroy(_first + i);
        }
        _allocator.deallocate(_first);

        /// 使用保存的 old_size
        _first = new_first;
        _last  = new_first + old_size;
        _end   = new_first + new_capacity;
    }
};

class Test
{
public:
    Test()
    {
        std::cout << "Test constructor called." << std::endl;
    }
    virtual ~Test()
    {
        std::cout << "Test destructor called." << std::endl;
    }
    Test(const Test& other)
    {
        std::cout << "Test copy constructor called." << std::endl;
    }
};

int main()
{
    Test t1, t2, t3;

    std::cout << "==========================================" << std::endl;
    vector<Test> vec;
    vec.push_back(t1);
    vec.push_back(t2);
    vec.push_back(t3);
    std::cout << "==========================================" << std::endl;
    vec.pop_back();
    std::cout << "==========================================" << std::endl;
    std::cout << "Done!" << std::endl;
    return 0;
}
