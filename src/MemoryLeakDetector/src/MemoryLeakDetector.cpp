#include <iostream>
#include <cstring>
/// 定义宏，从而实现new的管理
#define __NEW__OVERLOADED
#include "MemoryLeakDetector.h"


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

/// 开始分配内存
void *AllocMemory(size_t _size, bool _array, const char *_file, unsigned _line)
{
    size_t newSize = sizeof(_MemoryList) + _size;

    /// 用malloc来分配
    _MemoryList *newElem = (_MemoryList *)malloc(newSize);

    newElem->next    = _root.next;
    newElem->prev    = &_root;
    newElem->size    = _size;
    newElem->isArray = _array;
    newElem->file    = NULL;

    if (_file)
    {
        newElem->file = (char *)malloc(strlen(_file) + 1);
        strcpy(newElem->file, _file);
    }

    newElem->line = _line;

    /// 更新列表
    _root.next->prev = newElem;
    _root.next       = newElem;

    /// 记录到未释放的内存
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
        free(currentElem->file);
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
    unsigned int count = 0;
    _MemoryList *ptr   = _root.next;
    while (ptr && ptr != &_root)
    {
        if (ptr->isArray)
        {
            std::cout << "数组泄漏[] ";
        }
        else
        {
            std::cout << "泄漏 ";
        }
        std::cout << ptr << " 大小" << ptr->size;
        if (ptr->file)
        {
            std::cout << "位于" << ptr->file << "第" << ptr->line << "行";
        }
        else
        {
            std::cout << "没有对应的文件";
        }
        ++count;
        std::cout << std::endl;
        ptr = ptr->next;
    }
    if (count)
    {
        std::cout << "共发生了" << count << "处泄漏" << std::endl;
    }
    return count;
}
