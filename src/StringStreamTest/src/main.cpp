#include <iostream>
#include <sstream>
#include <string>

int main(int argc, char *argv[])
{
    /// stringstream 基础用法
    {
        std::stringstream ss; /// 日志的时候非常有用 /// 格式化所有的输入输出
        ss << "test stringstream:" << 100;
        ss << true;
        std::cout << ss.str() << std::endl;
        ss << std::boolalpha;
        ss << std::hex;
        ss << "\n" << false << 100 << std::endl;
        std::cout << ss.str() << std::endl;
        ss << std::noboolalpha;
        ss << std::dec;
        ss << "\n" << false << 100 << std::endl;
        std::cout << ss.str() << std::endl;
        ss.str(""); /// 清空
        std::cout << ss.str() << std::endl;
    }

    {
        /// stringstream 格式输入
        std::string       data = "test1 test2 test3";
        std::stringstream ss(data);
        std::string       tmp;
        ss >> tmp; /// std::stringstream 使用 >> 操作符时会自动按空格（包括空格、制表符、换行符等空白字符）进行分割。
        std::cout << tmp << ",";
        ss >> tmp;
        std::cout << tmp << ",";
        ss >> tmp;
        std::cout << tmp << ",";
        std::cout << std::endl;
    }

    {
        /// stringstream 单行读取
        std::string       data = "test1 test2 test3\ntest4 test5 test6\ntest7 test8 test9";
        std::string       line;
        std::stringstream ss(data);
        for (;;)
        {
            std::getline(ss, line);
            std::cout << "line:" << line << std::endl;
            if (ss.eof())
            {
                break;
            }
        }
    }


    return 0;
}
