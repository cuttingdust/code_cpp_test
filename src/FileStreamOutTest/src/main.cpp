#include <iostream>
#include <fstream>

int main(int argc, char *argv[])
{
    std::string testfile = "testfile.txt";
    /// out 写入文件 binary二进制处理\r\n不处理
    /// 输出默认清空文件的原内容
    std::fstream wfs(testfile,
                     std::ios::out | std::ios::binary); /// 默认不是二进制 但是如果不是二进制的话对于\r\n 跨平台会出问题
    //wfs.open(testfile, ios::out | ios::binary);
    wfs << "1234567890\n";
    wfs.close();
    std::ofstream ofs; /// app追加写入
    ofs.open(testfile, std::ios::app | std::ios::binary);
    ofs << "ABCDEFGHIJ\n" << std::flush;
    ofs.write("1234", 4);
    ofs.close();
    return 0;
}
