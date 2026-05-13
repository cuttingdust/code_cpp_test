#include <iostream>

/// 运算重载符

class CComplex
{
public:
    CComplex(int r = 0, int i = 0) : m_real(r), m_image(i)
    {
    }

public:
    CComplex operator+(const CComplex &other) const
    {
        return { m_real + other.m_real, m_image + other.m_image };
    }

    void display() const
    {
        std::cout << m_real << " + " << m_image << "i" << std::endl;
    }
    CComplex operator+(int value) const
    {
        return { m_real + value, m_image };
    }

    CComplex &operator++() /// 前置++
    {
        m_real += 1;
        m_image += 1;
        return *this;
    }

    CComplex operator++(int) /// 后置++
    {
        // CComplex temp = *this;
        // m_image += 1;
        // m_real += 1;
        // return temp;
        return CComplex{ m_real++, m_image++ };
    }

    CComplex operator+=(const CComplex &other)
    {
        m_real += other.m_real;
        m_image += other.m_image;
        return *this;
    }

private:
    int             m_real  = 0;
    int             m_image = 0;
    friend CComplex operator+(const CComplex &lhs, const CComplex &rhs);
    friend CComplex operator+(int value, const CComplex &comp);
    // friend CComplex operator+=(CComplex &lhs, const CComplex &rhs);
    friend std::ostream &operator<<(std::ostream &os, const CComplex &comp);
    friend std::istream &operator>>(std::istream &is, CComplex &comp);
};


CComplex operator+(const CComplex &lhs, const CComplex &rhs)
{
    return { lhs.m_real + rhs.m_real, lhs.m_image + rhs.m_image };
}

CComplex operator+(int value, const CComplex &comp)
{
    return { comp.m_real + value, comp.m_image };
}

// CComplex operator+=(CComplex &lhs, const CComplex &rhs)
// {
//     lhs.m_real += rhs.m_real;
//     lhs.m_image += rhs.m_image;
//     return lhs;
// }

std::ostream &operator<<(std::ostream &os, const CComplex &comp)
{
    os << comp.m_real << " + " << comp.m_image << "i";
    return os;
}

std::istream &operator>>(std::istream &is, CComplex &comp)
{
    /// 默认是空格读取
    std::cout << "请输入复数（格式：实部 虚部 或 实部,虚部）：";

    char ch;
    int  real, image;

    /// 读取第一个数
    is >> real;

    /// 检查下个字符是不是逗号
    if (is.peek() == ',')
    {
        is >> ch >> image; /// 读取逗号和虚部
    }
    else
    {
        is >> image; /// 读取虚部
    }

    comp.m_real  = real;
    comp.m_image = image;

    return is;
}


template <typename T>
void show(T a)
{
    ///  std::cout ::operator<<(cout, comp1) void << endl;
    ///  ostream& operator<<(ostream& os, const CComplex& comp);
    std::cout << a << std::endl;
}


int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "zh_CN.UTF-8");
    std::cout << "==========================================" << std::endl;


    CComplex comp1(10, 10);
    CComplex comp2(20, 20);

    show(comp1);

    /// CComplex comp3 = comp1 + comp2; // 这里会报错，因为没有定义 operator+
    CComplex comp3 = comp1.operator+(comp2);
    comp3.display();

    /// CComplex comp4 = comp1 + 10;
    CComplex comp4 = comp1.operator+(10);
    comp4.display();

    /// 编译器做对象运算得时候 会调用对象得运算符重载函数(优先调用成员函数);
    /// 如果没有成员函数满足要求，编译器会尝试调用全局函数 operator+，并且会进行隐式类型转换来匹配参数类型。
    CComplex comp5 = 30 + comp1;

    comp5.display();

    /// CComplex operator++(int)
    comp5++; /// ++ -- 单目运算符 operator++() 前置++ operator++(int) 后置++
    comp5.display();
    /// CComplex operator++() 前置++
    comp5 = ++comp1;
    comp5.display();

    // comp1.operator+=(comp2);  ::operator+=(comp1, comp2);
    comp1 += comp2; /// 这里会报错，因为没有定义 operator+=
    comp1.display();

    CComplex comp6;
    std::cin >> comp6;
    comp6.display();

    return 0;
}
