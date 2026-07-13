#include <algorithm>
#include <cassert>
#include <compare>
#include <concepts>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <cstdlib> // for rand()
#include <stdexcept>
#include <ctime>   // for time()

///容器的空间配置器，负责内存的分配和对象的构造与析构

template <typename T>
struct Allocator
{
    T *allocate(size_t size)
    {
        return (T *)malloc(sizeof(T) * size);
    }
    void deallocate(void *p)
    {
        free(p);
    }
    void construct(T *p, const T &value) /// 负责对象构造
    {
        new (p) T(value); // placement new
    }
    void destroy(T *p) /// 负责对象析构
    {
        p->~T();
    }
};


/// 容器底层内存开辟， 内存释放， 对象构造和析构的细节都交给 Allocator 来处理

template <typename T, typename Alloc = Allocator<T>>
class vector
{
public:
    class const_iterator;

    class iterator
    {
    public:
        using iterator_concept  = std::contiguous_iterator_tag;
        using iterator_category = std::random_access_iterator_tag;
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = T *;
        using reference         = T &;

        iterator() = default;

        reference operator*() const
        {
            check_valid();
            return *_ptr;
        }
        pointer operator->() const
        {
            check_valid();
            return _ptr;
        }
        reference operator[](difference_type offset) const
        {
            check_valid();
            return _ptr[offset];
        }

        iterator &operator++()
        {
            check_valid();
            ++_ptr;
            return *this;
        }

        iterator operator++(int)
        {
            iterator old = *this;
            ++*this;
            return old;
        }

        iterator &operator--()
        {
            check_valid();
            --_ptr;
            return *this;
        }

        iterator operator--(int)
        {
            iterator old = *this;
            --*this;
            return old;
        }

        iterator &operator+=(difference_type offset)
        {
            check_valid();
            _ptr += offset;
            return *this;
        }

        iterator &operator-=(difference_type offset)
        {
            return *this += -offset;
        }

        friend iterator operator+(iterator it, difference_type offset)
        {
            return it += offset;
        }
        friend iterator operator+(difference_type offset, iterator it)
        {
            return it += offset;
        }
        friend iterator operator-(iterator it, difference_type offset)
        {
            return it -= offset;
        }
        friend difference_type operator-(iterator lhs, iterator rhs)
        {
            lhs.check_same_owner(rhs);
            return lhs._ptr - rhs._ptr;
        }
        friend bool operator==(iterator lhs, iterator rhs)
        {
            lhs.check_same_owner(rhs);
            return lhs._ptr == rhs._ptr;
        }
        friend auto operator<=>(iterator lhs, iterator rhs)
        {
            lhs.check_same_owner(rhs);
            return lhs._ptr <=> rhs._ptr;
        }

    private:
        iterator(vector *owner, pointer ptr, size_t version) : _owner(owner), _ptr(ptr), _version(version)
        {
        }

        vector *_owner{ nullptr };
        pointer _ptr{ nullptr };
        size_t  _version{ 0 };

        void check_valid() const
        {
            if (_owner == nullptr || _version != _owner->_version)
            {
                throw std::runtime_error("invalid vector iterator");
            }
        }

        void check_same_owner(const iterator &other) const
        {
            check_valid();
            other.check_valid();
            if (_owner != other._owner)
            {
                throw std::runtime_error("iterators belong to different vectors");
            }
        }

        friend class vector;
        friend class const_iterator;
    };

    class const_iterator
    {
    public:
        using iterator_concept  = std::contiguous_iterator_tag;
        using iterator_category = std::random_access_iterator_tag;
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = const T *;
        using reference         = const T &;

        const_iterator() = default;
        const_iterator(iterator it) : _owner(it._owner), _ptr(it._ptr), _version(it._version)
        {
            it.check_valid();
        }

        reference operator*() const
        {
            check_valid();
            return *_ptr;
        }
        pointer operator->() const
        {
            check_valid();
            return _ptr;
        }
        reference operator[](difference_type offset) const
        {
            check_valid();
            return _ptr[offset];
        }

        const_iterator &operator++()
        {
            check_valid();
            ++_ptr;
            return *this;
        }

        const_iterator operator++(int)
        {
            const_iterator old = *this;
            ++*this;
            return old;
        }

        const_iterator &operator--()
        {
            check_valid();
            --_ptr;
            return *this;
        }

        const_iterator operator--(int)
        {
            const_iterator old = *this;
            --*this;
            return old;
        }

        const_iterator &operator+=(difference_type offset)
        {
            check_valid();
            _ptr += offset;
            return *this;
        }

        const_iterator &operator-=(difference_type offset)
        {
            return *this += -offset;
        }

        friend const_iterator operator+(const_iterator it, difference_type offset)
        {
            return it += offset;
        }
        friend const_iterator operator+(difference_type offset, const_iterator it)
        {
            return it += offset;
        }
        friend const_iterator operator-(const_iterator it, difference_type offset)
        {
            return it -= offset;
        }
        friend difference_type operator-(const_iterator lhs, const_iterator rhs)
        {
            lhs.check_same_owner(rhs);
            return lhs._ptr - rhs._ptr;
        }
        friend bool operator==(const_iterator lhs, const_iterator rhs)
        {
            lhs.check_same_owner(rhs);
            return lhs._ptr == rhs._ptr;
        }
        friend auto operator<=>(const_iterator lhs, const_iterator rhs)
        {
            lhs.check_same_owner(rhs);
            return lhs._ptr <=> rhs._ptr;
        }

