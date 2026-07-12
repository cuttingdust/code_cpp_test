#include <algorithm>
#include <cassert>
#include <compare>
#include <concepts>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <iterator>
#include <ostream>
#include <string>
#include <vector>

class Data
{
public:
    Data()
    {
        std::cout << "Create Data" << std::endl;
    }
    Data(const Data &d)
    {
        std::cout << "Copy Data" << std::endl;
    }
    ~Data()
    {
        std::cout << "Drop Data" << std::endl;
    }
};

void TestData(const std::vector<Data> &d)
{
    std::cout << "In TestData " << d.size() << std::endl;
}

//////////////////////////////////////////////////////////////////

/// 自定义字符串类String
class String
{
public:
    class const_iterator;

    class iterator
    {
    public:
        using iterator_concept  = std::contiguous_iterator_tag;
        using iterator_category = std::random_access_iterator_tag;
        using value_type        = char;
        using difference_type   = std::ptrdiff_t;
        using pointer           = char *;
        using reference         = char &;

        iterator() = default;

        reference operator*() const { return *_ptr; }
        pointer   operator->() const { return _ptr; }
        reference operator[](difference_type offset) const { return _ptr[offset]; }

        iterator &operator++()
        {
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
            if (offset != 0)
                _ptr += offset;
            return *this;
        }

        iterator &operator-=(difference_type offset) { return *this += -offset; }

        friend iterator operator+(iterator it, difference_type offset) { return it += offset; }
        friend iterator operator+(difference_type offset, iterator it) { return it += offset; }
        friend iterator operator-(iterator it, difference_type offset) { return it -= offset; }

        friend difference_type operator-(iterator lhs, iterator rhs)
        {
            return lhs._ptr == rhs._ptr ? 0 : lhs._ptr - rhs._ptr;
        }

        friend bool operator==(iterator lhs, iterator rhs) = default;
        friend auto operator<=>(iterator lhs, iterator rhs) = default;

    private:
        explicit iterator(pointer ptr) : _ptr(ptr) {}

        pointer _ptr{ nullptr };

        friend class String;
        friend class const_iterator;
    };

    class const_iterator
    {
    public:
        using iterator_concept  = std::contiguous_iterator_tag;
        using iterator_category = std::random_access_iterator_tag;
        using value_type        = char;
        using difference_type   = std::ptrdiff_t;
        using pointer           = const char *;
        using reference         = const char &;

        const_iterator() = default;
        const_iterator(iterator it) : _ptr(it._ptr) {}

        reference operator*() const { return *_ptr; }
        pointer   operator->() const { return _ptr; }
        reference operator[](difference_type offset) const { return _ptr[offset]; }

        const_iterator &operator++()
        {
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
            if (offset != 0)
                _ptr += offset;
            return *this;
        }

        const_iterator &operator-=(difference_type offset) { return *this += -offset; }

        friend const_iterator operator+(const_iterator it, difference_type offset) { return it += offset; }
        friend const_iterator operator+(difference_type offset, const_iterator it) { return it += offset; }
        friend const_iterator operator-(const_iterator it, difference_type offset) { return it -= offset; }

        friend difference_type operator-(const_iterator lhs, const_iterator rhs)
        {
            return lhs._ptr == rhs._ptr ? 0 : lhs._ptr - rhs._ptr;
        }

        friend bool operator==(const_iterator lhs, const_iterator rhs) = default;
        friend auto operator<=>(const_iterator lhs, const_iterator rhs) = default;

    private:
        explicit const_iterator(pointer ptr) : _ptr(ptr) {}

        pointer _ptr{ nullptr };

        friend class String;
    };

    void Clear()
    {
        delete[] _pstr;
        _pstr = nullptr;
        _size = 0;
    }
    ~String()
    {
        std::cout << "Drop String:" << _size << std::endl;
        Clear();
    }
    String &operator=(const char *str)
    {
        if (str == nullptr)
        {
            return *this;
        }
        if (str == _pstr)
        {
            return *this;
        }
        std::cout << "operator=(const char* str) String:" << str << std::endl;
        const size_t size = strlen(str);
        char        *pstr = nullptr;
        if (size > 0)
        {
            pstr = new char[size + 1]; /// +1存储\0  /// string 底层也是new出来的内存
            memcpy(pstr, str, size + 1);
        }

        Clear();
        _pstr = pstr;
        _size = size;
        return *this;
    }
    String(const char *p = "")
    {
        if (p == nullptr)
        {
            return;
        }
        std::cout << "Create String:" << p << std::endl;
        _size = strlen(p);
        if (_size == 0)
        {
            return;
        }
        _pstr = new char[_size + 1]; /// +1存储\0
        memcpy(_pstr, p, _size + 1);
    }
    /// 拷贝构造函数
    String(const String &s)
    {
        std::cout << "Copy String:" << s.c_str() << std::endl;
        _size = s._size;
        if (_size > 0)
        {
            _pstr = new char[_size + 1];
            memcpy(_pstr, s._pstr, _size + 1);
        }
    }
    /// 赋值操作符
    String &operator=(const String &s)
    {
        /// 对象原来空间处理
        if (this == &s)
        {
            return *this;
        }
        std::cout << "operator=(const String& s):" << s.c_str() << std::endl;

        char *pstr = nullptr;
        if (s._size > 0)
        {
            pstr = new char[s._size + 1];
            memcpy(pstr, s._pstr, s._size + 1);
        }

        Clear();
        _pstr = pstr;
        _size = s._size;
        return *this;
    }

