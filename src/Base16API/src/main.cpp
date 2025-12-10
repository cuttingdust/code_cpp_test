#include "base16.h"

#include <iostream>


int main()
{
    setlocale(LC_ALL, "zh_CN.UTF-8");
    /// vector和string直接转换
    /// char * => vector<unsigned char>
    /// char []=> vector<unsigned char>
    /// string => vector<unsigned char>

    const char* cstr{ "测试const char*到vector" };
    /// 复制内容到vector，可以传递
    /// 内存开始地址和结束地址
    /// 使用加法运算符移动指针
    /// 每加一移动一个类型大小的内容
    int size = strlen(cstr); /// 字符串长度，不包含结尾\0

    /// 一 数据开始的内存， 二数据最后内存的下一位
    /// \0的下一位
    std::vector<unsigned char> data(cstr, cstr + size + 1);
    std::cout << data.size() << std::endl;
    std::cout << data.data() << std::endl;


    char astr[]{ "测试数组到vector" };
    data.clear(); ///  清空vector
    /// 重新申请内存并复制内容
    data.assign(astr, &astr[sizeof(astr)]);
    std::cout << data.size() << std::endl;
    std::cout << data.data() << std::endl;

    std::string str{ "测试string到vector" };
    data.clear(); /// 清理空间标识，不一定释放空间
    data.assign(str.begin(), str.end());
    data.push_back('\0');
    std::cout << data.size() << std::endl;
    std::cout << data.data() << std::endl;


    /// vector转换成string
    std::string outstr(data.begin(), data.end());
    std::cout << "outstr = " << outstr << std::endl;

    /// base16编码
    {
        std::string                teststr{ "测试base16数据" };
        std::vector<unsigned char> data(teststr.begin(), teststr.end());
        data.push_back('\0');

        auto base16str = Base16Encode(data);
        std::cout << "base16 encode:" << base16str << std::endl;

        auto resdata = Base16Decode(base16str);
        std::cout << "base16 decode:" << resdata.data() << std::endl;
    }
}
