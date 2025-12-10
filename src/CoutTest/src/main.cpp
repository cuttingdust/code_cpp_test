#include <iostream>

/// friend

int main(int argc, char *argv[])
{
    /// 格式输出
    /// cout标准输出 ostream stdout 1
    std::cout << "hello world" << std::endl;
    std::cout << 100 << std::endl;                     /// 默认十进制输出
    std::cout << "0O" << std::oct << 100 << std::endl; /// 8进制输出
    std::cout << "0x" << std::hex << 100 << std::endl; /// 16进制输出
    std::cout << std::dec << 100 << std::endl;         /// 10进制输出
    std::cout << true << ":" << false << std::endl;
    std::cout << std::boolalpha;
    std::cout << true << ":" << false << std::endl;
    std::cout << std::noboolalpha;
    std::cout << "=======================" << std::endl;
    /// cout 无格式输入
    std::cout.put('A').put('B');
    std::cout.put('C');
    std::cout.put(68); /// asciic
    std::cout.write("123", 3);
    std::string str = "teststring";
    std::cout.write(str.c_str(), str.size());
    std::cout << std::flush; /// 刷新缓冲区 多线程 网络传递的时候常用
    std::cout << std::endl;

    /// cerr标准错误输出 无缓冲 stderr 2
    /// CoutTest.exe >1.txt
    /// CoutTest.exe 1 >1.txt 2>2.txt /// 标准输出到1.txt 标准错误输出到2.txt /// 通常日志都在一个文件处理
    /// CoutTest.exe >log.txt 2 > &1  /// 标准错误流重定向标定输出流 统一输出文件
    std::cerr << "test cerr 001\n";
    std::cerr << "TEST CERR 002\n";
    return 0;
}
