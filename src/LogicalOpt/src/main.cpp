#include <iostream>
#include <bitset>

int main(int argc, char *argv[])
{
    {
        /// 算数运算符
        /// 二进制 只包含 0和1
        /// 逐位非  ~      ~0 = 1 ~1=0
        /// ~101  = 010
        /// 逐位与  &
        /// 1&1 = 1 0&1 = 0 0&0=0
        /// 逐位或  |
        ///  1|1=1   0|1=1 1|0=1 0|0=0

        char a = 0b10000001; /// 0b c++14
        char b = 0b00000001;

        /// 用bitset输出二进制
        /// #include <bitset>
        std::cout << "a:\t" << std::bitset<8>(a) << std::endl;
        /// 逐位非
        std::cout << "~a:\t" << std::bitset<8>(~a) << std::endl;
        std::cout << "b:\t" << std::bitset<8>(b) << std::endl;
        std::cout << "~b:\t" << std::bitset<8>(~b) << std::endl;
        /// 8位一字节
        /// shellcode a = 0x81
        /// shellcode b = 0x01

        /// 逐位与
        std::cout << "a&b:\t" << std::bitset<8>(a & b) << std::endl;
        /// 逐位或
        std::cout << "a|b:\t" << std::bitset<8>(a | b) << std::endl;
    }

    {
        bool f1{ false };
        bool f2{ false };
        bool t1{ true };
        bool t2{ true };
        if (t1)
        {
            std::cout << "t1 is true" << std::endl;
        }

        std::cout << "f1 = " << f1 << std::endl;
        std::cout << "t1 = " << t1 << std::endl;

        std::cout << "f1|t1=" << (f1 | t1) << std::endl;
        if (f1 | t1)
            std::cout << "(f1|t1)true\n";
        std::cout << "f1|f2=" << (f1 | f2) << std::endl;

        std::cout << "f1&t2=" << (f1 & t2) << std::endl;
        std::cout << "t1&t2=" << (t1 & t2) << std::endl;

        /// 逻辑运算符 （代用运算符）
        /// 逻辑非 !        not
        /// 逻辑或 ||       or
        /// 逻辑与 &&       and
        /// and or 和 & | 的区别
        /// 逻辑运算符 短路求值
        /// 如果通过第一个操作数就能得到结果
        /// 这不求值第二个
        if (f1 or t1)
        {
            std::cout << "f1 or t1" << std::endl;
        }
        if (f1 and t1)
        {
        }
        else
        {
            std::cout << "f1 and t1 else" << std::endl;
        }
        if (not f1)
        {
            std::cout << "not f1" << std::endl;
        }
        if (not(f1 and t1))
        {
            std::cout << "not (f1 and t1)" << std::endl;
        }
    }

    {
        /// 验证短路求值
        ///
        int x1{ 0 };
        x1 = 10;
        if ((x1++) or (x1 += 2))
        {
            std::cout << "(x1++) or (x1 += 2)" << std::endl;
        }
        std::cout << "x1 = " << x1 << std::endl;


        x1 = 10;
        if ((x1++) | (x1 += 2))
        {
            std::cout << "(x1++) | (x1 += 2)" << std::endl;
        }
        std::cout << "x1 = " << x1 << std::endl;
    }

    std::cout << "hello world" << std::endl;

    return 0;
}
