#include <iostream>
#include <cstring>
#include <iomanip>
#include <ctime>

/// 定义宏，从而实现new的管理
#define __NEW__OVERLOADED
#include "MemoryLeakDetector.h"

#include <string>

typedef struct _MemoryList
{
    struct _MemoryList *next, *prev;
    size_t              size;
    bool                isArray; /// 是否有申请数组
    char               *file;
    unsigned int        line;
} _MemoryList;

static unsigned long _memory_allocated = 0;

/// 未释放的内存的大小
static _MemoryList _root = { &_root, &_root, /// 第一个元素自引，他引指针
                             0,      false,  NULL, 0 };

unsigned int MemoryLeakDetector::callCount = 0;

/// 获取当前时间字符串
static std::string GetCurrentTime()
{
    time_t now = time(nullptr);
    char   timeStr[64];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
    return std::string(timeStr);
}

/// 格式化内存大小
static std::string FormatMemorySize(size_t bytes)
{
    const char *units[]   = { "B", "KB", "MB", "GB" };
    size_t      unitIndex = 0;
    double      size      = static_cast<double>(bytes);

    while (size >= 1024.0 && unitIndex < 3)
    {
        size /= 1024.0;
        unitIndex++;
    }

    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.2f %s", size, units[unitIndex]);
    return std::string(buffer);
}

/// 截断长文件名，保留最后的部分
static std::string TruncateFileName(const char *filename, int maxLength = 50)
{
    if (!filename)
        return "未知";

    std::string str(filename);
    if (str.length() <= maxLength)
        return str;

    /// 保留最后maxLength个字符
    return "..." + str.substr(str.length() - maxLength + 3);
}

/// 开始分配内存
void *AllocMemory(size_t _size, bool _array, const char *_file, unsigned _line)
{
    size_t       newSize = sizeof(_MemoryList) + _size;
    _MemoryList *newElem = (_MemoryList *)malloc(newSize);

    newElem->next    = _root.next;
    newElem->prev    = &_root;
    newElem->size    = _size;
    newElem->isArray = _array;
    newElem->file    = NULL;

    if (_file)
    {
        newElem->file = strdup(_file);
    }

    newElem->line = _line;

    _root.next->prev = newElem;
    _root.next       = newElem;
    _memory_allocated += _size;

    return (char *)newElem + sizeof(_MemoryList);
}

void DeleteMemory(void *_ptr, bool _array)
{
    _MemoryList *currentElem = (_MemoryList *)((char *)_ptr - sizeof(_MemoryList));

    if (currentElem->isArray != _array)
        return;

    currentElem->prev->next = currentElem->next;
    currentElem->next->prev = currentElem->prev;

    if (currentElem->file)
    {
        free(currentElem->file);
    }
    free(currentElem);
}

/// 重载new运算符
void *operator new(size_t _size)
{
    return AllocMemory(_size, false, NULL, 0);
}

void *operator new[](size_t _size)
{
    return AllocMemory(_size, true, NULL, 0);
}

void *operator new(size_t _size, const char *file, unsigned _line)
{
    return AllocMemory(_size, false, file, _line);
}

void *operator new[](size_t _size, const char *file, unsigned _line)
{
    return AllocMemory(_size, true, file, _line);
}

/// 重载delete
void operator delete(void *_ptr) noexcept
{
    DeleteMemory(_ptr, false);
}

void operator delete[](void *_ptr) noexcept
{
    DeleteMemory(_ptr, true);
}

unsigned int MemoryLeakDetector::LeakDetector(void) noexcept
{
    unsigned int  count     = 0;
    unsigned long totalSize = 0;
    _MemoryList  *ptr       = _root.next;

    /// 检查是否有泄漏
    if (ptr == &_root)
    {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "                   内存泄漏检测报告" << std::endl;
        std::cout << std::string(60, '-') << std::endl;
        std::cout << "检测时间: " << GetCurrentTime() << std::endl;
        std::cout << "检测结果: [SUCCESS] 未检测到内存泄漏" << std::endl;
        std::cout << std::string(60, '=') << std::endl << std::endl;
        return 0;
    }

    /// 先统计总泄漏信息
    _MemoryList *temp = ptr;
    while (temp != &_root)
    {
        count++;
        totalSize += temp->size;
        temp = temp->next;
    }

    /// 打印报告头部
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "                     内存泄漏检测报告" << std::endl;
    std::cout << std::string(80, '-') << std::endl;
    std::cout << "检测时间: " << GetCurrentTime() << std::endl;
    std::cout << "泄漏总数: " << count << " 处" << std::endl;
    std::cout << "总泄漏量: " << FormatMemorySize(totalSize) << std::endl;
    std::cout << std::string(80, '-') << std::endl;

    /// 打印详细的泄漏信息
    int leakIndex = 1;
    ptr           = _root.next;
    while (ptr != &_root)
    {
        /// 泄漏编号
        std::cout << "\n泄漏 #" << std::setw(3) << std::left << leakIndex << " ";

        /// 泄漏类型
        if (ptr->isArray)
        {
            std::cout << "[数组] ";
        }
        else
        {
            std::cout << "[对象] ";
        }

        /// 泄漏大小
        std::cout << std::setw(12) << std::left << (FormatMemorySize(ptr->size) + " ") << " ";

        /// 内存地址
        void *userPtr = (char *)ptr + sizeof(_MemoryList);
        std::cout << "地址: 0x" << std::hex << std::setw(12) << std::setfill('0') << (uintptr_t)userPtr << std::dec
                  << std::setfill(' ') << " ";

        /// 文件位置
        if (ptr->file)
        {
            std::string truncatedFile = TruncateFileName(ptr->file);
            std::cout << "位置: " << std::setw(40) << std::left << (truncatedFile + ":" + std::to_string(ptr->line));
        }
        else
        {
            std::cout << "位置: " << std::setw(40) << std::left << "未知位置";
        }

        /// 内存内容预览（前16字节）
        if (ptr->size > 0)
        {
            unsigned char *data = (unsigned char *)userPtr;
            std::cout << "数据: ";
            int bytesToShow = (ptr->size > 16) ? 16 : ptr->size;
            for (int i = 0; i < bytesToShow; i++)
            {
                printf("%02x ", data[i]);
            }
            if (ptr->size > 16)
            {
                std::cout << "...";
            }
        }

        leakIndex++;
        ptr = ptr->next;
    }

    /// 打印总结
    std::cout << "\n" << std::string(80, '-') << std::endl;
    std::cout << "检测总结:" << std::endl;
    std::cout << "  × 共发现 " << count << " 处内存泄漏" << std::endl;
    std::cout << "  × 总泄漏内存: " << FormatMemorySize(totalSize) << std::endl;
    std::cout << "  × 建议: 请检查以上位置是否正确释放内存" << std::endl;
    std::cout << std::string(80, '=') << std::endl << std::endl;

    return count;
}
