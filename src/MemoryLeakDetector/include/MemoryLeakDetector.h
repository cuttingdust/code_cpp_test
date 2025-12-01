#ifndef MEMORY_LEAKDETECTOR_H
#define MEMORY_LEAKDETECTOR_H
#include <cstddef>

/// 内存泄漏检测器配置
class MemoryLeakDetectorConfig
{
public:
    static bool showMemoryContent; ///< 是否显示内存内容
    static int  maxFilenameLength; ///< 最大文件名长度
    static bool showTimestamp;     ///< 是否显示时间戳
    static bool showSummary;       ///< 是否显示总结

    static void SetDefaultConfig()
    {
        showMemoryContent = true;
        maxFilenameLength = 50;
        showTimestamp     = true;
        showSummary       = true;
    }
};

/// 我们要设计一个内存检查，就必须对默认的new做改造
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

    /// 配置函数
    static void SetShowMemoryContent(bool show)
    {
        MemoryLeakDetectorConfig::showMemoryContent = show;
    }
    static void SetMaxFilenameLength(int length)
    {
        MemoryLeakDetectorConfig::maxFilenameLength = length;
    }
    static void SetShowTimestamp(bool show)
    {
        MemoryLeakDetectorConfig::showTimestamp = show;
    }
    static void SetShowSummary(bool show)
    {
        MemoryLeakDetectorConfig::showSummary = show;
    }

private:
    static unsigned int LeakDetector() noexcept;
};

static MemoryLeakDetector _exit_counter;

#endif // MEMORY_LEAKDETECTOR_H
