#include <iostream>
#include <cstdlib> // for rand()
#include <ctime>   // for time()

template <typename T>
class vector
{
public:
    explicit vector(int size = 10) // 加 explicit
    {
        _first = new T[size];
        _last  = _first;
        _end   = _first + size;
    }

    ~vector()
    {
        delete[] _first;
        _first = _last = _end = nullptr;
    }

    vector(const vector<T>& other)
    {
        size_t size = other._end - other._first;
        _first      = new T[size];
        _last       = _first + (other._last - other._first);
        _end        = _first + size;
        for (T *src = other._first, *dst = _first; src != other._last; ++src, ++dst)
        {
            *dst = *src;
        }
    }

    vector<T>& operator=(const vector<T>& other)
    {
        if (this == &other)
            return *this;
        delete[] _first;
        size_t size = other._end - other._first;
        _first      = new T[size];
        _last       = _first + (other._last - other._first);
        _end        = _first + size;
        for (T *src = other._first, *dst = _first; src != other._last; ++src, ++dst)
        {
            *dst = *src;
        }
        return *this;
    }

    void push_back(const T& value)
    {
        if (full())
        {
            expand();
        }
        *_last = value;
        ++_last;
    }

    void pop_back()
    {
        if (empty())
            return;
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

    int size() const
    {
        return static_cast<int>(_last - _first); // 关键修复1
    }

private:
    T* _first = nullptr;
    T* _last  = nullptr;
    T* _end   = nullptr;

    void expand()
    {
        /// 关键修复2：先保存旧大小
        size_t old_size     = _last - _first;
        size_t old_capacity = _end - _first;
        size_t new_capacity = old_capacity * 2;

        T* new_first = new T[new_capacity];

        /// 复制元素
        for (size_t i = 0; i < old_size; ++i)
        {
            new_first[i] = _first[i];
        }

        delete[] _first;

        /// 使用保存的 old_size
        _first = new_first;
        _last  = new_first + old_size;
        _end   = new_first + new_capacity;
    }
};

int main()
{
    srand(time(nullptr)); // 初始化随机数种子

    vector<int> vec;
    for (int i = 0; i < 20; ++i)
    {
        vec.push_back(rand() % 100);
    }

    std::cout << "Before pop, size: " << vec.size() << std::endl;
    vec.pop_back();
    std::cout << "After pop, size: " << vec.size() << std::endl;

    while (!vec.empty())
    {
        std::cout << "Size: " << vec.size() << " Value: " << vec.back() << std::endl;
        vec.pop_back();
    }

    std::cout << "Done!" << std::endl;
    return 0;
}