    /// 移动构造函数
    String(String &&str) noexcept
    {
        std::cout << "Move String" << std::endl;
        _pstr     = str._pstr;
        _size     = str._size;
        str._pstr = nullptr;
        str._size = 0;
    }

    /// 移动赋值操作符
    String &operator=(String &&s) noexcept
    {
        /// 对象原来空间处理
        if (this == &s)
        {
            return *this;
        }
        Clear();

        std::cout << "operator=(String&& s) Move String" << std::endl;
        _pstr   = s._pstr;
        _size   = s._size;
        s._pstr = nullptr;
        s._size = 0;

        return *this;
    }

    bool operator>(const String &str) const
    {
        return Compare(str) > 0;
    }

    bool operator<(const String &str) const
    {
        return Compare(str) < 0;
    }

    bool operator>=(const String &str) const
    {
        return Compare(str) >= 0;
    }

    bool operator<=(const String &str) const
    {
        return Compare(str) <= 0;
    }

    bool operator==(const String &str) const
    {
        return Compare(str) == 0;
    }


    const char *c_str() const noexcept
    {
        if (!_pstr)
            return "";
        return _pstr;
    }

    size_t length() const noexcept
    {
        return _size;
    }

    iterator begin() noexcept
    {
        return iterator(_pstr);
    }

    iterator end() noexcept
    {
        return iterator(_pstr ? _pstr + _size : nullptr);
    }

    const_iterator begin() const noexcept
    {
        return const_iterator(_pstr);
    }

    const_iterator end() const noexcept
    {
        return const_iterator(_pstr ? _pstr + _size : nullptr);
    }

    const_iterator cbegin() const noexcept
    {
        return begin();
    }

    const_iterator cend() const noexcept
    {
        return end();
    }

    char &operator[](int index)
    {
        return _pstr[index];
    }

    const char &operator[](int index) const
    {
        return _pstr[index];
    }

    friend std::ostream &operator<<(std::ostream &os, const String &str)
    {
        if (str._size > 0)
        {
            os.write(str._pstr, static_cast<std::streamsize>(str._size));
        }
        return os;
    }

    String operator+(const String &str) const
    {
        String result;
        result._size = _size + str._size;
        if (result._size == 0)
        {
            return result;
        }

        result._pstr = new char[result._size + 1];
        if (_size > 0)
        {
            memcpy(result._pstr, _pstr, _size);
        }
        if (str._size > 0)
        {
            memcpy(result._pstr + _size, str._pstr, str._size);
        }
        result._pstr[result._size] = '\0';
        return result;
    }

private:
    int Compare(const String &str) const
    {
        const size_t min_size = std::min(_size, str._size);
        if (min_size > 0)
        {
            const int result = memcmp(_pstr, str._pstr, min_size);
            if (result != 0)
            {
                return result;
            }
        }

        if (_size == str._size)
        {
            return 0;
        }
        return _size < str._size ? -1 : 1;
    }

    char  *_pstr{ nullptr };
    size_t _size{ 0 };
};

static_assert(std::random_access_iterator<String::iterator>);
static_assert(std::contiguous_iterator<String::iterator>);
static_assert(std::random_access_iterator<String::const_iterator>);
static_assert(std::contiguous_iterator<String::const_iterator>);


int main(int argc, char *argv[])
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

    {
        String empty1;
        String empty2;
        String prefix("abc");
        String longer("abcd");
        String equal("abc");

        assert(empty1 == empty2);
        assert(empty1.length() == 0);
        assert(prefix.length() == 3);
        assert(longer.length() == 4);
        std::cout << "prefix = " << prefix << std::endl;
        assert(empty1 < prefix);
        assert(prefix <= equal);
        assert(prefix >= equal);
        assert(prefix == equal);
        assert(prefix < longer);
        assert(longer > prefix);
        assert(!(prefix > equal));

        String combined = prefix + String("def");
        assert(combined == String("abcdef"));
        assert((empty1 + prefix) == prefix);
        assert((prefix + empty1) == prefix);
        std::cout << "combined = " << combined << std::endl;

        String str1 = "aaa";
        String str2 = "bbb";
        assert(str1 < str2);
        std::cout << "str1 < str2" << std::endl;
    }

    {
        String combined("abcdef");

        std::string iterated;
        for (auto c : combined)
        {
            iterated.push_back(c);
        }
        assert(iterated == "abcdef");
        std::cout << "range for value = " << iterated << std::endl;

        String editable("abc");
        for (auto &c : editable)
        {
            c = static_cast<char>(c - 'a' + 'A');
        }
        assert(editable == String("ABC"));
        std::cout << "range for reference = " << editable << std::endl;

        std::cout << "iterator loop = ";
        for (String::iterator it = combined.begin(); it != combined.end(); ++it)
        {
            std::cout << *it;
        }
        std::cout << std::endl;

        const String const_string("iterator");
        std::string  const_iterated;
        for (String::const_iterator it = const_string.cbegin(); it != const_string.cend(); ++it)
        {
            const_iterated.push_back(*it);
        }
        assert(const_iterated == "iterator");

        String sortable("dcba");
        std::sort(sortable.begin(), sortable.end());
        assert(sortable == String("abcd"));

        *combined.begin() = 'A';
        assert(combined == String("Abcdef"));
    }

    getchar();
    return 0;
}
