#ifndef MEMORY_LEAKDETECTOR_H
#define MEMORY_LEAKDETECTOR_H
#include <cstddef>

/// 我们要设计一个内存检查，就必须对默认的new做改造
/// 中断

void* operator new(size_t _size, const char* file, unsigned int _line);
void* operator new[](size_t _size, const char* file, unsigned int _line);

#ifndef __NEW__OVERLOADED
#define new new (__FILE__, __LINE__)
#endif


class MemoryLeakDetector
{
public:
    static unsigned int callCount;

    MemoryLeakDetector() noexcept
    {
        ++callCount;
    }
    ~MemoryLeakDetector()
    {
        if (--callCount == 0)
        {
            LeakDetector();
        }
    }

private:
    static unsigned int LeakDetector() noexcept;
};

static MemoryLeakDetector _exit_counter;

#endif // MEMORY_LEAKDETECTOR_H
