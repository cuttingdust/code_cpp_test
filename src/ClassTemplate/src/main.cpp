#include <iostream>

template <typename T>
class SeqStack
{
public:
    /// 构造函数和析构函数不用加<T>, 其他出现模板的地方都要加上类型参数列表
    SeqStack(int size = 10) : _pstack(new T[size]), _top(0), _size(size)
    {
    }
    ~SeqStack()
    {
        delete[] _pstack;
        _pstack = NULL;
    }
    SeqStack(const SeqStack<T> &other) : _pstack(new T[other._size]), _top(0), _size(other._size)
    {
        for (int i = 0; i < other._top; ++i)
            _pstack[i] = other._pstack[i];
    }
    SeqStack<T> &operator=(const SeqStack<T> &other)
    {
        if (this == &other)
        {
            return *this;
        }
        delete[] _pstack;

        _pstack = new T[other._size];
        _top    = other._top;
        _size   = other._size;

        /// 不要用memcopy来复制对象数组，因为对象可能有自己的复制构造函数和赋值运算符，直接复制内存可能会导致资源管理问题。
        for (int i = 0; i < other._top; ++i)
            _pstack[i] = other._pstack[i];

        return *this;
    }

public:
    void push(const T &value) /// 入栈操作
            ;

    void pop() /// 出栈操作
            ;

    T top() const
    {
        if (empty())
        {
            throw std::runtime_error("Stack is empty");
        }
        return _pstack[_top - 1];
    }
    bool full() const
    {
        return _top == _size;
    }
    bool empty() const
    {
        return _top == 0;
    }

private:
    T  *_pstack = NULL;
    int _top    = -1;
    int _size   = 0;

    /// 顺序栈底层数组按2倍的方式扩容
    void expand()
    {
        int newSize  = _size * 2;
        T  *newStack = new T[newSize];
        for (int i = 0; i < _size; ++i)
            newStack[i] = _pstack[i];
        delete[] _pstack;
        _pstack = newStack;
        _size   = newSize;
    }
};

template <typename T>
void SeqStack<T>::push(const T &value)
{
    if (full())
    {
        expand();
    }
    _pstack[_top++] = value;
}

template <typename T>
void SeqStack<T>::pop()
{
    if (!empty())
    {
        --_top;
    }
}

int main(int argc, char *argv[])
{
    SeqStack<int> stack;
    stack.push(20);
    stack.push(78);
    stack.push(32);
    stack.push(15);
    stack.pop();
    std::cout << "Top element: " << stack.top() << std::endl; // 输出: Top element: 32

    return 0;
}
