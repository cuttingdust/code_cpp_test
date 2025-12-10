#include "base16.h"

#include <iostream>


int main()
{
    setlocale(LC_ALL, "zh_CN.UTF-8");
    std::string                teststr = "测试用于base16的字符串";
    std::vector<unsigned char> data(teststr.begin(), teststr.end());

    auto base16str = Base16Encode(data);
    std::cout << base16str << std::endl;

    /// base16解码

    auto        out_data = Base16Decode(base16str);
    std::string ostr(out_data.begin(), out_data.end());
    std::cout << ostr << std::endl;
}
