#include <algorithm>
#include <cassert>
#include <compare>
#include <concepts>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <cstdlib> // for rand()
#include <stdexcept>
#include <ctime> // for time()
#include <vector>

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

    iterator insert(iterator pos, const T &value)
    {
        pos.check_valid();
        if (pos._owner != this)
        {
            throw std::invalid_argument("insert iterator belongs to a different vector");
        }
        if (pos._ptr < _first || pos._ptr > _last)
        {
            throw std::out_of_range("insert iterator is out of range");
        }

        // 扩容会改变底层地址，因此先保存下标；value 也可能引用容器内部元素，
        // 必须在扩容和移动元素之前复制一份。
        const size_t index = static_cast<size_t>(pos._ptr - _first);
        T            value_copy(value);

        if (full())
        {
            expand();
        }

        T *insert_pos = _first + index;
        if (insert_pos == _last)
        {
            _allocator.construct(_last, value_copy);
        }
        else
        {
            // _last 指向未构造内存，先在这里复制构造最后一个元素，再将中间元素后移。
            _allocator.construct(_last, *(_last - 1));
            for (T *current = _last - 1; current != insert_pos; --current)
            {
                *current = *(current - 1);
            }
            *insert_pos = value_copy;
        }

        ++_last;
        ++_version; // 当前教学实现采用保守策略：insert 后所有旧迭代器均失效。
        return iterator(this, insert_pos, _version);
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
        size_t new_capacity = old_capacity == 0 ? 1 : old_capacity * 2;

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
        // ==================== 迭代器失效示例 ====================
        //
        // 示例 1：自定义 vector 扩容后，所有旧迭代器都会失效。
        // iterator 内部保存的是元素的内存地址。下面的 numbers 初始容量为 2，
        // 插入第三个元素时 push_back() 会调用 expand()：
        //   1. 申请一块更大的新内存；
        //   2. 把旧元素复制到新内存；
        //   3. 析构旧元素并释放旧内存；
        //   4. 让 _first、_last 和 _end 指向新内存。
        // old_it 仍然保存已经释放的旧内存地址，因此不能再解引用、递增或比较。
        vector<int> numbers(2);
        numbers.push_back(10);
        numbers.push_back(20);

        auto old_it = numbers.begin();
        auto index  = old_it - numbers.begin();

        numbers.push_back(30); // 容量不足，触发 expand()，old_it 从这里开始失效。

        // 普通裸指针迭代器在这里会产生未定义行为。当前教学版 iterator 保存了容器
        // 和版本号，因此比较、解引用或递增失效迭代器时会主动抛出异常。
        bool comparison_detected = false;
        try
        {
            (void)(old_it != numbers.end());
        }
        catch (const std::runtime_error &)
        {
            comparison_detected = true;
        }
        assert(comparison_detected);

        bool dereference_detected = false;
        try
        {
            (void)*old_it;
        }
        catch (const std::runtime_error &)
        {
            dereference_detected = true;
        }
        assert(dereference_detected);

        // 正确：扩容前保存元素下标，扩容后通过新的 begin() 重建迭代器。
        auto new_it = numbers.begin() + index;
        assert(*new_it == 10);

        // 示例 2：std::vector::erase() 会移动删除位置后面的元素。
        // 删除 20 后，30 和 40 会向前移动。原来指向 20 的迭代器已经没有对应元素，
        // 原来指向 30、40 和旧 end() 的迭代器也不再表示原来的位置。因此，删除位置
        // 以及它后面的迭代器、指针和引用都会失效，只有删除位置之前的仍然有效。
        std::vector<int> values{ 10, 20, 30, 40 };

        // 错误写法：erase(it) 后 it 已失效，循环末尾的 ++it 会产生未定义行为。
        // for (auto it = values.begin(); it != values.end(); ++it)
        // {
        //     if (*it == 20)
        //     {
        //         values.erase(it);
        //     }
        // }

        // 正确写法：erase() 返回删除位置后新的有效迭代器，用它继续遍历。
        for (auto it = values.begin(); it != values.end();)
        {
            if (*it == 20 || *it == 30)
            {
                it = values.erase(it);
            }
            else
            {
                ++it;
            }
        }

        assert((values == std::vector<int>{ 10, 40 }));

        // 示例 3：std::vector::insert() 也可能导致迭代器失效。
        // 情况 A：容量不足，insert() 触发重新分配。旧内存被释放，因此所有旧迭代器、
        //         指针和引用都会失效，这与 push_back() 触发扩容时完全相同。
        // 情况 B：容量足够，不需要重新分配。插入位置之后的元素仍要向后移动，因此
        //         插入位置以及它后面的迭代器（包括旧 end()）都会失效；只有插入位置
        //         之前的迭代器仍然有效。
        std::vector<int> insert_values{ 10, 30, 40 };
        insert_values.reserve(10); // 保证本例 insert() 不会触发扩容，只观察元素移动。

        auto before_insert = insert_values.begin();     // 指向 10，位于插入位置之前。
        auto at_insert     = insert_values.begin() + 1; // 指向 30，正好位于插入位置。

        // insert() 返回指向新插入元素的有效迭代器，应当使用这个返回值继续操作。
        auto inserted = insert_values.insert(at_insert, 20);

        assert(*before_insert == 10); // 插入位置之前的迭代器仍然有效。
        assert(*inserted == 20);
        assert((insert_values == std::vector<int>{ 10, 20, 30, 40 }));

        // 错误：at_insert 原来指向 30，但 30 已被移动到新位置，at_insert 已失效。
        // 即使 *at_insert 偶尔看起来还能读到 20，也不能继续使用，结果属于未定义行为。
        // std::cout << *at_insert << std::endl;

        // 通用正确做法：不要依赖 insert() 前位于插入点及其后的旧迭代器，
        // 直接保存并使用 insert() 返回的新迭代器；若要找其他元素，则重新获取迭代器。
        auto moved_30 = insert_values.begin() + 2;
        assert(*moved_30 == 30);
    }

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

    {
        vector<int> values(3);
        values.push_back(10);
        values.push_back(30);

        auto old_it   = values.begin();
        auto inserted = values.insert(values.begin() + 1, 20);

        assert(*inserted == 20);
        assert(values.size() == 3);
        assert(values[0] == 10);
        assert(values[1] == 20);
        assert(values[2] == 30);

        // insert 修改了容器结构，当前调试策略会让所有旧迭代器失效。
        bool invalidation_detected = false;
        try
        {
            (void)*old_it;
        }
        catch (const std::runtime_error &)
        {
            invalidation_detected = true;
        }
        assert(invalidation_detected);

        // 当前容量已满，头部插入会触发 expand()，返回值仍指向新内存中的插入元素。
        auto inserted_front = values.insert(values.begin(), 5);
        assert(*inserted_front == 5);
        assert(values[0] == 5);

        // 插入值引用容器内部元素；insert 会先复制该值，避免移动元素时改变它。
        auto inserted_end = values.insert(values.end(), values[1]);
        assert(*inserted_end == 10);
        assert(values[values.size() - 1] == 10);

        vector<int> other;
        bool        wrong_owner_detected = false;
        try
        {
            values.insert(other.begin(), 100);
        }
        catch (const std::invalid_argument &)
        {
            wrong_owner_detected = true;
        }
        assert(wrong_owner_detected);
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
