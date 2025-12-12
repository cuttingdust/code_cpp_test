#include <iostream>
#include <fstream>
#include <string>

int main(int argc, char *argv[])
{
    std::string testfile = "testfile.txt";
    {
        std::fstream rfs(testfile, std::ios::in | std::ios::binary);
        if (!rfs.is_open())
        {
            std::cerr << "open file " << testfile << " failed!" << std::endl;
            return -1;
        }

        char buf[4090]{ 0 };
        rfs.read(buf, sizeof(buf) - 1);
        std::cout << "rfs.gcount()  = " << rfs.gcount() << std::endl;
        std::cout << "-----------------------\n";
        std::cout << buf << std::endl;
        std::cout << "-----------------------\n";
        rfs.close();
    }
    {
        /// 获取文件大小 ios::ate文件尾部
        /// tellg 读取文件指针位置
        std::ifstream ifs(testfile, std::ios::ate);
        std::cout << testfile << " filesize=" << ifs.tellg() << std::endl;

        /// 读取文件更改后内容
        std::string line;
        for (;;)
        {
            getline(ifs, line);
            //cout << ifs.rdstate() <<endl;
            if (!line.empty())
                std::cout << "line:" << line << std::endl;
            ifs.clear();
        }
    }


    return 0;
}
