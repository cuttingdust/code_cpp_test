#include "base16.h"

#include <iostream>
#include <__msvc_ostream.hpp>

/// 静态全局变量 作用域本cpp文件
static const std::string enc_tab{ "0123456789ABCDEF" };

static const std::vector<char> dec_tab{
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /// 0~9
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /// 10~19
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /// 20~29
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /// 30~39
    -1, -1, -1, -1, -1, -1, -1, -1,         /// 40~47
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  /// 48~57
    -1, -1, -1, -1, -1, -1, -1,             /// 58~64
    10, 11, 12, 13, 14, 15                  /// 65~70
};

//////////////////////////////////////////////////////////////////

std::string Base16Encode(const std::vector<unsigned char> &data)
{
    std::string res;
    /// unsigned char和char的区别
    /// 8位一个字节
    /// unsigned 无符号在做算术运算不管符号
    /// char第一位是符号位 二进制不能有正负号
    for (unsigned char c : data)
    {
        char h = c >> 4; /// 移位丢弃低位 0100 0001 >>4 0000 0100
        /// c++14 c&0x0f 0100 0001=>0000 0001
        char l = c & 0b00001111; /// 0x0F
        res += enc_tab[h];       /// 0~15 => '0'~'9''A'~'F'
        res += enc_tab[l];
    }

    return res;
}

std::vector<unsigned char> Base16Decode(const std::string &str)
{
    if (str.size() % 2 != 0)
    {
        std::cerr << "base16 string is error" << std::endl;
        return {};
    }

    std::vector<unsigned char> res;
    for (int i = 0; i < str.size(); i += 2)
    {
        char ch = str[i];
        char cl = str[i + 1];
        /// 'A' =>10 '0'=>0   0~15   F:70
        /// 65 =>10  48=>0
        unsigned char h = dec_tab[ch];
        unsigned char l = dec_tab[cl];
        /// h 0000 1100    <<4 1100 0000
        ///                  l 0000 0011 h|l  =>1100 0011
        res.push_back(h << 4 | l);
    }
    return res;
}