    private:
        const_iterator(const vector *owner, pointer ptr, size_t version)
            : _owner(owner), _ptr(ptr), _version(version)
        {
        }

        const vector *_owner{ nullptr };
        pointer _ptr{ nullptr };
        size_t  _version{ 0 };

        void check_valid() const
        {
            if (_owner == nullptr || _version != _owner->_version)
            {
                throw std::runtime_error("invalid vector const_iterator");
            }
        }

        void check_same_owner(const const_iterator &other) const
        {
            check_valid();
            other.check_valid();
            if (_owner != other._owner)
            {
                throw std::runtime_error("const_iterators belong to different vectors");
            }
        }

        friend class vector;
    };

    explicit vector(int size = 10, const Alloc &alloc = Alloc()) // 加 explicit
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
        for (T *p = _first; p != _last; ++p)
        {
            _allocator.destroy(p); /// 把_first指针指向的堆内存中有效的元素析构掉
        }
        _allocator.deallocate(_first); /// 释放堆上的数组内存
        _first = _last = _end = nullptr;
    }

    vector(const vector<T> &other)
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

    vector<T> &operator=(const vector<T> &other)
    {
        if (this == &other)
            return *this;
        // delete[] _first;
        for (T *p = _first; p != _last; ++p)
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
        ++_version;
        return *this;
    }

    void push_back(const T &value)
    {
        if (full())
        {
            expand();
        }
        // *_last = value;
        _allocator.construct(_last, value);
        ++_last;
        ++_version;
    }

    void pop_back()
    {
        if (empty())
            return;
        // --_last;
        _allocator.destroy(_last - 1);
        --_last;
        ++_version;
    }

    T &back()
    {
        return *(_last - 1);
    }

    const T &back() const
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

    iterator begin() noexcept
    {
        return iterator(this, _first, _version);
    }

    iterator end() noexcept
    {
        return iterator(this, _last, _version);
    }

    const_iterator begin() const noexcept
    {
        return const_iterator(this, _first, _version);
    }

    const_iterator end() const noexcept
    {
        return const_iterator(this, _last, _version);
    }

    const_iterator cbegin() const noexcept
    {
        return begin();
    }

    const_iterator cend() const noexcept
    {
        return end();
    }

    T &operator[](size_t index)
    {
        if (index >= size())
        {
            throw std::out_of_range("vector index out of range");
        }
        return _first[index];
    }

    const T &operator[](size_t index) const
    {
        if (index >= size())
        {
            throw std::out_of_range("vector index out of range");
        }
        return _first[index];
    }

private:
    T    *_first = nullptr;
    T    *_last  = nullptr;
    T    *_end   = nullptr;
    Alloc _allocator; /// 定义容器的空间配置器对象
    size_t _version{ 0 }; /// 结构修改后递增，用于检测旧迭代器是否失效

    void expand()
    {
        /// 关键修复2：先保存旧大小
        size_t old_size     = _last - _first;
        size_t old_capacity = _end - _first;
        size_t new_capacity = old_capacity * 2;

        // T* new_first = new T[new_capacity];
        T *new_first = _allocator.allocate(new_capacity);

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

static_assert(std::contiguous_iterator<vector<int>::iterator>);
static_assert(std::contiguous_iterator<vector<int>::const_iterator>);

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
    Test(const Test &other)
    {
        std::cout << "Test copy constructor called." << std::endl;
    }
};

int main()
{
    {
        vector<int> numbers;
        numbers.push_back(10);
        numbers.push_back(20);
        numbers.push_back(30);

        assert(numbers[0] == 10);
        assert(numbers[1] == 20);
        numbers[1] = 200;
        assert(numbers[1] == 200);

        const vector<int> &const_numbers = numbers;
        assert(const_numbers[2] == 30);

        int sum = 0;
        for (auto value : numbers)
        {
            sum += value;
        }
        assert(sum == 240);

        for (vector<int>::iterator it = numbers.begin(); it != numbers.end(); ++it)
        {
            *it += 1;
        }
        assert(numbers[0] == 11);
        assert(numbers[1] == 201);
        assert(numbers[2] == 31);

        int const_sum = 0;
        for (vector<int>::const_iterator it = const_numbers.cbegin(); it != const_numbers.cend(); ++it)
        {
            const_sum += *it;
        }
        assert(const_sum == 243);

        std::sort(numbers.begin(), numbers.end());
        assert(numbers[0] == 11);
        assert(numbers[1] == 31);
        assert(numbers[2] == 201);

        bool mutable_out_of_range = false;
        try
        {
            (void)numbers[3];
        }
        catch (const std::out_of_range &)
        {
            mutable_out_of_range = true;
        }
        assert(mutable_out_of_range);

        bool const_out_of_range = false;
        try
        {
            (void)const_numbers[3];
        }
        catch (const std::out_of_range &)
        {
            const_out_of_range = true;
        }
        assert(const_out_of_range);
    }

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
