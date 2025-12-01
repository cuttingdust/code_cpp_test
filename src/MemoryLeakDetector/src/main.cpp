/// 检查内存泄漏,MyLeakDetector类来辅助检查
#include <iostream>
#include "MemoryLeakDetector.h"

/// VS自带的内存诊断
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

/// Detected memory leaks!
/// Dumping objects ->
/// {187} normal block at 0x000001A04DD49FB0, 4144 bytes long.
///  Data: <                > 00 10 D2 93 F6 7F 00 00 00 10 D2 93 F6 7F 00 00
/// Object dump complete.

class FClassErr
{
public:
    FClassErr(int n)
    {
        if (n <= 0)
        {
            throw 101; /// 101象征有无限的能力
        }
        data = new int[n];
    }
    ~FClassErr()
    {
        delete[] data;
    }
    /// 当我们开始设计class with pointer的时候
private:
    FClassErr(const FClassErr &)
    {
    }
    FClassErr &operator=(const FClassErr &)
    {
    }

private:
    int *data = nullptr;
};
int main()
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    int *a = new int;
    int *b = new int[1024];
    delete a;
    // delete[] b;
    try
    {
        FClassErr *e = new FClassErr(4);
        // delete e;
    }
    catch (int &ex)
    {
        std::cout << "出现了异常" << std::endl;
    }
    return 0;
}
