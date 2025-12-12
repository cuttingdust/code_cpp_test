#include <iostream>
#include <string>

int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "zh_CN.UTF-8");
    {
        for (;;)
        {
            std::cout << "请输入数字:" << std::flush;
            std::string line;
            int         x{ 0 };
            std::cin >> x;

            // std::cout << typeid(x).name() << std::endl;
            // if (cin.rdstate() == std::ios_base::failbit)
            if (std::cin.fail())
            {
                std::cin.clear(); /// 恢复状态为正常
                getline(std::cin, line);
                // std::cin.ignore();
                std::cout << "cin fail:" << line << std::endl;

                continue;
            }
            std::cout << "x=" << x << std::endl;
        }
    }

    {
        /// 单个字符输入get
        std::string cmd;
        for (;;)
        {
            char c = std::cin.get();
            if (c == '\n' || c == ';')
            {
                std::cout << "cmd:" << cmd << std::endl;
                cmd = "";
                continue;
            }
            cmd += c;
            //cout << c << flush;
        }
    }

    {
        /// 单行输入 getline
        std::string line;
        getline(std::cin, line);
        std::cout << "line:" << line << std::endl;

        for (;;)
        {
            char buf[1024]{ 0 };
            std::cout << ">>";
            std::cin.getline(buf, sizeof(buf) - 1);
            std::cout << "recv:" << buf << std::endl;
            if (strstr(buf, "exit"))
            {
                break;
            }
        }
    }

    getchar();
    return 0;
}
